#!/bin/bash

image_name=$1
is_golden=$2

sudo umount /boot/efi

IMAGE_FILE=/boot/efi/EFI/UpdateCapsule/CAPSULE_UPGRADE.bin
create_images() {
    # Generate upgrade metadata image
    sudo dd if=$image_name bs=128 count=1 of=$MDATA_FILE
    if [[ $? -ne 0 ]]; then
        echo "Failed to create metadata file"
        exit 1
    fi

    #check for version and build requirement
    tail $MDATA_FILE -c +57 | head -c +4 > /tmp/version.txt
    if [[ $? -ne 0 ]]; then
        echo "Failed to create version info file"
        rm $MDATA_FILE
        exit 1
    fi
    ver=$(cat /tmp/version.txt | awk '{print $1}')
    if [[ $? -ne 0 ]]; then
        echo "Failed to get version from mdata"
        rm $MDATA_FILE
        exit 1
    fi
    board_rev=$(weutil | grep "Product Production State" | awk '{print $4}')
    if [[ $? -ne 0 ]]; then
        echo "Failed to get board revision"
        rm $MDATA_FILE
        exit 1
    fi
    verArr=(${ver//./ })
    major=${verArr[0]}
    minor=${verArr[1]}
    echo "Upgrade image version: major=$major minor=$minor"
    if [[ "$major" == "0" ]] && [[ "$minor" > "34" ]]; then
        if [[ "$board_rev" < "2" ]]; then
            echo "Board Revision must be atleast P2 for bios to upgrade. Skipping!"
            rm $MDATA_FILE
            exit 1
        fi
    fi

    echo "Meta data image created"

    # Generate capsule image
    sudo dd if=$image_name bs=128 skip=1 of=$IMAGE_FILE
    if [[ $? -ne 0 ]]; then
        echo "Failed to create Capsule Upgrade File"
        exit 1
    fi
    echo "Capsule image created"
}

# Create efi directory
mkdir -p /boot/efi
if [[ $? -ne 0 ]]; then
    echo "Failed to create /boot/efi directory"
    exit 1
fi

# Mount efi partition
HDTYPE=/dev/sda1
if [[ -e "$HDTYPE" ]]; then
    sudo mount /dev/sda1 /boot/efi
    if [[ $? -ne 0 ]]; then
        echo "Failed to mount /dev/sda1 at /boot/efi"
        exit 1
    fi
else
    HDTYPE=/dev/nvme0n1
    if [[ -e "$HDTYPE" ]]; then
        sudo mount /dev/nvme0n1p1 /boot/efi
        if [[ $? -ne 0 ]]; then
            echo "Failed to mount /dev/nvme0n1p1 at /boot/efi"
            exit 1
        fi
    else
        echo "Failed to get HD type"
        exit 1
    fi
fi

# Create update capsule directory
sudo mkdir -p /boot/efi/EFI/UpdateCapsule
if [[ $? -ne 0 ]]; then
    echo "Failed to create UpdateCapsule directory"
    exit 1
fi

# Check if meta-data file exists
MDATA_FILE=/boot/efi/EFI/UpdateCapsule/BIOS_PRI_MDATA.bin
if [[ -f "$MDATA_FILE" ]]; then
    echo "BIOS could upgrade only one flash on reload."
    echo "Please activate already upgraded"
    exit 1
fi

rm -f /boot/efi/EFI/UpdateCapsule/*
if [[ $? -ne 0 ]]; then
    echo "Failed to contents of UpdateCapsule directory"
    exit 1
fi

# Check if efivars path exists
EFI_BIOS_UPG_VAR_PATH=/sys/firmware/efi/efivars
if [[ -d "$EFI_BIOS_UPG_VAR_PATH" ]]; then
    echo "EFI VAR PATH exists"
    # Create meta-data and Capsule images
    create_images
else
    echo "EFI VAR PATH Does not exist, existing BIOS upgrade"
    exit 1
fi

# Set OS Indication to upgrade BIOS on next reboot
if [ "$2" = "golden" ]; then
    sudo printf "\x07\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00" > /sys/firmware/efi/efivars/CiscoFlashSelected-59d1c24f-50f1-401a-b101-f33e0daed443
    sudo printf "\x07\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00" > /sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c
    echo "OS Indication set SUCCESS: BIOS Golden will be upgraded on next reboot"
else
    sudo printf "\x07\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00" > /sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c
    echo "OS Indication set SUCCESS: BIOS will be upgraded on next reboot"
fi

