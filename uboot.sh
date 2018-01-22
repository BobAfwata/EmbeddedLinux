sudo apt-get install gcc-arm-linux-gnueabi
git clone git://github.com/swarren/u-boot.git
git checkout -b rpi_dev origin/rpi_dev
make CROSS_COMPILE=arm-linux-gnueabi- distclean
make CROSS_COMPILE=arm-linux-gnueabi- rpi_b_config
make CROSS_COMPILE=arm-linux-gnueabi- u-boot.bin