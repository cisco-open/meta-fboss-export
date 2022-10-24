/**
 * @file FirmwareUpgrade.h
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

#ifndef FW_UPGRADE_CISCO8000_H_
#define FW_UPGRADE_CISCO8000_H_

#include "bsp/fpd.h"


class FirmwareUpgradeCisco8000 : public facebook::fboss::platform::fw_util::FirmwareUpgradeInterface
{
public:
    FirmwareUpgradeCisco8000()
        : FirmwareUpgradeInterface()
    {
    }

    virtual ~FirmwareUpgradeCisco8000() {}

    void upgradeFirmware(int, char**, std::string) override;

    void upgradable_components(std::string &upgradable_components) const;

    void print_version(std::string) const;

    void program_fw(std::string, std::string) const;

private:
    void print_usage(std::string &upgradable_components);
};

#endif // FW_UPGRADE_CISCO8000_H_
