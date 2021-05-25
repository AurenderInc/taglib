@ECHO off

@rem Build and Install script for a general purpose taglib
@rem JamesHwang@aurender.com

echo ======================================================================
@rem Specify the install directory
set INSTALL_DIRECTORY=D:/00TagLibJBH64
set INSTALL_DIRECTORY_DOS=D:\00TagLibJBH64
echo Install Directory: %INSTALL_DIRECTORY%
echo ======================================================================

echo ======================================================================
@rem Specify Qt directory
@rem C:\Qt\Qt5.12.3\5.12.3\msvc2017    --> 32bit Qt
@rem C:\Qt\Qt5.12.3\5.12.3\msvc2017_64 --> 64bit Qt
set QTDIR=C:\Qt\Qt5.12.3\5.12.3\msvc2017_64
@rem do not double quote ("") QTDIR
echo Qt directory: %QTDIR%

@rem Set up MSVC environments
if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)

set PATH=%QTDIR%\bin;%PATH%



@rem #################### Dependencies Top Directory
set TAGLIB_DEPS_ROOT=%CD%\..



@rem #################### zlib-1.2.11
echo ======================================================================
echo zlib started
cd %TAGLIB_DEPS_ROOT%\zlib-1.2.11
echo %CD%

if exist 00build-general-debug-x64 rd /s/q 00build-general-debug-x64
mkdir 00build-general-debug-x64

cd 00build-general-debug-x64
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=%INSTALL_DIRECTORY% -G "NMake Makefiles" > j_cmake 2>&1
jom /J 9 > j_jom 2>&1
nmake install
echo zlib finished.
echo ======================================================================



@rem #################### libiconv-win
echo ======================================================================
echo libiconv-win started
cd %TAGLIB_DEPS_ROOT%\libiconv-win
echo %CD%
COPY /Y include\iconv.h                    %INSTALL_DIRECTORY_DOS%\include\iconv.h
COPY /Y lib\x64\libiconv_a_debug.lib       %INSTALL_DIRECTORY_DOS%\lib\libiconv_a_debug.lib
COPY /Y lib\x64\libiconv_a_debug.pdb       %INSTALL_DIRECTORY_DOS%\lib\libiconv_a_debug.pdb
COPY /Y lib\x64\libiconv_a_debug.idb       %INSTALL_DIRECTORY_DOS%\lib\libiconv_a_debug.idb
echo libiconv-win finished.
echo ======================================================================



@rem #################### uchardet
echo ======================================================================
echo uchardet started
cd %TAGLIB_DEPS_ROOT%\uchardet
echo %CD%

if exist 00build-general-debug-x64 rd /s/q 00build-general-debug-x64
mkdir 00build-general-debug-x64

cd 00build-general-debug-x64
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC=ON -DCMAKE_INSTALL_PREFIX=%INSTALL_DIRECTORY% -G "NMake Makefiles" > j_cmake 2>&1
jom /J 9 > j_jom 2>&1
nmake install

echo uchardet finished.
echo ======================================================================



@rem #################### taglib-jbh
echo ======================================================================
echo taglib started
cd %TAGLIB_DEPS_ROOT%\taglib-jbh
echo %CD%

if exist 00build-general-debug-x64 rd /s/q 00build-general-debug-x64
mkdir 00build-general-debug-x64

cd 00build-general-debug-x64
@rem Note: build STATIC taglib for AMM-Windows
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DCMAKE_PREFIX_PATH=%INSTALL_DIRECTORY% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIRECTORY% -DJBH_BUILD_FOR_GENERAL=ON -DJBH_USE_EMBEDDED_UNICODE_ENCODER=ON -DWITH_ASF=ON -DWITH_MP4=ON -DBUILD_EXAMPLES=ON -G "NMake Makefiles" > j_cmake 2>&1
jom /J 9 && nmake install > j_jom 2>&1

@rem COPY /Y taglib\tag.pdb %INSTALL_DIRECTORY_DOS%\lib\tag.pdb
COPY /Y taglib\CMakeFiles\tag.dir\vc140.pdb %INSTALL_DIRECTORY_DOS%\lib\vc140.pdb

cd ../
echo taglib finished.
echo ======================================================================
