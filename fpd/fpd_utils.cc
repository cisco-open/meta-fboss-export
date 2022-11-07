/*!
 * fpd_utils.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <iostream>
#include <string.h>

#include "fpd_utils.h"

std::string 
exec_shell_command(const char* cmd) {
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
