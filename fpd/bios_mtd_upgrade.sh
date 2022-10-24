#!/bin/bash

# load helper script
. /opt/cisco/bin/bios_bmc_create_mtd.sh

image_name=$1
mtd_name=$2

IMAGE_FILE=/mnt/data1/bios_full.bin
UPGRADE_FILE1=/mnt/data1/bios_upgrade1.bin
UPGRADE_FILE2=/mnt/data1/bios_upgrade2.bin
TEMP_FILE=/mnt/data1/bios_temp.bin

create_images() {

    # Extract metadata
    sudo dd if=$image_name bs=128 skip=1 of=$IMAGE_FILE
    if [[ $? -ne 0 ]]; then
        echo "Failed to create bin upgrade file"
        exit 1
    fi

    if [[ "$mtd_name" == "bios_full" ]]; then
        UPGRADE_FILE=$IMAGE_FILE
    else
        # Generate upgrade image of lower 8MB
        sudo dd if=$IMAGE_FILE bs=256 skip=32768 of=$TEMP_FILE
        if [[ $? -ne 0 ]]; then
            echo "Failed to create bin upgrade file"
            exit 1
        fi

        #create image for 1st partition
        sudo dd if=$TEMP_FILE bs=256 count=23312 of=$UPGRADE_FILE1
        if [[ $? -ne 0 ]]; then
            echo "Failed to create first upgrade file"
            exit 1
        fi

        #create image for 2nd partition
        sudo dd if=$TEMP_FILE bs=256 skip=23664 of=$UPGRADE_FILE2
        if [[ $? -ne 0 ]]; then
            echo "Failed to create second upgrade file"
            exit 1
        fi
    fi

    echo "Upgrade image created"
}


program_image() {
    #check if x86 is on
    is_x86_on
    if [ $? == 0 ]; then
        echo "Turn off the host before invoking this command. Skipping uprade!!"
        exit 1
    fi

    #create mtd partition
    bmc_create_bios_mtd

    if [[ "$mtd_name" == "bios_upper" ]]; then
        # Get mtd partition
        part_str=$(cat /proc/mtd | grep bios_upper1 | awk '{print $1;}')
        mtd_part1="${part_str%?}"
        echo "Micronit MTD partition1: $mtd_part1"

        part_str=$(cat /proc/mtd | grep bios_upper2 | awk '{print $1;}')
        mtd_part2="${part_str%?}"
        echo "Micronit MTD partition2: $mtd_part2"

        # Program microinit image
        echo "Program first bios upper region"
        sudo flashcp -v $UPGRADE_FILE1 /dev/$mtd_part1
        if [[ $? -ne 0 ]]; then
            echo "Programming microinit image FAILED"
            exit 1
        fi

        echo "Program second bios upper region"
        sudo flashcp -v $UPGRADE_FILE2 /dev/$mtd_part2
        if [[ $? -ne 0 ]]; then
            echo "Programming microinit image FAILED"
            exit 1
        fi
    else 
        # Get mtd partition
        part_str=$(cat /proc/mtd | grep $mtd_name | awk '{print $1;}')
        mtd_part="${part_str%?}"
        echo "Micronit MTD partition: $mtd_part"

        # Program microinit image
        echo "Program bios full region"
        sudo flashcp -v $UPGRADE_FILE /dev/$mtd_part
        if [[ $? -ne 0 ]]; then
            echo "Programming microinit image FAILED"
            exit 1
        fi
    fi
    echo "Success to program Bios image into the flash"
}

# extract metadata and create upgrade image
create_images

# Program the image
program_image
