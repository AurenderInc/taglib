/***************************************************************************
    copyright            : (C) 2014 by Peking Duck Labs
    email                : pekingducklabs@gmail.com
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
 ***************************************************************************/

#include <stdint.h>

// Kid3: Remove taglib/ from includes
#include <tstring.h>
// Kid3: Use debug() instead of std::cerr
#include <tdebug.h>

#include "dsfproperties.h"
#include "dsffile.h"

using namespace TagLib;

class DSF::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate(DSF::File *f, ReadStyle s) :
    file(f),
    style(s),
    length(0),
    bitrate(0),
    sampleRate(0),
    channels(0),
    ID3v2Offset(0),
    sampleCount(0),
    fileSize(0),
    bitsPerSample(1),
    version(DSF::Header::Version1),
    channelType(DSF::Header::Stereo)
   {}

  DSF::File *file;
  TagLib::AudioProperties::ReadStyle style;
  int length;
  int bitrate;
  int sampleRate;
  int channels;
  uint64_t ID3v2Offset;
  uint64_t sampleCount;
  uint64_t fileSize;
  int bitsPerSample;
  DSF::Header::Version version;
  DSF::Header::ChannelType channelType;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

DSF::Properties::Properties(DSF::File *file, 
			     ReadStyle style) 
  : AudioProperties(style)
{
  d = new PropertiesPrivate(file, style);

  if(file && file->isOpen()) 
    read();
}

DSF::Properties::~Properties()
{
  delete d;
}

int DSF::Properties::length() const
{
  return d->sampleRate != 0 ? d->sampleCount / d->sampleRate : 0;
}

int DSF::Properties::bitrate() const
{
  return d->sampleRate * d->bitsPerSample / 1024;
}

int DSF::Properties::sampleRate() const
{
  return d->sampleRate;
}

int DSF::Properties::channels() const
{
  return d->channels;
}

DSF::Header::Version DSF::Properties::version() const
{
  return d->version;
}

DSF::Header::ChannelType DSF::Properties::channelType() const
{
  return d->channelType;
}

uint64_t DSF::Properties::ID3v2Offset() const 
{
  return d->ID3v2Offset;
}

uint64_t DSF::Properties::fileSize() const
{
  return d->fileSize;
}

uint64_t DSF::Properties::sampleCount() const
{
  return d->sampleCount;
}

int DSF::Properties::bitsPerSample() const
{
  return d->bitsPerSample;
}

int DSF::Properties::bitwidth() const //JBH
{
  return d->bitsPerSample;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void DSF::Properties::read()
{
  // Go to the beginning of the file
  d->file->seek(0);

  DSF::Header h(d->file->readBlock(DSF::Header::DSD_HEADER_SIZE + 
				 DSF::Header::FMT_HEADER_SIZE));

  if (!h.isValid()) {
    debug("DSF::Properties::read(): file header is not valid");
    return;
  }

  d->sampleRate = h.sampleRate();
  d->sampleCount = h.sampleCount();
  d->bitsPerSample = h.bitsPerSample();
  d->channels = h.channelNum();
  d->version = h.version();
  d->fileSize = h.fileSize();
  d->channelType = h.channelType();
  d->ID3v2Offset = h.ID3v2Offset();
}
