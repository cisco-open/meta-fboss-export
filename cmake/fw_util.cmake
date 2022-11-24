# fw_util

add_library(fw_util
    src/fw_util/FirmwareExport.cc
    fboss/platform/fw_util/FirmwareUpgrade.cc
    fboss/platform/fw_util/FirmwareSandia.cc
    fboss/platform/fw_util/FirmwareLassen.cc
    fboss/platform/fw_util/SandiaFw_utilConfig.cpp
    fboss/platform/fw_util/LassenFw_utilConfig.cpp
)

target_link_libraries(fw_util
    bsp-v2
    fpd
    dl
    gflags
    glog
    stdc++fs
    z
    pthread
)
target_include_directories(fw_util
    PUBLIC
      src/fw_util
      fboss/platform/fw_util
      fboss/platform
      include
      ${json_SOURCE_DIR}/include
      ${date_SOURCE_DIR}/include
)
install(
    TARGETS fw_util
    DESTINATION opt/cisco/lib
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_EXECUTE
)
