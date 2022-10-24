#!/bin/bash
#
# Copyright (c) 2022 by Cisco Systems, Inc.
# All rights reserved.
#

is_x86_on() {
    cat /sys/bus/platform/devices/pseq/power_state | grep "on"
}

bmc_create_bios_mtd() {
    # check if mtd partition alredy exists
    part_str=$(cat /proc/mtd | grep "bios" | awk '{print $1;}')
    mtd_part="${part_str%?}"

    # return if mtd partition already exists
    if [ "" != "${mtd_part}" ]; then
        return
    fi

    #setup the i2c mux
    i2cset -f -y 12 0x31 0x0c
    i2cset -f -y 12 0x32 0x00
    i2cset -f -y 12 0x33 0x00
    i2cset -f -y 12 0x34 0x00
    i2cset -f -y 12 0x35 0x03
    i2cset -f -y 12 0x36 0x0e
    i2cset -f -y 12 0x37 0x00
    i2cset -f -y 12 0x38 0x01
    i2cset -f -y 12 0x30 0x0

    echo 1e630000.spi > /sys/bus/platform/drivers/aspeed-smc/bind
}
