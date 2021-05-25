/**
 * Author: James Hwang
 * Copyright (c)  2019  aurender.com
 *
 */



#ifndef CHARSETDETECTOR_H
#define CHARSETDETECTOR_H

#include <taglib_export.h>
#include <tbytevector.h>
#include <tstring.h>

namespace TagLib  {

class TAGLIB_EXPORT CHARSETDETECTOR
{
public:
    static float detectCharSet(const TagLib::ByteVector &dataByteVector, std::string &charSet);
    static float detectCharSet(const TagLib::String &dataTagLibString, std::string &charSet);
};

} //namespace TagLib


#endif //CHARSETDETECTOR_H
