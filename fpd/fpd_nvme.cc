/*
 * fpd_nvme.cc
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
#include "fpd/nvme.h"
#include "fpd_utils.h"

#define SMART_VENDOR_ID "0x1235"
#define MICRON_VENDOR_ID "0x1344"

static std::string
get_nvme_attribute(std::string &input_str)
{
    auto pos = input_str.find(':');
    if (pos == input_str.npos) {
        throw std::runtime_error("Failed to get nvme attribute");
    }
    std::string result = input_str.substr(pos+1);
    result = std::regex_replace(result, std::regex("^ +| +$|( ) +"), "");
    return result;
}

void
Fpd_nvme::program(bool force) const
{
    auto helper = fpd_t::helper();
    std::vector <std::string> image_path = {fpd_t::path()};

    nvme_upgrade(image_path[0]);
}

void
Fpd_nvme::activate() const
{
    auto activate_path = fpd_t::get_activate_path();
    if (activate_path.string().length() > 0) {
        set_activate_path_value("1\n");
    } else {
        std::string info(__func__);
        info.append(": Activate path is null");
        throw std::system_error(ENOENT, std::generic_category(), info);
    }
}

std::string
Fpd_nvme::running_version(void) const
{
    const char *NVME_VER_CMD = "/usr/sbin/smartctl -i /dev/nvme0n1 | grep \"Firmware Version\"";
    std::string version = exec_shell_command(NVME_VER_CMD);
    return get_nvme_attribute(version);
}

std::string
Fpd_nvme::packaged_version() const
{
    // No implementation. TBD
    std::string info(__func__);
    info.append(": ");
    throw std::system_error(ENOTSUP, std::generic_category(), info);
}
