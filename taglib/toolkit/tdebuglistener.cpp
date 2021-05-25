/***************************************************************************
    copyright            : (C) 2013 by Tsuda Kageyu
    email                : tsuda.kageyu@gmail.com
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

//JBH ==========================================================================<
//JBH: defs of USE_APP_LOGGER, USE_QXTLOGGER, USE_JBHLOGGER
# include <config.h>
//JBH ==========================================================================>

#include "tdebuglistener.h"

#include <iostream>
#include <bitset>

#ifdef _WIN32
# include <windows.h>
#endif

//JBH ==========================================================================>
//#define USE_APP_LOGGER //JBH: define it as a cmake option, such as [cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_DIRECTORY -DUSE_APP_LOGGER=ON -DUSE_QXTLOGGER=ON -DUSE_JBHLOGGER=OFF -DWITH_ASF=ON -DWITH_MP4=ON -DBUILD_EXAMPLES=OFF]
#ifdef USE_APP_LOGGER
  #ifdef USE_QXTLOGGER
    #include <QxtLogger>
  #else  //USE_JBHLOGGER
    #include "jbh/Logger.h"
  #endif
#endif
//JBH ==========================================================================>
using namespace TagLib;

namespace
{
  class DefaultListener : public DebugListener
  {
  public:
    virtual void printMessage(const String &msg)
    {
//JBH ==========================================================================<
#ifdef USE_APP_LOGGER
  #ifdef USE_QXTLOGGER
      qxtLog->debug() << "[TagLib] " << TStringToQString(msg); //JBH: TStringToQString defined in tstring.h
  #else  //USE_JBHLOGGER
      tDebug() << "[TagLib] " << TStringToQString(msg);
  #endif
#else
    #ifdef _WIN32
      const wstring wstr = msg.toWString();
      const int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
      if(len != 0) {
        std::vector<char> buf(len);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &buf[0], len, NULL, NULL);
        std::cerr << std::string(&buf[0]) << std::endl;
      }
    #else

      std::cerr << msg.toCString(true /*encode to UTF-8*/) << std::endl;
    #endif 
#endif
//JBH ==========================================================================>
    }
  };

  DefaultListener defaultListener;

  //JBH NOTE: "class StdoutListener : public DebugListener" has been added/defined in tdebuglistener.h
}

namespace TagLib
{
  DebugListener *debugListener = &defaultListener;

  DebugListener::DebugListener()
  {
  }

  DebugListener::~DebugListener()
  {
  }

  void setDebugListener(DebugListener *listener)
  {
    if(listener)
      debugListener = listener;
    else
      debugListener = &defaultListener;
  }
}
