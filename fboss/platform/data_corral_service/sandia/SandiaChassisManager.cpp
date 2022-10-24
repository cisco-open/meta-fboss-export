/**
 * @file SandiaChassisManager.cpp
 *
 * @brief Cisco-8000 implementation of Sandia DataCorral service
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/data_corral_service/darwin/DarwinChassisManager.cpp
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#include <fboss/lib/CommonFileUtils.h>
#include <fboss/platform/data_corral_service/sandia/SandiaChassisManager.h>
#include <fboss/platform/data_corral_service/sandia/SandiaFruModule.h>
#include <fboss/platform/data_corral_service/sandia/SandiaPlatformConfig.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace {
// modules in sandia system
const std::string kSandiaFan = "SandiaFanModule";
const std::string kSandiaPsu = "SandiaPsuModule";
const std::string kSandiaPim = "SandiaPimModule";

// leds in sandia chasssis
const std::string kSystemLed = "SystemLed";
const std::string kPsuLed   = "PsuLed";
const std::string kScmLed   = "ScmLed";
const std::string kSmbLed   = "SmbLed";

const std::string kFanLed   = "FanLed";
const std::string kFan0Led  = "Fan0Led";
const std::string kFan1Led  = "Fan1Led";
const std::string kFan2Led  = "Fan2Led";
const std::string kFan3Led  = "Fan3Led";
const std::string kFan4Led  = "Fan4Led";

const std::string kPim1Led  = "Pim1Led";
const std::string kPim2Led  = "Pim2Led";
const std::string kPim3Led  = "Pim3Led";
const std::string kPim4Led  = "Pim4Led";
const std::string kPim5Led  = "Pim5Led";
const std::string kPim6Led  = "Pim6Led";
const std::string kPim7Led  = "Pim7Led";
const std::string kPim8Led  = "Pim8Led";

// colors available in leds
const std::string kLedAmber = "Amber";
const std::string kLedBlue  = "Blue";
const std::string kLedGreen = "Green";
const std::string kLedRed   = "Red";
} // namespace

namespace facebook::fboss::platform::data_corral_service {

void SandiaChassisLed::setColorPath(
    SandiaLedColor color,
    const std::string& path) {
  XLOG(DBG2) << "led " << name_ << ", color " << color << ", sysfs path "
             << path;
  paths_[color] = path;
}

void SandiaChassisLed::setColor(SandiaLedColor color) {
  if (color_ != color) {
    if (!facebook::fboss::writeSysfs(paths_[color], "1")) {
      XLOG(ERR) << "failed to set color " << color << " for led " << name_;
      return;
    }
    for (auto const& colorPath : paths_) {
      if (colorPath.first != color) {
        if (!facebook::fboss::writeSysfs(colorPath.second, "0")) {
          XLOG(ERR) << "failed to unset color " << color << " for led "
                    << name_;
          return;
        }
      }
    }
    XLOG(INFO) << "Set led " << name_ << " from color " << color_
               << " to color " << color;
    color_ = color;
  }
}

SandiaLedColor SandiaChassisLed::getColor() {
  for (auto const& colorPath : paths_) {
    std::string brightness = facebook::fboss::readSysfs(colorPath.second);
    try {
      if (std::stoi(brightness) > 0) {
        color_ = colorPath.first;
        return color_;
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "failed to parse present state from " << colorPath.second
                << " where the value is " << brightness;
      throw;
    }
  }
  return SandiaLedColor::OFF;
}

void SandiaChassisManager::initModules() {
  XLOG(DBG2) << "instantiate fru modules and chassis leds";
  auto platformConfig = apache::thrift::SimpleJSONSerializer::deserialize<
      DataCorralPlatformConfig>(getSandiaPlatformConfig());
  for (auto fru : *platformConfig.fruModules()) {
    auto name = *fru.name();
    auto fruModule = std::make_unique<SandiaFruModule>(name);
    fruModule->init(*fru.attributes());
    fruModules_.emplace(name, std::move(fruModule));
  }

  sysLed_  = std::make_unique<SandiaChassisLed>(kSystemLed);
  fanLed_  = std::make_unique<SandiaChassisLed>(kFanLed);
  psuLed_ = std::make_unique<SandiaChassisLed>(kPsuLed);
  scmLed_ = std::make_unique<SandiaChassisLed>(kScmLed);
  smbLed_ = std::make_unique<SandiaChassisLed>(kSmbLed);

  fan0Led_ = std::make_unique<SandiaChassisLed>(kFan0Led);
  fan1Led_ = std::make_unique<SandiaChassisLed>(kFan1Led);
  fan2Led_ = std::make_unique<SandiaChassisLed>(kFan2Led);
  fan3Led_ = std::make_unique<SandiaChassisLed>(kFan3Led);
  fan4Led_ = std::make_unique<SandiaChassisLed>(kFan4Led);

  pim1Led_ = std::make_unique<SandiaChassisLed>(kPim1Led);
  pim2Led_ = std::make_unique<SandiaChassisLed>(kPim2Led);
  pim3Led_ = std::make_unique<SandiaChassisLed>(kPim3Led);
  pim4Led_ = std::make_unique<SandiaChassisLed>(kPim4Led);
  pim5Led_ = std::make_unique<SandiaChassisLed>(kPim5Led);
  pim6Led_ = std::make_unique<SandiaChassisLed>(kPim6Led);
  pim7Led_ = std::make_unique<SandiaChassisLed>(kPim7Led);
  pim8Led_ = std::make_unique<SandiaChassisLed>(kPim8Led);

  for (auto attr : *platformConfig.chassisAttributes()) {

    if (*attr.name() == kSystemLed + kLedAmber) {
      sysLed_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kSystemLed + kLedBlue) {
      sysLed_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kSystemLed + kLedGreen) {
      sysLed_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kSystemLed + kLedRed) {
      sysLed_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kPsuLed + kLedAmber) {
      psuLed_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPsuLed + kLedBlue) {
      psuLed_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPsuLed + kLedGreen) {
      psuLed_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPsuLed + kLedRed) {
      psuLed_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kScmLed + kLedAmber) {
      scmLed_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kScmLed + kLedBlue) {
      scmLed_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kScmLed + kLedGreen) {
      scmLed_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kScmLed + kLedRed) {
      scmLed_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kSmbLed + kLedAmber) {
      smbLed_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kSmbLed + kLedBlue) {
      smbLed_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kSmbLed + kLedGreen) {
      smbLed_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kSmbLed + kLedRed) {
      smbLed_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kFanLed + kLedAmber) {
      fanLed_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kFanLed + kLedRed) {
      fanLed_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kFanLed + kLedBlue) {
      fanLed_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kFanLed + kLedGreen) {
      fanLed_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kFan0Led + kLedAmber) {
      fan0Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kFan0Led + kLedBlue) {
      fan0Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kFan0Led + kLedGreen) {
      fan0Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kFan1Led + kLedAmber) {
      fan1Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kFan1Led + kLedBlue) {
      fan1Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kFan1Led + kLedGreen) {
      fan1Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kFan2Led + kLedAmber) {
      fan2Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kFan2Led + kLedBlue) {
      fan2Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kFan2Led + kLedGreen) {
      fan2Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kFan3Led + kLedAmber) {
      fan3Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kFan3Led + kLedBlue) {
      fan3Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kFan3Led + kLedGreen) {
      fan3Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kFan4Led + kLedAmber) {
      fan4Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kFan4Led + kLedBlue) {
      fan4Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kFan4Led + kLedGreen) {
      fan4Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim1Led + kLedAmber) {
      pim1Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPim1Led + kLedBlue) {
      pim1Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPim1Led + kLedGreen) {
      pim1Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim1Led + kLedRed) {
      pim1Led_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kPim2Led + kLedAmber) {
      pim2Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPim2Led + kLedBlue) {
      pim2Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPim2Led + kLedGreen) {
      pim2Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim2Led + kLedRed) {
      pim2Led_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kPim3Led + kLedAmber) {
      pim3Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPim3Led + kLedBlue) {
      pim3Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPim3Led + kLedGreen) {
      pim3Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim3Led + kLedRed) {
      pim3Led_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kPim4Led + kLedAmber) {
      pim4Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPim4Led + kLedBlue) {
      pim4Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPim4Led + kLedGreen) {
      pim4Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim4Led + kLedRed) {
      pim4Led_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kPim5Led + kLedAmber) {
      pim5Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPim5Led + kLedBlue) {
      pim5Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPim5Led + kLedGreen) {
      pim5Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim5Led + kLedRed) {
      pim5Led_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kPim6Led + kLedAmber) {
      pim6Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPim6Led + kLedBlue) {
      pim6Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPim6Led + kLedGreen) {
      pim6Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim6Led + kLedRed) {
      pim6Led_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kPim7Led + kLedAmber) {
      pim7Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPim7Led + kLedBlue) {
      pim7Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPim7Led + kLedGreen) {
      pim7Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim7Led + kLedRed) {
      pim7Led_->setColorPath(SandiaLedColor::RED, *attr.path());
    } else if (*attr.name() == kPim8Led + kLedAmber) {
      pim8Led_->setColorPath(SandiaLedColor::AMBER, *attr.path());
    } else if (*attr.name() == kPim8Led + kLedBlue) {
      pim8Led_->setColorPath(SandiaLedColor::BLUE, *attr.path());
    } else if (*attr.name() == kPim8Led + kLedGreen) {
      pim8Led_->setColorPath(SandiaLedColor::GREEN, *attr.path());
    } else if (*attr.name() == kPim8Led + kLedRed) {
      pim8Led_->setColorPath(SandiaLedColor::RED, *attr.path());
    }
  }
}

void SandiaChassisManager::programChassis() {
  SandiaLedColor sysLedColor  = SandiaLedColor::BLUE;
  SandiaLedColor fanLedColor  = SandiaLedColor::BLUE;
  SandiaLedColor psuLedColor  = SandiaLedColor::BLUE;
  SandiaLedColor smbLedColor  = SandiaLedColor::BLUE;
  
  for (auto& fru : fruModules_) {
    if (!fru.second->isPresent()) {
      XLOG(DBG2) << "Fru module " << fru.first << " is absent";
      sysLedColor = SandiaLedColor::AMBER;
      if (std::strncmp(
              fru.first.c_str(), kSandiaFan.c_str(), kSandiaFan.length()) ==
          0) {
        fanLedColor = SandiaLedColor::AMBER;
      } else if (
          std::strncmp(
              fru.first.c_str(), kSandiaPsu.c_str(), kSandiaPsu.length()) ==
          0) {
        psuLedColor = SandiaLedColor::AMBER;
      } 
    }
  }
  if (sysLedColor == SandiaLedColor::BLUE) {
    XLOG(DBG4) << "All fru modules are present";
  }

  sysLed_->setColor(sysLedColor);
  fanLed_->setColor(fanLedColor);
  psuLed_->setColor(psuLedColor);
  smbLed_->setColor(smbLedColor);
}

} // namespace facebook::fboss::platform::data_corral_service
