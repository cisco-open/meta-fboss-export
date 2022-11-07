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

void 
nvme_upgrade(std::string image_path)
{
    char download_cmd[256] = {0};
    char commit_cmd[256] = {0};
    char commit_result[256] = {0};
    char *SLOT = "2";

    std::string info(__func__);

    const char* smartctl_cmd = "/usr/sbin/smartctl -i /dev/nvme0n1 | grep \"PCI Vendor\" | awk \'{print$4}\'";
    std::string vendor_id = exec_shell_command(smartctl_cmd);

    if (image_path == "\0") {
        if (vendor_id == "SMART_VENDOR_ID") {
            image_path = "/opt/cisco/fpd/nvme/smart_nvme_kodiak_upgrade_dlmc.bin";
            SLOT = "1";
        } else if(vendor_id == "MICRON_VENDOR_ID") {
              SLOT = "2";
              const char* model_cmd = "nvme list | grep /dev/nvme | awk \'{print $3}\'";
              std::string model = exec_shell_command(model_cmd);
              if (model.find("Micron_7400") != std::string::npos) {
                  image_path = "/opt/cisco/fpd/nvme/micron_7400_nvme_kodiak_release.ubi";
              } else if (model.find("Micron_7450") != std::string::npos) {
                    image_path = "/opt/cisco/fpd/nvme/micron_7450_nvme_kodiak_release.ubi";
              } else {
                    info.append("\nFW Upgrade not supported for this model: (").append(model).append(")");
                    throw std::runtime_error(info);
              }
        } else {
              info.append("\nFW Upgrade not supported for vendor_id: (").append(vendor_id).append(")");
              throw std::runtime_error(info);
        }
    } else {
        // By default slot is set to 2 but SMART uses slot 1
        if (vendor_id == "SMART_VENDOR_ID") {
            SLOT = "2";
        }
    }

    std::cout << "Downloading file into drive\n";
    snprintf(download_cmd, sizeof(download_cmd)-1,
             "sudo nvme fw-download /dev/nvme0n1 --fw=%s",
             image_path.c_str());

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
    std::cout << "Firmware upgrade done\n";
}
