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

#include <tbytevectorlist.h>
#include <id3v2tag.h>
#include <tdebug.h>
#include <tstringlist.h>

#include "commentsframe.h"
#include "tpropertymap.h"

//JBH <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
#include "charsetdetector.h"
#include "charsetconverter.h"
#endif
//JBH >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

using namespace TagLib;
using namespace ID3v2;

class CommentsFrame::CommentsFramePrivate
{
public:
  CommentsFramePrivate() : textEncoding(String::Latin1) {}
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
CommentsFrame::CommentsFrame(String::Type encoding, std::string orgCharSet, float orgCharSetConfidence) :
  Frame("COMM", orgCharSet, orgCharSetConfidence),
  d(new CommentsFramePrivate())
{
  d->textEncoding = encoding;
}

CommentsFrame::CommentsFrame(const ByteVector &data, std::string orgCharSet, float orgCharSetConfidence) :
  Frame(data, orgCharSet, orgCharSetConfidence),
  d(new CommentsFramePrivate())
{
  setData(data);
}
#else
CommentsFrame::CommentsFrame(String::Type encoding) :
  Frame("COMM"),
  d(new CommentsFramePrivate())
{
  d->textEncoding = encoding;
}

CommentsFrame::CommentsFrame(const ByteVector &data) :
  Frame(data),
  d(new CommentsFramePrivate())
{
  setData(data);
}
#endif

CommentsFrame::~CommentsFrame()
{
  delete d;
}

String CommentsFrame::toString() const
{
  return d->text;
}

ByteVector CommentsFrame::language() const
{
  return d->language;
}

String CommentsFrame::description() const
{
  return d->description;
}

String CommentsFrame::text() const
{
  return d->text;
}

void CommentsFrame::setLanguage(const ByteVector &languageEncoding)
{
  d->language = languageEncoding.mid(0, 3);
}

void CommentsFrame::setDescription(const String &s)
{
  d->description = s;
}

void CommentsFrame::setText(const String &s)
{
  d->text = s;
}

String::Type CommentsFrame::textEncoding() const
{
  return d->textEncoding;
}

void CommentsFrame::setTextEncoding(String::Type encoding)
{
  //JBH: possible enum values: {Latin1, UTF16, UTF16BE, UTF8, UTF16LE}
  d->textEncoding = encoding;
}

PropertyMap CommentsFrame::asProperties() const
{
  String key = description().upper();
  PropertyMap map;
  if(key.isEmpty() || key == "COMMENT")
    map.insert("COMMENT", text());
  else
    map.insert("COMMENT:" + key, text());
  return map;
}

CommentsFrame *CommentsFrame::findByDescription(const ID3v2::Tag *tag, const String &d) // static
{
  ID3v2::FrameList comments = tag->frameList("COMM");

  for(ID3v2::FrameList::ConstIterator it = comments.begin();
      it != comments.end();
      ++it)
  {
    CommentsFrame *frame = dynamic_cast<CommentsFrame *>(*it);
    if(frame && frame->description() == d)
      return frame;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void CommentsFrame::parseFields(const ByteVector &data)
{
  if(data.size() < 5) {
    debug("A comment frame must contain at least 5 bytes.");
    return;
  }

  //JBH: The first byte of a field is always the encoding type in id3v2 by the spec?
  d->textEncoding = String::Type(data[0]);
  d->language = data.mid(1, 3);

  int byteAlign = d->textEncoding == String::Latin1 || d->textEncoding == String::UTF8 ? 1 : 2;

  ByteVectorList l = ByteVectorList::split(data.mid(4), textDelimiter(d->textEncoding), byteAlign, 2);

  if(l.size() == 2) {
    if(d->textEncoding == String::Latin1) {
//JBH <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef JBH_USE_EMBEDDED_UNICODE_ENCODER
      int res;
      std::string charset;
      float confidence;
      String encoded_string;

      //confidence = CHARSETDETECTOR::detectCharSet(data, charset);
      //confidence = CHARSETDETECTOR::detectCharSet(l.front(), charset);
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
      d->description = String(l.front(), d->textEncoding);
      d->text = String(l.back(), d->textEncoding);
    }
  }
}

ByteVector CommentsFrame::renderFields() const
{
  ByteVector v;

  String::Type encoding = d->textEncoding;

  encoding = checkTextEncoding(d->description, encoding);
  encoding = checkTextEncoding(d->text, encoding);

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
CommentsFrame::CommentsFrame(const ByteVector &data, Header *h, std::string orgCharSet, float orgCharSetConfidence) :
  Frame(h, orgCharSet, orgCharSetConfidence),
  d(new CommentsFramePrivate())
{
  parseFields(fieldData(data));
}
#else
CommentsFrame::CommentsFrame(const ByteVector &data, Header *h) :
  Frame(h),
  d(new CommentsFramePrivate())
{
  parseFields(fieldData(data));
}
#endif
//JBH ==========================================================================>
