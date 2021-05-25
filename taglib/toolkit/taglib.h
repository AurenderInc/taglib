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

#ifndef TAGLIB_H
#define TAGLIB_H

#include "taglib_config.h"

#define TAGLIB_MAJOR_VERSION 1
#define TAGLIB_MINOR_VERSION 11
#define TAGLIB_PATCH_VERSION 1

//JBH ==========================================================================<
#define TAGLIB_JBH_VERSION "19.1212.0"
//18.0716.0: tagreader -v prints the JBH version number
//18.0722.0: RIFF: allow wrong data size
//18.1106.0: RIFF: increased the LIST chunk limit to 10K (Some wav files ripped by ACSRipper have a size of 9730)
//18.1113.0: RIFF: do not check validness of the chunk name, to allow "null" chunk name
//19.0110.0: MP4(M4A): do not take 0-length block to Atoms
//19.0203.0: RIFF: support/allow/read Over-4GB file, but read the audio property only, not the tags
//19.0206.0: add the Embedded Unicode Encoding feature
//19.0210.0: Embedded Unicode Encoding feature is working, merged to the main branch.
//19.0216.0: Embedded Unicode Encoding feature --> official release.
//19.0223.0: ON/OFF option to Smart Language Encoding --> "/srv/widealab/smartLangEncode"
//19.0419.0: Support dsf/dsdiff  512 for Aurender W20SE
//19.0502.0: Support dsf/dsdiff 1024 for Aurender W20SE
//19.0806.0: Comments on flacfile.cpp
//19.0808.0: flac: allow 0-length blocks only up to 10
//19.1212.0: add pinyin4cpp (pinyin4cpp is not related with taglib (and no change in taglib itself), but let's mark taglib here to denote the Deps version up)
//JBH ==========================================================================>


/*
 *===============================================================================
 *JBH: TagLib limits:
 *  32bit Linux:
 *   std::numeric_limits<int>::max():          4bytes  2147483647
 *   std::numeric_limits<unsigned int>::max(): 4bytes  4294967295
 *   std::numeric_limits<int64_t>::max():      8bytes  9223372036854775807
 *   std::numeric_limits<uint64_t>::max():     8bytes  18446744073709551615
 *   std::numeric_limits<long>::max():         4bytes  2147483647
 *   std::numeric_limits<unsigned long>::max():4bytes  4294967295
 *   std::numeric_limits<size_t>::max():       4bytes  4294967295
 *
 *  64bit Linux:
 *   std::numeric_limits<int>::max():          4bytes  2147483647
 *   std::numeric_limits<unsigned int>::max(): 4bytes  4294967295
 *   std::numeric_limits<int64_t>::max():      8bytes  9223372036854775807
 *   std::numeric_limits<uint64_t>::max():     8bytes  18446744073709551615
 *   std::numeric_limits<long>::max():         8bytes  9223372036854775807
 *   std::numeric_limits<unsigned long>::max():8bytes  18446744073709551615
 *   std::numeric_limits<size_t>::max():       8bytes  18446744073709551615
 *
 *   So,
 *   Use ideally "uint64_t" for "size", "file length", "offset"
 *   However, TagLib uses ultimately "fread()" in linux, which returns "size_t"
 *   Using "uint64_t" is meaningless, unless the fread() in linux support the expanded size_t via the LARGE_FILE_SUPPORT feature
 *===============================================================================
 */

 /*
  * JBH: Size Limit of RIFF (WAVE, AIFF) by the "SPEC"
  *
  *========================================================================================================
  * The max size of AIFF is 4GB by the AIFF "spec".
  * Do not try reading over 4GB aiff files!!
  *
  * The max size of WAVE is 4GB by the WAV "spec".
  * Do not try reading over 4GB wav files!!
  *
  *
  * Limitations: https://en.wikipedia.org/wiki/WAV
  * The WAV format is limited to files that are less than 4 GB,
  * because of its use of a 32-bit unsigned integer to record the file size header (some programs limit the file size to 2 GB).
  * 
  * The W64 format was therefore created for use in Sound Forge. Its 64-bit header allows for much longer recording times.
  * The RF64 (https://en.wikipedia.org/wiki/RF64) format specified by the European Broadcasting Union has also been created to solve this problem.
  *
  *========================================================================================================
  */

//JBH <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
#define CHARSETDETECTOR_CONFIDENCE_THRESHOLD 0.5         //JBH add
#define CHARSETCONVERTER_TO_CHARSET          "UTF-16BE"  //JBH add
#endif
//JBH >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 1)) || defined(__clang__)
#define TAGLIB_IGNORE_MISSING_DESTRUCTOR _Pragma("GCC diagnostic ignored \"-Wnon-virtual-dtor\"")
#else
#define TAGLIB_IGNORE_MISSING_DESTRUCTOR
#endif

#if (defined(_MSC_VER) && _MSC_VER >= 1600)
#define TAGLIB_CONSTRUCT_BITSET(x) static_cast<unsigned long long>(x)
#else
#define TAGLIB_CONSTRUCT_BITSET(x) static_cast<unsigned long>(x)
#endif

#include <string>

//! A namespace for all TagLib related classes and functions

/*!
 * This namespace contains everything in TagLib.  For projects working with
 * TagLib extensively it may be convenient to add a
 * \code
 * using namespace TagLib;
 * \endcode
 */

namespace TagLib {

  class String;

  // These integer types are deprecated. Do not use them.

  typedef wchar_t            wchar;   // Assumed to be sufficient to store a UTF-16 char.
  typedef unsigned char      uchar;
  typedef unsigned short     ushort;
  typedef unsigned int       uint;
  typedef unsigned long      ulong;
  typedef unsigned long long ulonglong;

  /*!
   * Unfortunately std::wstring isn't defined on some systems, (i.e. GCC < 3)
   * so I'm providing something here that should be constant.
   */
  typedef std::basic_string<wchar_t> wstring;
}

/*!
 * \mainpage TagLib
 *
 * \section intro Introduction
 *
 * TagLib is a library for reading and editing audio meta data, commonly know as \e tags.
 *
 * Features:
 * - A clean, high level, C++ API to handling audio meta data.
 * - Format specific APIs for advanced API users.
 * - ID3v1, ID3v2, APE, FLAC, Xiph, iTunes-style MP4 and WMA tag formats.
 * - MP3, MPC, FLAC, MP4, ASF, AIFF, WAV, TrueAudio, WavPack, Ogg FLAC, Ogg Vorbis, Speex and Opus file formats.
 * - Basic audio file properties such as length, sample rate, etc.
 * - Long term binary and source compatibility.
 * - Extensible design, notably the ability to add other formats or extend current formats as a library user.
 * - Full support for unicode and internationalized tags.
 * - Dual <a href="http://www.mozilla.org/MPL/MPL-1.1.html">MPL</a> and
 *   <a href="http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html">LGPL</a> licenses.
 * - No external toolkit dependencies.
 *
 * \section why Why TagLib?
 *
 * TagLib originally was written to provide an updated and improved ID3v2 implementation in C++ for use
 * in a variety of Open Source projects.  Since development began in 2002 and the 1.0 release in 2004
 * it has expanded to cover a wide variety of tag and file formats and is used in a wide variety of
 * Open Source and proprietary applications.  It now supports a variety of UNIXes, including Apple's OS
 * X, as well as Microsoft Windows.
 *
 * \section commercial Usage in Commercial Applications
 *
 * TagLib's licenses \e do allow usage within propriety (\e closed) applications, however TagLib is \e not
 * public domain.  Please note the requirements of the LGPL or MPL, and adhere to at least one of them.
 * In simple terms, you must at a minimum note your usage of TagLib, note the licensing terms of TagLib and
 * if you make changes to TagLib publish them.  Please review the licenses above before using TagLib in your
 * software.  Note that you may choose either the MPL or the LGPL, you do not have to fulfill the
 * requirements of both.
 *
 * \section installing Installing TagLib
 *
 * Please see the <a href="http://taglib.org/">TagLib website</a> for the latest
 * downloads.
 *
 * TagLib can be built using the CMake build system. TagLib installs a taglib-config and pkg-config file to
 * make it easier to integrate into various build systems.  Note that TagLib's include install directory \e must
 * be included in the header include path. Simply adding <taglib/tag.h> will \e not work.
 *
 * \section start Getting Started
 *
 * TagLib provides both simple, abstract APIs which make it possible to ignore the differences between tagging
 * formats and format specific APIs which allow programmers to work with the features of specific tagging
 * schemes.  There is a similar abstraction mechanism for AudioProperties.
 *
 * The best place to start is with the <b>Class Hierarchy</b> linked at the top of the page.  The File and
 * AudioProperties classes and their subclasses are the core of TagLib.  The FileRef class is also a convenient
 * way for using a value-based handle.
 *
 * \note When working with FileRef please consider that it has only the most basic (extension-based) file
 * type resolution.  Please see its documentation on how to plug in more advanced file type resolution.  (Such
 * resolution may be part of later TagLib releases by default.)
 *
 * Here's a very simple example with TagLib:
 *
 * \code
 *
 * TagLib::FileRef f("Latex Solar Beef.mp3");
 * TagLib::String artist = f.tag()->artist(); // artist == "Frank Zappa"
 *
 * f.tag()->setAlbum("Fillmore East");
 * f.save();
 *
 * TagLib::FileRef g("Free City Rhymes.ogg");
 * TagLib::String album = g.tag()->album(); // album == "NYC Ghosts & Flowers"
 *
 * g.tag()->setTrack(1);
 * g.save();
 *
 * \endcode
 *
 * More examples can be found in the \e examples directory of the source distribution.
 *
 * \section Contact
 *
 * Questions about TagLib should be directed to the TagLib mailing list, not directly to the author.
 *
 *  - <a href="http://taglib.org/">TagLib Homepage</a>
 *  - <a href="https://mail.kde.org/mailman/listinfo/taglib-devel">TagLib Mailing List (taglib-devel@kde.org)</a>
 *
 * \author <a href="https://github.com/taglib/taglib/blob/master/AUTHORS">TagLib authors</a>.
 */

#endif
