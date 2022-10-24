/**
 * @file weutil.cc
 *
 * @brief Cisco-8000 implementation of weutil utility
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/weutil/WeutilInterface.h
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#include <errno.h>
#include <sysexits.h>

#include <iostream>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "Weutil.h"

DEFINE_bool(json, false, "output in JSON format");
DECLARE_string(idproms);

using namespace facebook::fboss::platform;

int
main(int argc, char **argv)
{
    gflags::SetCommandLineOptionWithMode(
        "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // Cisco specific enhancements for OpenBMC build
    if (argc > 1) {
        std::string idproms;
        while (++argv, --argc) {
            idproms.append(",").append(*argv);
        }
        idproms.erase(0, 1);
        if (!FLAGS_idproms.empty()) {
            idproms.append(",").append(FLAGS_idproms);
        }
        FLAGS_idproms = idproms;
    }
    auto weutilInstance = get_plat_weutil();
    if (weutilInstance) {
        if (FLAGS_json) {
            weutilInstance->printInfoJson();
        } else {
            weutilInstance->printInfo();
        }
        return EX_OK;
    } else {
        std::cerr << "No platform weutil available" << std::endl;
        return EX_CONFIG;
    }
}
