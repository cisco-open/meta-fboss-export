/**
 * @file fw_util.cc
 *
 * @brief Cisco-8000 implementation of fw_util utility
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/fw_util/FirmwareUpgradeInterface.h
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#include <errno.h>
#include <sysexits.h>
#include <string>
#include <vector>

#include <glog/logging.h>

#include "fw_util.h"

using namespace facebook::fboss::platform::fw_util;

int 
main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);

    std::string upgradable_components = "";
    init_cisco();
    auto FirmwareUpgradeInstance = get_plat_type(upgradable_components);
    
    if (FirmwareUpgradeInstance) {
        FirmwareUpgradeInstance->upgradeFirmware(argc, argv, upgradable_components);
        return EX_OK;
    } else {
        LOG(ERROR) << "No platform fw_util available";
        return EX_CONFIG;
    }
}

