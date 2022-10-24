/*!
 * fpd_bmc_bios.cc
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
#include <sys/stat.h>

#include "commonUtil.h"
#include "fpd/bmc_bios.h"

#define BIOS_VER_SIGNATURE      "$BVDT$"
#define BIOS_VER_INFO_FILE      "/tmp/bios_ver_info.txt"
#define BIOS_MTD_INFO_FILE      "/tmp/mtd_bios.txt"
#define BIOS_VER_LEN            18
#define BIOS_VER_BVDT_LEN       6
#define BIOS_SIGNATURE_OFFSET   256
#define BIOS_VERSION_OFFSET     270

// BIOS version string is in format of x-x-abc-abc
// We need x.x as numeric version string
static std::string 
bios_extract_major_minor_version(std::string version)
{
    std::string result;
    auto pos = version.find('-', 1 + version.find('-'));
    if (pos == version.npos) {
        result = version;
    } else {
        result = version.substr(0, pos);
    }
    std::replace(result.begin(), result.end(), '-', '.');
    return result;
}

static std::string 
get_shell_cmd_output(const char* cmd) {
    char  result[128];
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        throw std::runtime_error("popen() failed!");
    }
    fgets(result, sizeof(result), fp);
    result[strcspn(result, "\n")] = 0;
    pclose(fp);
    return result;
}

std::string
Fpd_bmc_bios::get_bios_version_from_spiflash() const
{
    struct stat         img_statbuf;
    char                sys_cmd[256] = {0};
    char                flash_data[BIOS_VER_LEN] = { 0 };
    char                mtd_dev[16] = { 0 };
    int32_t             c;
    int                 i, rc;
    FILE                *img_fd;

    memset(sys_cmd, 0, sizeof(sys_cmd));
    snprintf(sys_cmd, sizeof(sys_cmd)-1,
             "cat /proc/mtd | grep -i bios_full | cut -d':' -f1 > %s",
             BIOS_MTD_INFO_FILE);
    unlink(BIOS_MTD_INFO_FILE);
    auto str = get_shell_cmd_output(sys_cmd);

    rc = stat(BIOS_MTD_INFO_FILE, &img_statbuf);
    if (rc) {
        std::string err_msg = "Unable to get size of file ";
        err_msg.append(BIOS_MTD_INFO_FILE);
        throw std::runtime_error(err_msg);
    }

    if (img_statbuf.st_size == 0) {
        std::string err_msg = BIOS_MTD_INFO_FILE;
        err_msg.append(" file is empty");
        throw std::runtime_error(err_msg);
    }

    img_fd = fopen(BIOS_MTD_INFO_FILE, "r");
    if (!img_fd) {
        std::string err_msg = "Unable to open file ";
        err_msg.append(BIOS_MTD_INFO_FILE);
        throw std::runtime_error(err_msg);
    }

    rc = fscanf(img_fd, "%s", mtd_dev);
    if (rc == 0) {
        fclose(img_fd);
        throw std::runtime_error("No MTD device found");
    }
    fclose(img_fd);

    auto version_offset = std::stoul(fpd_t::get_fpga_offset("flash_version_offset"), nullptr, 0);
    if (version_offset % 512) {
        std::string err_msg = "Invalid version offset: ";
        err_msg.append(std::to_string(version_offset));
        throw std::runtime_error(err_msg);
    }

    memset(sys_cmd, 0, sizeof(sys_cmd));
    snprintf(sys_cmd, sizeof(sys_cmd)-1,
             "dd if=/dev/%s of=%s count=1 skip=%lu>& /dev/null",
             mtd_dev, BIOS_VER_INFO_FILE, (version_offset/512));
    unlink(BIOS_VER_INFO_FILE);
    str = get_shell_cmd_output(sys_cmd);

    rc = stat(BIOS_VER_INFO_FILE, &img_statbuf);
    if (rc) {
        std::string err_msg = "Unable to get size of file ";
        err_msg.append(BIOS_VER_INFO_FILE);
        throw std::runtime_error(err_msg);
    }

    if (img_statbuf.st_size == 0) {
        std::string err_msg = BIOS_VER_INFO_FILE;
        err_msg.append(" file is empty");
        throw std::runtime_error(err_msg);
    }

    img_fd = fopen(BIOS_VER_INFO_FILE, "rb");
    if (!img_fd) {
        std::string err_msg = "Unable to open file ";
        err_msg.append(BIOS_VER_INFO_FILE);
        throw std::runtime_error(err_msg);
    }

    rc = fseek(img_fd, BIOS_SIGNATURE_OFFSET, SEEK_SET);
    if (rc) {
        fclose(img_fd);
        throw std::runtime_error("Failed to move to BIOS version signature offset.");
    }

    (void)memset(flash_data, 0, sizeof(flash_data));
    for (i = 0; i < BIOS_VER_BVDT_LEN; i++) {
        if ((c = fgetc(img_fd)) == EOF) {
            std::cerr << "Unexpected EOF" << std::endl;
            break;
        }
        flash_data[i] = c;
    }
    flash_data[i] = '\0';

    if (!strcmp(flash_data, BIOS_VER_SIGNATURE)) {
        rc = fseek(img_fd, BIOS_VERSION_OFFSET, SEEK_SET);
        if (rc) {
            fclose(img_fd);
            throw std::runtime_error("Failed to move to BIOS version offset.");
        }
        memset(flash_data, 0, sizeof(flash_data));

        for(i = 0; i < (BIOS_VER_LEN - 1); i++) {
            if ((c = fgetc(img_fd)) == EOF) {
                std::cerr << "Unexpected EOF" << std::endl;
                break;
            }
            flash_data[i] = c;
        }
        flash_data[i] = '\0';
        fclose(img_fd);
        return bios_extract_major_minor_version(std::string(flash_data));
    } else {
        fclose(img_fd);
        std::string err_msg = "Signature mismatch. Found: ";
        err_msg.append(flash_data);
        err_msg.append(" Expected: ");
        err_msg.append(BIOS_VER_SIGNATURE);
        throw std::runtime_error(err_msg);
    }
}

void 
Fpd_bmc_bios::program(bool force) const
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
    auto image_path = fpd_t::path();
    std::vector<std::string> mtd_name {image_path, fpd_t::get_fpga_offset("mtd_name")};

    fpd_t::invoke(helper, mtd_name);
}

std::string 
Fpd_bmc_bios::running_version(void) const
{
    std::vector<std::string> helper;
    std::vector<std::string> arg_list;
    std::string helper_script = fpd_t::get_fpga_offset("helper_script");

    helper.push_back(helper_script);
    fpd_t::invoke(helper, arg_list);
    return get_bios_version_from_spiflash();
}

std::string 
Fpd_bmc_bios::packaged_version() const
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

