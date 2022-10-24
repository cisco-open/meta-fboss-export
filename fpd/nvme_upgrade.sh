#!/bin/bash

image_name=$1

SLOT=2
SMART_VENDOR_ID=0x1235
MICRON_VENDOR_ID=0x1344

#TODO: TBD to use some data structures to store the variables and have a generic
#      implementation

vendor_id=$(/usr/sbin/smartctl -i /dev/nvme0n1 | grep "PCI Vendor" | awk '{print$4}')
if [[ $? -ne 0 ]]; then
    echo "Failed to get the nvme vendor id"
    exit 1
fi

if [ ! "$image_name" ];then
    if [[ "$vendor_id" == "$SMART_VENDOR_ID" ]];then 
        image_name='/opt/cisco/fpd/nvme/smart_nvme_kodiak_upgrade_dlmc.bin'
        SLOT=1
    elif [[ "$vendor_id" == "$MICRON_VENDOR_ID" ]];then
        SLOT=2
        model=$(nvme list | grep /dev/nvme | awk '{print $3}')
        if [[ $? -ne 0 ]]; then
            echo "Failed to get the nvme model"
            exit 1
        fi
        if [[ "$model" == *"Micron_7400"* ]]; then
            image_name='/opt/cisco/fpd/nvme/micron_7400_nvme_kodiak_release.ubi'
        elif [[ "$model" == *"Micron_7450"* ]]; then
            image_name='/opt/cisco/fpd/nvme/micron_7450_nvme_kodiak_release.ubi'
        else
            echo "FW Upgrade not supported on this model: $model"
            exit 1
        fi
    else
        echo "FW Upgrade not supported for vendor_id: $vendor_id"
        exit 1
    fi
else
    # By default slot is set to 2 but SMART uses slot 1
    if [[ "$vendor_id" == "$SMART_VENDOR_ID" ]];then
        SLOT=1
    fi
fi

echo "Downloading file into drive: $image_name"
sudo nvme fw-download /dev/nvme0n1 --fw=$image_name
if [[ $? -ne 0 ]]; then
    echo "Failed to download image into drive"
    exit 1
fi
echo "Image downloaded into the drive"

echo "Verify and commit firmware.."
sudo nvme fw-commit /dev/nvme0n1 --slot=$SLOT --action=1
if [[ $? -ne 0 ]]; then
    echo "Failed to upgrade the firmware"
    exit 1
fi
echo "Firmware upgrade done"

