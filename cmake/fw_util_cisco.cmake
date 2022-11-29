# fw_util_cisco

add_executable(fw_util_cisco
    src/fw_util/fw_util.cc
)

target_link_libraries(fw_util_cisco
    bsp-v2
    fpd
    dl
    gflags
    glog
    stdc++fs
    z
    fw_util
)
target_include_directories(fw_util_cisco
    PUBLIC
      src/fw_util
      fboss/platform/fw_util
      fboss/platform
      include
      ${json_SOURCE_DIR}/include
      ${date_SOURCE_DIR}/include
)
install(
    TARGETS fw_util_cisco
    DESTINATION opt/cisco/bin
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_EXECUTE
)
