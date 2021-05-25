/**
 *
 * Copyright (C) 2014 JBH@aurender.com
 *
 */


#ifndef TAGLIB_DFFFILE_H
#define TAGLIB_DFFFILE_H

#include "tfile.h"
#include "id3v2tag.h"
#include "dffproperties.h"

namespace TagLib
{

//! An implementation of DFF metadata

/*!
 * This is implementation of DFF metadata.
 *
 * This supports an ID3v2 tag as well as properties from the file.
 */

namespace DFF
{

//! An implementation of TagLib::File with DFF specific methods

/*!
  * This implements and provides an interface for DFF files to the
  * TagLib::Tag and TagLib::AudioProperties interfaces by way of implementing
  * the abstract TagLib::File API as well as providing some additional
  * information specific to DFF files.
  */

class TAGLIB_EXPORT File : public TagLib::File
{
public:
	/*!
	 * Contructs an DFF file from \a file.  If \a readProperties is true the
	 * file's audio properties will also be read using \a propertiesStyle.  If
	 * false, \a propertiesStyle is ignored.
	 */
	File(FileName file, bool readProperties = true,
	     Properties::ReadStyle propertiesStyle = Properties::Average);

	/*!
	 * Contructs an DFF file from \a file.  If \a readProperties is true the
	 * file's audio properties will also be read using \a propertiesStyle.  If
	 * false, \a propertiesStyle is ignored.
	 */
	File(IOStream *stream, bool readProperties = true,
	     Properties::ReadStyle propertiesStyle = Properties::Average);

	/*!
	 * Destroys this instance of the File.
	 */
	virtual ~File();

	/*!
	 * Returns the Tag for this file.
	 */
	virtual ID3v2::Tag *tag() const;

	/*!
	 * Implements the unified property interface -- export function.
	 * This method forwards to ID3v2::Tag::properties().
	 */
	PropertyMap properties() const;

	/*!
	 * Implements the unified property interface -- import function.
	 * This method forwards to ID3v2::Tag::setProperties().
	 */
	PropertyMap setProperties(const PropertyMap &);

	/*!
	 * Returns the AIFF::Properties for this file.  If no audio properties
	 * were read then this will return a null pointer.
	 */
	virtual Properties *audioProperties() const;

	/*!
	 * Saves the file.
	 */
	virtual bool save();

private:
	File(const File &);
	File &operator=(const File &);

	void read(bool readProperties, Properties::ReadStyle propertiesStyle);

	class FilePrivate;
	FilePrivate *d;
};
}
}

#endif