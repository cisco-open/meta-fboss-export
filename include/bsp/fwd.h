/**
 * @file fwd.h
 *
 * @brief Forward symbol declarations and simple inline helpers
 *
 * @copyright Copyright (c) 2021-2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */
#ifndef BSP_FWD_H_
#define BSP_FWD_H_

#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <sstream>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

/**
 * @brief Holds all public APIs
 */
namespace bsp2 {

//!
//! @brief A tray FRU holding multiple instances of C
//!
template<class C> class tray_t;

class cached_idprom_t;
class chassis_t;
class current_t;
class fan_t;
class fpd_t;
class fru_t;
class gpio_t;
class idprom_t;
class indicator_t;
class led_t;
class object_t;
class oid_t;
class platform_t;
class power_t;
class psu_t;
class pim_t;
class resolver_t;
class sensor_t;
class sfp_t;
class thermal_t;
class topology_t;
class voltage_t;
class watchdog_t;
class gpio_expander_t;
class misc_data_t;

class duty_cycle_map_t;

//! A fan tray
typedef tray_t<fan_t> fan_tray_t;

//! A power supply tray
typedef tray_t<psu_t> psu_tray_t;

//! A shared pointer to a class instance
template<class C>
using pointer = std::shared_ptr<C>;

//! A container of shared pointers to a class instance
template<class C>
using container = std::vector<pointer<C>>;

//! Shorthand for shared pointers
using cached_idprom_p=pointer<cached_idprom_t>;
using chassis_p=pointer<chassis_t>;
using current_p=pointer<current_t>;
using fan_p=pointer<fan_t>;
using fpd_p=pointer<fpd_t>;
using fru_p=pointer<fru_t>;
using fpio_p=pointer<gpio_t>;
using idprom_p=pointer<idprom_t>;
using indicator_p=pointer<indicator_t>;
using led_p=pointer<led_t>;
using object_p=pointer<object_t>;
using oid_p=pointer<oid_t>;
using platform_p=pointer<platform_t>;
using power_p=pointer<power_t>;
using psu_p=pointer<psu_t>;
using pim_p=pointer<pim_t>;
using resolver_p=pointer<resolver_t>;
using sensor_p=pointer<sensor_t>;
using sfp_p=pointer<sfp_t>;
using thermal_p=pointer<thermal_t>;
using topology_p=pointer<topology_t>;
using voltage_p=pointer<voltage_t>;
using watchdog_p=pointer<watchdog_t>;
using gpio_expander_p=pointer<gpio_expander_t>;
using misc_data_p=pointer<misc_data_t>;
using fan_tray_p=pointer<fan_tray_t>;
using psu_tray_p=pointer<psu_tray_t>;

} // namespace bsp2

#endif /* ndef BSP_FWD_H_ */
