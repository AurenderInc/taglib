/***************************************************************************
    copyright            : (C) 2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.com
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
#include <tdebug.h>

#include "apeitem.h"

using namespace TagLib;
using namespace APE;

class APE::Item::ItemPrivate
{
public:
  ItemPrivate() :
    type(Text),
    readOnly(false) {}

  Item::ItemTypes type;
  String key;
  ByteVector value;
  StringList text;
  bool readOnly;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

APE::Item::Item() :
  d(new ItemPrivate())
{
}

APE::Item::Item(const String &key, const String &value) :
  d(new ItemPrivate())
{
  d->key = key;
  d->text.append(value);
}

APE::Item::Item(const String &key, const StringList &values) :
  d(new ItemPrivate())
{
  d->key = key;
  d->text = values;
}

APE::Item::Item(const String &key, const ByteVector &value, bool binary) :
  d(new ItemPrivate())
{
  d->key = key;
  if(binary) {
    d->type = Binary;
    d->value = value;
  }
  else {
    d->text.append(value);
  }
}

APE::Item::Item(const Item &item) :
  d(new ItemPrivate(*item.d))
{
}

APE::Item::~Item()
{
  delete d;
}

Item &APE::Item::operator=(const Item &item)
{
  Item(item).swap(*this);
  return *this;
}

void APE::Item::swap(Item &item)
{
  using std::swap;

  swap(d, item.d);
}

void APE::Item::setReadOnly(bool readOnly)
{
  d->readOnly = readOnly;
}

bool APE::Item::isReadOnly() const
{
  return d->readOnly;
}

void APE::Item::setType(APE::Item::ItemTypes val)
{
  d->type = val;
}

APE::Item::ItemTypes APE::Item::type() const
{
  return d->type;
}

String APE::Item::key() const
{
  return d->key;
}

ByteVector APE::Item::binaryData() const
{
  return d->value;
}

void APE::Item::setBinaryData(const ByteVector &value)
{
  d->type = Binary;
  d->value = value;
  d->text.clear();
}

ByteVector APE::Item::value() const
{
  // This seems incorrect as it won't be actually rendering the value to keep it
  // up to date.

  return d->value;
}

void APE::Item::setKey(const String &key)
{
  d->key = key;
}

void APE::Item::setValue(const String &value)
{
  d->type = Text;
  d->text = value;
  d->value.clear();
}

void APE::Item::setValues(const StringList &value)
{
  d->type = Text;
  d->text = value;
  d->value.clear();
}

void APE::Item::appendValue(const String &value)
{
  d->type = Text;
  d->text.append(value);
  d->value.clear();
}

void APE::Item::appendValues(const StringList &values)
{
  d->type = Text;
  d->text.append(values);
  d->value.clear();
}

int APE::Item::size() const
{
  int result = 8 + d->key.size() + 1;
  switch(d->type) {
    case Text:
      if(!d->text.isEmpty()) {
        StringList::ConstIterator it = d->text.begin();

        result += it->data(String::UTF8).size();
        it++;
        for(; it != d->text.end(); ++it)
          result += 1 + it->data(String::UTF8).size();
      }
      break;

    case Binary:
    case Locator:
      result += d->value.size();
      break;
  }
  return result;
}

StringList APE::Item::toStringList() const
{
  return d->text;
}

StringList APE::Item::values() const
{
  return d->text;
}

String APE::Item::toString() const
{
  if(d->type == Text && !isEmpty())
    return d->text.front();
  else
    return String();
}

bool APE::Item::isEmpty() const
{
  switch(d->type) {
    case Text:
      if(d->text.isEmpty())
        return true;
      if(d->text.size() == 1 && d->text.front().isEmpty())
        return true;
      return false;
    case Binary:
    case Locator:
      return d->value.isEmpty();
    default:
      return false;
  }
}

void APE::Item::parse(const ByteVector &data)
{
  // 11 bytes is the minimum size for an APE item

  if(data.size() < 11) {
    debug("APE::Item::parse() -- no data in item");
    return;
  }

  /*
   * JBH NOTE: an APETageItem structure
   *
   *           valueLen(40)   |  flags(Text or Binary)  |   Key("Artist")      |   Value("Chopin")
   *           4bytes            4bytes                     variableLen bytes      valueLen bytes (40bytes in this case)
   *
   * The followings are "Standard" APE tag fields (Keys) defined in the APE2.0 specification.
   *     L"Title"
   *     L"Artist"
   *     L"Album"
   *     L"Comment"
   *     L"Year"
   *     L"Track"
   *     L"Genre"                JBH NOTE: Leo's ape reads only upto here. It can not read the cover art.
   *     L"Cover Art (front)"    JBH NOTE: some bullshits say "Cover Art (front)" and others say "Cover Art (Front)"
   *     L"Notes"
   *     L"Lyrics"
   *     L"Copyright"
   *     L"Buy URL"
   *     L"Artist URL"
   *     L"Publisher URL"
   *     L"File URL"
   *     L"Copyright URL"
   *     L"Media Jukebox Metadata"
   *     L"Tool Name"
   *     L"Tool Version"
   *     L"Peak Level"
   *     L"Replay Gain (radio)"
   *     L"Replay Gain (album)"
   *     L"Composer"
   *     L"Keywords"
   *
   * JBH NOTE: In the real world, APE files authored by some fucking program have some weired-out-of-spec Key , such as "ALBUM ARTIST"
   */

  const unsigned int valueLength  = data.toUInt(0, false);
  const unsigned int flags        = data.toUInt(4, false);

  // An item key can contain ASCII characters from 0x20 up to 0x7E, not UTF-8.
  // We assume that the validity of the given key has been checked.

//JBH ==========================================================================<
#define JBH_READ_SHORTER
#ifndef JBH_READ_SHORTER
  d->key = String(&data[8], String::Latin1);
#else
  /*
   * JBH FIXED: The original code make/get a key String ("Artist") using the whole ByteVector which contain the WHOLE TagItems data.
   *            This is apparently ineffective, and cause a crash "std::bad_alloc" on some systems.
   * 
   *            When we get a key string,
   *            Limiting ByteVector to a smaller fixed size (for example, 22 instead of 42247029 "the whole TagItems' ByteVector size")
   *            - would be much performative, and
   *            - most importantly, prevent the critiacal crash "std::bad_alloc" on some systems.
   *
   *            Well, Even though the length of "Key" is variable/unkown,
   *            We can assume that the maximum length would 22 which is the size of L"Media Jukebox Metadata".
   *
   *            22 might be enough, but Lets assume it as 480 for safety.
   *
   */
  // d->key = String(data.mid(8), String::Latin1);    //JBH NOTE: The original code make/get a key String ("Artist") using the whole ByteVector.
  uint dataSize = data.size();
  if (dataSize > 500)
  {
    /*
     * JBH NOTE: If larger than 500, shorten it to 480,
     *           which prevents from reading the whole ByteVector of (possibly) a large size such as X mega bytes due to the cover art.
     */
    const ByteVector keyByteVectorShortened = data.mid(8, 480); //JBH NOTE: Lets limit to use only the first 480 bytes.
    d->key = String(keyByteVectorShortened, String::Latin1);
  }
  else
  {
    /*
     * JBH NOTE: If smaller than 500, read it full using the whole ByteVector
     */
    d->key = String(data.mid(8), String::Latin1);
  }
  //JBH debug <
  uint keySize = d->key.size();
  String keyString = d->key;
  //JBH debug >
#endif
//JBH ==========================================================================>

//JBH ==========================================================================<
#define JBH_SPECIAL_TREAT_APE_COVERART
#ifdef JBH_SPECIAL_TREAT_APE_COVERART
  /*
   * JBH FIXED: Some fucking APE files does not set flags (TEXT|BINARY) properly, so a special treatment is required.
   *            Moreover, some bullshits say "Cover Art (front)" and others say "Cover Art (Front)".
   *            Check if "Key == Cover Art", set type to BINARY and read into d->value, not d->text.
   *                   
   */
  if ( d->key.startsWith(String("Cover Art")) )
  {
    //JBH FIXME: Do not read in the cover art binary for now, util tags/taglibfile.cpp is ready to process it.
    //           Also note that some APE file has 40M of the cover art binary, which takes much time to readin.
    //           As of 2015/04/25, changed to read in the cover art binary.
    setReadOnly(flags & 1);
    setType(Binary);
    const ByteVector value = data.mid(8 + d->key.size() + 1, valueLength);
    d->value = value;
    return;
  }
#endif
//JBH ==========================================================================>

  const ByteVector value = data.mid(8 + d->key.size() + 1, valueLength);

  setReadOnly(flags & 1);
  setType(ItemTypes((flags >> 1) & 3));

  if(Text == d->type)
    d->text = StringList(ByteVectorList::split(value, '\0'), String::UTF8);
  else
    d->value = value;
}

ByteVector APE::Item::render() const
{
  ByteVector data;
  unsigned int flags = ((d->readOnly) ? 1 : 0) | (d->type << 1);
  ByteVector value;

  if(isEmpty())
    return data;

  if(d->type == Text) {
    StringList::ConstIterator it = d->text.begin();

    value.append(it->data(String::UTF8));
    it++;
    for(; it != d->text.end(); ++it) {
      value.append('\0');
      value.append(it->data(String::UTF8));
    }
    d->value = value;
  }
  else
    value.append(d->value);

  data.append(ByteVector::fromUInt(value.size(), false));
  data.append(ByteVector::fromUInt(flags, false));
  data.append(d->key.data(String::Latin1));
  data.append(ByteVector('\0'));
  data.append(value);

  return data;
}
