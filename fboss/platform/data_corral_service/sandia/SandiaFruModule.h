/**
 * @file SandiaFruModule.h
 *
 * @brief Cisco-8000 implementation of Sandia DataCorral service
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/data_corral_service/darwin/DarwinFruModule.h
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#pragma once

#include <fboss/platform/data_corral_service/FruModule.h>

namespace facebook::fboss::platform::data_corral_service {

class SandiaFruModule : public FruModule {
 public:
  explicit SandiaFruModule(std::string id) : FruModule(id) {}
  void refresh() override;
  void init(std::vector<AttributeConfig>& attrs) override;

 private:
  std::string presentPath_;
};

} // namespace facebook::fboss::platform::data_corral_service
