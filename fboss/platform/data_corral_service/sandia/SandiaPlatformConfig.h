/**
 * @file SandiaPlatformConfig.h
 *
 * @brief Cisco-8000 implementation of Sandia DataCorral service
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/data_corral_service/darwin/DarwinPlatformConfig.h
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#pragma once

#include <string>

namespace facebook::fboss::platform::data_corral_service {

std::string getSandiaPlatformConfig();

} // namespace facebook::fboss::platform::data_corral_service
