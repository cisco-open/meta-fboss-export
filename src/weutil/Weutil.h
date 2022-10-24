/**
 * @file Weutil.h
 *
 * @brief Cisco-8000 implementation of weutil utility
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/weutil/WeutilInterface.h
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#pragma once

#include <memory>
#include "fboss/platform/weutil/WeutilInterface.h"
#include "fboss/platform/weutil/WeutilPlatform.h"

namespace facebook::fboss::platform {

std::unique_ptr<WeutilInterface> get_plat_weutil(void) {
    return get_cisco_weutil();
}

}
