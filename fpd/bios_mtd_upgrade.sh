#!/bin/bash

# load helper script
. /opt/cisco/bin/bios_bmc_helper.sh

image_name=$1
mtd_name=$2

IMAGE_FILE=/mnt/data1/bios_full.bin
UPGRADE_FILE=/mnt/data1/bios_upgrade.bin

create_images() {

    # Extract metadata
    sudo dd if=$image_name bs=128 skip=1 of=$IMAGE_FILE
    if [[ $? -ne 0 ]]; then
        echo "Failed to create bin upgrade file"
        unset_cpld_misc_bmcCpuSpiFlashSel
        exit 1
    fi

    if [[ "$mtd_name" == "bios_full" ]]; then
        UPGRADE_FILE=$IMAGE_FILE
    else
        # Generate upgrade image of lower 8MB
        sudo dd if=$IMAGE_FILE bs=256 skip=32768 of=$UPGRADE_FILE
        if [[ $? -ne 0 ]]; then
            echo "Failed to create bin upgrade file"
            unset_cpld_misc_bmcCpuSpiFlashSel
            exit 1
        fi
    fi

    echo "Upgrade image created"
}


program_image() {
    #check if x86 is on
    is_x86_on
    if [ $? == 0 ]; then
        echo "BIOS SPI flash upgrade requires x86 in power off state. Skipping upgrade!!"
        unset_cpld_misc_bmcCpuSpiFlashSel
        exit 1
    fi

    #create mtd partition
    bmc_create_bios_mtd

    if [[ "$mtd_name" == "bios" ]]; then
        # Program microinit image
        echo "Program bios region"
    else 
        # Program microinit image
        echo "Program bios full region"
    fi

    # Get mtd partition
    part_str=$(cat /proc/mtd | grep -w $mtd_name | awk '{print $1;}')
    mtd_part="${part_str%?}"
    echo "Micronit MTD partition: $mtd_part"

    sudo flashcp -v $UPGRADE_FILE /dev/$mtd_part
    if [[ $? -ne 0 ]]; then
        echo "Programming microinit image FAILED"
        unset_cpld_misc_bmcCpuSpiFlashSel
        exit 1
    fi

    echo "Successfully programmed x86 bios image"
}

# extract metadata and create upgrade image
create_images

# Program the image
program_image

# Unselect MUX
unset_cpld_misc_bmcCpuSpiFlashSel
