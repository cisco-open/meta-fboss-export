/**
 * @file SandiaPlatformConfig.cpp
 *
 * @brief Cisco-8000 implementation of Sandia DataCorral service
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/data_corral_service/darwin/DarwinPlatformConfig.cpp
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */

#include <string>

namespace facebook::fboss::platform::data_corral_service {

std::string getSandiaPlatformConfig() {
  // TODO: return based on the config file from FLAGS_config if necessary
  // For now, just hardcode sandia platform config
  return R"({
  "fruModules": [
    {
      "name": "SandiaFanModule-0",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/pwm.0/fan0_presence"
        }
      ]
    },
    {
      "name": "SandiaFanModule-1",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/pwm.0/fan1_presence"
        }
      ]
    },
    {
      "name": "SandiaFanModule-2",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/pwm.0/fan2_presence"
        }
      ]
    },
    {
      "name": "SandiaFanModule-3",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/pwm.0/fan3_presence"
        }
      ]
    },
    {
      "name": "SandiaFanModule-4",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/pwm.0/fan4_presence"
        }
      ]
    },
    {
      "name": "SandiaFanModule-5",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/pwm.0/fan5_presence"
        }
      ]
    },
//-TODO Update PSU presence path------------------- 
    {
      "name": "SandiaPsuModule-1",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/SCD_FPGA_FPGA/pem_present"
        }
      ]
    },
    {
      "name": "SandiaPsuModule-2",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/SCD_FPGA_FPGA/pem_present"
        }
      ]
    },
    {
      "name": "SandiaPsuModule-3",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/SCD_FPGA_FPGA/pem_present"
        }
      ]
    },
    {
      "name": "SandiaPsuModule-4",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/SCD_FPGA_FPGA/pem_present"
        }
      ]
    },
//-TODO Update PSU presence path------------------- 
    {
      "name": "SandiaPimModule-1",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/slpc-m.1/pim_ready"
        }
      ]
    },
    {
      "name": "SandiaPimModule-2",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/slpc-m.2/pim_ready"
        }
      ]
    },
    {
      "name": "SandiaPimModule-3",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/slpc-m.3/pim_ready"
        }
      ]
    },
    {
      "name": "SandiaPimModule-4",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/slpc-m.4/pim_ready"
        }
      ]
    },
    {
      "name": "SandiaPimModule-5",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/slpc-m.5/pim_ready"
        }
      ]
    },
    {
      "name": "SandiaPimModule-6",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/slpc-m.6/pim_ready"
        }
      ]
    },
    {
      "name": "SandiaPimModule-7",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/slpc-m.7/pim_ready"
        }
      ]
    },
    {
      "name": "SandiaPimModule-8",
      "attributes": [
        {
          "name": "present",
          "path": "/run/devmap/fpgas/IOB_FPGA/slpc-m.8/pim_ready"
        }
      ]
    }
  ],
  "chassisAttributes": [
    {
      "name": "FanLedBlue",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/fan:blue:status/brightness"
    },
    {
      "name": "FanLedGreen",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/fan:green:status/brightness"
    },
    {
      "name": "FanLedRed",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/fan:red:status/brightness"
    },

    {
      "name": "SystemLedAmber",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/sys:amber:status/brightness"
    },
    {
      "name": "SystemLedBlue",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/sys:blue:status/brightness"
    },
    {
      "name": "SystemLedGreen",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/sys:green:status/brightness"
    },
    {
      "name": "SystemLedRed",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/sys:red:status/brightness"
    },

    {
      "name": "PsuLedAmber",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/psu:amber:status/brightness"
    },
    {
      "name": "PsuLedBlue",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/psu:blue:status/brightness"
    },
    {
      "name": "PsuLedGreen",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/psu:green:status/brightness"
    },
    {
      "name": "PsuLedRed",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/psu:red:status/brightness"
    },

    {
      "name": "ScmLedAmber",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/scm:amber:status/brightness"
    },
    {
      "name": "ScmLedBlue",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/scm:blue:status/brightness"
    },
    {
      "name": "ScmLedGreen",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/scm:green:status/brightness"
    },
    {
      "name": "ScmLedRed",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/scm:red:status/brightness"
    },
    {
      "name": "SmbLedAmber",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/smb:amber:status/brightness"
    },
    {
      "name": "SmbLedBlue",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/smb:blue:status/brightness"
    },
    {
      "name": "SmbLedGreen",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/smb:green:status/brightness"
    },
    {
      "name": "SmbLedRed",
      "path": "/run/devmap/fpgas/SCM_FPGA/bmc-led.0/leds/smb:red:status/brightness"
    },
    {
      "name": "Fan0LedAmber",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray0:amber:status/brightness"
    },
    {
      "name": "Fan0LedBlue",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray0:blue:status/brightness"
    },
    {
      "name": "Fan0LedGreen",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray0:green:status/brightness"
    },
    {
      "name": "Fan1LedAmber",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray1:amber:status/brightness"
    },
    {
      "name": "Fan1LedBlue",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray1:blue:status/brightness"
    },
    {
      "name": "Fan1LedGreen",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray1:green:status/brightness"
    },
    {
      "name": "Fan2LedAmber",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray2:amber:status/brightness"
    },
    {
      "name": "Fan2LedBlue",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray2:blue:status/brightness"
    },
    {
      "name": "Fan2LedGreen",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray2:green:status/brightness"
    },
    {
      "name": "Fan3LedAmber",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray3:amber:status/brightness"
    },
    {
      "name": "Fan3LedBlue",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray3:blue:status/brightness"
    },
    {
      "name": "Fan3LedGreen",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray3:green:status/brightness"
    },
    {
      "name": "Fan4LedAmber",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray4:amber:status/brightness"
    },
    {
      "name": "Fan4LedBlue",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray4:blue:status/brightness"
    },
    {
      "name": "Fan4LedGreen",
      "path": "/run/devmap/fpgas/IOB_FPGA/bmc-led.1/leds/fantray4:green:status/brightness"
    },
    {
      "name": "PIM1LedAmber",
      "path": "/run/devmap/fpgas/PIM1_FPGA/bmc-led-pim1.11/leds/pim1:amber:status/brightness"
    },
    {
      "name": "PIM1LedBlue",
      "path": "/run/devmap/fpgas/PIM1_FPGA/bmc-led-pim1.11/leds/pim1:blue:status/brightness"
    },
    {
      "name": "PIM1LedGreen",
      "path": "/run/devmap/fpgas/PIM1_FPGA/bmc-led-pim1.11/leds/pim1:green:status/brightness"
    },
    {
      "name": "PIM1LedRed",
      "path": "/run/devmap/fpgas/PIM1_FPGA/bmc-led-pim1.11/leds/pim1:red:status/brightness"
    },
    {
      "name": "PIM2LedAmber",
      "path": "/run/devmap/fpgas/PIM2_FPGA/bmc-led-pim2.12/leds/pim2:amber:status/brightness"
    },
    {
      "name": "PIM2LedBlue",
      "path": "/run/devmap/fpgas/PIM2_FPGA/bmc-led-pim2.12/leds/pim2:blue:status/brightness"
    },
    {
      "name": "PIM2LedGreen",
      "path": "/run/devmap/fpgas/PIM2_FPGA/bmc-led-pim2.12/leds/pim2:green:status/brightness"
    },
    {
      "name": "PIM2LedRed",
      "path": "/run/devmap/fpgas/PIM2_FPGA/bmc-led-pim2.12/leds/pim2:red:status/brightness"
    },
    {
      "name": "PIM3LedAmber",
      "path": "/run/devmap/fpgas/PIM3_FPGA/bmc-led-pim3.13/leds/pim2:amber:status/brightness"
    },
    {
      "name": "PIM3LedBlue",
      "path": "/run/devmap/fpgas/PIM3_FPGA/bmc-led-pim3.13/leds/pim3:blue:status/brightness"
    },
    {
      "name": "PIM3LedGreen",
      "path": "/run/devmap/fpgas/PIM3_FPGA/bmc-led-pim3.13/leds/pim3:green:status/brightness"
    },
    {
      "name": "PIM3LedRed",
      "path": "/run/devmap/fpgas/PIM3_FPGA/bmc-led-pim3.13/leds/pim3:red:status/brightness"
    },
    {
      "name": "PIM4LedAmber",
      "path": "/run/devmap/fpgas/PIM4_FPGA/bmc-led-pim4.14/leds/pim4:amber:status/brightness"
    },
    {
      "name": "PIM4LedBlue",
      "path": "/run/devmap/fpgas/PIM4_FPGA/bmc-led-pim4.14/leds/pim4:blue:status/brightness"
    },
    {
      "name": "PIM4LedGreen",
      "path": "/run/devmap/fpgas/PIM4_FPGA/bmc-led-pim4.14/leds/pim4:green:status/brightness"
    },
    {
      "name": "PIM4LedRed",
      "path": "/run/devmap/fpgas/PIM4_FPGA/bmc-led-pim4.14/leds/pim4:red:status/brightness"
    },
    {
      "name": "PIM5LedAmber",
      "path": "/run/devmap/fpgas/PIM5_FPGA/bmc-led-pim5.15/leds/pim5:amber:status/brightness"
    },
    {
      "name": "PIM5LedBlue",
      "path": "/run/devmap/fpgas/PIM5_FPGA/bmc-led-pim5.15/leds/pim5:blue:status/brightness"
    },
    {
      "name": "PIM5LedGreen",
      "path": "/run/devmap/fpgas/PIM5_FPGA/bmc-led-pim5.15/leds/pim5:green:status/brightness"
    },
    {
      "name": "PIM5LedRed",
      "path": "/run/devmap/fpgas/PIM5_FPGA/bmc-led-pim5.15/leds/pim5:red:status/brightness"
    },
    {
      "name": "PIM6LedAmber",
      "path": "/run/devmap/fpgas/PIM6_FPGA/bmc-led-pim6.16/leds/pim6:amber:status/brightness"
    },
    {
      "name": "PIM6LedBlue",
      "path": "/run/devmap/fpgas/PIM6_FPGA/bmc-led-pim6.16/leds/pim6:blue:status/brightness"
    },
    {
      "name": "PIM6LedGreen",
      "path": "/run/devmap/fpgas/PIM6_FPGA/bmc-led-pim6.16/leds/pim6:green:status/brightness"
    },
    {
      "name": "PIM6LedRed",
      "path": "/run/devmap/fpgas/PIM6_FPGA/bmc-led-pim6.16/leds/pim6:red:status/brightness"
    },
    {
      "name": "PIM7LedAmber",
      "path": "/run/devmap/fpgas/PIM7_FPGA/bmc-led-pim7.17/leds/pim7:amber:status/brightness"
    },
    {
      "name": "PIM7LedBlue",
      "path": "/run/devmap/fpgas/PIM7_FPGA/bmc-led-pim7.17/leds/pim7:blue:status/brightness"
    },
    {
      "name": "PIM7LedGreen",
      "path": "/run/devmap/fpgas/PIM7_FPGA/bmc-led-pim7.17/leds/pim7:green:status/brightness"
    },
    {
      "name": "PIM7LedRed",
      "path": "/run/devmap/fpgas/PIM7_FPGA/bmc-led-pim7.17/leds/pim7:red:status/brightness"
    },
    {
      "name": "PIM8LedAmber",
      "path": "/run/devmap/fpgas/PIM8_FPGA/bmc-8ed-pi88.18/leds/pim8:amber:status/brightness"
    },
    {
      "name": "PIM8LedBlue",
      "path": "/run/devmap/fpgas/PIM8_FPGA/bmc-led-pim8.18/leds/pim8:blue:status/brightness"
    },
    {
      "name": "PIM8LedGreen",
      "path": "/run/devmap/fpgas/PIM8_FPGA/bmc-led-pim8.18/leds/pim8:green:status/brightness"
    },
    {
      "name": "PIM8LedRed",
      "path": "/run/devmap/fpgas/PIM8_FPGA/bmc-led-pim8.18/leds/pim8:red:status/brightness"
    }
  ]
})";
}

} // namespace facebook::fboss::platform::data_corral_service
