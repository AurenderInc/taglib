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

#include <tstring.h>
#include <tdebug.h>

#include "dsfproperties.h"

#define JBH_USE_OLD_DSF_FORMULA //JBH add

using namespace TagLib;

class DSF::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate() :
  formatVersion(0),
  formatID(0),
  channelType(0),
  channelNum(0),
  samplingFrequency(0),
  bitsPerSample(0),
  sampleCount(0),
  blockSizePerChannel(0),
  bitrate(0),
  length(0)
  {
  }

  // Nomenclature is from DSF file format specification
  unsigned int formatVersion;
  unsigned int formatID;
  unsigned int channelType;
  unsigned int channelNum;
  unsigned int samplingFrequency;
  unsigned int bitsPerSample;
  long long sampleCount;
  unsigned int blockSizePerChannel;

  // Computed
  unsigned int bitrate;
  unsigned int length;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

DSF::Properties::Properties(const ByteVector &data, ReadStyle style) : TagLib::AudioProperties(style)
{
  d = new PropertiesPrivate;
  read(data);
}

DSF::Properties::~Properties()
{
  delete d;
}

int DSF::Properties::length() const
{
//JBH ==========================================================================<
#ifdef  JBH_USE_OLD_DSF_FORMULA
  return lengthInMilliseconds();
#else
  return lengthInSeconds();
#endif
//JBH ==========================================================================>
}

int DSF::Properties::lengthInSeconds() const
{
  return d->length / 1000;
}

int DSF::Properties::lengthInMilliseconds() const
{
  return d->length;
}

int DSF::Properties::bitrate() const
{
  return d->bitrate;
}

int DSF::Properties::sampleRate() const
{
  return d->samplingFrequency;
}

int DSF::Properties::channels() const
{
  return d->channelNum;
}

// DSF specific
int DSF::Properties::formatVersion() const
{
  return d->formatVersion;
}

int DSF::Properties::formatID() const
{
  return d->formatID;
}

int DSF::Properties::channelType() const
{
  return d->channelType;
}

int DSF::Properties::bitsPerSample() const
{
  return d->bitsPerSample;
}

//JBH ==========================================================================<
int DSF::Properties::bitwidth() const //JBH
{
  return d->bitsPerSample;
}
//JBH ==========================================================================>

long long DSF::Properties::sampleCount() const
{
  return d->sampleCount;
}

int DSF::Properties::blockSizePerChannel() const
{
  return d->blockSizePerChannel;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void DSF::Properties::read(const ByteVector &data)
{
  d->formatVersion         = data.toUInt(0U,false); //JBH: same as data.mid(0, 4).toUInt(false) of the old taglib
  d->formatID              = data.toUInt(4U,false); //JBH: same as data.mid(4, 4).toUInt(false) of the old taglib
  d->channelType           = data.toUInt(8U,false);
  d->channelNum            = data.toUInt(12U,false);
  d->samplingFrequency     = data.toUInt(16U,false);

  d->bitsPerSample         = data.toUInt(20U,false);
//JBH ==========================================================================<
  //JBH: set bitwidth as DFF alike.
  //JBH FIXME: shouldn't we separate/add d->bitwidth, instead of sharing d->bitsPerSample?
  if (d->samplingFrequency == 5644800)
    d->bitsPerSample     = 128;
  else if (d->samplingFrequency == 2822400)
    d->bitsPerSample     = 64;
  else
    d->bitsPerSample     = 0; // shouldn't be here. JBH: "0 --> 1" would be safer for the default?
//JBH ==========================================================================>

  d->sampleCount           = data.toLongLong(24U,false); //JBH: same as data.mid(24, 8).toLongLong(false) of the old taglib
  d->blockSizePerChannel   = data.toUInt(32U,false);

//JBH ==========================================================================<
#ifdef  JBH_USE_OLD_DSF_FORMULA
  d->bitrate               = (d->samplingFrequency * d->bitsPerSample * d->channelNum) / 1000.0;
  d->length                = d->samplingFrequency > 0 ? d->sampleCount / d->samplingFrequency : 0;
#else
  d->bitrate
  = static_cast<unsigned int>((d->samplingFrequency * d->bitsPerSample * d->channelNum) / 1000.0 + 0.5);
  d->length
  = d->samplingFrequency > 0 ? static_cast<unsigned int>(d->sampleCount * 1000.0 / d->samplingFrequency + 0.5) : 0;
#endif
//JBH ==========================================================================<

//JBH ==========================================================================<
  if (d->channelNum > 2)
  {
      debug("DSF::Properties::read() -- multi channels detected. Aurender does not support more than 2 channels: " + d->channelNum);
  }
//JBH ==========================================================================>
}

