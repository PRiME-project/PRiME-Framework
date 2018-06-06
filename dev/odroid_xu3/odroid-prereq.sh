#!/bin/bash

# Script to configure prerequisites for the Odroid XU3/XU4 for the PRiME demonstrator.
# This will install several packages required for executing the UI and rebuilding the kernel.
# Kernel source will be downloaded and placed in /usr/src, config updated to enable PMC, rebuilt and installed

# Kernel install script written with reference to http://odroid.com/dokuwiki/doku.php?id=en:xu3_building_kernel
# and http://odroid.us/mediawiki/index.php?title=Step-by-step_Native_Compiling_a_Kernel
# and https://github.com/umiddelb/armhf/wiki/How-To-compile-a-custom-Linux-kernel-for-your-ARM-device
#
# Graeme Bragg, g.bragg@ecs.soton.ac.uk, April 2017

clear

if [[ $UID != 0 ]]; then
    echo -e "\nPlease run this script with sudo next time:"
    echo -e "\e[1m\e[97msudo $0 $*\e[0m \n"
    echo -e "\e[1m\e[33mI'll do it for you this time though...\e[0m"
    sudo $0 "$@"
    exit 1
fi

echo -e "\n\n\e[1m\e[32mPRiME Pre-requisites install script for the Odroid XU3/XU4\e[0m"
echo -e "This will install pre-requisites from apt, install npyscreen"
echo -e "and download, build and install a modified Kernel to enable"
echo -e "userspace PMU access.\n"
echo -e "This will \e[1m\e[31mreplace\e[0m the current kernel with 3.10.105-PRiME and"
echo -e "reboot.\n"
echo -e "This has been tested on the Mate and Minimal build images"
echo -e "with the 3.10.103 kernel but will \e[1m\e[31mNOT\e[0m work on images using"
echo -e "a 4.x Kernel.\n"
read  -p "Press Enter to Start"
read  -p "Are you sure?"


echo -e "\e[1m\e[32mFix for Minimal image locale issue\e[0m"
locale-gen "en_GB.UTF-8" || exit
export LC_ALL=C

echo -e "\e[1m\e[32mInstalling pre-requisites\e[0m"
apt-get update
apt-get install --assume-yes git cmake libopencv-dev libboost-all-dev python3-pip gcc build-essential \
	libncurses5-dev p7zip-full sl flex bison libdw-dev libnewt-dev bc \
	binutils-dev libaudit-dev libgtk2.0-dev libperl-dev libpython-dev \
	libunwind-* unzip rand telnet mali-fbdev sysfsutils libgsl-dev libgsl2 libgsl-dbg\
	|| exit

sudo -u $(logname) pip3 install --upgrade pip || exit
pip3 install npyscreen || exit



echo -e "\e[1m\e[32mGetting the Kernel Source\e[0m"
mkdir -p /usr/src || exit
chmod 4777 /usr/src || exit
cd /usr/src || exit

# Tidy up any previous source zips
if [ -f odroidxu3-3.10.y.zip ]; then
	echo -e "\e[1m\e[33mRemoving existing kernel source zip\e[0m"
	sudo -u $(logname) rm -f odroidxu3-3.10.y.zip || exit
fi

# Get the Source
sudo -u $(logname) wget --no-check-certificate https://github.com/PRiME-project/linux/archive/odroidxu3-3.10.y.zip || exit

echo -e "\e[1m\e[32mUnzipping Kernel Source\e[0m"
# Unzip the source
if [ -d linux-odroidxu3-3.10.y ]; then
	echo -e "\e[1m\e[33mRemoving existing kernel source directory\e[0m"
	rm -rf linux-odroidxu3-3.10.y || exit
fi

sudo -u $(logname) unzip -qo odroidxu3-3.10.y.zip || exit
sudo -u $(logname) chmod 775 linux-odroidxu3-3.10.y || exit
#sudo rm odroidxu3-3.10.y.zip


echo -e "\e[1m\e[32mBuilding the Kernel\e[0m"
cd linux-odroidxu3-3.10.y || exit
sudo -u $(logname) make odroidxu3_defconfig || exit
sudo -u $(logname) make -j 8 || exit


echo -e "\e[1m\e[32mPut everything in the right place\e[0m"
kver=$(make kernelrelease) 

cp arch/arm/boot/zImage arch/arm/boot/dts/exynos5422-odroidxu3.dtb /media/boot || exit
cp .config /boot/config-${kver} || exit


echo -e "\e[1m\e[32mInstalling modules, firmware & headers\e[0m"
make modules_install || exit
make firmware_install || exit
make headers_install INSTALL_HDR_PATH=/usr || exit


echo -e "\e[1m\e[32mUpdating Initrd\e[0m"
update-initramfs -c -k ${kver} || exit
mkimage -A arm -O linux -T ramdisk -C none -a 0 -e 0 -n uInitrd -d /boot/initrd.img-${kver} /boot/uInitrd-${kver} || exit
cp /boot/uInitrd-${kver} /media/boot/uInitrd || exit


echo -e "\e[1m\e[32mCreating Symlinks\e[0m"
if [[ ( -L /usr/src/linux-source-${kver} ) || ( -L /usr/src/linux  ) || ( -L /lib/modules/${kver} ) ]]; then
	echo -e "\e[1m\e[33mRemoving existing symlinks\e[0m"
	rm -f /usr/src/linux-source-${kver} || exit
	rm -f /usr/src/linux || exit
	rm -f /lib/modules/${kver}/build || exit
fi

ln -s /usr/src/linux-odroidxu3-3.10.y /usr/src/linux-source-${kver} || exit
ln -s /usr/src/linux-odroidxu3-3.10.y /usr/src/linux || exit
ln -s /usr/src/linux-odroidxu3-3.10.y /lib/modules/${kver}/build || exit


echo -e "\e[1m\e[32mPrepare for building Kernel Modules\e[0m"
cd /lib/modules/${kver}/build || exit
sudo -u $(logname) make prepare || exit
sudo -u $(logname) make modules_prepare || exit


echo -e "\e[1m\e[32mBuilding Perf\e[0m"
make -j 8 -C tools liblk || exit
make -j 8 -C tools perf_install || exit
cp tools/perf/perf /usr/bin || exit

echo -e "\e[1m\e[32mFixing CPU Control Permissions\e[0m"
echo "mode devices/system/cpu/cpu0/cpufreq/scaling_governor = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu1/cpufreq/scaling_governor = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu2/cpufreq/scaling_governor = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu3/cpufreq/scaling_governor = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu4/cpufreq/scaling_governor = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu5/cpufreq/scaling_governor = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu6/cpufreq/scaling_governor = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu7/cpufreq/scaling_governor = 666" | sudo tee --append /etc/sysfs.conf > /dev/null

echo "mode devices/system/cpu/cpu0/cpufreq/scaling_setspeed = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu1/cpufreq/scaling_setspeed = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu2/cpufreq/scaling_setspeed = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu3/cpufreq/scaling_setspeed = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu4/cpufreq/scaling_setspeed = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu5/cpufreq/scaling_setspeed = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu6/cpufreq/scaling_setspeed = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu7/cpufreq/scaling_setspeed = 666" | sudo tee --append /etc/sysfs.conf > /dev/null

echo "mode devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq = 444" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu1/cpufreq/cpuinfo_cur_freq = 444" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu2/cpufreq/cpuinfo_cur_freq = 444" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu3/cpufreq/cpuinfo_cur_freq = 444" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu4/cpufreq/cpuinfo_cur_freq = 444" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu5/cpufreq/cpuinfo_cur_freq = 444" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu6/cpufreq/cpuinfo_cur_freq = 444" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode devices/system/cpu/cpu7/cpufreq/cpuinfo_cur_freq = 444" | sudo tee --append /etc/sysfs.conf > /dev/null

echo "mode class/misc/mali0/device/clock = 666" | sudo tee --append /etc/sysfs.conf > /dev/null
echo "mode class/misc/mali0/device/dvfs = 666" | sudo tee --append /etc/sysfs.conf > /dev/null


echo -e "\e[1m\e[32mPre-requisites succesfully installed &\e[0m"
echo -e "\e[1m\e[32mKernel ${KVER} successfully compiled\e[0m"
read  -p "Press Enter to Reboot"
su -c "sl -l && exit" $(logname) 
reboot
