/*!
 * fpd_iofpga.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <sys/stat.h>

#include "commonUtil.h"
#include "fpd_flash.h"
#include "fpd/flash.h"

std::string
Fpd_flash::get_running_version() const
{
    if (fpd_t::is_golden_fpd()) {
        uint32_t mdata_offset = std::stoul(fpd_t::get_fpga_offset("mdata_offset"), nullptr, 0);
        std::string block_name = fpd_t::get_fpga_offset("uio_block_name");

        uint16_t golden_version = get_iofpga_version_from_flash(block_name.c_str(), mdata_offset);
        auto major = golden_version >> 8;
        auto minor = golden_version & 0xFF;
        return (std::to_string(major) + "." + std::to_string(minor));
    }

    std::string version = fpd_t::get_version();
    auto pos = version.find(".", 1 + version.find("."));
    if (pos == version.npos) {
        return version;
    }
    return version.substr(0, pos);
}

void 
Fpd_flash::program(bool force) const {
    int ret;

    if (!force) {
        std::string run_version = get_running_version();
        std::string pack_version = packaged_version();

        if (fpd_t::compare_version(run_version, pack_version)) {
            std::string info(__func__);
            info.append(": Running version(").append(run_version)
                .append(") is greater than or same as packaged version(")
                .append(pack_version).append("). Only upgrades are allowed!");
            throw std::system_error(EPERM, std::generic_category(), info);
        }
    }
    auto image_path = fpd_t::path();
    int len = image_path.size();
    char path[len+1];
    image_path.copy(path, len, 0);
    path[len] = '\0';

    uint32_t image_offset = std::stoul(fpd_t::get_fpga_offset("image_offset"), nullptr, 0);
    uint32_t image_size = std::stoul(fpd_t::get_fpga_offset("image_size"), nullptr, 0);
    uint32_t mdata_offset = std::stoul(fpd_t::get_fpga_offset("mdata_offset"), nullptr, 0);
    uint32_t mdata_size = std::stoul(fpd_t::get_fpga_offset("mdata_size"), nullptr, 0);
    std::string block_name = fpd_t::get_fpga_offset("uio_block_name");

    ret = program_iofpga(path, image_offset, image_size,
                         mdata_offset, mdata_size, block_name.c_str());
    if (ret) {
        std::string info("Failed to program iofpga");
        throw std::system_error(ret, std::generic_category(), info);
    }
}

void 
Fpd_flash::erase() const {
    int ret;

    uint32_t image_offset = std::stoul(fpd_t::get_fpga_offset("image_offset"), nullptr, 0);
    uint32_t image_size = std::stoul(fpd_t::get_fpga_offset("image_size"), nullptr, 0);
    uint32_t mdata_offset = std::stoul(fpd_t::get_fpga_offset("mdata_offset"), nullptr, 0);
    uint32_t mdata_size = std::stoul(fpd_t::get_fpga_offset("mdata_size"), nullptr, 0);
    std::string block_name = fpd_t::get_fpga_offset("uio_block_name");

    ret = erase_iofpga(image_offset, image_size,
                       mdata_offset, mdata_size, block_name.c_str());
    if (ret) {
        std::string info("Failed to erase iofpga");
        throw std::system_error(ret, std::generic_category(), info);
    }
}

std::string 
Fpd_flash::running_version(void) const
{
    if (fpd_t::is_golden_fpd()) {
        uint32_t mdata_offset = std::stoul(fpd_t::get_fpga_offset("mdata_offset"), nullptr, 0);
        std::string block_name = fpd_t::get_fpga_offset("uio_block_name");

        uint16_t golden_version = get_iofpga_version_from_flash(block_name.c_str(), mdata_offset);
        auto major = golden_version >> 8;
        auto minor = golden_version & 0xFF;
        return (std::to_string(major) + "." + std::to_string(minor));
    }

    std::string version = get_running_version();
    std::string block_name = fpd_t::get_fpga_offset("uio_block_name");
    int val = is_golden_booted(block_name.c_str());
    if (val == -1) {
        throw std::system_error(val, std::generic_category(), "Failed to get image boot status");
    }
    if (val >= 10) {
        return version + " (Primary)";
    } else {
        return version + " (Golden)";
    }
}

std::string 
Fpd_flash::packaged_version() const 
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
