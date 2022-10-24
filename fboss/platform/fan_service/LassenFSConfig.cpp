/** 
 * @file LassenFSConfig.cpp
 *
 * @brief Lassen fan service configuration file for FBOSS
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */

#include <string>

namespace facebook::fboss::platform {

std::string
getLassenFSConfig() {
    return R"json({
    "bsp": "lassen",
    "boost_on_dead_fan": true,
    "boost_on_dead_sensor": false,
    "boost_on_no_qsfp_after": 90,
    "pwm_boost_value": 35,
    "pwm_transition_value": 35,
    "pwm_percent_lower_limit": 5,
    "pwm_percent_upper_limit": 35,
    "shutdown_command": "wedge_power reset -s",
    "sensors": {
    "TMP421_LOCAL_BB": {
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
            "alarm_major": 50,
            "alarm_minor": 45,
            "alarm_minor_soak": 15
        },
        "normal_down_table": [
            [
                [
                    25,
                    5
                ],
                [
                    30,
                    10
                ],
                [
                    35,
                    20
                ],
                [
                    40,
                    30
                ],
                [
                    110,
                    35
                ]
            ]
        ],
        "normal_up_table": [
            [
                [
                    25,
                    5
                ],
                [
                    30,
                    10
                ],
                [
                    35,
                    20
                ],
                [
                    40,
                    30
                ],
                [
                    110,
                    35
                ]
            ]
        ],
        "onefail_down_table": [
            [
                [
                    25,
                    5
                ],
                [
                    30,
                    10
                ],
                [
                    35,
                    20
                ],
                [
                    40,
                    30
                ],
                [
                    110,
                    35
                ]
            ]
        ],
        "onefail_up_table": [
            [
                [
                    25,
                    5
                ],
                [
                    30,
                    10
                ],
                [
                    35,
                    20
                ],
                [
                    40,
                    30
                ],
                [
                    110,
                    35
                ]
            ]
        ],
        "scale": 1000.0,
        "type": "linear_four_curves"
    }
},
    "fans": {
    "FANTRAY1_FAN0": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY1_FAN0/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN0/pwm11",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN0/fan11_input",
            "source": "sysfs"
        }
    },
    "FANTRAY1_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY1_FAN1/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN1/pwm12",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY1_FAN1/fan12_input",
            "source": "sysfs"
        }
    },
    "FANTRAY2_FAN0": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY2_FAN0/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN0/pwm9",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN0/fan9_input",
            "source": "sysfs"
        }
    },
    "FANTRAY2_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY2_FAN1/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN1/pwm10",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY2_FAN1/fan10_input",
            "source": "sysfs"
        }
    },
    "FANTRAY3_FAN0": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY3_FAN0/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN0/pwm7",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN0/fan7_input",
            "source": "sysfs"
        }
    },
    "FANTRAY3_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY3_FAN1/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN1/pwm8",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY3_FAN1/fan8_input",
            "source": "sysfs"
        }
    },
    "FANTRAY4_FAN0": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY4_FAN0/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN0/pwm5",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN0/fan5_input",
            "source": "sysfs"
        }
    },
    "FANTRAY4_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY4_FAN1/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN1/pwm6",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY4_FAN1/fan6_input",
            "source": "sysfs"
        }
    },
    "FANTRAY5_FAN0": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY5_FAN0/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN0/pwm3",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN0/fan3_input",
            "source": "sysfs"
        }
    },
    "FANTRAY5_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY5_FAN1/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN1/pwm4",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY5_FAN1/fan4_input",
            "source": "sysfs"
        }
    },
    "FANTRAY6_FAN0": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY6_FAN0/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY6_FAN0/pwm1",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY6_FAN0/fan1_input",
            "source": "sysfs"
        }
    },
    "FANTRAY6_FAN1": {
        "fan_missing_val": 0,
        "fan_present_val": 1,
        "presence": {
            "path": "/run/devmap/cisco/devices/FANTRAY6_FAN1/presence",
            "source": "gpio"
        },
        "pwm": {
            "path": "/run/devmap/sensors/FANTRAY6_FAN1/pwm2",
            "source": "sysfs"
        },
        "pwm_range_max": 100,
        "pwm_range_min": 1,
        "rpm": {
            "path": "/run/devmap/sensors/FANTRAY6_FAN1/fan2_input",
            "source": "sysfs"
        }
    }
},
    "zones": {
    "zone1": {
        "fans": [
            "FANTRAY1_FAN0",
            "FANTRAY1_FAN1",
            "FANTRAY2_FAN0",
            "FANTRAY2_FAN1",
            "FANTRAY3_FAN0",
            "FANTRAY3_FAN1",
            "FANTRAY4_FAN0",
            "FANTRAY4_FAN1",
            "FANTRAY5_FAN0",
            "FANTRAY5_FAN1",
            "FANTRAY6_FAN0",
            "FANTRAY6_FAN1"
        ],
        "sensors": [
            "TMP421_LOCAL_BB"
        ],
        "slope": 3,
        "zone_type": "max"
    }
}
    })json";
} // getLassenFSConfig()

} // namespace facebook::fboss::platform
