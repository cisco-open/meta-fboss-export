/*!
 * fpd_powercpld.cc
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

#include "commonUtil.h"
#include "fpd/powercpld.h"

void 
Fpd_powercpld::program(bool force) const
{
    if (!force) {
        std::string run_version = running_version();
        std::string pack_version = packaged_version();
        if (fpd_t::compare_version(run_version, pack_version)) {
            std::string info(__func__);
            info.append(": Running version(").append(run_version)
                .append(") is greater than or same as packaged version(")
                .append(pack_version).append("). Only upgrades are allowed!");
            throw std::system_error(EPERM, std::generic_category(), info);
        }
    }
    auto helper = fpd_t::helper();
    std::vector <std::string> image_path = {fpd_t::path()};

    auto mdata_size = get_metadata_size(image_path[0].c_str());
    image_path.insert(image_path.begin(), std::to_string(mdata_size));

    fpd_t::invoke(helper, image_path);
}

std::string 
Fpd_powercpld::running_version(void) const
{
    // If sysfs path is available get version from there.
    std::string version = fpd_t::get_version();
    if (!version.empty()) {
        return version;
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
Fpd_powercpld::packaged_version() const
{
    uint32_t version;
    uint16_t major;
    uint16_t minor;
    auto image_path = fpd_t::path();

    version = get_image_version(image_path.c_str());
    major = version >> 16;
    minor = version & 0xFFFF;
    return (std::to_string(major) + "." + std::to_string(minor));
}
