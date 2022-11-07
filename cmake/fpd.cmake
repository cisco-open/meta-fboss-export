# fpd

add_library(fpd
    fpd/biosUtil.c
    fpd/fpd_bios.cc
    fpd/fpd_bios_upgrade.cc 
    fpd/fpd_bmc_bios.cc
    fpd/fpd_flash.cc
    fpd/sjtagUtil.c
    fpd/commonUtil.c
    fpd/fpd_utils.cc
    fpd/fpd_cpucpld.cc
    fpd/fpd_powercpld.cc
    fpd/fpd_powercpld_upgrade.cc
    fpd/fpd_nvme.cc
    fpd/fpd_nvme_upgrade.cc
    fpd/fpd_ssd.cc
    fpd/ssd.c
)
target_include_directories( fpd
    PUBLIC
      include
      ${json_SOURCE_DIR}/include
      ${date_SOURCE_DIR}/include
)
