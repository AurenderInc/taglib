/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2006 by Urs Fleisch
    email                : ufleisch@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
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

#include "unsynchronizedlyricsframe.h"
#include <tbytevectorlist.h>
#include <id3v2tag.h>
#include <tdebug.h>
#include <tpropertymap.h>

//JBH <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
#include "charsetdetector.h"
#include "charsetconverter.h"
#endif
//JBH >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

using namespace TagLib;
using namespace ID3v2;

class UnsynchronizedLyricsFrame::UnsynchronizedLyricsFramePrivate
{
public:
  UnsynchronizedLyricsFramePrivate() : textEncoding(String::Latin1) {}
  String::Type textEncoding;
  ByteVector language;
  String description;
  String text;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

//JBH ==========================================================================<
#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
UnsynchronizedLyricsFrame::UnsynchronizedLyricsFrame(String::Type encoding, std::string orgCharSet, float orgCharSetConfidence) :
  Frame("USLT", orgCharSet, orgCharSetConfidence),
  d(new UnsynchronizedLyricsFramePrivate())
{
  d->textEncoding = encoding;
}

UnsynchronizedLyricsFrame::UnsynchronizedLyricsFrame(const ByteVector &data, std::string orgCharSet, float orgCharSetConfidence) :
  Frame(data, orgCharSet, orgCharSetConfidence),
  d(new UnsynchronizedLyricsFramePrivate())
{
  setData(data);
}
#else
UnsynchronizedLyricsFrame::UnsynchronizedLyricsFrame(String::Type encoding) :
  Frame("USLT"),
  d(new UnsynchronizedLyricsFramePrivate())
{
  d->textEncoding = encoding;
}

UnsynchronizedLyricsFrame::UnsynchronizedLyricsFrame(const ByteVector &data) :
  Frame(data),
  d(new UnsynchronizedLyricsFramePrivate())
{
  setData(data);
}
#endif
//JBH ==========================================================================>

UnsynchronizedLyricsFrame::~UnsynchronizedLyricsFrame()
{
  delete d;
}

String UnsynchronizedLyricsFrame::toString() const
{
  return d->text;
}

ByteVector UnsynchronizedLyricsFrame::language() const
{
  return d->language;
}

String UnsynchronizedLyricsFrame::description() const
{
  return d->description;
}

String UnsynchronizedLyricsFrame::text() const
{
  return d->text;
}

void UnsynchronizedLyricsFrame::setLanguage(const ByteVector &languageEncoding)
{
  d->language = languageEncoding.mid(0, 3);
}

void UnsynchronizedLyricsFrame::setDescription(const String &s)
{
  d->description = s;
}

void UnsynchronizedLyricsFrame::setText(const String &s)
{
  d->text = s;
}


String::Type UnsynchronizedLyricsFrame::textEncoding() const
{
  return d->textEncoding;
}

void UnsynchronizedLyricsFrame::setTextEncoding(String::Type encoding)
{
  //JBH: possible enum values: {Latin1, UTF16, UTF16BE, UTF8, UTF16LE}
  d->textEncoding = encoding;
}

PropertyMap UnsynchronizedLyricsFrame::asProperties() const
{
  PropertyMap map;
  String key = description().upper();
  if(key.isEmpty() || key == "LYRICS")
    map.insert("LYRICS", text());
  else
    map.insert("LYRICS:" + key, text());
  return map;
}

UnsynchronizedLyricsFrame *UnsynchronizedLyricsFrame::findByDescription(const ID3v2::Tag *tag, const String &d) // static
{
  ID3v2::FrameList lyrics = tag->frameList("USLT");

  for(ID3v2::FrameList::ConstIterator it = lyrics.begin(); it != lyrics.end(); ++it){
    UnsynchronizedLyricsFrame *frame = dynamic_cast<UnsynchronizedLyricsFrame *>(*it);
    if(frame && frame->description() == d)
      return frame;
  }
  return 0;
}
////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void UnsynchronizedLyricsFrame::parseFields(const ByteVector &data)
{
  /*
   *=================================================================================
   *JBH: ID3v2 Lyrics frame spec?:
   *     data[0]   --> [textEncoding: 1byte(0x00:Latin1,UTF8, 0x01:UTF16)]
   *     data[1~3] --> [language: 3bytes]
   *     data[4,5] --> [UTF16-BOM: 2bytes]
   *     data[6,7] --> [textDelimeter: 1byte(Latin1,UTF8) or 2bytes(UTF16)]
   *     data[8,9] --> [UTF16-BOM: 2bytes]
   *     data[A,B] --> [UTF16-BOM: 2bytes]
   *     data[C~]  --> [Lyrics....]
   *=================================================================================
   */

  /*
   *JBH: "갈색 추억.mp3" 가사 frame
   *
   *  data[0]: 0x01        textEncoding --> 1 --> UTF16
   *
   *  data[1]: 0x6b 'k'    language --> "kor"
   *  data[2]: 0x6f 'o'    
   *  data[3]: 0x72 'r'    
   *
   *  data[4]: 0xff        UTF16-BOM
   *  data[5]: 0xfe        
   *
   *  data[6]: 0x00        Frame::textDelimiter(UTF16) --> 2 bytes ByteVector --> ByteVector(2, '\0')
   *  data[7]: 0x00        
   *
   *  data[8]: 0xff        UTF16-BOM
   *  data[9]: 0xfe        
   *
   *  data[A]: 0xff        UTF16-BOM
   *  data[B]: 0xfe        
   *
   *  0x6c 희     0xd76c
   *  0xd7
   *
   *  0xf8 미     0xbbf8
   *  0xbb
   *
   *  0x5c 한
   *  0xd5
   *
   *  0x20 ' '   <space>
   *  0x00
   *
   *  0x08 갈
   *  0xac
   *
   *  0xc9 색
   *  0xc0
   *
   *  0x20 ' '   <space>
   *  0x00
   */

  if(data.size() < 5) {
    debug("An unsynchronized lyrics frame must contain at least 5 bytes.");
    return;
  }

  //JBH: The first byte of a field is always the encoding type in id3v2 by the spec?
  //JBH: TagLib::String::Latin1(0), TagLib::String::UTF8(0), TagLib::String::UTF16(1)
  d->textEncoding = String::Type(data[0]);
  //JBH: data[i] --> value @ "data->offset + i"

  //JBH: language --> "kor"
  d->language = data.mid(1, 3);

  int byteAlign
    = d->textEncoding == String::Latin1 || d->textEncoding == String::UTF8 ? 1 : 2;
    //JBH: 1 byte align for Latin1/UTF8, 2 bytes align for UTF16

  ByteVectorList l =
    ByteVectorList::split(data.mid(4), textDelimiter(d->textEncoding), byteAlign, 2);


  if(l.size() == 2) {
    if(d->textEncoding == String::Latin1) {
//JBH <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
      int res;
      std::string charset;
      float confidence;
      String encoded_string;
  
      //confidence = CHARSETDETECTOR::detectCharSet(data, charset);
      confidence = CHARSETDETECTOR::detectCharSet(l.front(), charset);
      charset    = getOrgCharSet();
      confidence = getOrgCharSetConfidence();

      if ( (confidence > CHARSETDETECTOR_CONFIDENCE_THRESHOLD) && (charset != "UNDETECTED") && (charset != "UNKNOWN") && (charset != "NOUSE") && (charset != "ASCII") && (charset != "WINDOWS-1252") && (charset != "UTF-8") )
      {
        res = CHARSETCONVERTER::encodeByteVectorToUTF16(l.front(), encoded_string, charset, CHARSETCONVERTER_TO_CHARSET);
        if (res ==0)
        {
          d->description = encoded_string;
        }
        else
        {
          //fallback upon failure
          d->description = Tag::latin1StringHandler()->parse(l.front());
        }
      }
      else
      {
        d->description = Tag::latin1StringHandler()->parse(l.front());
      }

      //confidence = CHARSETDETECTOR::detectCharSet(l.back(), charset);
      if ( (confidence > CHARSETDETECTOR_CONFIDENCE_THRESHOLD) && (charset != "UNDETECTED") && (charset != "UNKNOWN") && (charset != "NOUSE") && (charset != "ASCII") && (charset != "WINDOWS-1252") && (charset != "UTF-8") )
      {
        res = CHARSETCONVERTER::encodeByteVectorToUTF16(l.back(), encoded_string, charset, CHARSETCONVERTER_TO_CHARSET);
        if (res ==0)
        {
          d->text = encoded_string;
        }
        else
        {
          //fallback upon failure
          d->text = Tag::latin1StringHandler()->parse(l.back());
        }
      }
      else
      {
        d->text = Tag::latin1StringHandler()->parse(l.back());
      }
#else
      /*
       * JBH: callback the client-side-defined unicode encoder, if registered at the init stage of TagLib.
       *      EX: at taglibfile.cpp@kid3, TagLib::ID3v2::Tag::setLatin1StringHandler(m_textCodecStringHandlerForID3v2); //JBH: register our own unicode encoder to TagLib, so that TagLib will call back.
       *
       * JBH: All "Non-Unicode" strings, such as EUC-KR and real Latin1, are marked "Latin1" in taglib.
       */
      d->description = Tag::latin1StringHandler()->parse(l.front());
      d->text = Tag::latin1StringHandler()->parse(l.back());
#endif
//JBH >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>   
    } else {
      //JBH: already unicode, no need to encode.
      d->description = String(l.front(), d->textEncoding);
      d->text = String(l.back(), d->textEncoding);
    }
  }
}

ByteVector UnsynchronizedLyricsFrame::renderFields() const
{
  StringList sl;
  sl.append(d->description);
  sl.append(d->text);

  const String::Type encoding = checkTextEncoding(sl, d->textEncoding);

  ByteVector v;

  v.append(char(encoding));
  v.append(d->language.size() == 3 ? d->language : "XXX");
  v.append(d->description.data(encoding));
  v.append(textDelimiter(encoding));
  v.append(d->text.data(encoding));

  return v;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

//JBH ==========================================================================<
#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
UnsynchronizedLyricsFrame::UnsynchronizedLyricsFrame(const ByteVector &data, Header *h, std::string orgCharSet, float orgCharSetConfidence) :
  Frame(h, orgCharSet, orgCharSetConfidence),
  d(new UnsynchronizedLyricsFramePrivate())
{
  parseFields(fieldData(data));
}
#else
UnsynchronizedLyricsFrame::UnsynchronizedLyricsFrame(const ByteVector &data, Header *h) :
  Frame(h),
  d(new UnsynchronizedLyricsFramePrivate())
{
  parseFields(fieldData(data));
}
#endif
//JBH ==========================================================================>
