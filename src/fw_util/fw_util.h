/**
 * @file fw_util.h
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

#pragma once

#include <memory>
#include "fw_util/FirmwareUpgradeInterface.h"

namespace facebook::fboss::platform::fw_util {

std::unique_ptr<FirmwareUpgradeInterface> get_plat_type(std::string &);
void init_cisco();
void init_sandia();
void init_lassen();
}
