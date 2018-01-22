opkg install kmod-rt2800-lib kmod-rt2800-usb kmod-rt2x00-lib kmod-rt2x00-usb

dd if=/home/username/Downloads/openwrt-brcm2708-bcm2709-sdcard-vfat-ext4.img of=/dev/sdX bs=2M conv=fsync



openwrt


dd if=/dev/mtdblock/1 of=/tmp/backup20080704.trx
scp backup20080704.trx pp:/mirror/firewall/


The upload the firmware image, and reflash from within OpenWRT
mtd -r write firmware.trx linux