/*
 * @file WeutilSandia.h
 *
 * @brief Cisco-8000 implementation of weutil utility
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */

#pragma once

#include <memory>
#include "fboss/platform/weutil/WeutilInterface.h"
#include "bsp/idprom.h"

namespace facebook::fboss::platform {

std::unique_ptr<WeutilInterface> get_sandia_weutil();

} //namespace facebook::fboss::platform
