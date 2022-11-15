/**
 * @file WeutilLassen.cpp
 *
 * @brief Cisco-8000 implementation of board specific data for weutil utility
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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "fboss/platform/weutil/WeutilPlatform.h"
#include "fboss/platform/weutil/WeutilConfig.h"
#include "fboss/platform/weutil/LassenWeutilConfig.h"

using json = nlohmann::json;

namespace facebook::fboss::platform {


std::unique_ptr<WeutilInterface>
get_lassen_weutil()
{
    std::string platform = getLassenIdpromsData();
    std::string weutil = getIdpromsData();

    return std::make_unique<WeutilCisco>(platform, weutil);
}

} // namespace facebook::fboss::platform
