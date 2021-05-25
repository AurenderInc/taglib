/***************************************************************************
 copyright            : (C) 2013 by Stephen F. Booth
 email                : me@sbooth.org
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
#include <id3v2tag.h>
#include <tstringlist.h>
#include <tpropertymap.h>
#include <tagutils.h> //JBH added

#include "dsffile.h"

using namespace TagLib;

// The DSF specification is located at http://dsd-guide.com/sites/default/files/white-papers/DSFFileFormatSpec_E.pdf

class DSF::File::FilePrivate
{
public:
  FilePrivate() :
  properties(0),
  tag(0)
  {
  }

  ~FilePrivate()
  {
    delete properties;
    delete tag;
  }

  long long fileSize;
  long long metadataOffset;
  Properties *properties;
  ID3v2::Tag *tag;
};

//JBH ==========================================================================<
////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

bool DSF::File::isSupported(IOStream *stream) //JBH FIXME
{
#if JBH_VERIFIED_THE_FOLLOWING_LOGIC
  // A DSF file has an ID "DSD" somewhere. An ID3v2 tag may precede.

  const ByteVector buffer = Utils::readHeader(stream, bufferSize(), true); //JBH FIXME
  return (buffer.find("DSD") >= 0); //JBH FIXME
#else
  return 0; //JBH FIXME: return true util the above logic fixed/verified
#endif
}
//JBH ==========================================================================>

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

DSF::File::File(FileName file, bool readProperties,
                Properties::ReadStyle propertiesStyle) :
  TagLib::File(file),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties, propertiesStyle);
}

DSF::File::File(IOStream *stream, bool readProperties,
                Properties::ReadStyle propertiesStyle) :
  TagLib::File(stream),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties, propertiesStyle);
}

DSF::File::~File()
{
  delete d;
}

ID3v2::Tag *DSF::File::tag() const
{
  return d->tag;
}

PropertyMap DSF::File::properties() const
{
  return d->tag->properties();
}

PropertyMap DSF::File::setProperties(const PropertyMap &properties)
{
  return d->tag->setProperties(properties);
}

DSF::Properties *DSF::File::audioProperties() const
{
  return d->properties;
}

bool DSF::File::save()
{
  if(readOnly()) {
    debug("DSF::File::save() -- File is read only.");
//JBH ==========================================================================<
  #ifdef _WIN32
    debug("JBH: DSF file: " + static_cast<FileName>(name()).toString()); //JBH add
  #else
    debug("JBH: DSF file: " + String(name(), String::UTF8)); //JBH add
  #endif
//JBH ==========================================================================>
    return false;
  }

  if(!isValid()) {
    debug("DSF::File::save() -- Trying to save invalid file.");
//JBH ==========================================================================<
  #ifdef _WIN32
    debug("JBH: DSF file: " + static_cast<FileName>(name()).toString()); //JBH add
  #else
    debug("JBH: DSF file: " + String(name(), String::UTF8)); //JBH add
  #endif
//JBH ==========================================================================>
    return false;
  }

  // Three things must be updated: the file size, the tag data, and the metadata offset

  if(d->tag->isEmpty()) {
    long long newFileSize = d->metadataOffset ? d->metadataOffset : d->fileSize;

    // Update the file size
    if(d->fileSize != newFileSize) {
      insert(ByteVector::fromLongLong(newFileSize, false), 12, 8);
      d->fileSize = newFileSize;
    }

    // Update the metadata offset to 0 since there is no longer a tag
    if(d->metadataOffset) {
      insert(ByteVector::fromLongLong(0ULL, false), 20, 8);
      d->metadataOffset = 0;
    }

    // Delete the old tag
    truncate(newFileSize);
  }
  else {
    ByteVector tagData = d->tag->render();

    long long newMetadataOffset = d->metadataOffset ? d->metadataOffset : d->fileSize;
    long long newFileSize = newMetadataOffset + tagData.size();
    long long oldTagSize = d->fileSize - newMetadataOffset;

    // Update the file size
    if(d->fileSize != newFileSize) {
      insert(ByteVector::fromLongLong(newFileSize, false), 12, 8);
      d->fileSize = newFileSize;
    }

    // Update the metadata offset
    if(d->metadataOffset != newMetadataOffset) {
      insert(ByteVector::fromLongLong(newMetadataOffset, false), 20, 8);
      d->metadataOffset = newMetadataOffset;
    }

    // Delete the old tag and write the new one
    insert(tagData, newMetadataOffset, static_cast<size_t>(oldTagSize));
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////


void DSF::File::read(bool readProperties, Properties::ReadStyle propertiesStyle)
{
  // A DSF file consists of four chunks: DSD chunk, format chunk, data chunk, and metadata chunk
  // The file format is not chunked in the sense of a RIFF File, though

  // DSD chunk
  ByteVector chunkName = readBlock(4);
  if(chunkName != "DSD ") {
    debug("DSF::File::read() -- Not a DSF file.");
//JBH ==========================================================================<
  #ifdef _WIN32
    debug("JBH: DSF file: " + static_cast<FileName>(name()).toString()); //JBH add
  #else
    debug("JBH: DSF file: " + String(name(), String::UTF8)); //JBH add
  #endif
//JBH ==========================================================================>
    setValid(false);
    return;
  }

  long long chunkSize = readBlock(8).toLongLong(false);
  //JBH: Note "true" for DFF, "false" for DSF.
  //     DFF uses the BigEndian, while DSF uses the LittleEndian.

  // Integrity check
  if(28 != chunkSize) {
    debug("DSF::File::read() -- File is corrupted, wrong chunk size");
//JBH ==========================================================================<
  #ifdef _WIN32
    debug("JBH: DSF file: " + static_cast<FileName>(name()).toString()); //JBH add
  #else
    debug("JBH: DSF file: " + String(name(), String::UTF8)); //JBH add
  #endif
//JBH ==========================================================================>
    setValid(false);
    return;
  }


//JBH ==========================================================================<
  long fileLength     = length(); //JBH add
  long lengthPosition = tell();   //JBH: save the current seek position of length
  bool isBigEndian    = false;    //JBH: DFF uses the BigEndian, while DSF uses the LittleEndian.

  d->fileSize = readBlock(8).toLongLong(false); //JBH: try LittleEndian
  long curSeekPosition = tell();   //JBH debug

//#define JBH_DSF_CHECK_FILESIZE //JBH: Do not define for now. Too much strict rule is applied and lots of files will not be read.
#ifdef  JBH_DSF_CHECK_FILESIZE
  #define JBH_RETRY_WITH_BIGENDIAN
  #ifdef  JBH_RETRY_WITH_BIGENDIAN
  if(d->fileSize != fileLength) {
    /*
     *JBH: DFF uses the BigEndian, while DSF uses the LittleEndian.
     *     so, "true" for DFF, "false" for DSF.
     *
     *     Some fucking dsf files intermixed:
     *     "01-01. I. Missa. 1. Kyrie eleison.dsf" has the "chunSize" block correct (LE), while  the "fileSize" block wrong (BE).
     *     In this case, "true" in readBlock(8).toLongLong(true) may provide the correct value.
     */
    seek(lengthPosition);
    curSeekPosition = tell();   //JBH debug
    isBigEndian = true;
    d->fileSize = readBlock(8).toLongLong(isBigEndian); //JBH: try BigEndian
    curSeekPosition = tell();   //JBH debug
  }
  #endif //JBH_RETRY_WITH_BIGENDIAN

  // File is malformed or corrupted
  if(d->fileSize != length()) {
#define JBH_DSF_FILESIZE_WORKAROUND
#ifdef  JBH_DSF_FILESIZE_WORKAROUND
    debug("DSF::File::read() -- JBH: wrong fileSize. Forced fileSize to the physical file length!");
//JBH ==========================================================================<
  #ifdef _WIN32
    debug("JBH: DSF file: " + static_cast<FileName>(name()).toString()); //JBH add
  #else
    debug("JBH: DSF file: " + String(name(), String::UTF8)); //JBH add
  #endif
//JBH ==========================================================================>
    d->fileSize = length(); //JBH: force fileSize to the physical file length.
#else
    debug("DSF::File::read() -- File is corrupted wrong length");
//JBH ==========================================================================<
  #ifdef _WIN32
    debug("JBH: DSF file: " + static_cast<FileName>(name()).toString()); //JBH add
  #else
    debug("JBH: DSF file: " + String(name(), String::UTF8)); //JBH add
  #endif
//JBH ==========================================================================>
    setValid(false);
    return;
#endif //JBH_DSF_FILESIZE_WORKAROUND
  }
#endif //JBH_DSF_CHECK_FILESIZE
//JBH ==========================================================================>

  d->metadataOffset = readBlock(8).toLongLong(false);

  // File is malformed or corrupted
  if(d->metadataOffset > d->fileSize) {
    debug("DSF::File::read() -- Invalid metadata offset.");
//JBH ==========================================================================<
  #ifdef _WIN32
    debug("JBH: DSF file: " + static_cast<FileName>(name()).toString()); //JBH add
  #else
    debug("JBH: DSF file: " + String(name(), String::UTF8)); //JBH add
  #endif
//JBH ==========================================================================>
    setValid(false);
    return;
  }

  // Format chunk
  chunkName = readBlock(4);
  if(chunkName != "fmt ") {
    debug("DSF::File::read() -- Missing 'fmt ' chunk.");
//JBH ==========================================================================<
  #ifdef _WIN32
    debug("JBH: DSF file: " + static_cast<FileName>(name()).toString()); //JBH add
  #else
    debug("JBH: DSF file: " + String(name(), String::UTF8)); //JBH add
  #endif
//JBH ==========================================================================>
    setValid(false);
    return;
  }

  chunkSize = readBlock(8).toLongLong(false);

  d->properties = new Properties(readBlock(chunkSize), propertiesStyle);

  // Skip the data chunk

//JBH ==========================================================================<
//JBH FIXME  #define JBH_DSF_CHECK_FILESIZE //JBH: Do not define for now, which results in an empty d->tag.
#ifdef  JBH_DSF_CHECK_FILESIZE
  // A metadata offset of 0 indicates the absence of an ID3v2 tag
  if(0 == d->metadataOffset)
    d->tag = new ID3v2::Tag();
  else
    d->tag = new ID3v2::Tag(this, d->metadataOffset);
#else
  d->tag = new ID3v2::Tag(this, d->metadataOffset);
#endif
}

