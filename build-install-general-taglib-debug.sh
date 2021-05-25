#!/bin/bash

# Build and Install script for the AurenderMediaManager dependencies
# JamesHwang@aurender.com

# Specify the install directory
if [ $# == 0 ]
then
    INSTALL_DIRECTORY="$HOME/00TagLibJBH"
else
    INSTALL_DIRECTORY=$1
fi
echo "INSTALL_DIRECTORY: $INSTALL_DIRECTORY"


#################### add $INSTALL_DIRECTORY to PKG_CONFIG_PATH and PATH
export PKG_CONFIG_PATH=$INSTALL_DIRECTORY/lib/pkgconfig:$PKG_CONFIG_PATH
export PATH=$INSTALL_DIRECTORY/bin:$PATH


#################### Dependencies Top Directory
cd ..


#################### zlib-1.2.11
cd zlib-1.2.11

if [ -d 00build-general ]
then
    rm -rf 00build-general
fi
mkdir 00build-general

cd 00build-general
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$INSTALL_DIRECTORY 2>&1 | tee j_cmake
make -j9 && make install 
if [ "$?" != "0" ]
then
    echo "Fail to build and install zlib, check the error message below."
exit 1
fi

cd ../..
echo "zlib installed."


#################### taglib-jbh
if [ -d 00build-general ]
then
	rm -rf 00build-general
fi
mkdir 00build-general

cd 00build-general
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DCMAKE_PREFIX_PATH=$INSTALL_DIRECTORY -DCMAKE_INSTALL_PREFIX=$INSTALL_DIRECTORY -DJBH_BUILD_FOR_GENERAL=ON -DWITH_ASF=ON -DWITH_MP4=ON -DBUILD_EXAMPLES=ON 2>&1 | tee j_cmake
make -j9 && make install 
if [ "$?" != "0" ]
then
    echo "Fail to build and install taglib, check the error message below."
exit 1
fi

#JBH FIXME cp examples/tagreader $INSTALL_DIRECTORY/bin/.

cd ../..
echo "taglib installed."


########## All done
echo " "
echo "======================"
echo "All done successfully!"
echo "======================"
exit 0



