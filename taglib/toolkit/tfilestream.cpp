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

#include "tfilestream.h"
#include "tstring.h"
#include "tdebug.h"

#ifdef _WIN32
# include <windows.h>
#else
# include <stdio.h>
# include <unistd.h>
#endif

//#define JBH_DEBUG_TAGLIB_OPENCLOSE
//#define JBH_DEBUG_FILESTREAM_OPENCLOSE

using namespace TagLib;

namespace
{
#ifdef _WIN32

  // Uses Win32 native API instead of POSIX API to reduce the resource consumption.

  typedef FileName FileNameHandle;
  typedef HANDLE FileHandle;

  const FileHandle InvalidFileHandle = INVALID_HANDLE_VALUE;

  FileHandle openFile(const FileName &path, bool readOnly)
  {
//JBH ==========================================================================<
#ifdef JBH_DEBUG_TAGLIB_OPENCLOSE_FILEHANDLE
    debug("openFile() called");
#endif
//JBH ==========================================================================>

    const DWORD access = readOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);

#if defined (PLATFORM_WINRT)
    return CreateFile2(path.wstr().c_str(), access, FILE_SHARE_READ, OPEN_EXISTING, NULL);
#else
    return CreateFileW(path.wstr().c_str(), access, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
#endif
  }

  void closeFile(FileHandle file)
  {
//JBH ==========================================================================<
#ifdef JBH_DEBUG_TAGLIB_OPENCLOSE_FILEHANDLE
    debug("closeFile() called");
#endif
//JBH ==========================================================================>
    CloseHandle(file);
  }

  size_t readFile(FileHandle file, ByteVector &buffer)
  {
    DWORD length;
    if(ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &length, NULL))
      return static_cast<size_t>(length);
    else
      return 0;
  }

  size_t writeFile(FileHandle file, const ByteVector &buffer)
  {
    DWORD length;
    if(WriteFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &length, NULL))
      return static_cast<size_t>(length);
    else
      return 0;
  }

#else   // _WIN32

  struct FileNameHandle : public std::string
  {
    FileNameHandle(FileName name) : std::string(name) {}
    operator FileName () const { return c_str(); }
  };

  typedef FILE* FileHandle;

  const FileHandle InvalidFileHandle = 0;

  FileHandle openFile(const FileName &path, bool readOnly)
  {
//JBH ==========================================================================<
#ifdef JBH_DEBUG_TAGLIB_OPENCLOSE_FILEHANDLE
    debug("openFile() called");
#endif
//JBH ==========================================================================>

    return fopen(path, readOnly ? "rb" : "rb+");
  }

  void closeFile(FileHandle file)
  {
//JBH ==========================================================================<
#ifdef JBH_DEBUG_TAGLIB_OPENCLOSE_FILEHANDLE
    debug("closeFile() called");
#endif
//JBH ==========================================================================>
    fclose(file);
  }

  size_t readFile(FileHandle file, ByteVector &buffer)
  {
    //JBH: fread returns "size_t", so TagLib is limited by the size of size_t on a platform (32bit, 64bit linux)
    //     On a 32 bit linux, the max size is limited to 4GB.
    return fread(buffer.data(), sizeof(char), buffer.size(), file);
  }

  size_t writeFile(FileHandle file, const ByteVector &buffer)
  {
    return fwrite(buffer.data(), sizeof(char), buffer.size(), file);
  }

#endif  // _WIN32
}

class FileStream::FileStreamPrivate
{
public:
  FileStreamPrivate(const FileName &fileName)
    : file(InvalidFileHandle)
    , name(fileName)
    , readOnly(true)
  {
  }

  FileHandle file;
  FileNameHandle name;
  bool readOnly;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

FileStream::FileStream(FileName fileName, bool openReadOnly)
  : d(new FileStreamPrivate(fileName))
{
//JBH ==========================================================================<
#ifdef JBH_DEBUG_FILESTREAM_OPENCLOSE
  #ifdef _WIN32
    debug("FileStream Constructor:" + fileName.toString());
  #else
    debug("FileStream Constructor:" + String(static_cast<const char *>(d->name)));
  #endif
#endif
//JBH ==========================================================================>

  // First try with read / write mode, if that fails, fall back to read only.

  if(!openReadOnly)
    d->file = openFile(fileName, false);

  if(d->file != InvalidFileHandle)
    d->readOnly = false;
  else
    d->file = openFile(fileName, true);

  if(d->file == InvalidFileHandle)
  {
# ifdef _WIN32
    debug("Could not open file " + fileName.toString());
# else
    debug("Could not open file " + String(static_cast<const char *>(d->name)));
# endif
  }
}

FileStream::~FileStream()
{
//JBH ==========================================================================<
#ifdef JBH_DEBUG_FILESTREAM_OPENCLOSE
  #ifdef _WIN32
    debug("FileStream Destructor:" + d->file->fileName.toString());
  #else
    //debug("FileStream Destructor:" + String(static_cast<const char *>(d->name)));
    debug("FileStream Destructor:" + String(static_cast<const char *>(d->file->name)));
  #endif
#endif
//JBH ==========================================================================>

  if(isOpen())
    closeFile(d->file);

  delete d;
}

FileName FileStream::name() const
{
  return d->name;
}

ByteVector FileStream::readBlock(unsigned long length)
{
  if(!isOpen()) {
    debug("FileStream::readBlock() -- invalid file.");
    return ByteVector();
  }

  if(length == 0)
    return ByteVector();

  const unsigned long streamLength = static_cast<unsigned long>(FileStream::length());
  if(length > bufferSize() && length > streamLength)
    length = streamLength;

  ByteVector buffer(static_cast<unsigned int>(length));

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

  //JBH: readFile calls fread(), which returns "size_t", so TagLib is limited by the size of size_t on a platform (32bit, 64bit linux)
  //     On a 32 bit linux, the max size is limited to 4GB.
  const size_t count = readFile(d->file, buffer);
  buffer.resize(static_cast<unsigned int>(count));

  return buffer;
}

void FileStream::writeBlock(const ByteVector &data)
{
  if(!isOpen()) {
    debug("FileStream::writeBlock() -- invalid file.");
    return;
  }

  if(readOnly()) {
    debug("FileStream::writeBlock() -- read only file.");
    return;
  }

  writeFile(d->file, data);
}

void FileStream::insert(const ByteVector &data, unsigned long start, unsigned long replace)
{
  if(!isOpen()) {
    debug("FileStream::insert() -- invalid file.");
    return;
  }

  if(readOnly()) {
    debug("FileStream::insert() -- read only file.");
    return;
  }

  if(data.size() == replace) {
    seek(start);
    writeBlock(data);
    return;
  }
  else if(data.size() < replace) {
    seek(start);
    writeBlock(data);
    removeBlock(start + data.size(), replace - data.size());
    return;
  }

  // Woohoo!  Faster (about 20%) than id3lib at last.  I had to get hardcore
  // and avoid TagLib's high level API for rendering just copying parts of
  // the file that don't contain tag data.
  //
  // Now I'll explain the steps in this ugliness:

  // First, make sure that we're working with a buffer that is longer than
  // the *differnce* in the tag sizes.  We want to avoid overwriting parts
  // that aren't yet in memory, so this is necessary.

  unsigned long bufferLength = bufferSize();

  while(data.size() - replace > bufferLength)
    bufferLength += bufferSize();

  // Set where to start the reading and writing.

  long readPosition = start + replace;
  long writePosition = start;

  ByteVector buffer = data;
  ByteVector aboutToOverwrite(static_cast<unsigned int>(bufferLength));

  while(true)
  {
    // Seek to the current read position and read the data that we're about
    // to overwrite.  Appropriately increment the readPosition.

    seek(readPosition);
    const unsigned int bytesRead = static_cast<unsigned int>(readFile(d->file, aboutToOverwrite));
    aboutToOverwrite.resize(bytesRead);
    readPosition += bufferLength;

    // Check to see if we just read the last block.  We need to call clear()
    // if we did so that the last write succeeds.

    if(bytesRead < bufferLength)
      clear();

    // Seek to the write position and write our buffer.  Increment the
    // writePosition.

    seek(writePosition);
    writeBlock(buffer);

    // We hit the end of the file.

    if(bytesRead == 0)
      break;

    writePosition += buffer.size();

    // Make the current buffer the data that we read in the beginning.

    buffer = aboutToOverwrite;
  }
}

void FileStream::removeBlock(unsigned long start, unsigned long length)
{
  if(!isOpen()) {
    debug("FileStream::removeBlock() -- invalid file.");
    return;
  }

  unsigned long bufferLength = bufferSize();

  long readPosition = start + length;
  long writePosition = start;

  ByteVector buffer(static_cast<unsigned int>(bufferLength));

  for(unsigned int bytesRead = -1; bytesRead != 0;)
  {
    seek(readPosition);
    bytesRead = static_cast<unsigned int>(readFile(d->file, buffer));
    readPosition += bytesRead;

    // Check to see if we just read the last block.  We need to call clear()
    // if we did so that the last write succeeds.

    if(bytesRead < buffer.size()) {
      clear();
      buffer.resize(bytesRead);
    }

    seek(writePosition);
    writeFile(d->file, buffer);

    writePosition += bytesRead;
  }

  truncate(writePosition);
}

bool FileStream::readOnly() const
{
  return d->readOnly;
}

bool FileStream::isOpen() const
{
  return (d->file != InvalidFileHandle);
}

void FileStream::seek(long offset, Position p)
{
  if(!isOpen()) {
    debug("FileStream::seek() -- invalid file.");
    return;
  }

#ifdef _WIN32

  if(p != Beginning && p != Current && p != End) {
    debug("FileStream::seek() -- Invalid Position value.");
    return;
  }

  LARGE_INTEGER liOffset;
  liOffset.QuadPart = offset;

  if(!SetFilePointerEx(d->file, liOffset, NULL, static_cast<DWORD>(p))) {
    debug("FileStream::seek() -- Failed to set the file pointer.");
  }

#else

  int whence;
  switch(p) {
  case Beginning:
    whence = SEEK_SET;
    break;
  case Current:
    whence = SEEK_CUR;
    break;
  case End:
    whence = SEEK_END;
    break;
  default:
    debug("FileStream::seek() -- Invalid Position value.");
    return;
  }

  fseek(d->file, offset, whence);

#endif
}

void FileStream::clear()
{
#ifdef _WIN32

  // NOP

#else

  clearerr(d->file);

#endif
}

long FileStream::tell() const
{
#ifdef _WIN32

  const LARGE_INTEGER zero = {};
  LARGE_INTEGER position;

  if(SetFilePointerEx(d->file, zero, &position, FILE_CURRENT) && position.QuadPart <= LONG_MAX) {
    return static_cast<long>(position.QuadPart);
  }
  else {
    debug("FileStream::tell() -- Failed to get the file pointer.");
    return 0;
  }

#else

  return ftell(d->file);

#endif
}

long FileStream::length()
{
  if(!isOpen()) {
    debug("FileStream::length() -- invalid file.");
    return 0;
  }

#ifdef _WIN32

  LARGE_INTEGER fileSize;

  if(GetFileSizeEx(d->file, &fileSize) && fileSize.QuadPart <= LONG_MAX) {
    return static_cast<long>(fileSize.QuadPart);
  }
  else {
    debug("FileStream::length() -- Failed to get the file size.");
    return 0;
  }

#else

  const long curpos = tell();

  seek(0, End);
  const long endpos = tell();

  seek(curpos, Beginning);

  return endpos;

#endif
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void FileStream::truncate(long length)
{
#ifdef _WIN32

  const long currentPos = tell();

  seek(length);

  if(!SetEndOfFile(d->file)) {
    debug("FileStream::truncate() -- Failed to truncate the file.");
  }

  seek(currentPos);

#else

  const int error = ftruncate(fileno(d->file), length);
  if(error != 0) {
    debug("FileStream::truncate() -- Coundn't truncate the file.");
  }

#endif
}

unsigned int FileStream::bufferSize()
{
  return 1024;
}
