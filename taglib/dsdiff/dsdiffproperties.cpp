/***************************************************************************
 copyright            : (C) 2016 by Damien Plisson, Audirvana
 email                : damien78@audirvana.com
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

#include "dsdiffproperties.h"

using namespace TagLib;

class DSDIFF::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate() :
//JBH ==========================================================================<
  formatVersion(0),
  formatID(0),
  channelType(0),
  dstEncoded(false),
//JBH ==========================================================================>
  length(0),
  bitrate(0),
  sampleRate(0),
  channels(0),
  sampleWidth(0),
  sampleCount(0)
  {
  }

//JBH ==========================================================================<
  int formatVersion;
  int formatID;
  int channelType;
  bool dstEncoded;
//JBH ==========================================================================>
  int length;
  int bitrate;
  int sampleRate;
  int channels;
  int sampleWidth;
  unsigned long long sampleCount;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

DSDIFF::Properties::Properties(const unsigned int sampleRate,
                               const unsigned short channels,
                               const unsigned long long samplesCount,
                               const int bitrate,
                               const bool dstEncoded, //JBH added
                               ReadStyle style) : AudioProperties(style)
{
  d = new PropertiesPrivate;

  d->channels    = channels;
  d->sampleCount = samplesCount;
  d->sampleWidth = 1;
  d->sampleRate  = sampleRate;
  d->bitrate     = bitrate;
  d->length      = d->sampleRate > 0
    ? static_cast<int>((d->sampleCount * 1000.0) / d->sampleRate + 0.5)
    : 0;

//JBH ==========================================================================<
  d->dstEncoded  = dstEncoded; //JBH added
  if (d->dstEncoded) {
        //JBH NOT: Aurender does not support "DST Encoded audio data",
        //         Invalidate the property by clearing values.
        d->sampleRate = 0;
        d->channels = 0;
        debug("JBH: DFF::Properties::read() -- 'DST' encoded. Aurender does NOT support DST!!!. Treat Broken by setting sampleRate=0 channels=0");    
  }

  /*
   * DSD sample rates are multiple 44100 Hz:
   * 
   * DSD   64   : DSD  2.8 MHz   =  2822400 Hz = 44100 Hz x   64 times;
   * DSD  128   : DSD  5.6 MHz   =  5644800 Hz = 44100 Hz x  128 times;
   * DSD  256   : DSD 11.2 MHz   = 11289600 Hz = 44100 Hz x  256 times;
   * DSD  512   : DSD 22.6 MHz   = 22579200 Hz = 44100 Hz x  512 times;
   * DSD 1024   : DSD 45.2 MHz   = 45158400 Hz = 44100 Hz x 1024 times;
   */
  /*
   *JBH: Aurender supports:
   *     - A30/W20SE/ACS: upto DSD1024
   *     - Elses:         upto DSD128
   */
  if      (d->sampleRate ==  2822400)
    d->sampleWidth =   64;
  else if (d->sampleRate ==  5644800)
    d->sampleWidth =  128;
  else if (d->sampleRate == 11289600)
    d->sampleWidth =  256;
  else if (d->sampleRate == 22579200)
    d->sampleWidth =  512;  //Aurender W20SE supports  512. 2019/04/19
  else if (d->sampleRate == 45158400)
    d->sampleWidth = 1024;  //Aurender W20SE supports 1024. 2019/05/02
  else
    d->sampleWidth =    0;  // shouldn't be here
//JBH ==========================================================================>
}

DSDIFF::Properties::~Properties()
{
  delete d;
}

int DSDIFF::Properties::length() const
{
  return lengthInSeconds();
}

int DSDIFF::Properties::lengthInSeconds() const
{
  return d->length / 1000;
}

int DSDIFF::Properties::lengthInMilliseconds() const
{
  return d->length;
}

int DSDIFF::Properties::bitrate() const
{
  return d->bitrate;
}

int DSDIFF::Properties::sampleRate() const
{
  return d->sampleRate;
}

int DSDIFF::Properties::channels() const
{
  return d->channels;
}

int DSDIFF::Properties::bitsPerSample() const
{
  return d->sampleWidth;
}

//JBH ==========================================================================<
int DSDIFF::Properties::bitwidth() const //JBH
{
  return d->sampleWidth;
}

// DFF specific
int DSDIFF::Properties::formatVersion() const
{
  return d->formatVersion;
}

int DSDIFF::Properties::formatID() const
{
  return d->formatID;
}

int DSDIFF::Properties::channelType() const
{
  return d->channelType;
}
//JBH ==========================================================================>

long long DSDIFF::Properties::sampleCount() const
{
  return d->sampleCount;
}

