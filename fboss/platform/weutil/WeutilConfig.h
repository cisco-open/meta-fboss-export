/**
 * @file WeutilConfig.h
 *
 * @brief Header file for we_util configuration API for Cisco-8000.
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */

#pragma once

#include <string>

namespace facebook::fboss::platform {

    std::string getIdpromsData();
    std::string getLassenIdpromsData();
    std::string getSandiaIdpromsData();

} // namespace facebook::fboss::platform
