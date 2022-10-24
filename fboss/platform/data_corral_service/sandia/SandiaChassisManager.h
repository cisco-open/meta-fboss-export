/**
 * @file SandiaChassisManager.h
 *
 * @brief Cisco-8000 implementation of Sandia DataCorral service
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/data_corral_service/darwin/DarwinChassisManager.h
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#pragma once

#include <fboss/platform/data_corral_service/ChassisManager.h>

namespace facebook::fboss::platform::data_corral_service {

enum SandiaLedColor {
  OFF = 0,
  RED = 1,
  GREEN = 2,
  AMBER = 3,
  BLUE = 4,
};

class SandiaChassisLed {
 public:
  explicit SandiaChassisLed(const std::string& name)
      : name_(name), color_(SandiaLedColor::OFF) {}
  void setColorPath(SandiaLedColor color, const std::string& path);
  const std::string& getName() {
    return name_;
  }
  void setColor(SandiaLedColor color);
  SandiaLedColor getColor();

 private:
  std::string name_;
  SandiaLedColor color_;
  std::unordered_map<SandiaLedColor, std::string> paths_;
};

class SandiaChassisManager : public ChassisManager {
 public:
  explicit SandiaChassisManager(int refreshInterval)
      : ChassisManager(refreshInterval) {}
  virtual void initModules() override;
  virtual void programChassis() override;

 private:
  std::unique_ptr<SandiaChassisLed> sysLed_;
  std::unique_ptr<SandiaChassisLed> fanLed_;
  std::unique_ptr<SandiaChassisLed> scmLed_;
  std::unique_ptr<SandiaChassisLed> smbLed_;
  std::unique_ptr<SandiaChassisLed> fan0Led_;
  std::unique_ptr<SandiaChassisLed> fan1Led_;
  std::unique_ptr<SandiaChassisLed> fan2Led_;
  std::unique_ptr<SandiaChassisLed> fan3Led_;
  std::unique_ptr<SandiaChassisLed> fan4Led_;
  std::unique_ptr<SandiaChassisLed> psuLed_;
  std::unique_ptr<SandiaChassisLed> pim1Led_;
  std::unique_ptr<SandiaChassisLed> pim2Led_;
  std::unique_ptr<SandiaChassisLed> pim3Led_;
  std::unique_ptr<SandiaChassisLed> pim4Led_;
  std::unique_ptr<SandiaChassisLed> pim5Led_;
  std::unique_ptr<SandiaChassisLed> pim6Led_;
  std::unique_ptr<SandiaChassisLed> pim7Led_;
  std::unique_ptr<SandiaChassisLed> pim8Led_;
};

} // namespace facebook::fboss::platform::data_corral_service
