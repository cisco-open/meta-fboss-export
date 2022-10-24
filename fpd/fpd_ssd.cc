/*!
 * fpd_ssd.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <iostream>
#include <fstream>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include "commonUtil.h"
#include "fpd_ssd.h"
#include "fpd/ssd.h"

void
Fpd_ssd::program(bool force) const
{
    int ret = 0;
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
    ret = ssd_fpd_upgrade();
    if (ret == -1) {
        std::string info("program:");
        info.append("Failed to Program SSD");
        throw std::system_error(ret, std::generic_category(), info);
    }
}

std::string 
Fpd_ssd::running_version(void) const 
{
    uint32_t version;
    uint16_t major;
    uint16_t minor;

    version = get_ssd_fpd_version();
    major = version >> 16;
    minor = version & 0xFFFF;
    return std::to_string(major) + "." + std::to_string(minor);
}

std::string 
Fpd_ssd::packaged_version() const 
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
