#!/bin/bash

# This thread stuff detects the number of cores 
# and actually sets the -j flag for you :)
THREADS=$(expr 1 + $(grep processor /proc/cpuinfo | wc -l))

# This cleans out crud and makes new config
make clean -j$THREADS
make mrproper -j$THREADS
rm -rf ../package/system/lib/modules/*.ko
rm -f ../package/zImage
rm -f ../package/ramdisk.gz
#make plague_defconfig ARCH=arm CROSS_COMPILE=arm-eabi- -j5

make ARCH=arm plague_defconfig
# Finally making the kernel
#export ARCH=arm
#export CROSS_COMPILE=arm-eabi-
#export PATH=$PATH:~/Adroid/CM9/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin
make -j$THREADS ARCH=arm CROSS_COMPILE=~/Android/CM9/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-


# These move files to easier locations
echo
echo "Copying kernel modules to package/system/lib/modules"
echo
find -name '*.ko' -exec cp -av {} ../package/system/lib/modules/ \;
cp arch/arm/boot/zImage ../package/

# This part packs the img up :)
echo
echo "Packing boot.img ;)"
echo
../package/mkbootfs ../package/ramdisk_tmp | gzip > ../package/ramdisk.gz
../package/mkbootimg --kernel ../package/zImage --ramdisk ../package/ramdisk.gz --cmdline "console=ttyMSM0,115200,n8 androidboot.hardware=m3" -o ../package/boot.img --base 0x00200000
