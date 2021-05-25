/**
 *
 * Copyright (C) 2014 JBH@aurender.com
 *
 */

#include <tbytevector.h>
#include <tdebug.h>
#include <id3v2tag.h>
#include <tstringlist.h>
#include <tpropertymap.h>

#include "dfffile.h"

using namespace TagLib;

// The DFF specification is located at http://dsd-guide.com/sites/default/files/white-papers/DFFFileFormatSpec_E.pdf

class DFF::File::FilePrivate
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

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

DFF::File::File(FileName file, bool readProperties,
                Properties::ReadStyle propertiesStyle) : TagLib::File(file)
{
	d = new FilePrivate;

	if(isOpen())
		read(readProperties, propertiesStyle);
}

DFF::File::File(IOStream *stream, bool readProperties,
                Properties::ReadStyle propertiesStyle) : TagLib::File(stream)
{
	d = new FilePrivate;

	if(isOpen())
		read(readProperties, propertiesStyle);
}

DFF::File::~File()
{
	delete d;
}

ID3v2::Tag *DFF::File::tag() const
{
	return d->tag;
}

PropertyMap DFF::File::properties() const
{
	return d->tag->properties();
}

PropertyMap DFF::File::setProperties(const PropertyMap &properties)
{
	return d->tag->setProperties(properties);
}

DFF::Properties *DFF::File::audioProperties() const
{
	return d->properties;
}

bool DFF::File::save()
{
	if(readOnly())
	{
		debug("DFF::File::save() -- File is read only.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return false;
	}

	if(!isValid())
	{
		debug("DFF::File::save() -- Trying to save invalid file.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return false;
	}

	// Two things must be updated: the file size and the tag data
	// The metadata offset doesn't change

	ByteVector tagData = d->tag->render();

	long long oldTagSize = d->fileSize - d->metadataOffset;
	long long newFileSize = d->metadataOffset + tagData.size();

	// Write the file size
//  insert(ByteVector::fromUInt64LE(newFileSize), 12, 8);
	insert(ByteVector::fromLongLong(newFileSize), 12, 8);

	// Delete the old tag and write the new one
	insert(tagData, d->metadataOffset, oldTagSize);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void DFF::File::read(bool readProperties, Properties::ReadStyle propertiesStyle)
{
	long long currentIndex = 0;
	long long chunkDataSize = 0;
	ByteVector chunkName;
	ByteVector chunkDataVector;

	//JBH: type chunk
	chunkName = readBlock(4);
	if(chunkName != "FRM8")
	{
		debug("DFF::File::read() -- Not a DFF file.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return;
	}
	currentIndex += 4;

	//JBH: filesize chunk
	d->fileSize = readBlock(8).toLongLong(true);
	//JBH: Note "true" for DFF, "false" for DSF.
	//     DFF uses the BigEndian, while DSF uses the LittleEndian.
	currentIndex += 8;

	//JBH: format chunk
	chunkName = readBlock(4);
	if(chunkName != "DSD ")
	{
		debug("DFF::File::read() -- Missing 'DSD ' chunk.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return;
	}
	currentIndex += 4;

	//JBH: FVER subchunk =========================================
	//subchunk name block
	chunkName = readBlock(4);
	if(chunkName != "FVER")
	{
		debug("DFF::File::read() -- Missing 'FVER' subchunk.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return;
	}
	currentIndex += 4;

	//subchunk size block
	chunkDataSize = readBlock(8).toLongLong(true);
	currentIndex += 8;

	//subchunk data block
	chunkDataVector = readBlock(chunkDataSize);
	int fversion = chunkDataVector.toUInt(true); //fversion not used anywhere
	currentIndex += chunkDataSize;


	//JBH: PROP subchunk =========================================
	//subchunk name block
	chunkName = readBlock(4);
	if(chunkName != "PROP")
	{
		debug("DFF::File::read() -- Missing 'PROP' subchunk.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return;
	}
	currentIndex += 4;

	//subchunk size block
	chunkDataSize = readBlock(8).toLongLong(true);
	currentIndex += 8;

	//subchunk data block
	//get SamplingFrequency and the number of Channels
	chunkDataVector = readBlock(chunkDataSize);
	d->properties = new Properties(chunkDataVector, propertiesStyle);
	currentIndex += chunkDataSize;


    //JBH: DSD subchunk =========================================
    //JBH NOTE: Some fucking file has the "JUNK" subchunk at this moment, while the "DSD " subchunk is expected here.
    //          To work around, discard this "JUNK" subchunk and advance to the next subchunk (hopefully "DSD ").
    chunkName = readBlock(4);
    if(chunkName == "JUNK")
    {
        debug("DFF::File::read() -- 'JUNK' subchunk detected while 'DSD ' expected. AMS will discard it and advance to the next subchunk");
    # ifdef _WIN32
        debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
        debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif

        //advance to the data size block
        currentIndex += 4;

        //discard the "JUNK" subchunk, advance to the next block (hopefully "DSD ").
        chunkDataSize = readBlock(8).toLongLong(true); // size of the JUNK data block
        currentIndex += 8;

        //chunkDataVector = readBlock(chunkDataSize); //read and discard the JUNK data block, advance to the next block.
        currentIndex += chunkDataSize; //forward the buffer index.
        seek(currentIndex, Beginning); //JBH: do not actually read (which takes long time) the data, just forward the file seek index. 

        //read the next chunk name (hopefully "DSD ").
        chunkName = readBlock(4);
    }

	if(chunkName != "DSD ")
	{
		debug("DFF::File::read() -- Missing 'DSD ' subchunk.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return;
	}
	currentIndex += 4;


	//JBH: Music Data subchunk =========================================
	//subchunk size block
	chunkDataSize = readBlock(8).toLongLong(true); // size of the music data block
	currentIndex += 8;

	//subchunk music data block
	//chunkDataVector = readBlock(chunkDataSize); //do nothing for the music data block. just forward the buffer index.
	currentIndex += chunkDataSize;
	seek(currentIndex, Beginning); //do nothing for the music data block. just forward the buffer index. 

    //calculate Music-Run-Length, and set it to Properties
    uint musiclength = ((chunkDataSize / d->properties->channels()) * 8) / d->properties->sampleRate();
    d->properties->setLength(musiclength);


	//JBH: DIIN subchunk =========================================
	//subchunk name block
	chunkName = readBlock(4);
	if(chunkName != "DIIN")
	{
		debug("DFF::File::read() -- Missing 'DIIN' subchunk.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return;
	}
	currentIndex += 4;

	//subchunk size block
	chunkDataSize = readBlock(8).toLongLong(true);
	currentIndex += 8;

	//subchunk data block
	//chunkDataVector = readBlock(chunkDataSize);
	currentIndex += chunkDataSize;
	seek(currentIndex, Beginning); //do nothing for this data block. just forward the buffer index. 

	//JBH: COMT subchunk =========================================
	//subchunk name block
	chunkName = readBlock(4);
	if(chunkName != "COMT")
	{
		debug("DFF::File::read() -- Missing 'COMT' subchunk.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return;
	}
	currentIndex += 4;

	//subchunk size block
	chunkDataSize = readBlock(8).toLongLong(true);
	currentIndex += 8;

	//subchunk data block
	//chunkDataVector = readBlock(chunkDataSize);
	currentIndex += chunkDataSize;
	seek(currentIndex, Beginning); //do nothing for this data block. just forward the buffer index. 


	//JBH: ID3 subchunk =========================================
	//subchunk name block
	chunkName = readBlock(4);
	if(chunkName != "ID3 ")
	{
		debug("DFF::File::read() -- Missing 'ID3 ' subchunk.");
    # ifdef _WIN32
      	debug("JBH: DFF file " + static_cast<FileName>(name()).toString()); //JBH add
    # else
      	debug("JBH: DFF file " + String(name(), String::UTF8)); //JBH add
    # endif
		return;
	}
	currentIndex += 4;

	//subchunk size block
	chunkDataSize = readBlock(8).toLongLong(true);
	currentIndex += 8;

	//read the ID3v2 tags
	d->tag = new ID3v2::Tag(this, currentIndex);
}
