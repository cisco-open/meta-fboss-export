/**
 * @file SandiaFruModule.cpp
 *
 * @brief Cisco-8000 implementation of Sandia DataCorral service
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/data_corral_service/darwin/DarwinFruModule.cpp
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#include <fboss/lib/CommonFileUtils.h>
#include <fboss/platform/data_corral_service/sandia/SandiaFruModule.h>
#include <folly/logging/xlog.h>
#include <filesystem>

namespace facebook::fboss::platform::data_corral_service {

void SandiaFruModule::init(std::vector<AttributeConfig>& attrs) {
  XLOG(DBG2) << "init " << getFruId();
  for (auto attr : attrs) {
    if (*attr.name() == "present") {
      presentPath_ = *attr.path();
    }
  }
  refresh();
}

void SandiaFruModule::refresh() {
  if (std::filesystem::exists(std::filesystem::path(presentPath_))) {
    std::string presence = facebook::fboss::readSysfs(presentPath_);
    try {
      isPresent_ = (std::stoi(presence) > 0);
    } catch (const std::exception& ex) {
      XLOG(ERR) << "failed to parse present state from " << presentPath_
                << " where the value is " << presence;
      throw;
    }
    XLOG(DBG4) << "refresh " << getFruId() << " present state is " << isPresent_
               << " after reading " << presentPath_;
  } else {
    XLOG(ERR) << "\"" << presentPath_ << "\""
              << " does not exists";
  }
}

} // namespace facebook::fboss::platform::data_corral_service
