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

#include <algorithm>
#include <vector>

#include <tbytevector.h>
#include <tdebug.h>
#include <tstring.h>

#include "rifffile.h"
#include "riffutils.h"

#define JBH_USE_OLD_CODE

using namespace TagLib;

struct Chunk
{
  ByteVector   name;
  unsigned int offset;
  unsigned int size;
  unsigned int padding;
};

class RIFF::File::FilePrivate
{
public:
  FilePrivate(Endianness endianness) :
    endianness(endianness),
    size(0),
    sizeOffset(0) {}

  const Endianness endianness;

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

  unsigned int size;
  long sizeOffset;

//JBH ==========================================================================<
#ifdef JBH_USE_OLD_CODE
  ByteVector type;
  ByteVector format;
#endif
//JBH ==========================================================================>

  std::vector<Chunk> chunks;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

RIFF::File::~File()
{
  delete d;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

RIFF::File::File(FileName file, Endianness endianness) :
  TagLib::File(file),
  d(new FilePrivate(endianness))
{
  if(isOpen())
    read();
}

RIFF::File::File(IOStream *stream, Endianness endianness) :
  TagLib::File(stream),
  d(new FilePrivate(endianness))
{
  if(isOpen())
    read();
}

unsigned int RIFF::File::riffSize() const
{
  return d->size;
}

unsigned int RIFF::File::chunkCount() const
{
  return static_cast<unsigned int>(d->chunks.size());
}

unsigned int RIFF::File::chunkDataSize(unsigned int i) const
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkDataSize() - Index out of range. Returning 0.");
    return 0;
  }

  return d->chunks[i].size;
}

unsigned int RIFF::File::chunkOffset(unsigned int i) const
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkOffset() - Index out of range. Returning 0.");
    return 0;
  }

  return d->chunks[i].offset;
}

unsigned int RIFF::File::chunkPadding(unsigned int i) const
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkPadding() - Index out of range. Returning 0.");
    return 0;
  }

  return d->chunks[i].padding;
}

ByteVector RIFF::File::chunkName(unsigned int i) const
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkName() - Index out of range. Returning an empty vector.");
    return ByteVector();
  }

  return d->chunks[i].name;
}

ByteVector RIFF::File::chunkData(unsigned int i)
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkData() - Index out of range. Returning an empty vector.");
    return ByteVector();
  }

  seek(d->chunks[i].offset);
  return readBlock(d->chunks[i].size);
}

void RIFF::File::setChunkData(unsigned int i, const ByteVector &data)
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::setChunkData() - Index out of range.");
    return;
  }

  // Now update the specific chunk

  std::vector<Chunk>::iterator it = d->chunks.begin();
  std::advance(it, i);

  const long long originalSize = static_cast<long long>(it->size) + it->padding;

  writeChunk(it->name, data, it->offset - 8, it->size + it->padding + 8);

  it->size    = data.size();
  it->padding = data.size() % 2;

  const long long diff = static_cast<long long>(it->size) + it->padding - originalSize;

  // Now update the internal offsets

  for(++it; it != d->chunks.end(); ++it)
    it->offset += static_cast<int>(diff);

  // Update the global size.

  updateGlobalSize();
}

void RIFF::File::setChunkData(const ByteVector &name, const ByteVector &data)
{
  setChunkData(name, data, false);
}

void RIFF::File::setChunkData(const ByteVector &name, const ByteVector &data, bool alwaysCreate)
{
  if(d->chunks.empty()) {
    debug("RIFF::File::setChunkData - No valid chunks found.");
    return;
  }

  if(alwaysCreate && name != "LIST") {
    debug("RIFF::File::setChunkData - alwaysCreate should be used for only \"LIST\" chunks.");
    return;
  }

  if(!alwaysCreate) {
    for(unsigned int i = 0; i < d->chunks.size(); i++) {
      if(d->chunks[i].name == name) {
        setChunkData(i, data);
        return;
      }
    }
  }

  // Couldn't find an existing chunk, so let's create a new one.

  // Adjust the padding of the last chunk to place the new chunk at even position.

  Chunk &last = d->chunks.back();

  long offset = last.offset + last.size + last.padding;
  if(offset & 1) {
    if(last.padding == 1) {
      last.padding = 0; // This should not happen unless the file is corrupted.
      offset--;
      removeBlock(offset, 1);
    }
    else {
      insert(ByteVector("\0", 1), offset, 0);
      last.padding = 1;
      offset++;
    }
  }

  // Now add the chunk to the file.

  writeChunk(name, data, offset, 0);

  // And update our internal structure

  Chunk chunk;
  chunk.name    = name;
  chunk.size    = data.size();
  chunk.offset  = offset + 8;
  chunk.padding = data.size() % 2;

  d->chunks.push_back(chunk);

  // Update the global size.

  updateGlobalSize();
}

void RIFF::File::removeChunk(unsigned int i)
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::removeChunk() - Index out of range.");
    return;
  }

  std::vector<Chunk>::iterator it = d->chunks.begin();
  std::advance(it, i);

  const unsigned int removeSize = it->size + it->padding + 8;
  removeBlock(it->offset - 8, removeSize);
  it = d->chunks.erase(it);

  for(; it != d->chunks.end(); ++it)
    it->offset -= removeSize;

  // Update the global size.

  updateGlobalSize();
}

void RIFF::File::removeChunk(const ByteVector &name)
{
  for(int i = static_cast<int>(d->chunks.size()) - 1; i >= 0; --i) {
    if(d->chunks[i].name == name)
      removeChunk(i);
  }
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void RIFF::File::read()
{
// Original  before JBH update
#if 0
  const bool bigEndian = (d->endianness == BigEndian);

  long offset = tell();

  offset += 4;
  d->sizeOffset = offset;

  seek(offset);
  d->size = readBlock(4).toUInt(bigEndian);

  offset += 8;

  // + 8: chunk header at least, fix for additional junk bytes
  while(offset + 8 <= length()) {

    seek(offset);
    const ByteVector   chunkName = readBlock(4);
    const unsigned int chunkSize = readBlock(4).toUInt(bigEndian);

    if(!isValidChunkName(chunkName)) {
      debug("RIFF::File::read() -- Chunk '" + chunkName + "' has invalid ID");
      setValid(false);
      break;
    }

    if(static_cast<long long>(offset) + 8 + chunkSize > length()) {
      debug("RIFF::File::read() -- Chunk '" + chunkName + "' has invalid size (larger than the file size)");
      setValid(false);
      break;
    }

    Chunk chunk;
    chunk.name    = chunkName;
    chunk.size    = chunkSize;
    chunk.offset  = offset + 8;
    chunk.padding = 0;

    offset = chunk.offset + chunk.size;

    // Check padding

    if(offset & 1) {
      seek(offset);
      const ByteVector iByte = readBlock(1);
      if(iByte.size() == 1 && iByte[0] == '\0') {
        chunk.padding = 1;
        offset++;
      }
    }

    d->chunks.push_back(chunk);
  }
#else
  /*
   * JBH NOTE: This RIFF::read() reads just the chunk infos (chunk name, size, offset) and constructs chunk blocks.
   *           The actual "parsing" of each chunk block will be done either in wavfile.cpp or aifffile.cpp.
   *           In other word, the "read()" here is to classify chunks.
   *
   *           WAVE: There could be 4 chunks: "fmt ", "data", "LIST", "ID3 ".
   *           AIFF: There could be 4 chunks: "COMM", "SSND", "LIST", "ID3 ".
   */

  const bool bigEndian = (d->endianness == BigEndian);

  long offset = tell();
  long filelength = length(); //JBH add
  int numNullChunks = 0; //JBH add
  bool isOver4GB = false; //JBH add

  if ( filelength > 4294967295 ) 
  {
    //JBH: max riff file size is 4GB by the RIFF (WAVE, AIFF) spec.
    isOver4GB = true;
    #ifdef _WIN32
      debug("!!   RIFF::File::read() -- Too Big:" + String::number(filelength) + ", Max 4GB allowed by RIFF Spec: " + static_cast<FileName>(name()).toString());
    #else
      debug("!!   RIFF::File::read() -- Too Big:" + String::number(filelength) + ", Max 4GB allowed by RIFF Spec: " + String(name(), String::UTF8));
    #endif
      //JBH JUST_WARNING    setValid(false);
      //JBH JUST_WARNING    return;
  }

//JBH ==========================================================================<
#ifdef JBH_USE_OLD_CODE
  d->type = readBlock(4); //JBH: "RIFF" for WAVE, "FORM" for AIFF, "TVLG" for ChaClassic500
  offset += 4; //JBH NOTE: let offset point to "size" block.
  //#define JBH_CHECK_RIFF_TYPE
  #ifdef  JBH_CHECK_RIFF_TYPE
  if (d->type != "RIFF" && d->type != "FORM" && d->type != "TVLG") //JBH: aiff may contain "FORM". ChaClassic500 for TVLogic may contain "TVLG"
  {
    #ifdef _WIN32
      debug("!!!! RIFF::File::read() -- Invalid type (should be RIFF or FORM): [" + d->type +"] " + static_cast<FileName>(name()).toString());
    #else
      debug("!!!! RIFF::File::read() -- Invalid type (should be RIFF or FORM): [" + d->type +"] " + String(name(), String::UTF8));
    #endif
      setValid(false);
      return;
  }
  #endif //JBH_CHECK_RIFF_TYPE

  d->sizeOffset = offset;
  /*
   * JBH: The RIFF "spec" itself defines only "4 bytes" for the size field, which limits the max size of a RIFF to 4GB (unsigned int)
   *      The max 4GB limit of a RIFF is an inherent limit of the RIFF spec!
   *      So, d->size will have a round off value of the real size!.
   *      Ex: For a 7537928006 sized aiff, d->size --> 3242960702 which is (7537928006 - 4294967296) - 8
   */
  d->size = readBlock(4).toUInt(bigEndian); //JBH NOTE: size+8 --> file length
  offset += 4; //JBH NOTE: let offset point to "format" block.
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


  /*
   * JBH: Example (over 4GB)
   *
   * AIFF_7GB.aiff --> case: d->size < file size
   *      file size: 7537928006
   *      type:RIFF, size:3242960702 (round off value) --> should +4GB --> 7537927998 -->  (NOTE: size+8 --> file size)
   *      format:AIFF
   *
   * What happens When using 4bytes for size:
   *
   *   type(4)   size(4)     format(4)  cName(4)  cSize(4)   cData(16)  cName(4)  cSize(4)      cData(26789280)    cName(4)  cSize(4)         cData(104)        cName(4)  cSize(4)      
   * +---------+------------+---------+---------+----------+----------+---------+-------------+------------------+---------+----------------+-----------------+---------+--------------+
   * |   RIFF  | 3242960702 |   AIFF  |  'COMM' |    18    | 18 bytes |  'SSND' | 3242960664  | 3242960664 bytes |  'FUCK' |    143258850   | 143258850 bytes |  'SHIT' | 4165334125   |
   * +---------+------------+---------+---------+----------+----------+---------+-------------+------------------+---------+----------------+-----------------+---------+--------------+
   * |         |            |         |         |          |          |         |             |                  |         |                |                 |         |              |
   * 0         4            8         12        16         20         38        42            46                 3242960710                 3242960718        3386219568               7551553697
   *                 ^                                                                 ^                                                                                           ^               
   *                 |                                                                 |                                                                                           |               
   *              round off                                                          round off                                                                                     EOF (7537928006)               
   *              should +4GB "7537927998"                                           should +4GB "7537927960"
   *                 |                                                                 |
   *                 v                                                                 v
   *   
   *   
   * What is Correct: Real Data
   *   
   * +---------+------------+---------+---------+----------+----------+---------+-------------+------------------+
   * |   RIFF  | 3242960702 |   AIFF  |  'COMM' |    18    | 18 bytes |  'SSND' | 3242960664  | 7537927960 bytes |
   * +---------+------------+---------+---------+----------+----------+---------+-------------+------------------+
   * |         |            |         |         |          |          |         |             |                  |
   * 0         4            8         12        16         20         38        42            46                 EOF (7537928006)
   */

  #define JBH_CORRECT_RIFF_SIZE
  #ifdef  JBH_CORRECT_RIFF_SIZE
  //JBH: filter out serverely damaged "size" field early.
  //     Correct size for this type may cause an "hang" (example: The Ritary Gaguenetti Quartet-Le Quecumbar Live in London with The Ritary Gaguenetti Quartet/05 - What is this thing called love.wav)
  if (d->size <= 0)
  {
    #ifdef _WIN32
      debug("!!!! RIFF::File::read() -- Invalid RIFF size:" + String::number(d->size) + " " + static_cast<FileName>(name()).toString());
    #else
      debug("!!!! RIFF::File::read() -- Invalid RIFF size:" + String::number(d->size) + " " + String(name(), String::UTF8));
    #endif
      setValid(false);
      return;
  }

  if ( d->size != (unsigned int)(filelength-8) )
  {
    //JBH NOTE: "!!  ": just a warning, "!!!!": warning+reject
    //JBH NOTE: There are a lot of the fucking AIFF files malformed in this way --> Causes a lot of logs.
    #ifdef _WIN32
      //JBH_TOOMANYLOG  debug("!!   RIFF::File::read() -- Invalid RIFF size:" + String::number(d->size) + " --> Corrected to (" + String::number(filelength) + "-8) " + static_cast<FileName>(name()).toString());
    #else
      //JBH_TOOMANYLOG  debug("!!   RIFF::File::read() -- Invalid RIFF size:" + String::number(d->size) + " --> Corrected to (" + String::number(filelength) + "-8) " + String(name(), String::UTF8));
    #endif
      d->size = filelength-8;
      //JBH JUST_WARNING    setValid(false);
      //JBH JUST_WARNING    return;
  }
  #endif //JBH_CORRECT_RIFF_SIZE

  #define JBH_CHECK_RIFF_SIZE //JBH: JBH_CHECK_RIFF_SIZE may not need if JBH_CORRECT_RIFF_SIZE
  #ifdef  JBH_CHECK_RIFF_SIZE
  if ( d->size != (unsigned int)(filelength-8) )
  {
    //JBH NOTE: "!!  ": just a warning, "!!!!": warning+reject
    //JBH NOTE: There are a lot of the fucking AIFF files malformed in this way --> Causes a lot of logs.
    #ifdef _WIN32
      debug("!!   RIFF::File::read() -- Invalid RIFF size:" + String::number(d->size) + " should be (" + String::number(filelength) + "-8) " + static_cast<FileName>(name()).toString());
    #else
      debug("!!   RIFF::File::read() -- Invalid RIFF size:" + String::number(d->size) + " should be (" + String::number(filelength) + "-8) " + String(name(), String::UTF8));
    #endif
      //JBH JUST_WARNING    setValid(false);
      //JBH JUST_WARNING    return;
  }

  if (d->size <= 0)
  {
    #ifdef _WIN32
      debug("!!!! RIFF::File::read() -- Invalid RIFF size:" + String::number(d->size) + " " + static_cast<FileName>(name()).toString());
    #else
      debug("!!!! RIFF::File::read() -- Invalid RIFF size:" + String::number(d->size) + " " + String(name(), String::UTF8));
    #endif
      setValid(false);
      return;
  }
  #endif //JBH_CHECK_RIFF_SIZE

  d->format = readBlock(4); //JBH: "WAVE" or "AIFF" or "AIFC"?
  offset += 4; //JBH NOTE: let offset point to "format" block.
  if (d->format != "WAVE" && d->format != "AIFF" && d->format != "AIFC") //JBH: AIFC --> AIFF with Codecs
  {
    #ifdef _WIN32
      debug("!!!! RIFF::File::read() -- Invalid format (should be WAVE or AIFF or AIFC): [" + d->format + "] " + static_cast<FileName>(name()).toString());
    #else
      debug("!!!! RIFF::File::read() -- Invalid format (should be WAVE or AIFF or AIFC): [" + d->format + "] " + String(name(), String::UTF8));
    #endif

    //JBH FIXME: Some fucking files have unknown format values (such as "AIFC").
    //           I am not sure that what fucking else would be there. So do not verify the format for now.
    //JBH  #define JBH_CHECK_RIFF_FORMAT
    #ifdef  JBH_VERIFY_RIFF_FORMAT
      setValid(false);
      return;
    #endif //JBH_VERIFY_RIFF_FORMAT
  }

  /*
   * JBH NOTE: in case of a wav file which is used for the following explanations,
   *     d->type:   'RIFF'
   *     d->size:    29812
   *     d->format: 'WAVE'
   */
#else //not JBH_USE_OLD_CODE
//JBH ==========================================================================>
  /*
   *JBH NOTE:
   *    offset 0, 4 bytes: d->type
   *    offset 4, 4 bytes: d->size
   *    offset 8, 4 bytes: d->format
   */
  offset += 4; //JBH NOTE: skip "type" block, let offset point to "size" block.
  d->sizeOffset = offset;

  //JBH NOTE: advance the seek point forcefully to "size" block, without reading the first (4 bytes) "type" block (d->type = readBlock(4))
  seek(offset);
  d->size = readBlock(4).toUInt(bigEndian);

  offset += 8; //JBH NOTE: we are skipping here reading the "format" (4 bytes) block.
#endif //JBH_USE_OLD_CODE

  // + 8: chunk header at least, fix for additional junk bytes
//JBH ==========================================================================<
//JBH: Reading the last (just) 8 bytes chunk (having no data) is meaningless. Moreover, it causes a fatal overflow error!
#define JBH_DO_NOT_READ_LAST_8BYTES_CHUNK
#ifdef  JBH_DO_NOT_READ_LAST_8BYTES_CHUNK
  while(offset + 8 <  length()) {
#else
  while(offset + 8 <= length()) {
#endif
//JBH ==========================================================================>

/*
 * JBH NOTE: Types of chunks followed depend on (FORM='WAVE') or (FORM='AIFF')
 *
 *    (FORM='WAVE'): 'fmt ', 'data', 'LIST', 'ID3 ', ...
 *    (FORM='AIFF'): 'COMM', 'SSND'
 *
 */

/*
 *====================================================
 * JBH: Example (format=='WAVE' case)
 *====================================================
 *
 * Jesse Cook - 10 - Gipsy.wav
 *      file size: 26932804
 *      type:RIFF, size:26932796, format:WAVE (NOTE: size+8 --> file size)
 *      chunk: 'fmt ', 16,       20
 *      chunk: 'data', 26789280, 44
 *      chunk: 'LIST', 104,      26789332
 *      chunk: 'id3 ', 143360,   26789444
 *
 *
 *   type(4)   size(4)   format(4)  cName(4)  cSize(4)   cData(16)  cName(4)  cSize(4)   cData(26789280)  cName(4)  cSize(4)   cData(104)  cName(4)  cSize(4)   cData(143360)
 * +---------+----------+---------+---------+----------+----------+---------+----------+----------------+---------+----------+-----------+---------+----------+--------------+
 * |   RIFF  | 26932796 |   WAVE  |  'fmt ' |    16    | 16 bytes |  'data' | 26789280 | 26789280 bytes |  'LIST' |    104   | 104 bytes |  'id3 ' | 143360   | 143360 bytes |
 * +---------+----------+---------+---------+----------+----------+---------+----------+----------------+---------+----------+-----------+---------+----------+--------------+
 * |         |          |         |         |          |          |         |          |                |         |          |           |         |          |              |
 * 0         4          8         12        16         20         36        40         44               26789324  26789328   26789332    26789436  26789440   26789444       26932804   
 *
 *                                ^
 *                                |
 *                                This while-loop reads from here (offset12) to the end!
 *
 *
     * JBH NOTE: read the file, and then get/construct the 'Chunk's array, such as
     *
     * chunk[0]
     *     chunkName:   'fmt ' --> a chunk that contains PCM-format, channels, sampleRate, sampleWidth, byteRate(bitrate)
     *     chunkSize:   16    
     *     chunkOffset: 20     --> means the real data for the 'fmt ' chunk is at the absolute file offset 20 with 16 bytes long.
     *
     * chunk[1]
     *     chunkName:   'data' --> a chunk that contains the actual PCM data
     *     chunkSize:   26789280  --> the streamLength (bytes), the length (bytes) of the pure PCM data.
     *     chunkOffset: 44     --> means the real data for the 'data' chunk is at the absolute file offset 44 with 26789280 bytes long.
     *
     * chunk[2]
     *     chunkName:   'LIST' --> a chunk that contains the "LIST INFO" metadata
     *     chunkSize:   104
     *     chunkOffset: 26789332  --> means the real data for the 'LIST' chunk is at the absolute file offset 26789332 with 104 bytes long.
     *
     * chunk[3]
     *     chunkName:   'ID3 ' --> a chunk that contains the metadata
     *     chunkSize:   143360
     *     chunkOffset: 26789444  --> means the real data for the 'ID3 ' chunk is at the absolute file offset 26789444 with 143360 bytes long.
     *
     *
     * Note: The "wav" file keeps the metadata either in the "LIST INFO" chunk and/or the "ID3"chunk.
     *       A wav file which does not have the metadata does not contain the 'ID3 ' and 'LIST INFO' chunk.
     *
     * Note: the last chunkOffset (26789444) + the last chunkSize (143360) should be equal to the the file size (26932804).
     *       In some malformed file, the above sum is bigger than the file size.
     */


/*
 *====================================================
 * JBH: Example (format=='AIFF' case)
 *====================================================
 *
 *    +----------------------------+
 *    | FORM AIFF Chunk            |
 *    |   ckID='FORM'              |
 *    |   formType='AIFF' or 'AIFC'|
 *    |                            |
 *    |     +------------------+   |
 *    |     | Common Chunk     |   |  'COMM' chunk contains audio properties
 *    |     |   ckID='COMM'    |   |
 *    |     |                  |   |
 *    |     +------------------+   |
 *    |     +------------------+   |
 *    |     | Sound Data Chunk |   |
 *    |     |   ckID='SSND'    |   |
 *    |     |                  |   |
 *    |     +------------------+   |
 *    |     +------------------+   |
 *    |     | Metadata Chunk   |   |
 *    |     |   ckID='ID3 '    |   |
 *    |     |                  |   |
 *    |     +------------------+   |
 *    +----------------------------+
 *
 */


    seek(offset);
    const ByteVector   chunkName = readBlock(4); //JBH: we call it "FourCC" in general.
    unsigned int chunkSize = readBlock(4).toUInt(bigEndian); //JBH omit const


//JBH ==========================================================================<
    /*
     * JBH NOTE: Some malformed music files have chunks with 0 length, such as:
     *
     *      cName(4)  cSize(4)   cData(16)        cName(4)  cSize(4)   cName(4)  cSize(4)   cName(4)  cSize(4)         cName(4)  cSize(4)   cData(104) 
     *    +---------+----------+----------+     +---------+----------+---------+----------+---------+----------+     +---------+----------+-----------+
     *    |  'fmt ' |    16    | 16 bytes | ... |  '    ' |     0    |  '    ' |     0    |  'TAG ' |     0    | ... |  'LIST' |    104   | 104 bytes | ... 
     *    +---------+----------+----------+     +---------+----------+---------+----------+---------+----------+     +---------+----------+-----------+
     *                                          |    "Null Chunk"    |    "Null Chunk"    |    "Null Chunk"    |                           
     *
     *    The Null Chunk name is usually empty ("\0\0\0\0"), sometimes "TAG ".
     *    The "Null Chunk"s seems to be there for "padding".
     *    Lets just skip those null chunks by 8 bytes (cName(4)+cSize(4)), and continue reading on util we find a valid chunk.
     *
     *
     *    However, there are really fucking malformed music files which have lots of zero-length chunks.
     *    This takes a lot of time (to advance each of 8 bytes chunk for million times) and AMM seems to be halted (in infinite-loop).
     *    Some file takes 12 minuites!!!
     *    JBH FIXME: Allow fixed amounts of null chunks.
     */
    if (chunkSize == 0)
    {
      /*
       * JBH NOTE: When chunSize==0, the chunkName is usually "    " ("\0\0\0\0")
       */
      numNullChunks++;
#if 0
      debug("!!! [WARNING:JBH] RIFF::File::read() Null Chunks (0-length) detected so far: " + String::number(numNullChunks));
#ifdef _WIN32
      debug("JBH: RIFF file: " + static_cast<FileName>(name()).toString());
#else
      debug("JBH: RIFF file: " + String(name(), String::UTF8));
#endif
#endif
      offset += 8; //JBH: advance offset by 8 bytes ( cName(4) + cSize(4) )
      continue;    //JBH: continue reading on util we find a valid chunk.
    }
//JBH ==========================================================================>

//#define JBH_CHECK_VALID_CHUNKNAME
#ifdef JBH_CHECK_VALID_CHUNKNAME
    //JBH: invalid case1: null chunk name --> possibly "padding" chunk?
    if(!isValidChunkName(chunkName)) {
//JBH ==========================================================================<
    #ifdef _WIN32
      debug("!!!! RIFF::File::read() -- Chunk [" + chunkName + "] not valid name " + static_cast<FileName>(name()).toString());
    #else
      debug("!!!! RIFF::File::read() -- Chunk [" + chunkName + "] not valid name " + String(name(), String::UTF8));
    #endif
//JBH ==========================================================================>
      setValid(false);
      break;
    }
#endif

#define JBH_ALLOW_RIFF_DATASIZE_OVERFLOW
#ifdef  JBH_ALLOW_RIFF_DATASIZE_OVERFLOW
    //JBH FIXME: This may not be necessary, if JBH_CORRECT_RIFF_SIZE done above.
    /*
     * JBH: Example: wrong "data" chunk
     * 01 the most beautiful girlcharlie rich.WAV
     *      fileLength: 29247164
     *      type:RIFF
     *      size:       29247200 --> WRONG!! --> bigger than fileLength --> should be fileLength-8
     *      format:WAVE
     *      chunk: 'fmt ', 16,       20
     *      chunk: 'data', 29247164, 44 --> WRONG!! --> overflow --> The size of data chunk should be fileLength-44, or smaller (if the trailing tags exist)
     *
     *
     *   type(4)   size(4)   format(4)  cName(4)  cSize(4)   cData(16)  cName(4)  cSize(4)   cData(26789280) 
     * +---------+----------+---------+---------+----------+----------+---------+----------+---------------------+
     * |   RIFF  | 29247200 |   WAVE  |  'fmt ' |    16    | 16 bytes |  'data' | 29247164 | 29247164 bytes      |
     * +---------+----------+---------+---------+----------+----------+---------+----------+---------------------+
     * |         | WRONG!!  |         |         |          |          |         | WRONG!!  |                     |
     * 0         4          8         12        16         20         36        40         44                    29247200 
     *
     * It should be authored as follows!!
     *
     * +---------+----------+---------+---------+----------+----------+---------+----------+----------------+
     * |   RIFF  | 29247156 |   WAVE  |  'fmt ' |    16    | 16 bytes |  'data' | 29247120 | 29247120 bytes |
     * +---------+----------+---------+---------+----------+----------+---------+----------+----------------+
     * |         | fLeng-8  |         |         |          |          |         |          | (max)fLength-44|
     * 0         4          8         12        16         20         36        40         44               29247164 
     */
  //#define JBH_RIFF_RESIZE_DATACHUNK_OVERFLOW_ONLY
  #ifdef  JBH_RIFF_RESIZE_DATACHUNK_OVERFLOW_ONLY
    if( (static_cast<long long>(offset) + 8 + chunkSize > length()) && (chunkName=="data") ) { //JBH NOTE: resize "data" chunk only
  #else
    if( (static_cast<long long>(offset) + 8 + chunkSize > length()) ) { //JBH NOTE: resize "every" chunks ("data", "list", "id3 " chunks)
  #endif
      unsigned int chunkSizeOrg = chunkSize;
      chunkSize = length() - (offset + 8);
    #ifdef _WIN32
      debug("!!   RIFF::File::read() -- Overflow chunk [" + chunkName + "] adjusted:" + String::number(chunkSizeOrg) + "-->" + String::number(chunkSize) + " " + static_cast<FileName>(name()).toString());
    #else
      debug("!!   RIFF::File::read() -- Overflow chunk [" + chunkName + "] adjusted:" + String::number(chunkSizeOrg) + "-->" + String::number(chunkSize) + " " + String(name(), String::UTF8));
    #endif
    }
#endif //JBH_ALLOW_RIFF_DATASIZE_OVERFLOW


    if(static_cast<long long>(offset) + 8 + chunkSize > length()) {
//JBH ==========================================================================<
    #ifdef _WIN32
      debug("!!!! RIFF::File::read() -- Chunk [" + chunkName + "] overflows the file size " + static_cast<FileName>(name()).toString());
    #else
      debug("!!!! RIFF::File::read() -- Chunk [" + chunkName + "] overflows the file size " + String(name(), String::UTF8));
    #endif
//JBH ==========================================================================>
      setValid(false);
      break;
    }

//JBH ==========================================================================<
    /*
     * JBH NOTE: Some malformed wav files may have a wrong (ridiculously big) chunk size for the "LIST" chunk.
     *           The "LIST" chunk keeps the text-typed metadata and should not be unusally big.
     *           As a real world example, a (broken) wav file declares the LIST chunk size as 1.3G, which is apparently wrong.
     *           This causes a SEGFAULT in the std::vector allocation, which causes eventually an hang of AMS.
     *           Limit the size of LIST metadata chunk upto 1024.
     *
     *           2016/10/11 increased the limit to 4096
     *           2018/11/06 increased the limit to 10K  (Some wav files ripped by ACSRipper have a size of  9730)
     *           2018/12/11 increased the limit to 100K (Some wav files ripped by ACSRipper have a size of 15553)
     */
    if ( chunkName=="LIST" && chunkSize>102400 ) {
    #ifdef _WIN32
      debug("!!!! RIFF::File::read() -- Chunk [" + chunkName + "] too big, invalidated " + static_cast<FileName>(name()).toString());
    #else
      debug("!!!! RIFF::File::read() -- Chunk [" + chunkName + "] too big, invalidated " + String(name(), String::UTF8));
    #endif
      //JBH NOTE: If set to false, AudioProperties will be determined as INVALID at TagLibFile::readAudioProperties() due to RIFF::File==invalid.
      setValid(false);
      break;
    }
//JBH ==========================================================================>

    Chunk chunk;
    chunk.name    = chunkName;
    chunk.size    = chunkSize;
    chunk.offset  = offset + 8;
    chunk.padding = 0;

    offset = chunk.offset + chunk.size;

    // Check padding

    if(offset & 1) {
      seek(offset);
      const ByteVector iByte = readBlock(1);
      if(iByte.size() == 1 && iByte[0] == '\0') {
        chunk.padding = 1;
        offset++;
      }
    }

    d->chunks.push_back(chunk);

//JBH ==========================================================================<
    if ( isOver4GB && (chunkName=="data" || chunkName=="SSND") ) {
      //JBH: do not try read further chunks ("LIST", "ID3 " chunk) over 4GB file.
      //     NOTE: so far, we should have gotten the "fmt " chunk (WAVE case) or "COMM" chunk (AIFF case), which contains the audio properties.
      break;
    }
//JBH ==========================================================================>

  } //end of "while-loop"
#endif /// JBH update
}

void RIFF::File::writeChunk(const ByteVector &name, const ByteVector &data,
                            unsigned long offset, unsigned long replace)
{
  ByteVector combined;

  combined.append(name);
  combined.append(ByteVector::fromUInt(data.size(), d->endianness == BigEndian));
  combined.append(data);

  if(data.size() & 1)
    combined.resize(combined.size() + 1, '\0');

  insert(combined, offset, replace);
}

void RIFF::File::updateGlobalSize()
{
  const Chunk first = d->chunks.front();
  const Chunk last  = d->chunks.back();
  d->size = last.offset + last.size + last.padding - first.offset + 12;

  const ByteVector data = ByteVector::fromUInt(d->size, d->endianness == BigEndian);
  insert(data, d->sizeOffset, 4);
}
