/*!
 * bios_upgrade.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <iostream>

#include "commonUtil.h"
#include "biosUtil.h"
#include "fpd_utils.h"
#include "fpd/bios.h"

void
create_images(std::string image_path){

    char upgrade_mdata_cmd[256] = {0};
    char generate_capsule_cmd[256] = {0};

    std::string info(__func__);

    // Generate upgrade metadata image
    const char* MDATA_FILE = "/boot/efi/EFI/UpdateCapsule/BIOS_PRI_MDATA.bin";

    snprintf(upgrade_mdata_cmd, sizeof(upgrade_mdata_cmd)-1,
             "sudo dd if=%s bs=128 count=1 of=%s",
             image_path.c_str(),MDATA_FILE);

    std::string upgrade_metadata = exec_shell_command(upgrade_mdata_cmd);

    std::cout << "Meta data image created\n";

    // Generate capsule image
    const char* IMAGE_FILE = "/boot/efi/EFI/UpdateCapsule/CAPSULE_UPGRADE.bin";
    snprintf(generate_capsule_cmd, sizeof(generate_capsule_cmd)-1,
             "sudo dd if=%s bs=128 skip=1 of=%s",
             image_path.c_str(),IMAGE_FILE);
    std::string generate_capsule_image = exec_shell_command(generate_capsule_cmd);

    std::cout << "Capsule image created\n";
}

void 
bios_upgrade(std::string image_path, int golden)
{
    std::string info(__func__);

    // Create efi directory
    const char* efi_dir = "mkdir -p /boot/efi";
    std::string mkdir_p_efi = exec_shell_command(efi_dir);
    if (mkdir_p_efi != "\0") {
        info.append("\nFailed to create /boot/efi directory");
        throw std::runtime_error(info);
    }

    // Mount efi partition
    std::string HDTYPE = "/dev/sda1";
    if (std::filesystem::exists(HDTYPE)) {
        const char* efi = "sudo mount /dev/sda1 /boot/efi";
        std::string mnt_efi = exec_shell_command(efi);
        if (mnt_efi != "\0") {
            info.append("\nFailed to mount /dev/sda1 at /boot/efi");
            throw std::runtime_error(info);
        } 
    } else {
          std::string HDTYPE = "/dev/nvme0n1";
          if (std::filesystem::exists(HDTYPE)) {
              const char* efi = "sudo mount /dev/nvme0n1p1 /boot/efi";
              std::string mnt_efi = exec_shell_command(efi);
          } else {
                info.append("\nFailed to get HD type");
                throw std::runtime_error(info);
          }
    }

    // Create update capsule directory
    const char* capsule_dir = "mkdir -p /boot/efi/EFI/UpdateCapsule";
    std::string mkdir_p_capsule = exec_shell_command(capsule_dir);

    //Check if meta-data file exists
    const char* MDATA_FILE = "/boot/efi/EFI/UpdateCapsule/BIOS_PRI_MDATA.bin";
    if (std::filesystem::exists(MDATA_FILE)) {
        info.append("\nBIOS could upgrade only one flash on reload.");
        info.append("\nPlease activate already upgraded");
        throw std::runtime_error(info);
    }

    const char* remove_dir = "rm -f /boot/efi/EFI/UpdateCapsule/*";
    std::string remove_efi = exec_shell_command(remove_dir);

    //Check if efivars path exists
    const char* EFI_BIOS_UPG_VAR_PATH = "/sys/firmware/efi/efivars";
    if (std::filesystem::exists(EFI_BIOS_UPG_VAR_PATH)) {
        std::cout << "EFI VAR PATH exists\n";
        create_images(image_path);
    } else {
          info.append("\nEFI VAR PATH Does not exist, existing BIOS upgrade");
          throw std::runtime_error(info);
    }

    // Set OS Indication to upgrade BIOS on next reboot
    if (golden) {
        const char* upgrade_cmd_1 = "sudo printf \"\\x07\\x00\\x00\\x00\\x04\\x00\\x00\\x00\\x00\\x00\\x00\\x00\" > /sys/firmware/efi/efivars/CiscoFlashSelected-59d1c24f-50f1-401a-b101-f33e0daed443";
        std::string upgrade_bios = exec_shell_command(upgrade_cmd_1);

        const char* upgrade_cmd_2 = "sudo printf \"\\x07\\x00\\x00\\x00\\x04\\x00\\x00\\x00\\x00\\x00\\x00\\x00\" > /sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c";
        upgrade_bios = exec_shell_command(upgrade_cmd_2);
        std::cout << "OS Indication set SUCCESS: BIOS Golden will be upgraded on next reboot\n";
    } else {
          const char* upgrade_cmd = "sudo printf \"\\x07\\x00\\x00\\x00\\x04\\x00\\x00\\x00\\x00\\x00\\x00\\x00\" > /sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c";
          std::string upgrade_bios = exec_shell_command(upgrade_cmd);
          std::cout << "OS Indication set SUCCESS: BIOS will be upgraded on next reboot\n";
    }
}
