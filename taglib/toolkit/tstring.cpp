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

#include <cerrno>
#include <climits>

#include <utf8-cpp/checked.h>

#include <tdebug.h>
#include <tstringlist.h>
#include <trefcounter.h>
#include <tutils.h>

#include "tstring.h"

namespace
{
  using namespace TagLib;

  // Returns the native format of std::wstring.
  String::Type wcharByteOrder()
  {
    if(Utils::systemByteOrder() == Utils::LittleEndian)
      return String::UTF16LE;
    else
      return String::UTF16BE;
  }

  // Converts a Latin-1 string into UTF-16(without BOM/CPU byte order)
  // and copies it to the internal buffer.
  /*
   *JBH: "Converts" is misleading. No unicode conversion occurs here. This just expands/copies 1 byte of s[n] into 2-bytes of data[n], with 0x00 prefixed.
   */
  /*
   *JBH NOTE: For the equal opposite operation of copyFromLatin1 (supposedly "copyFromUTF16toLatin1"), use toCString().
   */  
  void copyFromLatin1(std::wstring &data, const char *s, size_t length)
  {
    //JBH: length is the number of "bytes" meaninigfull in s[]. length is not the number of characters
    data.resize(length);

    for(size_t i = 0; i < length; ++i)
      data[i] = static_cast<unsigned char>(s[i]);

    /*
     *JBH:
     *    Ex1:
     *    s[]="갈색 추억TPE1", length=9, so meaningful s[] is "갈색 추억" which consists of 9 (char) bytes and 5 characters.
     *    length is the number (9) of "bytes" meaninigfull in s[]. length is not the number (5) of characters in s[].
     *    The input s[] is a EUC-KR (Windows949) characters array.
     *
     *    '갈  s[0] b0  -->  data[0] 0x00b0
     *         s[1] a5  -->  data[1] 0x00a5
     *    '색' s[2] bb  -->  data[2] 0x00bb
     *         s[3] f6  -->  data[3] 0x00f6
     *    ' '  s[4] 20  -->  data[4] 0x0020
     *    '추' s[5] c3  -->  data[5] 0x00c3
     *         s[6] df  -->  data[6] 0x00df
     *    '억' s[7] be  -->  data[7] 0x00be
     *         s[8] ef  -->  data[8] 0x00ef
     *
     *    data[] will be displayed L"°¥»ö Ãß¾ï" (looks broken), while s[] remains "갈색 추억"
     *
     *
     *    Ex2:
     *    s[]="BalladUSLT", length=6, so meaningful s[] is "Ballad" which consists of 6 (char) bytes and 6 characters.
     *
     *    'B' s[0] 0x42  -->  data[0] 0x0042
     *    'a' s[0] 0x61  -->  data[1] 0x0061
     *    'l' s[0] 0x6c  -->  data[2] 0x006c
     *    'l' s[0] 0x6c  -->  data[3] 0x006c
     *    'a' s[0] 0x61  -->  data[4] 0x0061
     *    'd' s[0] 0x64  -->  data[5] 0x0064
     *
     *    data[] will be displayed L"Ballad", while s[] remains "Ballad"
     */
  }

  // Converts a UTF-8 string into UTF-16(without BOM/CPU byte order)
  // and copies it to the internal buffer.
  void copyFromUTF8(std::wstring &data, const char *s, size_t length)
  {
    /*
     *JBH: Example: Weston Non-1-Byte UTF8 encoded string of Western Languages:
     *     "Dvořák: Cello Concerto"
     *
     * 44 76 6f c5 99 c3 a1 6b 3a 20 43 65 6c 6c 6f 20 43 6f 6e 63 65 72 74 6f
     * D  v  o  ř     á     k  :     C  e  l  l  o     C  o  n  c  e  r  t  o
     */

    data.resize(length);

    try {
      const std::wstring::iterator dstEnd = utf8::utf8to16(s, s + length, data.begin());
      data.resize(dstEnd - data.begin());
    }
    catch(const utf8::exception &e) {
      const String message(e.what());
      //JBH too many logs    debug("String::copyFromUTF8() - UTF8-CPP error: " + message);
      data.clear();
    }
  }

  // Helper functions to read a UTF-16 character from an array.
  template <typename T>
  unsigned short nextUTF16(const T **p);

  template <>
  unsigned short nextUTF16<wchar_t>(const wchar_t **p)
  {
    return static_cast<unsigned short>(*(*p)++);
  }

  template <>
  unsigned short nextUTF16<char>(const char **p)
  {
    union {
      unsigned short w;
      char c[2];
    } u;
    u.c[0] = *(*p)++;
    u.c[1] = *(*p)++;
    return u.w;
  }

  // Converts a UTF-16 (with BOM), UTF-16LE or UTF16-BE string into
  // UTF-16(without BOM/CPU byte order) and copies it to the internal buffer.
  template <typename T>
  void copyFromUTF16(std::wstring &data, const T *s, size_t length, String::Type t)
  {
    /*
     *JBH: This is typically called for a file path, (not for frame/tag which is mostly encoded by UTF8)
     *
     *     L"D:/OpenShare/00 Music1/Mstislav Rostropovich -  Dvořák Cello Concerto (2017) [Hi-Res]/01. Cello Concerto in B Minor, Op. 104 I. Allegro.flac"
     */

    bool swap;
    if(t == String::UTF16) {
      //JBH: UTF16, but LE/BE not determined yet case
      if(length < 1) {
        debug("String::copyFromUTF16() - Invalid UTF16 string. Too short to have a BOM.");
        return;
      }

      const unsigned short bom = nextUTF16(&s);
      if(bom == 0xfeff)
        swap = false; // Same as CPU endian. No need to swap bytes.
      else if(bom == 0xfffe)
        swap = true;  // Not same as CPU endian. Need to swap bytes.
      else {
        debug("String::copyFromUTF16() - Invalid UTF16 string. BOM is broken.");
        return;
      }

      length--;
    }
    else {
      //JBH: UTF16, and LE/BE determined case
      swap = (t != wcharByteOrder());
    }

    data.resize(length);
    for(size_t i = 0; i < length; ++i) {
      const unsigned short c = nextUTF16(&s);
      if(swap)
        data[i] = Utils::byteSwap(c);
      else
        data[i] = c;
    }
  }
}

namespace TagLib {

class String::StringPrivate : public RefCounter
{
public:
  StringPrivate() :
    RefCounter() {}

  /*!
   * Stores string in UTF-16. The byte order depends on the CPU endian.
   */
  TagLib::wstring data;
  //JBH: TagLib::wstring is defined in taglib.h, such as "typedef std::basic_string<wchar_t> wstring"

  /*!
   * This is only used to hold the the most recent value of toCString().
   */
  std::string cstring;
};

String String::null;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

String::String() :
  d(new StringPrivate())
{
}

String::String(const String &s) :
  d(s.d)
{
  d->ref();
}

String::String(const std::string &s, Type t) :
  d(new StringPrivate())
{
  if(t == Latin1)
    copyFromLatin1(d->data, s.c_str(), s.length());
  else if(t == String::UTF8)
    copyFromUTF8(d->data, s.c_str(), s.length());
  else {
    debug("String::String() -- std::string should not contain UTF16.");
  }
}

String::String(const wstring &s, Type t) :
  d(new StringPrivate())
{
  if(t == UTF16 || t == UTF16BE || t == UTF16LE) {
    // This looks ugly but needed for the compatibility with TagLib1.8.
    // Should be removed in TabLib2.0.
    if (t == UTF16BE)
      t = wcharByteOrder();
    else if (t == UTF16LE)
      t = (wcharByteOrder() == UTF16LE ? UTF16BE : UTF16LE);

    copyFromUTF16(d->data, s.c_str(), s.length(), t);
  }
  else {
    debug("String::String() -- TagLib::wstring should not contain Latin1 or UTF-8.");
  }
}

String::String(const wchar_t *s, Type t) :
  d(new StringPrivate())
{
  if(t == UTF16 || t == UTF16BE || t == UTF16LE) {
    // This looks ugly but needed for the compatibility with TagLib1.8.
    // Should be removed in TabLib2.0.
    if (t == UTF16BE)
      t = wcharByteOrder();
    else if (t == UTF16LE)
      t = (wcharByteOrder() == UTF16LE ? UTF16BE : UTF16LE);

    copyFromUTF16(d->data, s, ::wcslen(s), t);
  }
  else {
    debug("String::String() -- const wchar_t * should not contain Latin1 or UTF-8.");
  }
}

String::String(const char *s, Type t) :
  d(new StringPrivate())
{
  if(t == Latin1)
    copyFromLatin1(d->data, s, ::strlen(s));
  else if(t == String::UTF8)
    copyFromUTF8(d->data, s, ::strlen(s));
  else {
    debug("String::String() -- const char * should not contain UTF16.");
  }
}

String::String(wchar_t c, Type t) :
  d(new StringPrivate())
{
  if(t == UTF16 || t == UTF16BE || t == UTF16LE)
    copyFromUTF16(d->data, &c, 1, t);
  else {
    debug("String::String() -- wchar_t should not contain Latin1 or UTF-8.");
  }
}

String::String(char c, Type t) :
  d(new StringPrivate())
{
  if(t == Latin1)
    copyFromLatin1(d->data, &c, 1);
  else if(t == String::UTF8)
    copyFromUTF8(d->data, &c, 1);
  else {
    debug("String::String() -- char should not contain UTF16.");
  }
}

String::String(const ByteVector &v, Type t) :
  d(new StringPrivate())
{
  if(v.isEmpty())
    return;

  if(t == Latin1)
    copyFromLatin1(d->data, v.data(), v.size());
  else if(t == UTF8)
    copyFromUTF8(d->data, v.data(), v.size());
  else
    copyFromUTF16(d->data, v.data(), v.size() / 2, t);

  // If we hit a null in the ByteVector, shrink the string again.
  d->data.resize(::wcslen(d->data.c_str()));
}

////////////////////////////////////////////////////////////////////////////////

String::~String()
{
  if(d->deref())
    delete d;
}

std::string String::to8Bit(bool unicode) const
{
  const ByteVector v = data(unicode ? UTF8 : Latin1);
  return std::string(v.data(), v.size());
}

TagLib::wstring String::toWString() const
{
  return d->data;
}

const char *String::toCString(bool unicode) const
{
  //JBH NOTE: This toCString() could be used for the equal opposite of copyFromLatin1().
  d->cstring = to8Bit(unicode);
  return d->cstring.c_str();
}

const wchar_t *String::toCWString() const
{
  return d->data.c_str();
}

String::Iterator String::begin()
{
  detach();
  return d->data.begin();
}

String::ConstIterator String::begin() const
{
  return d->data.begin();
}

String::Iterator String::end()
{
  detach();
  return d->data.end();
}

String::ConstIterator String::end() const
{
  return d->data.end();
}

int String::find(const String &s, int offset) const
{
  return static_cast<int>(d->data.find(s.d->data, offset));
}

int String::rfind(const String &s, int offset) const
{
  return static_cast<int>(d->data.rfind(s.d->data, offset));
}

StringList String::split(const String &separator) const
{
  StringList list;
  for(int index = 0;;) {
    int sep = find(separator, index);
    if(sep < 0) {
      list.append(substr(index, size() - index));
      break;
    }
    else {
      list.append(substr(index, sep - index));
      index = sep + separator.size();
    }
  }
  return list;
}

bool String::startsWith(const String &s) const
{
  if(s.length() > length())
    return false;

  return substr(0, s.length()) == s;
}

String String::substr(unsigned int position, unsigned int n) const
{
  if(position == 0 && n >= size())
    return *this;
  else
    return String(d->data.substr(position, n));
}

String &String::append(const String &s)
{
  detach();
  d->data += s.d->data;
  return *this;
}

String & String::clear()
{
  *this = String();
  return *this;
}

String String::upper() const
{
  String s;
  s.d->data.reserve(size());

  for(ConstIterator it = begin(); it != end(); ++it) {
    if(*it >= 'a' && *it <= 'z')
      s.d->data.push_back(*it + 'A' - 'a');
    else
      s.d->data.push_back(*it);
  }

  return s;
}

unsigned int String::size() const
{
  return static_cast<unsigned int>(d->data.size());
}

unsigned int String::length() const
{
  return size();
}

bool String::isEmpty() const
{
  return d->data.empty();
}

bool String::isNull() const
{
  return d == null.d;
}

ByteVector String::data(Type t) const
{
  switch(t)
  {
  case Latin1:
    {
      ByteVector v(size(), 0);
      char *p = v.data();

      for(ConstIterator it = begin(); it != end(); ++it)
        *p++ = static_cast<char>(*it);

      return v;
    }
  case UTF8:
    {
      ByteVector v(size() * 4, 0);

      try {
        const ByteVector::Iterator dstEnd = utf8::utf16to8(begin(), end(), v.begin());
        v.resize(static_cast<unsigned int>(dstEnd - v.begin()));
      }
      catch(const utf8::exception &e) {
        const String message(e.what());
        //JBH too many logs    debug("String::data() - UTF8-CPP error: " + message);
        v.clear();
      }

      return v;
    }
  case UTF16:
    {
      ByteVector v(2 + size() * 2, 0);
      char *p = v.data();

      // We use little-endian encoding here and need a BOM.

      *p++ = '\xff';
      *p++ = '\xfe';

      for(ConstIterator it = begin(); it != end(); ++it) {
        *p++ = static_cast<char>(*it & 0xff);
        *p++ = static_cast<char>(*it >> 8);
      }

      return v;
    }
  case UTF16BE:
    {
      ByteVector v(size() * 2, 0);
      char *p = v.data();

      for(ConstIterator it = begin(); it != end(); ++it) {
        *p++ = static_cast<char>(*it >> 8);
        *p++ = static_cast<char>(*it & 0xff);
      }

      return v;
    }
  case UTF16LE:
    {
      ByteVector v(size() * 2, 0);
      char *p = v.data();

      for(ConstIterator it = begin(); it != end(); ++it) {
        *p++ = static_cast<char>(*it & 0xff);
        *p++ = static_cast<char>(*it >> 8);
      }

      return v;
    }
  default:
    {
      debug("String::data() - Invalid Type value.");
      return ByteVector();
    }
  }
}

int String::toInt() const
{
  return toInt(0);
}

int String::toInt(bool *ok) const
{
  const wchar_t *begin = d->data.c_str();
  wchar_t *end;
  errno = 0;
  const long value = ::wcstol(begin, &end, 10);

  // Has wcstol() consumed the entire string and not overflowed?
  if(ok) {
    *ok = (errno == 0 && end > begin && *end == L'\0');
    *ok = (*ok && value > INT_MIN && value < INT_MAX);
  }

  return static_cast<int>(value);
}

String String::stripWhiteSpace() const
{
  static const wchar_t *WhiteSpaceChars = L"\t\n\f\r ";

  const size_t pos1 = d->data.find_first_not_of(WhiteSpaceChars);
  if(pos1 == std::wstring::npos)
    return String();

  const size_t pos2 = d->data.find_last_not_of(WhiteSpaceChars);
  return substr(static_cast<unsigned int>(pos1), static_cast<unsigned int>(pos2 - pos1 + 1));
}

bool String::isLatin1() const
{
  for(ConstIterator it = begin(); it != end(); ++it) {
    if(*it >= 256)
      return false;
  }
  return true;
}

bool String::isAscii() const
{
  for(ConstIterator it = begin(); it != end(); ++it) {
    if(*it >= 128)
      return false;
  }
  return true;
}

String String::number(int n) // static
{
  return Utils::formatString("%d", n);
}

//JBH ==========================================================================<
String String::number(unsigned int n) // static
{
  return Utils::formatString("%u", n);
}
String String::number(long n) // static
{
  return Utils::formatString("%ld", n);
}
String String::number(unsigned long n) // static
{
  return Utils::formatString("%lu", n);
}
//JBH ==========================================================================<

wchar_t &String::operator[](int i)
{
  detach();
  return d->data[i];
}

const wchar_t &String::operator[](int i) const
{
  return d->data[i];
}

bool String::operator==(const String &s) const
{
  return (d == s.d || d->data == s.d->data);
}

bool String::operator!=(const String &s) const
{
  return !(*this == s);
}

bool String::operator==(const char *s) const
{
  const wchar_t *p = toCWString();

  while(*p != L'\0' || *s != '\0') {
    if(*p++ != static_cast<unsigned char>(*s++))
      return false;
  }
  return true;
}

bool String::operator!=(const char *s) const
{
  return !(*this == s);
}

bool String::operator==(const wchar_t *s) const
{
  return (d->data == s);
}

bool String::operator!=(const wchar_t *s) const
{
  return !(*this == s);
}

String &String::operator+=(const String &s)
{
  detach();

  d->data += s.d->data;
  return *this;
}

String &String::operator+=(const wchar_t *s)
{
  detach();

  d->data += s;
  return *this;
}

String &String::operator+=(const char *s)
{
  detach();

  for(int i = 0; s[i] != 0; i++)
    d->data += static_cast<unsigned char>(s[i]);
  return *this;
}

String &String::operator+=(wchar_t c)
{
  detach();

  d->data += c;
  return *this;
}

String &String::operator+=(char c)
{
  detach();

  d->data += static_cast<unsigned char>(c);
  return *this;
}

String &String::operator=(const String &s)
{
  String(s).swap(*this);
  return *this;
}

String &String::operator=(const std::string &s)
{
  String(s).swap(*this);
  return *this;
}

String &String::operator=(const wstring &s)
{
  String(s).swap(*this);
  return *this;
}

String &String::operator=(const wchar_t *s)
{
  String(s).swap(*this);
  return *this;
}

String &String::operator=(char c)
{
  String(c).swap(*this);
  return *this;
}

String &String::operator=(wchar_t c)
{
  String(c, wcharByteOrder()).swap(*this);
  return *this;
}

String &String::operator=(const char *s)
{
  String(s).swap(*this);
  return *this;
}

String &String::operator=(const ByteVector &v)
{
  String(v).swap(*this);
  return *this;
}

void String::swap(String &s)
{
  using std::swap;

  swap(d, s.d);
}

bool String::operator<(const String &s) const
{
  return (d->data < s.d->data);
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void String::detach()
{
  if(d->count() > 1)
    String(d->data.c_str()).swap(*this);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

const String::Type String::WCharByteOrder = wcharByteOrder();
}

////////////////////////////////////////////////////////////////////////////////
// related non-member functions
////////////////////////////////////////////////////////////////////////////////

const TagLib::String operator+(const TagLib::String &s1, const TagLib::String &s2)
{
  TagLib::String s(s1);
  s.append(s2);
  return s;
}

const TagLib::String operator+(const char *s1, const TagLib::String &s2)
{
  TagLib::String s(s1);
  s.append(s2);
  return s;
}

const TagLib::String operator+(const TagLib::String &s1, const char *s2)
{
  TagLib::String s(s1);
  s.append(s2);
  return s;
}

std::ostream &operator<<(std::ostream &s, const TagLib::String &str)
{
  s << str.to8Bit();
  return s;
}

