#!/bin/bash -eux
# Build x-loader

[ -f ~/.bash-android ] && source ~/.bash-android
[ -f $CLOUDSURFER_ROOT/bin/bash-android ] && source $CLOUDSURFER_ROOT/bin/bash-android

target=$PWD/MLO

make="make -j4"

#exec &> OUT

# Preconfigure
if [ ! -e include/config.mk ]; then
  $make omap3530lv_som_config
fi

# Erase existing binary
if [ -e $target ]; then
  rm -rf $target
fi

# Build
$make CROSS_COMPILE=arm-eabi- all
scripts/signGP
cp x-load.bin.ift $target

if [ $? -ne 0 ] || [ ! -e $target ]; then
  echo FAIL
  exit 1
fi

echo SUCCESS

exit 0
