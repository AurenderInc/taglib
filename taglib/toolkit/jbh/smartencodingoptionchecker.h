/**
 * Author: James Hwang
 * Copyright (c)  2019  aurender.com
 *
 */



#ifndef SMARTENCODINGOPTIONCHECKER_H
#define SMARTENCODINGOPTIONCHECKER_H

#include <taglib_export.h>

namespace TagLib  {

class TAGLIB_EXPORT SmartEncodingOptionChecker
{
public:
	static SmartEncodingOptionChecker *instance();
    bool enabled();

private:
    SmartEncodingOptionChecker();
    ~SmartEncodingOptionChecker();
    static SmartEncodingOptionChecker *checkerInstance;
    bool optionEnabled;

};

} //namespace TagLib

#endif //SMARTENCODINGOPTIONCHECKER_H
