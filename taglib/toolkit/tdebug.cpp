/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#define USE_APP_LOGGER //JBH: define it as a cmake option, such as [cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_DIRECTORY -DUSE_APP_LOGGER=ON -DUSE_QXTLOGGER=ON -DUSE_JBHLOGGER=OFF -DWITH_ASF=ON -DWITH_MP4=ON -DBUILD_EXAMPLES=OFF]
//JBH_COMMENTED_OUT #if !defined(NDEBUG) || defined(TRACE_IN_RELEASE)

#include "tdebug.h"
#include "tstring.h"
#include "tdebuglistener.h"
#include "tutils.h"

#include <bitset>
#include <cstdio>
#include <cstdarg>

namespace TagLib
{
  // The instance is defined in tdebuglistener.cpp.
  extern DebugListener *debugListener;

  void debug(const String &s)
  {
#ifdef USE_APP_LOGGER
    debugListener->printMessage(s);
#else
  #define JBH_PRINT_ON_STDOUT
  #ifdef  JBH_PRINT_ON_STDOUT
    //std::cerr << s.toCString(true /*encode to UTF-8*/) << std::endl;
    std::cout << s.toCString(true /*encode to UTF-8*/) << std::endl;
  #else
    debugListener->printMessage("TagLib: " + s + "\n");
  #endif
#endif
  }

  void debugData(const ByteVector &v)
  {
    for(unsigned int i = 0; i < v.size(); ++i) {
      const std::string bits = std::bitset<8>(v[i]).to_string();
      const String msg = Utils::formatString(
        "*** [%u] - char '%c' - int %d, 0x%02x, 0b%s\n",
        i, v[i], v[i], v[i], bits.c_str());

      debugListener->printMessage(msg);
    }
  }
}

//JBH_COMMENTED_OUT #endif
