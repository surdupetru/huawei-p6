#!/system/bin/sh

BUSYBOX="/system/xbin/busybox"


${BUSYBOX} mount -o remount,rw rootfs /
${BUSYBOX} mount -o remount,rw /system

${BUSYBOX} cp -r /system/bin/adbd /sbin/adbd
${BUSYBOX} chown 0.0 /sbin/adbd
${BUSYBOX} chmod 0750 /sbin/adbd

${BUSYBOX} mount -o remount,ro rootfs /
${BUSYBOX} mount -o remount,ro /system