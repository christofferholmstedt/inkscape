#!/bin/bash
#
#  Inkscape compilation and packaging script for Mac OS X
#
# Please see
#  http://wiki.inkscape.org/wiki/index.php?title=CompilingMacOsX
# for more complete information
#
# Author:
#	Jean-Olivier Irisson <jo.irisson@gmail.com>
# with information from
#	Kees Cook
#	Michael Wybrow
#
# Copyright (C) 2006-2007
# Released under GNU GPL, read the file 'COPYING' for more information
#

############################################################

# User modifiable parameters
#----------------------------------------------------------
# Configure flags
CONFFLAGS="--disable-static --enable-shared --enable-osxapp"
# Libraries prefix (Warning: NO trailing slash)
LIBPREFIX="/opt/local"
# User name on Modevia
MODEVIA_NAME=""

############################################################

# Help message
#----------------------------------------------------------
help()
{

echo -e "
Compilation script for Inkscape on Mac OS X.

\033[1mUSAGE\033[0m
  $0 [options] action[s]

\033[1mACTIONS & OPTIONS\033[0m
  \033[1mh,help\033[0m	
    display this help message
  \033[1mu,up,update\033[0m
    update an existing checkout from svn (run svn up)
  \033[1ma,auto,autogen\033[0m
    prepare configure script (run autogen.sh). This is only necessary
    for a fresh svn checkout or after make distclean.
  \033[1mc,conf,configure\033[0m
    configure the build (run configure). Edit your configuration
    options in $0
    \033[1m-p,--prefix\033[0m	specify install prefix (configure step only)
  \033[1mb,build\033[0m
    build Inkscape (run make)
  \033[1mi,install\033[0m
    install the build products locally, inside the source
    directory (run make install)
  \033[1mp,pack,package\033[0m
    package Inkscape in a double clickable .app bundle 
    \033[1m-s,--strip\033[0m	remove debugging information in Inkscape package
    \033[1m-py,--with-python\033[0m	specify python modules path for inclusion into the app bundle
  \033[1md,dist,distrib\033[0m
    store Inkscape.app in a disk image (dmg) for distribution
  \033[1mput,upload\033[0m
    upload the dmg and the associate info file on Modevia server
  \033[1mall\033[0m
    do everything (update, configure, build, install, package, distribute)

\033[1mEXAMPLES\033[0m
  \033[1m$0 conf build install\033[0m
    configure, build and install a dowloaded version of Inkscape in the default
    directory, keeping debugging information.	
  \033[1m$0 u a c b -p ~ i -s -py ~/site-packages/ p d\033[0m
    update an svn checkout, prepare configure script, configure,
    build and install Inkscape in the user home directory (~). 	
    Then package Inkscape without debugging information,
    with python packages from ~/site-packages/ and prepare 
    a dmg for distribution."
}

# Parameters
#----------------------------------------------------------
# Paths
HERE=`pwd`
SRCROOT=$HERE/../..		# we are currently in packaging/macosx

# Defaults
if [ "$INSTALLPREFIX" = "" ]
then
	INSTALLPREFIX=$SRCROOT/Build/
fi
SVNUPDATE="f"
AUTOGEN="f"
CONFIGURE="f"
BUILD="f"
INSTALL="f"
PACKAGE="f"
DISTRIB="f"
UPLOAD="f"

STRIP=""
PYTHON_MODULES=""

# Parse command line options
#----------------------------------------------------------
while [ "$1" != "" ]
do
	case $1 in
	h|help)
		help 
		exit 1 ;;
	all)            
		SVNUPDATE="t"
		CONFIGURE="t"
		BUILD="t" 
		INSTALL="t"
		PACKAGE="t"
		DISTRIB="t" ;;
   u|up|update)
		SVNUPDATE="t" ;;
   a|auto|autogen)
		AUTOGEN="t" ;;
	c|conf|configure)
		CONFIGURE="t" ;;
	b|build)
		BUILD="t" ;;
	i|install)
		INSTALL="t" ;;
	p|pack|package)
		PACKAGE="t" ;;
	d|dist|distrib)
		DISTRIB="t" ;;
	put|upload)
		UPLOAD="t" ;;
	-p|--prefix)
	  	INSTALLPREFIX=$2
	  	shift 1 ;;
	-s|--strip)
	     	STRIP="-s" ;;
	-py|--with-python)
		PYTHON_MODULES="$2"
		shift 1 ;;
	*)
		echo "Invalid command line option: $1" 
		exit 2 ;;
	esac
	shift 1
done


# Set environment variables
# ----------------------------------------------------------
export LIBPREFIX

# Specific environment variables
#  automake seach path
export CPATH="$LIBPREFIX/include"
#  configure search path
export CPPFLAGS="-I$LIBPREFIX/include"
# export CPPFLAGS="-I$LIBPREFIX/include -I /System/Library/Frameworks/Carbon.framework/Versions/Current/Headers"
export LDFLAGS="-L$LIBPREFIX/lib"
#  compiler arguments
export CFLAGS="-O3 -Wall"
export CXXFLAGS="$CFLAGS"


# Actions
# ----------------------------------------------------------
if [[ "$SVNUPDATE" == "t" ]]
then
	cd $SRCROOT
	svn up
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nSVN update failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$AUTOGEN" == "t" ]]
then
	cd $SRCROOT
	./autogen.sh
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nautogen failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$CONFIGURE" == "t" ]]
then
	ALLCONFFLAGS=`echo "$CONFFLAGS --prefix=$INSTALLPREFIX"`
	cd $SRCROOT
	if [ ! -f configure ]
	then
		echo "Configure script not found in $SRCROOT. Run '$0 autogen' first"
		exit 1
	fi
	./configure $ALLCONFFLAGS
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nConfigure failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$BUILD" == "t" ]]
then
	cd $SRCROOT
	make
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nBuild failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$INSTALL" == "t" ]] 
then
	cd $SRCROOT
	make install
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nInstall failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$PACKAGE" == "t" ]]
then
	
	# Test the existence of required files
	if [ ! -e $INSTALLPREFIX/bin/inkscape ]
	then
		echo "The inkscape executable \"$INSTALLPREFIX/bin/inkscape\" cound not be found."
		exit 1
	fi
	if [ ! -e $SRCROOT/Info.plist ]
	then
		echo "The file \"$SRCROOT/Info.plist\" could not be found, please re-run configure."
		exit 1
	fi
	
	# Set python command line option (if PYTHON_MODULES location is not empty, then add the python call to the command line, otherwise, stay empty)
	if [[ "$PYTHON_MODULES" != "" ]]; then
		PYTHON_MODULES="-py $PYTHON_MODULES"
		# TODO: fix this: it does not allow for spaces in the PATH under this form and cannot be quoted
	fi

	# Create app bundle
	./osx-app.sh $STRIP -b $INSTALLPREFIX/bin/inkscape -p $SRCROOT/Info.plist $PYTHON_MODULES
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nApplication bundle creation failed"
		exit $status
	fi
fi

# Fetch some information
REVISION=`head -n 4 ../../.svn/entries | tail -n 1`
ARCH=`arch | tr [p,c] [P,C]`
MINORVERSION=`/usr/bin/sw_vers | grep ProductVersion | cut -f2 -d \.`
NEWNAME="Inkscape-$REVISION-10.$MINORVERSION-$ARCH"
DMGFILE="$NEWNAME.dmg"
INFOFILE="$NEWNAME-info.txt"

if [[ "$DISTRIB" == "t" ]]
then
	# Create dmg bundle
	./osx-dmg.sh -p "Inkscape.app"
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nDisk image creation failed"
		exit $status
	fi

	mv Inkscape.dmg $DMGFILE
	
	# Prepare information file
	echo "Version information on $DATE for `whoami`:
	OS X       `/usr/bin/sw_vers | grep ProductVersion | cut -f2 -d \:`
	Architecture $ARCH
	DarwinPorts  `port version | cut -f2 -d \ `
	GCC          `gcc --version | grep GCC`
	GTK          `pkg-config --modversion gtk+-2.0`
	GTKmm        `pkg-config --modversion gtkmm-2.4`
	Cairo        `pkg-config --modversion cairo`
	Cairomm      `pkg-config --modversion cairomm-1.0`
	CairoPDF     `pkg-config --modversion cairo-pdf`
	Pango        `pkg-config --modversion pango`
Configure options:
	$CONFFLAGS" > $INFOFILE
	if [[ "$STRIP" == "t" ]]; then
		echo "Debug info
	no" >> $INFOFILE
	else
		echo "Debug info
	yes" >> $INFOFILE
	fi	
fi

if [[ "$UPLOAD" == "t" ]]
then
	# Provide default for user name on modevia
	if [[ "$MODEVIA_NAME" == "" ]]; then
		MODEVIA_NAME=$USER
	fi
	# Uploasd file
	scp $DMGFILE $INFOFILE "$MODEVIA_NAME"@inkscape.modevia.com:inkscape/docs/macosx-snap/
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nUpload failed"
		exit $status
	fi
fi

if [[ "$PACKAGE" == "t" || "$DISTRIB" == "t" ]]; then
	# open a Finder window here to admire what we just produced
	open .
fi

exit 0
