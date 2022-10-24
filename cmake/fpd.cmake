# fpd

add_library(fpd
    fpd/fpd_bmc_bios.cc
    fpd/commonUtil.c
    fpd/fpd_cpucpld.cc
    fpd/fpd_powercpld.cc
    fpd/fpd_nvme.cc
    fpd/fpd_ssd.cc
    fpd/ssd.c
)
target_include_directories( fpd
    PUBLIC
      include
      ${json_SOURCE_DIR}/include
      ${date_SOURCE_DIR}/include
)
