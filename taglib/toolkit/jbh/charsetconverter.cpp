/**
 * Author: James Hwang
 * Copyright (c)  2019  aurender.com
 *
 */


#include <iconv.h>
#include <string.h> //for strncpy
#include <stdlib.h> //malloc for the old gcc
#include <tdebug.h>

#include "charsetconverter.h"

#define MAX_BUF_SIZE 4096

using namespace TagLib;

int CHARSETCONVERTER::encodeByteVectorToUTF16(TagLib::ByteVector inputByteVector, TagLib::String &outputTagLibString, const std::string &fromCharSet, const std::string &toCharSet, bool ignore_error)
{
    iconv_t iconv_converter = ::iconv_open(toCharSet.c_str(), fromCharSet.c_str());
    if (iconv_converter == (iconv_t)-1)
    {
        if (errno == EINVAL)
        {
        	TagLib::debug("[encodeByteVectorToUTF16] not supported from " + fromCharSet + " to " + toCharSet);
        }
        else
        {
        	TagLib::debug("[encodeByteVectorToUTF16] unknown error:" + TagLib::String::number(errno));
        }
        outputTagLibString = TagLib::String(L'\0');
        return errno;
    }

    char   *in_chars_buf = inputByteVector.data();
    char   *src_ptr      = in_chars_buf;
    size_t src_size      = inputByteVector.size();

    //limit the src size to the half of the output buffer
    if (src_size > MAX_BUF_SIZE/2)
    {
    	src_size = MAX_BUF_SIZE/2;
    	src_ptr[MAX_BUF_SIZE/2] = '\0';
    }

    TagLib::String encoded_taglib_string;
    std::vector<char> output_buffer(MAX_BUF_SIZE); //alloc char buffer

    while (src_size > 0)
    {
        char*  dst_ptr  = (char *) &output_buffer[0];
        size_t dst_size = output_buffer.size();

        size_t res = ::iconv(iconv_converter, &src_ptr, &src_size, &dst_ptr, &dst_size);
        if (res == (size_t)-1)
        {
            if (errno == E2BIG)
            {
            	//JBH: E2BIG 7 Argument list too long
                //     ignore this error
                int shit = 7; //debug
            }
            else if (ignore_error)
            {
                // skip character
                ++src_ptr;
                --src_size;
            }
            else
            {
    			switch (errno)
    			{
    			    case EILSEQ: //JBH: 84 Illegal byte sequence
        				TagLib::debug("[encodeByteVectorToUTF16] invalid multibyte chars: " + TagLib::String::number(errno));
                        break;
    			    case EINVAL: //JBH: 22 Invalid argument
        				TagLib::debug("[encodeByteVectorToUTF16] invalid multibyte chars: " + TagLib::String::number(errno));
                        break;
    			    default:
        				TagLib::debug("[encodeByteVectorToUTF16] unknown error: " + TagLib::String::number(errno));
                        break;
    			}
                outputTagLibString = TagLib::String(L'\0');
    			return errno;
            }
        }

        int output_data_size   = output_buffer.size() - dst_size;

        /*
         *JBH: For "1" (0x61): From EUC-KR to UTF-16
         *     dst_chars_size == 4
         *     0xFE, 0xFF,            0x00, 0x61
         *     |<-- UTF-16 BOM -->|   |<-- 2 bytes for "1" -->|
         *
         *     NOTE: BOM is suffixed only for "UTF-16".
         *           Not suffixed for "UTF-16BE" and "UTF-16LE".
         */

    	std::vector<wchar_t> wchar_t_buf(output_data_size+1); //alloc wchar_t buffer
    	for (int i=0; i<output_data_size/2; i++)
    	{
    	    unsigned short c1 = (unsigned short) (output_buffer[i*2] << 8); c1 = c1 & 0xFF00;
    	    unsigned short c0 = (unsigned short) (output_buffer[i*2+1]);    c0 = c0 & 0x00FF;
    	    wchar_t_buf[i] = (wchar_t) (c0+c1);
    	    wchar_t_buf[i] = (wchar_t) (c0+c1) & 0x0000FFFF;
    	}
    	wchar_t_buf[output_data_size] = L'\0'; //EOS marking

        //The fucking old gcc (4.4.3) on U10.04 can not resolve "TagLib::String::Type::UTF16BE", but "TagLib::String::UTF16BE"
        TagLib::String partial_taglib_string = TagLib::String( (const wchar_t *)&wchar_t_buf[0], TagLib::String::UTF16BE );
        encoded_taglib_string.append(partial_taglib_string);
    }

    ::iconv_close(iconv_converter);

    encoded_taglib_string.swap(outputTagLibString);
    return 0; //JBH: 0 as Success
}

int CHARSETCONVERTER::encodeTagLibStringToUTF16(TagLib::String  inputTagLibString, TagLib::String &outputTagLibString, const std::string &fromCharSet, const std::string &toCharSet, bool ignore_error)
{
	TagLib::ByteVector inputByteVector = inputTagLibString.data(TagLib::String::Latin1);
	int res = encodeByteVectorToUTF16(inputByteVector, outputTagLibString, fromCharSet, toCharSet);
	return res;
}

/** Convert std::string @a s to a TagLib::String. */
TagLib::String CHARSETCONVERTER::StdStringtoTString(const std::string& s)
{
    int len = s.size();

    std::wstring ws = std::wstring(s.begin(), s.end());
    int len_ws = ws.size();
    const wchar_t* ws_cs = ws.c_str();

    TagLib::String ws_ts = TagLib::String(ws_cs, TagLib::String::UTF16BE);
    return ws_ts;
    //return TagLib::String(ws_cs, TagLib::String::UTF16BE);

#if 0
    std::string curLocale = setlocale(LC_ALL, "");
    const char* _Source = s.c_str();
    size_t _Dsize = mbstowcs(NULL, _Source, 0) + 1;
    wchar_t *_Dest = new wchar_t[_Dsize];
    wmemset(_Dest, 0, _Dsize);
    mbstowcs(_Dest,_Source,_Dsize);
    std::wstring result = _Dest;
    delete []_Dest;
    setlocale(LC_ALL, curLocale.c_str());
    return result;
#endif

}


int CHARSETCONVERTER::encodeByteVectorToUTF16_V2(TagLib::ByteVector inputByteVector, TagLib::String &outputTagLibString, const std::string &fromCharSet, const std::string &toCharSet, bool ignore_error)
{
    size_t input_length = inputByteVector.size();

    // We need a C string working copy that isn’t const
    char* copy = (char*) malloc(input_length + 1); // Terminating NUL
#ifdef _WIN32
    strncpy_s(copy, input_length, inputByteVector.data(), input_length);
#else
    strncpy(copy, inputByteVector.data(), input_length);
#endif
    copy[input_length + 1] = '\0';

    // Set up the encoding converter
    iconv_t converter    = ::iconv_open(toCharSet.c_str(), fromCharSet.c_str());
    size_t outbytes_left = 0;
    size_t inbytes_left  = input_length;

    if (converter == (iconv_t)-1)
    {
        if (errno == EINVAL)
        {
            TagLib::debug("[encodeByteVectorToUTF16_V2] not supported from " + fromCharSet + " to " + toCharSet);
        }
        else
        {
            TagLib::debug("[encodeByteVectorToUTF16_V2] unknown error:" + TagLib::String::number(errno));
        }
        outputTagLibString = TagLib::String(L'\0');
        return errno;
    }

    /*
     * There is no way to know how much space iconv() will need. So we keep allocating more and more memory as needed.
     * `current_size' keeps track of how large our memory blob is currently.
     * `outbuf' is the pointer to that memory blob.
     */
    size_t current_size = input_length + 1; // NUL
    char* outbuf        = NULL;
    char* inbuf         = copy; // Copy the pointer
    
    int errno_saved = 0;
    outbytes_left = current_size;
    while(true)
    {
        outbuf         = (char*) realloc(outbuf - (current_size - outbytes_left), current_size + 10);
        current_size  += 10;
        outbytes_left += 10;
    
        errno  = 0;
        errno_saved = 0;

        iconv(converter, &inbuf, &inbytes_left, &outbuf, &outbytes_left); // sets outbytes_left to 0 or very low values if not enough space (E2BIG)

        errno_saved = errno;
    
        if (errno_saved == E2BIG)
        {
        	//JBH: E2BIG 7 Argument list too long
        	//     For errno descriptions, run "error -l"
            continue;
        }
        else
        {
        	break;
        }
    }

    ::iconv_close(converter);
    free(copy);
    
    size_t count = current_size - outbytes_left;
    outbuf -= count; // iconv() advances the pointer!
    
    if (errno_saved != 0)
    {
        free(outbuf);
        TagLib::debug("[encodeByteVectorToUTF16_V2] errno_saved:" + TagLib::String::number(errno_saved));
        outputTagLibString = TagLib::String(L'\0');
        return errno;
    }
    
    int s1 = sizeof(std::basic_string<wchar_t>);
    std::vector<wchar_t> wchar_t_buf(count); //alloc wchar_t buffer
    for (int i=0; i< (int)count/2; i++)
    {
        unsigned short c1 = (unsigned short) (outbuf[i*2] << 8); c1 = c1 & 0xFF00;
        unsigned short c0 = (unsigned short) outbuf[i*2+1];      c0 = c0 & 0x00FF;
        //wchar_t_buf[i] = outbuf[i] + outbuf[i+1];
        wchar_t_buf[i] = (wchar_t) (c0+c1);
        wchar_t_buf[i] = (wchar_t) (c0+c1) & 0x0000FFFF;
    }
    free(outbuf);

    outputTagLibString = TagLib::String( (const wchar_t *)&wchar_t_buf[0], TagLib::String::UTF16BE );
    return 0;
}


std::string CHARSETCONVERTER::convertTagLibStringCharSet(TagLib::String  inputTagLibString, const std::string &toCharSet, bool ignore_error)
{
    iconv_t iconv_converter = ::iconv_open(toCharSet.c_str(), "UTF-16BE");
    if (iconv_converter == (iconv_t)-1)
    {
        if (errno == EINVAL)
        {
            TagLib::debug("[convertTagLibStringCharSet] not supported from UTF-16BE to " + toCharSet);
        }
        else
        {
            TagLib::debug("[convertTagLibStringCharSet] unknown error:" + TagLib::String::number(errno));
        }
        return std::string();
    }

    TagLib::ByteVector inputByteVector = inputTagLibString.data(TagLib::String::UTF16BE);
    char   *in_chars_buf = inputByteVector.data();
    char   *src_ptr      = in_chars_buf;
    size_t src_size      = inputByteVector.size();

    //limit the src size to the half of the output buffer
    if (src_size > MAX_BUF_SIZE/2)
    {
        src_size = MAX_BUF_SIZE/2;
        src_ptr[MAX_BUF_SIZE/2] = '\0';
    }

    std::string encoded_std_string;
    std::vector<char> output_buffer(MAX_BUF_SIZE); //alloc char buffer

    while (src_size > 0)
    {
        char*  dst_ptr  = (char *) &output_buffer[0];
        size_t dst_size = output_buffer.size();

        size_t res = ::iconv(iconv_converter, &src_ptr, &src_size, &dst_ptr, &dst_size);
        if (res == (size_t)-1)
        {
            if (errno == E2BIG)
            {
                //JBH: E2BIG 7 Argument list too long
                //     ignore this error
                int shit = 7; //debug
            }
            else if (ignore_error)
            {
                // skip character
                ++src_ptr;
                --src_size;
            }
            else
            {
                switch (errno)
                {
                    case EILSEQ: //JBH: 84 Illegal byte sequence
                        TagLib::debug("[convertTagLibStringCharSet] invalid multibyte chars: " + TagLib::String::number(errno));
                        break;
                    case EINVAL: //JBH: 22 Invalid argument
                        TagLib::debug("[convertTagLibStringCharSet] invalid multibyte chars: " + TagLib::String::number(errno));
                        break;
                    default:
                        TagLib::debug("[convertTagLibStringCharSet] unknown error: " + TagLib::String::number(errno));
                        break;
                }
                return std::string();
            }
        }

        int output_data_size   = output_buffer.size() - dst_size;
        std::string partial_std_string = std::string( (const char *)&output_buffer[0] );
        encoded_std_string.append(partial_std_string);
    }

    ::iconv_close(iconv_converter);

    return encoded_std_string;
}


std::string CHARSETCONVERTER::convertUTF16TagLibStringToUTF8StdString(TagLib::String  inputTagLibString, bool ignore_error)
{
    std::string utf8StdString = convertTagLibStringCharSet(inputTagLibString, "UTF-8", ignore_error);
    return utf8StdString;

#if 0
    iconv_t iconv_converter = ::iconv_open("UTF-8", "UTF-16BE");
    if (iconv_converter == (iconv_t)-1)
    {
        if (errno == EINVAL)
        {
            TagLib::debug("[encodeByteVectorToUTF16] not supported from UTF-16BE to UTF-8");
        }
        else
        {
            TagLib::debug("[encodeByteVectorToUTF16] unknown error:" + TagLib::String::number(errno));
        }
        return std::string();
    }

    TagLib::ByteVector inputByteVector = inputTagLibString.data(TagLib::String::UTF16BE);
    char   *in_chars_buf = inputByteVector.data();
    char   *src_ptr      = in_chars_buf;
    size_t src_size      = inputByteVector.size();

    //limit the src size to the half of the output buffer
    if (src_size > MAX_BUF_SIZE/2)
    {
        src_size = MAX_BUF_SIZE/2;
        src_ptr[MAX_BUF_SIZE/2] = '\0';
    }

    std::string encoded_std_string;
    std::vector<char> output_buffer(MAX_BUF_SIZE); //alloc char buffer

    while (src_size > 0)
    {
        char*  dst_ptr  = (char *) &output_buffer[0];
        size_t dst_size = output_buffer.size();

        size_t res = ::iconv(iconv_converter, &src_ptr, &src_size, &dst_ptr, &dst_size);
        if (res == (size_t)-1)
        {
            if (errno == E2BIG)
            {
                //JBH: E2BIG 7 Argument list too long
                //     ignore this error
                int shit = 7; //debug
            }
            else if (ignore_error)
            {
                // skip character
                ++src_ptr;
                --src_size;
            }
            else
            {
                switch (errno)
                {
                    case EILSEQ: //JBH: 84 Illegal byte sequence
                        TagLib::debug("[encodeByteVectorToUTF16] invalid multibyte chars: " + TagLib::String::number(errno));
                        break;
                    case EINVAL: //JBH: 22 Invalid argument
                        TagLib::debug("[encodeByteVectorToUTF16] invalid multibyte chars: " + TagLib::String::number(errno));
                        break;
                    default:
                        TagLib::debug("[encodeByteVectorToUTF16] unknown error: " + TagLib::String::number(errno));
                        break;
                }
                return std::string();
            }
        }

        int output_data_size   = output_buffer.size() - dst_size;
        std::string partial_std_string = std::string( (const char *)&output_buffer[0] );
        encoded_std_string.append(partial_std_string);
    }

    ::iconv_close(iconv_converter);

    return encoded_std_string;
#endif
}
