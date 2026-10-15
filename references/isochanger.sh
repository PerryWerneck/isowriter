#!/bin/bash

tmpfile=$(mktemp)

wget \
	--output-document=${tmpfile} \
	https://zypper.intranet.bb.com.br/sle/16/16.0/iso/netinstall.iso

if [ "$?" != "0" ]; then
	exit -1
fi

xorriso \
	-indev ${tmpfile} \
	-outdev tmf-ag.iso \
	-map ./tmfag.cfg /boot/grub2/grub.cfg \
	-boot_image any replay \
	-compliance no_emul_doc \
	-padding included
if [ "$?" != "0" ]; then
	exit -1
fi

rm -f ${tmpfile}
