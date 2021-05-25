/* Copyright (C) 2003 Scott Wheeler <wheeler@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <limits>   //JBH add for numeric_limits for the old gcc
#include <stdint.h> //JBH add for uint64_t for the old gcc


#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>
//JBH <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#include <taglib.h>         //JBH add for TAGLIB_JBH_VERSION
#include <tdebuglistener.h> //JBH add for StdoutListener

/*
 *JBH: Windows Console uses UTF-16, so do not convert to UTF-8 for console display
 *     Bash (git bash or MSYS2) uses UTF-8, so convert to UTF-8 for Bash.
 */
#ifdef _WIN32
  //#undef JBH_USE_EMBEDDED_UNICODE_ENCODER
  #define CONSOLE_CHARSET "cp949"
#else
  #define CONSOLE_CHARSET "UTF-8"
#endif

#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
#include <charsetdetector.h>
#include <charsetconverter.h>
#endif
//JBH >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

using namespace std;

int main(int argc, char *argv[])
{
  TagLib::setDebugListener(new TagLib::StdoutListener()); //JBH add: Do not use DefaultListener (which outputs to QxtLogger).

//JBH ==========================================================================<
  cout << "TAGLIB_JBH_VERSION: " << TAGLIB_JBH_VERSION << endl;
  cout << "--------" << endl;
  cout << "std::numeric_limits<int>::max():          " << sizeof(int) << "bytes  "          << std::numeric_limits<int>::max() << endl;
  cout << "std::numeric_limits<unsigned int>::max(): " << sizeof(unsigned int) << "bytes  " << std::numeric_limits<unsigned int>::max() << endl;
  cout << "std::numeric_limits<int64_t>::max():      " << sizeof(int64_t) << "bytes  "      << std::numeric_limits<int64_t>::max() << endl;
  cout << "std::numeric_limits<uint64_t>::max():     " << sizeof(uint64_t) << "bytes  "     << std::numeric_limits<uint64_t>::max() << endl;
  cout << "std::numeric_limits<long>::max():         " << sizeof(long) << "bytes  "         << std::numeric_limits<long>::max() << endl;
  cout << "std::numeric_limits<unsigned long>::max():" << sizeof(unsigned long) << "bytes  "<< std::numeric_limits<unsigned long>::max() << endl;
  cout << "std::numeric_limits<size_t>::max():       " << sizeof(size_t) << "bytes  "       << std::numeric_limits<size_t>::max() << endl;
  cout << "--------" << endl;
//JBH ==========================================================================>

  for(int i = 1; i < argc; i++) {

    cout << "******************** \"" << argv[i] << "\" ********************" << endl;

    TagLib::FileRef f(argv[i]);

    if(!f.isNull() && f.tag()) {

      TagLib::Tag *tag = f.tag();

#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
      cout << "-- TAG (basic) --" << endl;
      cout << "title   - \"" << TagLib::CHARSETCONVERTER::convertTagLibStringCharSet( tag->title(),   CONSOLE_CHARSET )  << "\"" << endl;
      cout << "artist  - \"" << TagLib::CHARSETCONVERTER::convertTagLibStringCharSet( tag->artist(),  CONSOLE_CHARSET )  << "\"" << endl;
      cout << "album   - \"" << TagLib::CHARSETCONVERTER::convertTagLibStringCharSet( tag->album(),   CONSOLE_CHARSET )  << "\"" << endl;
      cout << "year    - \"" << tag->year()    << "\"" << endl;
      cout << "comment - \"" << TagLib::CHARSETCONVERTER::convertTagLibStringCharSet( tag->comment(), CONSOLE_CHARSET )  << "\"" << endl;
      cout << "track   - \"" << tag->track()   << "\"" << endl;
      cout << "genre   - \"" << tag->genre()   << "\"" << endl;
#else
      cout << "-- TAG (basic) --" << endl;
      cout << "title   - \"" << tag->title()   << "\"" << endl;
      cout << "artist  - \"" << tag->artist()  << "\"" << endl;
      cout << "album   - \"" << tag->album()   << "\"" << endl;
      cout << "year    - \"" << tag->year()    << "\"" << endl;
      cout << "comment - \"" << tag->comment() << "\"" << endl;
      cout << "track   - \"" << tag->track()   << "\"" << endl;
      cout << "genre   - \"" << tag->genre()   << "\"" << endl;
#endif
      TagLib::PropertyMap tags = f.file()->properties();

      unsigned int longest = 0;
      for(TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
        if (i->first.size() > longest) {
          longest = i->first.size();
        }
      }

      cout << "-- TAG (properties) --" << endl;
#if 0
      for(TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
        for(TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
          cout << left << std::setw(longest) << i->first << " - " << '"' << *j << '"' << endl;
        }
      }
#endif
      //JBH ==========================================================================>
      for (TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i)
      {
        for(TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j)
        {
#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
          std::string charset;
          TagLib::String contentTagLibString = *j;
          std::string contentConsole;
          float confidence = TagLib::CHARSETDETECTOR::detectCharSet(contentTagLibString, charset);
          //if ( (confidence>0.8) && (charset=="UTF-16") )
          {
              contentConsole = TagLib::CHARSETCONVERTER::convertTagLibStringCharSet(contentTagLibString, CONSOLE_CHARSET);
              cout << left << std::setw(longest) << i->first << " - " << '"' << contentConsole << '"' << endl;
          }
          //else
          //{
          //    cout << left << std::setw(longest) << i->first << " - " << '"' << *j << '"' << endl;
          //}
#else
          cout << left << std::setw(longest) << i->first << " - " << '"' << *j << '"' << endl;
#endif
        }
      }

      TagLib::PropertyMap map = tag->properties();
      cout << " Unsupported " << endl;
      cout << map.unsupportedData() << endl;
      //JBH ==========================================================================>

    }

    if(!f.isNull() && f.audioProperties()) {

      TagLib::AudioProperties *properties = f.audioProperties();

      int seconds = properties->length() % 60;
      int minutes = (properties->length() - seconds) / 60;

      cout << "-- AUDIO --" << endl;
      cout << "bitrate     - " << properties->bitrate() << endl;
      cout << "sample rate - " << properties->sampleRate() << endl;
      cout << "channels    - " << properties->channels() << endl;
      cout << "length      - " << minutes << ":" << setfill('0') << setw(2) << seconds << endl;
    }
  }
  return 0;
}
