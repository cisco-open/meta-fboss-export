# data_corral_service_sandia

add_library(data_corral_service_sandia
    fboss/platform/data_corral_service/sandia/SandiaPlatformConfig.cpp
    fboss/platform/data_corral_service/sandia/SandiaFruModule.cpp
    fboss/platform/data_corral_service/sandia/SandiaChassisManager.cpp
)

target_link_libraries(data_corral_service_sandia
    bsp-v2
    dl
    gflags
    glog
    stdc++fs
)
target_include_directories(data_corral_service_sandia
    PUBLIC
      src/data_corral_service/sandia
      fboss/platform/data_corral_service/sandia
      fboss/platform
      include
      ${json_SOURCE_DIR}/include
      ${date_SOURCE_DIR}/include
      $ENV{FBOSS_PATH}
)
install(
    TARGETS data_corral_service_sandia
    DESTINATION opt/cisco/lib
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_EXECUTE
)
