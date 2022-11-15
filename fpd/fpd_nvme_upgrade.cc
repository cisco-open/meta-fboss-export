/*
 * fpd_nvme.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <iostream>

#include "commonUtil.h"
#include "fpd/nvme.h"
#include "fpd_utils.h"

#define SMART_VENDOR_ID "0x1235"
#define MICRON_VENDOR_ID "0x1344"
#define NVME_TMP_FILE "/tmp/nvme-img.tmp"

void 
nvme_upgrade(std::string image_path, std::string pid)
{
    char err_msg[128];
    uint32_t msg_size = 128;
    char download_cmd[256] = {0};
    char commit_cmd[256] = {0};
    char commit_result[256] = {0};
    char *SLOT = "2";
    char *image_vendor = NULL;
    char *image_type = "NVME";
    fpd_imgs_t *fpd_imgs;
    int count;
    void *data;
    FILE *fp;
    int rc;

    std::string info(__func__);

    const char* smartctl_cmd = "/usr/sbin/smartctl -i /dev/nvme0n1 | grep \"PCI Vendor\" | awk \'{print$4}\'";
    std::string vendor_id = exec_shell_command(smartctl_cmd);

    if (vendor_id == SMART_VENDOR_ID) {
        image_vendor = "SMART";
        SLOT = "1";
    } else if(vendor_id == MICRON_VENDOR_ID) {
        SLOT = "2";
        image_vendor = "VEN:MICRON";
        const char* model_cmd = "nvme list | grep /dev/nvme | awk \'{print $3}\'";
        std::string model = exec_shell_command(model_cmd);
        if (model.find("Micron_7400") != std::string::npos) {
             image_vendor = "MICRON_7400";
        } else if (model.find("Micron_7450") != std::string::npos) {
             image_vendor = "MICRON_7450";
        } else {
            info.append("\nFW Upgrade not supported for this model: (").append(model).append(")");
            throw std::runtime_error(info);
        }
    } else {
        info.append("\nFW Upgrade not supported for vendor_id: (").append(vendor_id).append(")");
        throw std::runtime_error(info);
    }

    rc = get_imgs_info(image_path.c_str(), &fpd_imgs, err_msg, msg_size);
    if (rc) {
        printf("rc: %d\n", rc);
        info.append("\nFailed to parse file: (").append(image_path).append(")");
        throw std::runtime_error(info);
        return;
    }

    count = fpd_find_img(fpd_imgs, pid.c_str(), image_type, image_vendor);
    if (count != 1) {
        fpd_print_imgs_info(fpd_imgs);
        info.append("\nFailed to  get image file no match found: (").append(image_path).append(")");
        throw std::runtime_error(info);
    }

    rc = img_inflate(fpd_imgs->match_list[0], &data, err_msg, msg_size);
    if (rc) {
        info.append("\nFailed to  inflate image rc %d", rc);
        return;
    }
    fp = fopen(NVME_TMP_FILE, "wb");
    fwrite(data, fpd_imgs->match_list[0]->img_msize, 1, fp);
    fclose(fp);

    std::cout << "Downloading file into drive\n";
    snprintf(download_cmd, sizeof(download_cmd)-1,
             "sudo nvme fw-download /dev/nvme0n1 --fw=%s",
             NVME_TMP_FILE);

    std::string download_file = exec_shell_command(download_cmd);
    if (download_file != "Firmware download success") {
        info.append("\nFailed to download file: (").append(download_file).append(")");
        throw std::runtime_error(info);
    }
    std::cout << "Image downloaded into the drive\n";

    std::cout << "Verify and commit firmware..";
    snprintf(commit_cmd, sizeof(commit_cmd)-1,
             "sudo nvme fw-commit /dev/nvme0n1 --slot=%s --action=1",
             SLOT);

    std::string commit_verify = exec_shell_command(commit_cmd);
    snprintf(commit_result, sizeof(commit_result)-1,
             "Success committing firmware action:1 slot:%s",
             SLOT);
    if (commit_verify != commit_result) {
        info.append("\nFailed to commit verify image: (").append(commit_verify).append(")");
        throw std::runtime_error(info);
    }
    remove(NVME_TMP_FILE);
    std::cout << "Firmware upgrade done\n";
}
