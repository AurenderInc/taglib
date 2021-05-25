@ECHO off

@rem Build and Install script for the AurenderMediaManager dependencies
@rem JamesHwang@aurender.com


echo ======================================================================
@rem Specify Qt directory
@rem C:\Qt\Qt5.12.3\5.12.3\msvc2017    --> 32bit Qt
@rem C:\Qt\Qt5.12.3\5.12.3\msvc2017_64 --> 64bit Qt
set QTDIR=C:\Qt\Qt5.12.3\5.12.3\msvc2017_64
@rem do not double quote ("") QTDIR
echo Qt directory: %QTDIR%

@rem Specify the install directory
set INSTALL_DIRECTORY=D:/AMM-LIBS-DEBUG-x64
set INSTALL_DIRECTORY_DOS=D:\AMM-LIBS-DEBUG-x64
echo Install Directory: %INSTALL_DIRECTORY%
echo ======================================================================

echo ======================================================================
@rem For FindQt5.cmake
set CMAKE_PREFIX_PATH="%QTDIR%;%INSTALL_DIRECTORY%"
echo CMAKE_PREFIX_PATH: %CMAKE_PREFIX_PATH%


@rem Set up MSVC environments
if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)

set PATH=%QTDIR%\bin;%PATH%
@rem echo %PATH%

@rem #################### taglib-jbh
echo ======================================================================
echo taglib started

if exist 00build-debug-x64 rd /s/q 00build-debug-x64
mkdir 00build-debug-x64

cd 00build-debug-x64
@rem Note: build STATIC taglib for AMM-Windows
@rem cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DCMAKE_PREFIX_PATH=%INSTALL_DIRECTORY% -DCMAKE_INSTALL_PREFIX=%INSTALL_DIRECTORY% -DUSE_APP_LOGGER=ON -DUSE_QXTLOGGER=ON -DUSE_JBHLOGGER=OFF -DWITH_ASF=ON -DWITH_MP4=ON -DBUILD_EXAMPLES=ON -G "NMake Makefiles" > j_cmake 2>&1
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=%INSTALL_DIRECTORY% -DUSE_QSTRING=ON -DUSE_APP_LOGGER=ON -DUSE_QXTLOGGER=ON -DUSE_JBHLOGGER=OFF -DWITH_ASF=ON -DWITH_MP4=ON -DBUILD_EXAMPLES=ON -G "NMake Makefiles" > j_cmake 2>&1
jom /J 9 && nmake install > j_jom 2>&1

@rem COPY /Y taglib\tag.pdb %INSTALL_DIRECTORY_DOS%\lib\tag.pdb
COPY /Y taglib\CMakeFiles\tag.dir\vc140.pdb %INSTALL_DIRECTORY_DOS%\lib\vc140.pdb

cd ../
echo taglib finished.
echo ======================================================================
