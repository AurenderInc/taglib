/**
 * Author: James Hwang
 * Copyright (c)  2019  aurender.com
 *
 */


#include <uchardet/uchardet.h>
#include <tdebug.h>

#include "charsetdetector.h"

using namespace TagLib;

float CHARSETDETECTOR::detectCharSet(const TagLib::ByteVector &dataByteVector, std::string &charSet)
{
    uchardet_t handle = uchardet_new();

    int retval = uchardet_handle_data(handle, dataByteVector.data(), dataByteVector.size());
    if (retval == HANDLE_DATA_RESULT_ERROR)
    {
        TagLib::debug("[CHARSETDETECTOR] Handle data error");
        return 0.0;
    }

    uchardet_data_end(handle);

    const char * cs         = uchardet_get_charset(handle);
    float        confidence = uchardet_get_confidence(handle);
    charSet = std::string(cs);
#if 0
    if (*cs)
        printf("CHARSETDETECTOR: { encoding=%s, confidence=%f }\n", cs, confidence);
    else
        printf("CHARSETDETECTOR: unknown\n");
#endif
    
    uchardet_delete(handle);

    return confidence;
}

float CHARSETDETECTOR::detectCharSet(const TagLib::String &dataTagLibString, std::string &charSet)
{
    uchardet_t handle = uchardet_new();

    int retval = uchardet_handle_data(handle, dataTagLibString.toCString(), dataTagLibString.size());
    if (retval == HANDLE_DATA_RESULT_ERROR)
    {
        TagLib::debug("[CHARSETDETECTOR] Handle data error");
        return 0.0;
    }

    uchardet_data_end(handle);

    const char * cs         = uchardet_get_charset(handle);
    float        confidence = uchardet_get_confidence(handle);

    charSet = std::string(cs);
#if 0
    if (*cs)
        printf("CHARSETDETECTOR: { encoding=%s, confidence=%f }\n", cs, confidence);
    else
        printf("CHARSETDETECTOR: unknown\n");
#endif

    uchardet_delete(handle);

    return confidence;
}