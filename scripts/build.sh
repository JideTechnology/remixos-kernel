#!/bin/bash
set -e

#Setup common variables
export ARCH=arm
if [ -n "`echo ${LICHEE_CHIP} | grep "sun5[0-9]i"`" ]; then \
	export ARCH=arm64
fi

export CROSS_COMPILE=${ARCH}-linux-gnueabi-
if [ -d "${LICHEE_TOOLCHAIN_PATH}" ]; then
	GCC=$(find ${LICHEE_TOOLCHAIN_PATH} -perm /a+x -a -regex '.*-gcc');
	export CROSS_COMPILE="${GCC%-*}-";
elif [ -n "${LICHEE_CROSS_COMPILER}" ]; then
	export CROSS_COMPILE="${LICHEE_CROSS_COMPILER}-"
fi

export AS=${CROSS_COMPILE}as
export LD=${CROSS_COMPILE}ld
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export NM=${CROSS_COMPILE}nm
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump
export LOCALVERSION=""
export MKBOOTIMG=${LICHEE_TOOLS_DIR}/pack/pctools/linux/android/mkbootimg

KERNEL_VERSION=`make -s kernelversion -C ./`
LICHEE_KDIR=`pwd`
LICHEE_MOD_DIR=${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}
export LICHEE_KDIR

update_kern_ver()
{
	if [ -r include/generated/utsrelease.h ]; then
		KERNEL_VERSION=`cat include/generated/utsrelease.h |awk -F\" '{print $2}'`
	fi
	LICHEE_MOD_DIR=${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}
}

show_help()
{
	printf "
Build script for Lichee platform

Invalid Options:

	help         - show this help
	kernel       - build kernel
	modules      - build kernel module in modules dir
	clean        - clean kernel and modules

"
}

NAND_ROOT=${LICHEE_KDIR}/modules/nand

build_nand_lib()
{
	echo "build nand library"
}


build_gpu()
{
	echo "build GPU"
}


clean_gpu()
{
    echo clean gpu module 
}


build_kernel()
{
	echo "Building kernel"

	cd ${LICHEE_KDIR}

	rm -rf output/
	echo "${LICHEE_MOD_DIR}"
	mkdir -p ${LICHEE_MOD_DIR}

	# We need to copy rootfs files to compile kernel for linux image

	if [ ! -f .config ] ; then
		printf "\n\033[0;31;1mUsing default config ${LICHEE_KERN_DEFCONF} ...\033[0m\n\n"
		cp arch/arm/configs/${LICHEE_KERN_DEFCONF} .config
	fi


	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j${LICHEE_JLEVEL} all modules
	update_kern_ver

	#The Image is origin binary from vmlinux.
	cp -vf arch/arm/boot/Image      output/bImage
	cp -vf arch/arm/boot/[zu]Image  output/
	cp -vf arch/arm/boot/Image.gz   output/

	cp .config output/

	tar -jcf output/vmlinux.tar.bz2 vmlinux


	for file in $(find drivers sound crypto block fs security net -name "*.ko"); do
		cp $file ${LICHEE_MOD_DIR}
	done
	cp -f Module.symvers ${LICHEE_MOD_DIR}

}

build_modules()
{
	echo "Building modules"

}

regen_rootfs_cpio()
{
	echo "regenerate rootfs cpio"

}

build_ramfs()
{
	#local bss_sz=0;
	#local CHIP="";

	#local BIMAGE="output/bImage";
	#local RAMDISK="output/rootfs.cpio.gz";
	#local BASE="";
	#local OFFSET="";

	## update rootfs.cpio.gz with new module files
	#regen_rootfs_cpio

	#CHIP=`echo ${LICHEE_CHIP} | sed -e 's/.*\(sun[0-9x]i\).*/\1/g'`;

	#if [ "${CHIP}" = "sun9i" ]; then
	#	BASE="0x20000000";
	#else
	#	BASE="0x40000000";
	#fi

	#if [ -n "`echo ${LICHEE_CHIP} | sed -n -e 's/.*\(sun8iw8\).*/\1/pg'`" ]; then
	#	BIMAGE="output/zImage";
	#fi

	#if [ -f vmlinux ]; then
	#	bss_sz=`${CROSS_COMPILE}readelf -S vmlinux | \
	#		awk  '/\.bss/ {print strtonum("0x"$5)+strtonum("0x"$6)}'`;
	#fi
	##bss_sz=`du -sb ${BIMAGE} | awk '{printf("%u", $1)}'`;
	##
	## If the size of bImage larger than 16M, will offset 0x02000000
	##
	#if [ ${bss_sz} -gt $((16#1000000)) ]; then
	#	OFFSET="0x02000000";
	#else
	#	OFFSET="0x01000000";
	#fi

	#${MKBOOTIMG} --kernel ${BIMAGE} \
	#	--ramdisk ${RAMDISK} \
	#	--board ${CHIP} \
	#	--base ${BASE} \
	#	--ramdisk_offset ${OFFSET} \
	#	-o output/boot.img
	
	# If uboot use *bootm* to boot kernel, we should use uImage.
	echo build_ramfs
    	echo "Copy boot.img to output directory ..."
    	cp output/boot.img   ${LICHEE_PLAT_OUT}
    	cp output/uImage     ${LICHEE_PLAT_OUT}
    	cp output/zImage     ${LICHEE_PLAT_OUT}
	cp output/Image.gz   ${LICHEE_PLAT_OUT}
	cp output/vmlinux.tar.bz2 ${LICHEE_PLAT_OUT}

        if [ ! -f output/arisc ]; then
        	echo "arisc" > output/arisc
        fi
        cp output/arisc    ${LICHEE_PLAT_OUT}
}

gen_output()
{
    if [ "x${LICHEE_PLATFORM}" = "xandroid" ] ; then
        echo "Copy modules to target ..."
        rm -rf ${LICHEE_PLAT_OUT}/lib
        cp -rf ${LICHEE_KDIR}/output/* ${LICHEE_PLAT_OUT}
        return
    fi

    if [ -d ${LICHEE_BR_OUT}/target ] ; then
        echo "Copy modules to target ..."
        local module_dir="${LICHEE_BR_OUT}/target/lib/modules"
        rm -rf ${module_dir}
        mkdir -p ${module_dir}
        cp -rf ${LICHEE_MOD_DIR} ${module_dir}
    fi
}

clean_kernel()
{
	echo "Cleaning kernel"
	make ARCH=${ARCH} clean
	rm -rf output/*
}

clean_modules()
{
	echo "Cleaning modules"
	clean_gpu
}

#####################################################################
#
#                      Main Runtine
#
#####################################################################

case "$1" in
kernel)
	build_kernel
	;;
modules)
	build_modules
	;;
clean)
	clean_kernel
	clean_modules
	;;
*)
	build_kernel
	build_modules
	build_ramfs
	gen_output
	echo -e "\n\033[0;31;1m${LICHEE_CHIP} compile Kernel successful\033[0m\n\n"
	;;
esac

