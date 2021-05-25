/**
 *
 * Copyright (C) 2014 JBH@aurender.com
 *
 */


#include <tstring.h>
#include <tdebug.h>

#include "dffproperties.h"

using namespace TagLib;

class DFF::Properties::PropertiesPrivate
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
        //JBH add
        dstEncoded(0),
        bitrate(0),
        length(0)
    {

    }

    // Nomenclature is from DFF file format specification
    uint formatVersion;
    uint formatID;
    uint channelType;
    uint channelNum;
    uint samplingFrequency;
    uint bitsPerSample;
    long long sampleCount;
    uint blockSizePerChannel;
    //JBH add
    uint dstEncoded;

    // Computed
    uint bitrate;
    uint length;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

//DFF::Properties::Properties(const ByteVector &data, ReadStyle style)
DFF::Properties::Properties(const ByteVector &data, ReadStyle style) : AudioProperties(style)
{
    d = new PropertiesPrivate;
    read(data);
}

DFF::Properties::~Properties()
{
    delete d;
}

int DFF::Properties::length() const
{
    return d->length;
}

void DFF::Properties::setLength(uint runlength)
{
    d->length = runlength;
}

int DFF::Properties::bitrate() const
{
    return d->bitrate;
}

int DFF::Properties::sampleRate() const
{
    return d->samplingFrequency;
}

int DFF::Properties::channels() const
{
    return d->channelNum;
}

// DFF specific
int DFF::Properties::formatVersion() const
{
    return d->formatVersion;
}

int DFF::Properties::formatID() const
{
    return d->formatID;
}

int DFF::Properties::channelType() const
{
    return d->channelType;
}

int DFF::Properties::bitsPerSample() const
{
    return d->bitsPerSample;
}

int DFF::Properties::bitwidth() const //JBH
{
    return d->bitsPerSample;
}

void DFF::Properties::setBitsPerSample(uint bitwidth)
{
    d->bitsPerSample = bitwidth;
}

long long DFF::Properties::sampleCount() const
{
    return d->sampleCount;
}

int DFF::Properties::blockSizePerChannel() const
{
    return d->blockSizePerChannel;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void DFF::Properties::read(const ByteVector &data)
{
    /*
     * JBH NOTE: PROP chunk's local chunks (no order imposed on the local chunks)
     *           - "SND": property type (4 bytes)
     *           - FS chunk:              sample rate
     *           - CHNL chunk:            # of audio channels
     *           - CMPR chunk:            compression type (DSD|DST)
     *           - ABSS chunk (optional): absolute start time
     *           - LSCO chunk (optional): loundspeaker configuration
     */

    if (data.mid(0, 4) != "SND ")
    {
        debug("DFF::Properties::read() -- 'PROP' is not 'SND' type.");
        return;
    }

    uint index = 4;
    while (index < data.size())
    {
        ByteVector chunkName = data.mid(index, 4);
        uint       chunkSize = (uint) data.mid(index+4, 8).toLongLong(true); //JBH NOTE: toUInt truncates anything larger than 4 bytes

        if      (chunkName == "FS  ")
        {
            d->samplingFrequency = data.mid(index+12, 4).toUInt(true);
        }
        else if (chunkName == "CHNL")
        {
            d->channelNum = data.mid(index+12, 2).toUInt(true);
        }
        else if (chunkName == "CMPR")
        {
            if (data.mid(index+12, 4) == "DST ")
            {
                //DST Encoded audio data
                d->dstEncoded = 1;
            }
            else // "DSD "
            {
                //Uncompressed, plain DSD audio data
                d->dstEncoded = 0;
            }
        }
        else if (chunkName == "ABSS")
        {
            //JBH NOTE: not use for now            
        }
        else if (chunkName == "LSCO")
        {
            //JBH NOTE: not use for now            
        }
        else
        {
            //JBH NOTE: not use for now            
        }

        index += 12 + chunkSize;
    }

    if (d->dstEncoded)
    {
        //JBH NOT: Aurender does not support "DST Encoded audio data",
        //         invalidate the property by clearing values.
        d->samplingFrequency = 0;
        d->channelNum = 0;
        debug("DFF::Properties::read() -- 'DST' encoded. Aurender does NOT support DST!!!. Treat Broken by setting samplingFrequency==0");
    }

    // d->formatVersion         = data.mid(0, 4).toUInt(true);
    // d->formatID              = data.mid(12, 4).toUInt(true);
    // d->channelType           = data.mid(8, 4).toUInt(true);
    // d->samplingFrequency     = data.mid(16, 4).toUInt(true);
    // d->channelNum            = data.mid(32, 2).toUInt(true);
    
    if      (d->samplingFrequency ==  2822400)
        d->bitsPerSample =  64;
    else if (d->samplingFrequency ==  5644800)
        d->bitsPerSample = 128;
    else if (d->samplingFrequency == 11289600)
        d->bitsPerSample = 256;
    //JBH TURNOFF_FORNOW  else if (d->samplingFrequency == 22579200)
    //JBH TURNOFF_FORNOW      d->bitsPerSample = 512;
    else
        d->bitsPerSample =   0; // shouldn't be here

    // d->sampleCount           = data.mid(24, 8).toLongLong(true);
    // d->blockSizePerChannel   = data.mid(32, 4).toUInt(true);

    d->bitrate               = (d->samplingFrequency * d->bitsPerSample * d->channelNum) / 1000.0;
    // d->length                = d->samplingFrequency > 0 ? d->sampleCount / d->samplingFrequency : 0;

}
