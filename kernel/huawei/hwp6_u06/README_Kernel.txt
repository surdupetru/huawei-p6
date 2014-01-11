################################################################################

1. How to Build
	- get Toolchain
		From android git server , codesourcery and etc ..
		 - arm-linux-androideabi-4.6

	- edit Makefile
		edit "CROSS_COMPILE" to right toolchain path(You downloaded).
		  EX)   CROSS_COMPILE= $(android platform directory you download)/android/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin/arm-linux-androideabi-
          Ex)   CROSS_COMPILE=/usr/local/toolchain/arm-linux-androideabi-4.6/bin/arm-linux-androideabi-          // check the location of toolchain
		  or
		  Ex)	export CROSS_COMPILE=arm-linux-androideabi-
		  Ex)	export PATH=$PATH:<toolchain_parent_dir>/arm-linux-androideabi-4.6/bin

		$ make ARCH=arm hisi_k3v2oem1_defconfig
		$ make ARCH=arm zImage

2. Output files
	- Kernel : arch/arm/boot/zImage
	- module : drivers/*/*.ko

3. How to Clean
		$ make ARCH=arm distclean
################################################################################
