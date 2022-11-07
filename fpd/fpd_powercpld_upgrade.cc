/*
 * fpd_powercpld_upgrade.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <iostream>
#include "fpd_utils.h"


void
program_powercpld_image (uint16_t mdata_size, std::string image_path)
{
    char upgrade_image_cmd[256] = {0};
    char part_str_cmd[256] = {0};
    char flashcp_cmd[256] = {0};
    const char *IMAGE_FILE="/opt/cisco/fpd/power_cpld/power_cpld.bit";

    // Create .bit upgrade image from mdata image
    snprintf(upgrade_image_cmd, sizeof(upgrade_image_cmd)-1,
             "sudo dd if=%s bs=%d skip=1 of=%s",
             image_path.c_str(), mdata_size, IMAGE_FILE);
    std::string upgrade_image = exec_shell_command(upgrade_image_cmd);

    std::cout << "Upgrade image created" << std::endl;

    // Find mtd partition of power-cpld
    snprintf(part_str_cmd, sizeof(part_str_cmd)-1,
            "cat /proc/mtd | grep \"power-cpld\" | awk '{print $1;}'");
    std::string part_str = exec_shell_command(part_str_cmd);

    // Program microinit image
    snprintf(flashcp_cmd, sizeof(flashcp_cmd)-1,
            "sudo flashcp -v %s /dev/%s",
            IMAGE_FILE, part_str.c_str());
    std::string flashcp = exec_shell_command(flashcp_cmd);
}
