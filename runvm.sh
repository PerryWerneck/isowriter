#!/bin/bash
ISONAME=/tmp/test.iso
IMGNAME=$(basename ${ISONAME} | sed "s@.iso@@g")
IMGFILE=/tmp/reinstall.qcow2
NVRAM=/tmp/reinstall.nvram
HDLEN=40G
XML="kvm/efi-usb.xml"

if [ -z ${LIBVIRT_DEFAULT_URI} ]; then
	echo "LIBVIRT_DEFAULT_URI is not defined"
	exit -1
fi

meson compile -C .build
if [ "$?" != "0" ]; then
	exit $?
fi

.build/isowriter -O ${ISONAME} -i
if [ "$?" != "0" ]; then
	exit $?
fi

sudo qemu-img \
	create \
	-f qcow2 \
	${IMGFILE} \
	${HDLEN}
if [ "$?" != "0" ]; then
	exit -1
fi

virsh undefine --nvram --remove-all-storage --tpm reinstall 2> /dev/null
virsh define ${XML}
	
if [ "$?" != "0" ]; then
	rm -f ${IMGFILE}
	exit -1
fi



