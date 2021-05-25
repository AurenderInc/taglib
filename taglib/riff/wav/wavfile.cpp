/***************************************************************************
    copyright            : (C) 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#include <tbytevector.h>
#include <tdebug.h>
#include <tstringlist.h>
#include <tpropertymap.h>
#include <tagutils.h>

#include "wavfile.h"
#include "id3v2tag.h"
#include "infotag.h"
#include "tagunion.h"

using namespace TagLib;

namespace
{
  enum { ID3v2Index = 0, InfoIndex = 1 };
}

class RIFF::WAV::File::FilePrivate
{
public:
  FilePrivate() :
    properties(0),
    hasID3v2(false),
    hasInfo(false) {}

  ~FilePrivate()
  {
    delete properties;
  }

  Properties *properties;
  TagUnion tag;

  bool hasID3v2;
  bool hasInfo;
};

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

bool RIFF::WAV::File::isSupported(IOStream *stream)
{
  // A WAV file has to start with "RIFF????WAVE".

  const ByteVector id = Utils::readHeader(stream, 12, false);
  return (id.startsWith("RIFF") && id.containsAt("WAVE", 8));
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

RIFF::WAV::File::File(FileName file, bool readProperties, Properties::ReadStyle) :
  RIFF::File(file, LittleEndian),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

RIFF::WAV::File::File(IOStream *stream, bool readProperties, Properties::ReadStyle) :
  RIFF::File(stream, LittleEndian),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

RIFF::WAV::File::~File()
{
  delete d;
}

ID3v2::Tag *RIFF::WAV::File::tag() const
{
  return ID3v2Tag();
}

ID3v2::Tag *RIFF::WAV::File::ID3v2Tag() const
{
  return d->tag.access<ID3v2::Tag>(ID3v2Index, false);
}

RIFF::Info::Tag *RIFF::WAV::File::InfoTag() const
{
  return d->tag.access<RIFF::Info::Tag>(InfoIndex, false);
}

void RIFF::WAV::File::strip(TagTypes tags)
{
  removeTagChunks(tags);

  if(tags & ID3v2)
    d->tag.set(ID3v2Index, new ID3v2::Tag());

  if(tags & Info)
    d->tag.set(InfoIndex, new RIFF::Info::Tag());
}

PropertyMap RIFF::WAV::File::properties() const
{
  return d->tag.properties();
}

void RIFF::WAV::File::removeUnsupportedProperties(const StringList &unsupported)
{
  d->tag.removeUnsupportedProperties(unsupported);
}

PropertyMap RIFF::WAV::File::setProperties(const PropertyMap &properties)
{
  InfoTag()->setProperties(properties);
  return ID3v2Tag()->setProperties(properties);
}

RIFF::WAV::Properties *RIFF::WAV::File::audioProperties() const
{
  return d->properties;
}

bool RIFF::WAV::File::save()
{
  return RIFF::WAV::File::save(AllTags);
}

bool RIFF::WAV::File::save(TagTypes tags, bool stripOthers, int id3v2Version)
{
  if(readOnly()) {
    debug("RIFF::WAV::File::save() -- File is read only.");
    return false;
  }

  if(!isValid()) {
    debug("RIFF::WAV::File::save() -- Trying to save invalid file.");
    return false;
  }

  if(stripOthers)
    strip(static_cast<TagTypes>(AllTags & ~tags));

  if(tags & ID3v2) {
    removeTagChunks(ID3v2);

    if(ID3v2Tag() && !ID3v2Tag()->isEmpty()) {
      setChunkData("ID3 ", ID3v2Tag()->render(id3v2Version));
      d->hasID3v2 = true;
    }
  }

  if(tags & Info) {
    removeTagChunks(Info);

    if(InfoTag() && !InfoTag()->isEmpty()) {
      setChunkData("LIST", InfoTag()->render(), true);
      d->hasInfo = true;
    }
  }

  return true;
}

bool RIFF::WAV::File::hasID3v2Tag() const
{
  return d->hasID3v2;
}

bool RIFF::WAV::File::hasInfoTag() const
{
  return d->hasInfo;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void RIFF::WAV::File::read(bool readProperties)
{
  /*
   * JBH NOTE: The "wav" file consists of "chunks", such as
   *           "fmt " chunk  --> format chunk --> "wave"
   *           "data" chunk  --> the pcm
   *           "LIST" chunk  --> metadata
   *           "ID3 " chunk  --> metadata
   *           "id3 " chunk  --> metadata
   *           "ID32" chunk  --> metadata (is ID32 valid?)
   *           "JUNK" chunk  --> junk data
   *
   *            The "wav" file keeps the metadata either in the "LIST INFO" chunk or the "ID3"chunk,
   *            and we are interested only in the "LIST INFO" or "ID3" chunk.
   *
   *            The "LIST INFO" chunk is the original tagging of the wav, while the "ID3" chunk is introduced later.
   *            "LIST INFO" tags:
   *                Artist (IART)
   *                Title (INAM) - called "Track Title" in Metadata Editor
   *                Product (IPRD) - called "Album Title" in Metadata Editor
   *                Track Number (ITRK) (not specified in the original RIFF standard but players supporting LIST INFO tags often support it)
   *                Date Created (ICRD) - called "Year" in Metadata Editor
   *                Genre (IGNR)
   *                Comments (ICMT)
   *                Copyright (ICOP)
   *                Software (ISFT)
   */

  for(unsigned int i = 0; i < chunkCount(); ++i) {
    const ByteVector name = chunkName(i);
//JBH ==========================================================================<
//JBH: Some malformed wav has "ID32" for the id3 chunk name, which is illegal but lets allow.
#define JBH_ALLOW_ID32_AS_VALID_CHUNKNAME
#ifdef  JBH_ALLOW_ID32_AS_VALID_CHUNKNAME
    if(name == "ID3 " || name == "id3 " || name == "ID32" || name == "id32" || name == "ID3H") {
#else
    if(name == "ID3 " || name == "id3 ") {
#endif
//JBH ==========================================================================<
      if(!d->tag[ID3v2Index]) {
        d->tag.set(ID3v2Index, new ID3v2::Tag(this, chunkOffset(i)));
        d->hasID3v2 = true;
      }
      else {
        //JBH too many logs    debug("RIFF::WAV::File::read() - Duplicate ID3v2 tag found. New tag will not be added"); //JBH
//JBH ==========================================================================<
//JBH FIXME  #ifdef _WIN32
//JBH FIXME          debug("JBH: WAV file: " + static_cast<TagLib::FileName>(name()).toString());
//JBH FIXME  #else
//JBH FIXME          debug("JBH: WAV file: " + String(name(), String::UTF8));
//JBH FIXME  #endif
//JBH ==========================================================================>
      }
    }
//JBH ==========================================================================<
//JBH: This block used to be there in the previous version of TagLib.
#if 0
    else if(name == "fmt " && readProperties)
      formatData = chunkData(i);
    else if(name == "data" && readProperties)
      streamLength = chunkDataSize(i); //JBH NOTE: the chunk size of the "data" chunk is the streamLength, the length of the pure PCM data.
#endif
//JBH ==========================================================================>
    else if(name == "LIST") {
      const ByteVector data = chunkData(i);
      if(data.startsWith("INFO")) {
        if(!d->tag[InfoIndex]) {
          d->tag.set(InfoIndex, new RIFF::Info::Tag(data));
          d->hasInfo = true;
        }
        else {
          debug("RIFF::WAV::File::read() - Duplicate INFO tag found. New tag will not be added"); //JBH
//JBH ==========================================================================<
//JBH FIXME  #ifdef _WIN32
//JBH FIXME            debug("JBH: WAV file: " + static_cast<FileName>(name()).toString());
//JBH FIXME  #else
//JBH FIXME            debug("JBH: WAV file: " + String(name(), String::UTF8));
//JBH FIXME  #endif
//JBH ==========================================================================>
        }
      }
    }
  }

  /*
   * JBH NOTE: The old TagLib used to consolidate "RIFF::Info::Tag" into "ID3v2::Tag", such as
   *
   *           if(!d->tag[ID3v2Index]) {
   *               d->tag = new ID3v2::Tag;
   *               if (d->tag[InfoIndex]) {
   *                  TagLib::Tag::duplicate(d->tag[InfoIndex], d->tag, true);
   *               }
   *           }
   *
   *           New TagLib (1.9.1) has changed this policy and does not consolidate.
   *           The client should check the existence of "RIFF::Info::Tag" and "ID3v2::Tag".
   *
   *           NOTE: It seems that TagLib 2.0 changes this policy again back to the consolidation.
   */

  if(!d->tag[ID3v2Index])
    d->tag.set(ID3v2Index, new ID3v2::Tag());

  if(!d->tag[InfoIndex])
    d->tag.set(InfoIndex, new RIFF::Info::Tag());

  if(readProperties)
    d->properties = new Properties(this, Properties::Average);
}

void RIFF::WAV::File::removeTagChunks(TagTypes tags)
{
  if((tags & ID3v2) && d->hasID3v2) {
    removeChunk("ID3 ");
    removeChunk("id3 ");

    d->hasID3v2 = false;
  }

  if((tags & Info) && d->hasInfo) {
    for(int i = static_cast<int>(chunkCount()) - 1; i >= 0; --i) {
      if(chunkName(i) == "LIST" && chunkData(i).startsWith("INFO"))
        removeChunk(i);
    }

    d->hasInfo = false;
  }
}
