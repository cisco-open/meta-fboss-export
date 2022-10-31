#!/bin/bash
#
# Copyright (c) 2022 by Cisco Systems, Inc.
# All rights reserved.
#

is_x86_on() {
    cat /sys/bus/platform/devices/pseq/power_state | grep "on"
}

set_cpld_misc_bmcCpuSpiFlashSel() {

    # Expected default on this register: 0x000c0a03
    # To access SPI flash from BMC we are setting bit 10 & 24
    # as per hardware recommendation. Updating the register to
    # 0x010c0e03 accordingly.

    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x03
    i2cset -f -y 12 0x36 0x0e
    i2cset -f -y 12 0x37 0x0c
    i2cset -f -y 12 0x38 0x01
    i2cset -f -y 12 0x30 0x00

}

unset_cpld_misc_bmcCpuSpiFlashSel() {

    # To restore SPI access back to x86 CPU we are clearing bit 10 & 24
    # as per hardware recommendation. Updating the register to
    # 0x000c0a03 accordingly.

    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x03
    i2cset -f -y 12 0x36 0x0a
    i2cset -f -y 12 0x37 0x0c
    i2cset -f -y 12 0x38 0x00
    i2cset -f -y 12 0x30 0x00

}

bmc_create_bios_mtd() {

    # check if mtd partition alredy exists
    part_str=$(cat /proc/mtd | grep "bios" | awk '{print $1;}')
    mtd_part="${part_str%?}"

    # return if mtd partition already exists
    if [ "" != "${mtd_part}" ]; then
        return
    fi

    # setup the spi mux from cpu-cpld
    set_cpld_misc_bmcCpuSpiFlashSel

    if [ -w /sys/bus/platform/drivers/spi-aspeed-smc/1e630000.spi ]; then
        echo 1e630000.spi > /sys/bus/platform/drivers/spi-aspeed-smc/unbind
    fi
    echo 1e630000.spi > /sys/bus/platform/drivers/spi-aspeed-smc/bind

}


