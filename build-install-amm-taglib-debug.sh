#!/bin/bash

# Build and Install script for the AurenderMediaManager dependencies
# JamesHwang@aurender.com

# Specify the install directory
if [ $# == 0 ]
then
    INSTALL_DIRECTORY="/etc/amm"
    sudo mkdir -p /etc/amm
    sudo chmod 777 /etc/amm
else
    INSTALL_DIRECTORY=$1
fi
echo "INSTALL_DIRECTORY: $INSTALL_DIRECTORY"


#################### add Qt5.3.0 to PKG_CONFIG_PATH and PATH
#JBH NOTE: Do not use any relative variable such as $HOME for the RELEASE build.
#          The Aurender Dev Environment requres everything fixed. It does not prefer something flexible.
#
#          Use any relative variable such as $HOME for the DEBUG build NOT ON AURENDER.
export QTDIR=$HOME/Qt5.3.0/5.3/gcc
#export QTDIR=/home/widealab/Qt5.3.0/5.3/gcc
export PKG_CONFIG_PATH=$QTDIR/lib/pkgconfig:$PKG_CONFIG_PATH
export PATH=$QTDIR/bin:$PATH


export QTDIR=$HOME/Qt5.3.0/5.3/gcc
export PKG_CONFIG_PATH=$QTDIR/lib/pkgconfig:$PKG_CONFIG_PATH
export PATH=$QTDIR/bin:$PATH

#################### add $INSTALL_DIRECTORY to PKG_CONFIG_PATH and PATH
export PKG_CONFIG_PATH=$INSTALL_DIRECTORY/lib/pkgconfig:$PKG_CONFIG_PATH
export PATH=$INSTALL_DIRECTORY/bin:$PATH

#################### Display Info on Qt which is being used for building libs
echo ##################################################
qmake --version
echo ##################################################


#################### taglib-1.11.1-jbh-20180614
if [ -d 00build ]
then
	rm -rf 00build
fi
mkdir 00build

cd 00build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON -DCMAKE_PREFIX_PATH=$INSTALL_DIRECTORY -DCMAKE_INSTALL_PREFIX=$INSTALL_DIRECTORY -DUSE_QSTRING=ON -DUSE_APP_LOGGER=ON -DUSE_QXTLOGGER=ON -DUSE_JBHLOGGER=OFF -DWITH_ASF=ON -DWITH_MP4=ON -DBUILD_EXAMPLES=ON 2>&1 | tee j_cmake
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



