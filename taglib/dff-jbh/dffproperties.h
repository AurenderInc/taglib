/**
 *
 * Copyright (C) 2014 JBH@aurender.com
 *
 */


#ifndef TAGLIB_DFFPROPERTIES_H
#define TAGLIB_DFFPROPERTIES_H

#include "audioproperties.h"

namespace TagLib
{

namespace DFF
{

class File;

//! An implementation of audio property reading for DFF

/*!
 * This reads the data from a DFF stream found in the AudioProperties
 * API.
 */

class TAGLIB_EXPORT Properties : public AudioProperties
{
public:
	/*!
	 * Create an instance of DFF::Properties with the data read from the
	 * ByteVector \a data.
	 */
	Properties(const ByteVector &data, ReadStyle style);

	/*!
	 * Destroys this AIFF::Properties instance.
	  */
	virtual ~Properties();

	// Reimplementations.

	virtual int length() const;
	virtual int bitrate() const;
	virtual int sampleRate() const;
	virtual int channels() const;
	virtual int bitwidth() const; //JBH

	virtual void setLength(uint runlength);
	virtual void setBitsPerSample(uint bitwidth);

	int formatVersion() const;
	int formatID() const;
	int channelType() const;
	int bitsPerSample() const;
	long long sampleCount() const;
	int blockSizePerChannel() const;

private:
	Properties(const Properties &);
	Properties &operator=(const Properties &);

	void read(const ByteVector &data);

	class PropertiesPrivate;
	PropertiesPrivate *d;
};
}
}

#endif