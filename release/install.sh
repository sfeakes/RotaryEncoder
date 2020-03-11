#!/bin/bash
#


BUILD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SERVICE="rotaryencoder"

BIN="rotaryencoder"
SRV="rotaryencoder.service"
DEF="rotaryencoder"


BINLocation="/usr/local/bin"
SRVLocation="/etc/systemd/system"
DEFLocation="/etc/default"

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

if [[ $(mount | grep " / " | grep "(ro,") ]]; then
  echo "Root filesystem is readonly, can't install" 
  exit 1
fi

# Exit if we can't find systemctl
command -v systemctl >/dev/null 2>&1 || { echo "This script needs systemd's systemctl manager, Please check path or install manually" >&2; exit 1; }

# stop service, hide any error, as the service may not be installed yet
systemctl stop $SERVICE > /dev/null 2>&1
SERVICE_EXISTS=$(echo $?)

# Clean everything if requested.
if [ "$1" == "clean" ]; then
  echo "Deleting install"
  systemctl disable $SERVICE > /dev/null 2>&1
  if [ -f $BINLocation/$BIN ]; then
    rm -f $BINLocation/$BIN
  fi
  if [ -f $SRVLocation/$SRV ]; then
    rm -f $SRVLocation/$SRV
  fi
  if [ -f $DEFLocation/$DEF ]; then
    rm -f $DEFLocation/$DEF
  fi
  if [ -d $WEBLocation ]; then
    rm -rf $WEBLocation
  fi
  systemctl daemon-reload
  exit
fi

# copy files to locations, but only copy cfg if it doesn;t already exist

cp $BUILD/$BIN $BINLocation/$BIN
cp $BUILD/$SRV $SRVLocation/$SRV

if [ -f $DEFLocation/$DEF ]; then
  echo "EotaryEncoder defaults exists, did not copy new defaults to $DEFLocation/$DEF"
else
  cp $BUILD/$DEF.defaults $DEFLocation/$DEF
fi


systemctl enable $SERVICE
systemctl daemon-reload

if [ $SERVICE_EXISTS -eq 0 ]; then
  echo "Starting daemon $SERVICE"
  systemctl start $SERVICE
else
  echo "Please edit $DEFLocation/$DEF, then start RotaryEncoder service using \"sudo systemctl start $SERVICE\""
fi

