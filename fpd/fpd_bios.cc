/*!
 * fpd_bios.cc
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
#include "fpd/bios.h"
#include "biosUtil.h"

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
Fpd_bios::get_bios_version() const
{
    if (fpd_t::is_golden_fpd()) {
        return get_bios_version_from_spiflash();
    }
    const char* BIOS_VER_CMD = "dmidecode -s bios-version";
    std::string version = get_shell_cmd_output(BIOS_VER_CMD);
    return bios_extract_major_minor_version(version);
}

static void
switch_bios_flash_region(std::string block_name)
{
    int rc = 0;
    rc = fpd_bios_switch_flash(block_name.c_str());
    if (rc) {
        throw std::runtime_error("Failed to switch to golden bios flash region");
    }
}

std::string
Fpd_bios::get_bios_version_from_spiflash() const
{
    struct stat         img_statbuf;
    char                sys_cmd[256] = {0};
    char                flash_data[BIOS_VER_LEN] = { 0 };
    char                mtd_dev[16] = { 0 };
    int32_t             c;
    int                 i, rc;
    FILE                *img_fd;
    bool                switch_bios_flag = false;

    std::string block_name = fpd_t::get_fpga_offset("uio_block_name");
    int active_flash = fpd_bios_get_active_flash(block_name.c_str());
    if (active_flash == 0) {
        switch_bios_flash_region(block_name);
        switch_bios_flag = true;
        active_flash = fpd_bios_get_active_flash(block_name.c_str());
    }

    memset(sys_cmd, 0, sizeof(sys_cmd));
    snprintf(sys_cmd, sizeof(sys_cmd)-1,
             "cat /proc/mtd | grep -i bios | cut -d':' -f1 > %s",
             BIOS_MTD_INFO_FILE);
    unlink(BIOS_MTD_INFO_FILE);
    auto str = get_shell_cmd_output(sys_cmd);

    rc = stat(BIOS_MTD_INFO_FILE, &img_statbuf);
    if (rc) {
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        std::string err_msg = "Unable to get size of file ";
        err_msg.append(BIOS_MTD_INFO_FILE);
        throw std::runtime_error(err_msg);
    }

    if (img_statbuf.st_size == 0) {
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        std::string err_msg = BIOS_MTD_INFO_FILE;
        err_msg.append(" file is empty");
        throw std::runtime_error(err_msg);
    }

    img_fd = fopen(BIOS_MTD_INFO_FILE, "r");
    if (!img_fd) {
        if (switch_bios_flag && (active_flash == 1)){
            switch_bios_flash_region(block_name);
        }
        std::string err_msg = "Unable to open file ";
        err_msg.append(BIOS_MTD_INFO_FILE);
        throw std::runtime_error(err_msg);
    }

    rc = fscanf(img_fd, "%s", mtd_dev);
    if (rc == 0) {
        fclose(img_fd);
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        throw std::runtime_error("No MTD device found");
    }
    fclose(img_fd);

    auto version_offset = std::stoul(fpd_t::get_fpga_offset("flash_version_offset"), nullptr, 0);
    if (version_offset % 512) {
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        std::string err_msg = "Invalid version offset: ";
        err_msg.append(std::to_string(version_offset));
        throw std::runtime_error(err_msg);
    }

    memset(sys_cmd, 0, sizeof(sys_cmd));
    snprintf(sys_cmd, sizeof(sys_cmd)-1,
             "dd if=/dev/%s of=%s count=1 skip=%lu status=none",
             mtd_dev, BIOS_VER_INFO_FILE, (version_offset/512));
    unlink(BIOS_VER_INFO_FILE);
    str = get_shell_cmd_output(sys_cmd);

    rc = stat(BIOS_VER_INFO_FILE, &img_statbuf);
    if (rc) {
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        std::string err_msg = "Unable to get size of file ";
        err_msg.append(BIOS_VER_INFO_FILE);
        throw std::runtime_error(err_msg);
    }

    if (img_statbuf.st_size == 0) {
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        std::string err_msg = BIOS_VER_INFO_FILE;
        err_msg.append(" file is empty");
        throw std::runtime_error(err_msg);
    }

    img_fd = fopen(BIOS_VER_INFO_FILE, "rb");
    if (!img_fd) {
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        std::string err_msg = "Unable to open file ";
        err_msg.append(BIOS_VER_INFO_FILE);
        throw std::runtime_error(err_msg);
    }

    rc = fseek(img_fd, BIOS_SIGNATURE_OFFSET, SEEK_SET);
    if (rc) {
        fclose(img_fd);
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
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
            if (switch_bios_flag && (active_flash == 1)) {
                switch_bios_flash_region(block_name);
            }
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
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        return bios_extract_major_minor_version(std::string(flash_data));
    } else {
        fclose(img_fd);
        if (switch_bios_flag && (active_flash == 1)) {
            switch_bios_flash_region(block_name);
        }
        std::string err_msg = "Signature mismatch. Found: ";
        err_msg.append(flash_data);
        err_msg.append(" Expected: ");
        err_msg.append(BIOS_VER_SIGNATURE);
        throw std::runtime_error(err_msg);
    }
}

void 
Fpd_bios::program(bool force) const
{
    if (!force) {
        std::string run_version = get_bios_version();
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
    int golden = fpd_t::is_golden_fpd();
    if (golden) {
        image_path.push_back("golden");
    }

    bios_upgrade(image_path[0], golden);
}

std::string 
Fpd_bios::running_version(void) const
{
    if (fpd_t::is_golden_fpd()) {
        return get_bios_version_from_spiflash();
    }
    std::string version = get_bios_version();
    const std::string block_name = fpd_t::get_fpga_offset("uio_block_name");
    int golden = bios_is_golden_booted(block_name.c_str());
    if (golden == 1) {
        return version + " (Golden)";
    } else if (golden == 0) {
        return version + " (Primary)";
    } else {
        std::string info(__func__);
        info.append("Failed to get image boot status");
        throw std::system_error(ENOENT, std::generic_category(), info);
    }
}

std::string 
Fpd_bios::packaged_version() const
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
