/**
 * @file FirmwareUpgrade.cc
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

#include <fstream>
#include <filesystem>
#include <errno.h>
#include <sysexits.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

#include "fw_util.h"
#include "FirmwareUpgrade.h"
#include "bsp/find.h"

void 
FirmwareUpgradeCisco8000::print_usage(std::string &upgradable_components)
{
    std::cout << "usage:" << std::endl;
    std::cout << "fw_util <all|binary_name> <action> <binary_file>" << std::endl;
    std::cout << "<binary_name> : " << upgradable_components << std::endl;
    std::cout << "<action> : program, verify, read, version" << std::endl;
    std::cout
        << "<binary_file> : path to binary file which is NOT supported when pulling fw version"
        << std::endl;
    std::cout
        << "all: only supported when pulling fw version. Ex:fw_util all version"
        << std::endl;
}

void
FirmwareUpgradeCisco8000::upgradable_components(std::string &fpds) const
{
    for (const auto &it : bsp2::fpd_t::factory("")) {
        fpds = fpds + it->name() + ",";
    }
    if (!fpds.empty()) {
        fpds.pop_back();
    }
}

void
FirmwareUpgradeCisco8000::print_version(std::string fw_name) const
{
    std::vector<std::shared_ptr<bsp2::fpd_t>> objs;
    if (fw_name == "all") {
        objs = bsp2::fpd_t::factory("");
    } else {
        objs = bsp2::fpd_t::factory(fw_name);
    }
    if (!objs.size()) {
        std::cout << fw_name << ": not present" << std::endl;
        return;
    }
    for (auto fpd : objs) {
        std::string version("not present");
        try {
            if (fpd->is_present()) {
                version = fpd->running_version();
            }
        } catch (const std::exception &ex) {
            version = std::string("[ERROR: ") + ex.what() + "]";
        }
        std::cout << fpd->name() << ": " << version << std::endl;
    }
}

void
FirmwareUpgradeCisco8000::program_fw(std::string name, std::string path) const
{
    std::vector<std::shared_ptr<bsp2::fpd_t>> objs = bsp2::fpd_t::factory(name);
    if (!objs.size()) {
        std::cout << name << ": not present" << std::endl;
        return;
    }
    for (auto fpd : objs) {
        std::string program_msg("not present");
        if (!fpd->set_file_path(path)) {
            std::cerr << fpd->name() << ": invalid path " << path << std::endl;
        }
        try {
            if (fpd->is_present()) {
                fpd->program(true);
                program_msg = "Program successful. Reboot or activate is required to complete the firmware upgrade";
            }
        } catch (const std::exception &ex) {
            program_msg = std::string("[ERROR: ") + ex.what() + "]";
        }
        std::cout << fpd->name() << ": " << program_msg << std::endl;
    }
}

void
FirmwareUpgradeCisco8000::upgradeFirmware(int argc, char **argv,
                                          std::string upgradable_components)
{
    try {
        if (argc == 1) {
            print_usage(upgradable_components);
        } else if (argc >= 3 && std::string(argv[2]) == std::string("version")) {
            if (argc > 3) {
                std::cout << std::string(argv[3]) << " cannot be part of version command"
                          << std::endl
                          << "To pull firmware version, please enter the following command"
                          << std::endl
                          << "fw_util <all|binary_name> <action>"
                          << std::endl;
            }
            print_version(std::string(argv[1]));
        } else if (argc != 4) {
            std::cout << "missing argument" << std::endl
                      << "please follow the usage below" << std::endl;
            print_usage(upgradable_components);
        } else if (std::string(argv[2]) == std::string("program")) {
            program_fw(std::string(argv[1]), std::string(argv[3]));
        } else {
            std::cout << "wrong usage. Please follow the usage" << std::endl;
            print_usage(upgradable_components);
        }
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
    }
}

std::unique_ptr<facebook::fboss::platform::fw_util::FirmwareUpgradeInterface>
facebook::fboss::platform::fw_util::get_plat_type(std::string &fpds)
{
    auto pp = std::make_unique<FirmwareUpgradeCisco8000>();
    pp->upgradable_components(fpds);
    return pp;
}
