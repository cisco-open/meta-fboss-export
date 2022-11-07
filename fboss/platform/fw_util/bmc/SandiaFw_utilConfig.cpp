// 
// Copyright (c) 2022 by Cisco Systems, Inc.
// All rights reserved.
//
// This file was generated by gen_Fw_utilConfig.py
// Do not modify!
//

#include <string>

namespace facebook::fboss::platform {

std::string getSandiaFpdsData() {
  return R"json({
    "fpds": [
        {
            "cmdline": [
                "/opt/cisco/bin/bios_mtd_upgrade.sh",
                "{PATH}",
                "{PATH}"
            ],
            "description": "BIOS - Basic Input Output System",
            "dllpath": "/opt/cisco/lib/libfpd_bmc_bios.so.1.0.1",
            "dllsymbol": "get_fpd_obj_bmc_bios",
            "match": [],
            "name": "BIOS",
            "offsets": {
                "flash_version_offset": "0xDACE00",
                "helper_script": "/opt/cisco/bin/bios_bmc_create_mtd.sh",
                "mtd_name": "bios",
                "select_mux_script": "/opt/cisco/bin/bios_bmc_select_mux.sh",
                "unselect_mux_script": "/opt/cisco/bin/bios_bmc_unselect_mux.sh"
            },
            "oid": {
                "index": 1,
                "type": "fpd"
            },
            "parents": [
                {
                    "index": 1,
                    "type": "platform"
                }
            ],
            "path": "/mnt/data1/fpd/spf_scm_bios_upgrade.img"
        }
    ],
    "pid": "85_SCM_O_BMC"
}
  )json";

} // getSandiaFpds()

} // namespace facebook::fboss::platform
