/*
 * fpd_cpucpld.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <wait.h>
#include <unistd.h>
#include <regex>
#include <fcntl.h>
#include <filesystem>

#include "fpd/cpucpld.h"
#include "commonUtil.h"

void
Fpd_cpucpld::program(bool force) const
{
    unsigned int i2c_bus_no, device_address;
    std::filesystem::path fs_path = fpd_t::get_i2c_info();
    std::string device_path = fs_path.filename();

    if (std::filesystem::is_symlink(fs_path)) {
        device_path = std::filesystem::read_symlink(fs_path).filename();
    }
    auto pos = device_path.find("-");
    if (pos == device_path.npos) {
        std::string info(__func__);
        info.append(": Failed to get correct i2c device info");
        throw std::system_error(ENOTSUP, std::generic_category(), info);
    }
    i2c_bus_no = std::stoul(device_path.substr(0, pos));
    device_address = std::stoul(device_path.substr(pos+1), 0, 16);

    auto image_path = fpd_t::path();
    int path_len = image_path.size();
    char path[path_len+1];
    image_path.copy(path, path_len);
    path[path_len] = '\0';

    int ret = ENOTSUP; // cpld_upgrade(path, i2c_bus_no, device_address);
    (void) i2c_bus_no;
    (void) device_address;
    if (ret != 0) {
        std::string info(__func__);
        info.append(": Failed to Program CPU_CPLD");
        throw std::system_error(ret, std::generic_category(), info);
    }
}

std::string
Fpd_cpucpld::running_version(void) const
{
    // If sysfs path is available get version from there.
    std::string version = fpd_t::get_version();
    if (!version.empty()) {
        auto version_str = fpd_t::get_version();
        unsigned int version = std::stoul(version_str, nullptr, 0);
        version = version & 0xFF;
        version = version >> 1;
        return std::to_string(version);
    }

    // Get version from the p2pm light block
    uint64_t block_offset = std::stoul(fpd_t::get_fpga_offset("version_block_addr"), nullptr, 0);
    uint64_t version_mask = std::stoul(fpd_t::get_fpga_offset("version_mask"), nullptr, 0);
    uint32_t reg_offset = std::stoul(fpd_t::get_fpga_offset("version_reg_offset"), nullptr, 0);
    uint16_t reg_shift = std::stoul(fpd_t::get_fpga_offset("version_reg_shift"), nullptr, 0);
    auto cpld_version = get_cpld_version(block_offset, version_mask, reg_offset, reg_shift);

    return std::string("0.") + std::to_string(cpld_version);
}

std::string
Fpd_cpucpld::packaged_version() const
{
    // No implementation. TBD
    std::string info(__func__);
    info.append(": ");
    throw std::system_error(ENOTSUP, std::generic_category(), info);
}
