/**
 * Author: James Hwang
 * Copyright (c)  2019  aurender.com
 *
 */


#include <stdio.h>
#include <string>
#include <tstring.h>
#include <tdebug.h>

#include "smartencodingoptionchecker.h"

using namespace TagLib;

SmartEncodingOptionChecker* SmartEncodingOptionChecker::checkerInstance = NULL;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

SmartEncodingOptionChecker *SmartEncodingOptionChecker::instance()
{
    if (checkerInstance == NULL)
    {
        checkerInstance = new SmartEncodingOptionChecker();
    }
    return checkerInstance;
}

bool SmartEncodingOptionChecker::enabled()
{
    return optionEnabled;
}

inline bool fileExists (const std::string& name)
{
    if (FILE *file = fopen(name.c_str(), "r"))
    {
        fclose(file);
        return true;
    } else
    {
        return false;
    }   
}

SmartEncodingOptionChecker::SmartEncodingOptionChecker()
{
#ifdef _WIN32
    if ( fileExists("./smartLangEncode") )
#else
    if ( fileExists("/srv/widealab/smartLangEncode") )
#endif
    {
        optionEnabled = true;
    }
    else
    {
        optionEnabled = false;
    }
    TagLib::String opt_str = optionEnabled ? TagLib::String("TRUE") : TagLib::String("FALSE");
    TagLib::debug("[SmartEncodingOptionChecker] set optionEnabled: " + opt_str);
}

SmartEncodingOptionChecker::~SmartEncodingOptionChecker()
{
}
