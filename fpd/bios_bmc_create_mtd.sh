#!/bin/bash
#
# Shell Script to create fpd mtd partition
#
# Copyright (c) 2022 by Cisco Systems, Inc.
# All rights reserved.
#

# load helper script
. /opt/cisco/bin/bios_bmc_helper.sh

#check if x86 is on
is_x86_on
if [ $? == 0 ]; then
    echo "Turn off the host before invoking this command. Skipping!!"
    exit 1
fi

#create mtd partition
bmc_create_bios_mtd

