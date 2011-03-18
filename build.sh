#!/bin/bash -eu
# Build x-loader

target=x-load.bin

workspace=$PWD/..
PATH=/usr/lib/jvm/java-1.5.0-sun/bin:/usr/bin:/bin
PATH+=:$workspace/android-kernel/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin

exec &> OUT

# Preconfigure
if [ ! -e include/config.mk ]; then
  make omap3530lv_som_config
fi

# Erase existing binary
if [ -e $target ]; then
  rm -rf $target
fi

# Build
make CROSS_COMPILE=arm-eabi- all

if [ $? -ne 0 ] || [ ! -e $target ]; then
  echo FAIL
  exit 1
fi

# Install
scripts/signGP
cp x-load.bin.ift MLO

echo SUCCESS

exit 0
