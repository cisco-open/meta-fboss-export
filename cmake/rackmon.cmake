# rackmon

set(rackmon_bmc_FILES
    rackmond.service
    Device.cpp
    Device.h
    Log.h
    ModbusCmds.cpp
    ModbusCmds.h
    Modbus.cpp
    Modbus.h
    ModbusError.h
    Msg.cpp
    Msg.h
    UARTDevice.cpp
    UARTDevice.h
    Register.cpp
    Register.h
    ModbusDevice.cpp
    ModbusDevice.h
    Rackmon.cpp
    Rackmon.h
    PollThread.h
    RackmonSvcUnix.cpp
    RackmonCliUnix.cpp
    UnixSock.cpp
    UnixSock.h
    configs/registser_map/orv2_psu.json
)
set(rackmon_bmc_SOURCE_DIR
    "${CMAKE_BINARY_DIR}/_deps/rackmon_bmc-src")
set(rackmon_bmc_SHA1 "ced97f183a40d3190ba873f72aa210603ec3872f")
set(rackmon_bmc_URL
    "https://raw.githubusercontent.com/facebook/openbmc/${rackmon_bmc_SHA1}/common/recipes-core/rackmon2/rackmon")

file(MAKE_DIRECTORY ${rackmon_bmc_SOURCE_DIR})
foreach(file ${rackmon_bmc_FILES})
    file(DOWNLOAD
         ${rackmon_bmc_URL}/${file}
         ${rackmon_bmc_SOURCE_DIR}/${file}
        )
endforeach(file)

###################################################################

set(cli11_FILES
    App.hpp
    CLI.hpp
    Config.hpp
    ConfigFwd.hpp
    Error.hpp
    Formatter.hpp
    FormatterFwd.hpp
    Macros.hpp
    Option.hpp
    Optional.hpp
    Split.hpp
    StringTools.hpp
    Timer.hpp
    TypeTools.hpp
    Validators.hpp
    Version.hpp
)
set(cli11_SOURCE_DIR
    "${CMAKE_BINARY_DIR}/_deps/cli11-src")
set(cli11_SHA1 "13becaddb657eacd090537719a669d66d393b8b2")
set(cli11_URL
    "https://raw.githubusercontent.com/CLIUtils/CLI11/${cli11_SHA1}/include/CLI"
)

file(MAKE_DIRECTORY ${cli11_SOURCE_DIR}/CLI)
foreach(file ${cli11_FILES})
    file(DOWNLOAD
         ${cli11_URL}/${file}
         ${cli11_SOURCE_DIR}/CLI/${file}
        )
endforeach(file)

###################################################################

add_executable(rackmond
    ${rackmon_bmc_SOURCE_DIR}/Device.cpp
    ${rackmon_bmc_SOURCE_DIR}/ModbusCmds.cpp
    ${rackmon_bmc_SOURCE_DIR}/Modbus.cpp
    ${rackmon_bmc_SOURCE_DIR}/Msg.cpp
    ${rackmon_bmc_SOURCE_DIR}/UARTDevice.cpp
    ${rackmon_bmc_SOURCE_DIR}/ModbusDevice.cpp
    ${rackmon_bmc_SOURCE_DIR}/Register.cpp
    ${rackmon_bmc_SOURCE_DIR}/Rackmon.cpp
    ${rackmon_bmc_SOURCE_DIR}/RackmonSvcUnix.cpp
    ${rackmon_bmc_SOURCE_DIR}/UnixSock.cpp
)
target_link_libraries(rackmond
    glog
    stdc++fs
    pthread
)
target_include_directories(rackmond
    PUBLIC
        ${rackmon_bmc_SOURCE_DIR}
        ${json_SOURCE_DIR}/include
)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^Clang")
    target_compile_options(rackmond
        PUBLIC
            -Wno-error=missing-field-initializers
            -Wno-error=uninitialized
    )
else()
    target_compile_options(rackmond
        PUBLIC
            -Wno-error=missing-field-initializers
    )
endif()

add_executable(rackmoncli
    ${rackmon_bmc_SOURCE_DIR}/RackmonCliUnix.cpp
    ${rackmon_bmc_SOURCE_DIR}/UnixSock.cpp
)
target_link_libraries(rackmoncli
    glog
    stdc++fs
)
target_include_directories(rackmoncli
    PUBLIC
        ${rackmon_bmc_SOURCE_DIR}
        ${json_SOURCE_DIR}/include
        ${cli11_SOURCE_DIR}
)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^Clang")
    target_compile_options(rackmoncli
        PUBLIC
            -Wno-error=missing-field-initializers
            -Wno-error=range-loop-construct
            -Wno-error=uninitialized
    )
elseif("${CMAKE_CXX_COMPILER_VERSION}" VERSION_GREATER_EQUAL "11")
    target_compile_options(rackmoncli
        PUBLIC
            -Wno-error=missing-field-initializers
            -Wno-error=range-loop-construct
    )
else()
    target_compile_options(rackmoncli
        PUBLIC
            -Wno-error=missing-field-initializers
    )
endif()
install(
    TARGETS rackmond rackmoncli
    DESTINATION usr/local/bin
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_EXECUTE
)
