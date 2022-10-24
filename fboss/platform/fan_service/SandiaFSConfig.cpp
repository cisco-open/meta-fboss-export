/** 
 * @file SandiaFSConfig.cpp
 *
 * @brief Sandia fan service configuration file for FBOSS
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */

#include <string>

namespace facebook::fboss::platform {

std::string
getSandiaFSConfig() {
    return R"json({
    "bsp": "sandia",
    "boost_on_dead_fan": true,
    "boost_on_dead_sensor": false,
    "boost_on_no_qsfp_after": 90,
    "pwm_boost_value": 60,
    "pwm_transition_value": 60,
    "pwm_percent_lower_limit": 23,
    "pwm_percent_upper_limit": 100,
    "shutdown_command": "wedge_power reset -s",
    "sensors": {
    "TEMP_SCM_INLET": {
        "access": {
            "source": "thrift"
        },
        "adjustment": [
            [
                0,
                0
            ]
        ],
        "alarm": {
            "alarm_major": 75,
            "alarm_minor": 70,
            "alarm_minor_soak": 15
        },
        "normal_down_table": [
            [
                [
                    20,
                    23
                ],
                [
                    25,
                    29
                ],
                [
                    30,
                    39
                ],
                [
                    35,
                    63
                ],
                [
                    40,
                    100
                ]
            ]
        ],
        "normal_up_table": [
            [
                [
                    20,
                    23
                ],
                [
                    25,
                    29
                ],
                [
                    30,
                    39
                ],
                [
                    35,
                    63
                ],
                [
                    40,
                    100
                ]
            ]
        ],
        "onefail_down_table": [
            [
                [
                    20,
                    23
                ],
                [
                    25,
                    29
                ],
                [
                    30,
                    39
                ],
                [
                    35,
                    63
                ],
                [
                    40,
                    100
                ]
            ]
        ],
        "onefail_up_table": [
            [
                [
                    20,
                    23
                ],
                [
                    25,
                    29
                ],
                [
                    30,
                    39
                ],
                [
                    35,
                    63
                ],
                [
                    40,
                    100
                ]
            ]
        ],
        "scale": 1000.0,
        "type": "linear_four_curves"
    }
},
    "fans": {
    "FANTRAY1_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN1/device/fan0_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN1/pwm1",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN1/fan1_input",
            "source": "sysfs"
        }
    },
    "FANTRAY1_FAN2": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN2/device/fan0_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN2/pwm2",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN2/fan2_input",
            "source": "sysfs"
        }
    },
    "FANTRAY2_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN1/device/fan1_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN1/pwm3",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN1/fan3_input",
            "source": "sysfs"
        }
    },
    "FANTRAY2_FAN2": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN2/device/fan1_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN2/pwm4",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN2/fan4_input",
            "source": "sysfs"
        }
    },
    "FANTRAY3_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN1/device/fan2_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN1/pwm5",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN1/fan5_input",
            "source": "sysfs"
        }
    },
    "FANTRAY3_FAN2": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN2/device/fan2_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN2/pwm6",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN2/fan6_input",
            "source": "sysfs"
        }
    },
    "FANTRAY4_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN1/device/fan3_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN1/pwm7",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN1/fan7_input",
            "source": "sysfs"
        }
    },
    "FANTRAY4_FAN2": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN2/device/fan3_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN2/pwm8",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN2/fan8_input",
            "source": "sysfs"
        }
    },
    "FANTRAY5_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN1/device/fan4_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN1/pwm9",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN1/fan9_input",
            "source": "sysfs"
        }
    },
    "FANTRAY5_FAN2": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN2/device/fan4_presence",
            "source": "sysfs"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN2/pwm10",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN2/fan10_input",
            "source": "sysfs"
        }
    }
},
    "zones": {
    "zone1": {
        "fans": [
            "FANTRAY1_FAN1",
            "FANTRAY1_FAN2",
            "FANTRAY2_FAN1",
            "FANTRAY2_FAN2",
            "FANTRAY3_FAN1",
            "FANTRAY3_FAN2",
            "FANTRAY4_FAN1",
            "FANTRAY4_FAN2",
            "FANTRAY5_FAN1",
            "FANTRAY5_FAN2"
        ],
        "sensors": [
            "TEMP_SCM_INLET"
        ],
        "slope": 3,
        "zone_type": "max"
    }
}
    })json";
} // getSandiaFSConfig()

} // namespace facebook::fboss::platform
