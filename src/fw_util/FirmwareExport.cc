/**
 * @file FirmwareExport.cc
 *
 * @brief Cisco-8000 implementation of fw_util utility
 *   This file is setup to bridge the export utility.
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

#include "fw_util.h"

void
facebook::fboss::platform::fw_util::init_cisco()
{
    init_sandia();
}
