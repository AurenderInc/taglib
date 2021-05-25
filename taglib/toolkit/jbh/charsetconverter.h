/**
 * Author: James Hwang
 * Copyright (c)  2019  aurender.com
 *
 */



#ifndef CHARSETCONVERTER_H
#define CHARSETCONVERTER_H

#include <taglib_export.h>
#include <tbytevector.h>
#include <tstring.h>

namespace TagLib  {

class TAGLIB_EXPORT CHARSETCONVERTER
{
public:
    static int encodeByteVectorToUTF16(TagLib::ByteVector    inputByteVector,   TagLib::String &outputTagLibString, const std::string &fromCharSet, const std::string &toCharSet, bool ignore_error=true);
    static int encodeTagLibStringToUTF16(TagLib::String      inputTagLibString, TagLib::String &outputTagLibString, const std::string &fromCharSet, const std::string &toCharSet, bool ignore_error=true);
    static int encodeByteVectorToUTF16_V2(TagLib::ByteVector inputByteVector,   TagLib::String &outputTagLibString, const std::string &fromCharSet, const std::string &toCharSet, bool ignore_error=true);
    static std::string convertTagLibStringCharSet(TagLib::String  inputTagLibString, const std::string &toCharSet, bool ignore_error=true);
    static std::string convertUTF16TagLibStringToUTF8StdString(TagLib::String  inputTagLibString, bool ignore_error=true);
    static TagLib::String StdStringtoTString(const std::string& s);
};

} //namespace TagLib

#endif //CHARSETCONVERTER_H
