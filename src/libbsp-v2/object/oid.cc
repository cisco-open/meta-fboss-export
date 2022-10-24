/*!
 * oid.cc
 *
 * Copyright (c) 2021-2022 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <vector>

#include "bsp/fwd.h"
#include "bsp/oid.h"

namespace bsp2 {

NLOHMANN_JSON_SERIALIZE_ENUM(oid_t::type_t, {
     {oid_t::type_t::unspecified, "unspecified"},
     {oid_t::type_t::chassis, "chassis"},
     {oid_t::type_t::module, "module"},
     {oid_t::type_t::thermal, "thermal"},
     {oid_t::type_t::fan, "fan"},
     {oid_t::type_t::psu, "psu"},
     {oid_t::type_t::pim, "pim"},
     {oid_t::type_t::led, "led"},
     {oid_t::type_t::sfp, "sfp"},
     {oid_t::type_t::watchdog, "watchdog"},
     {oid_t::type_t::fpd, "fpd"},
     {oid_t::type_t::voltage, "voltage"},
     {oid_t::type_t::current, "current"},
     {oid_t::type_t::idprom, "idprom"},
     {oid_t::type_t::np, "np"},
     {oid_t::type_t::power, "power"},
     {oid_t::type_t::fan_tray, "fan_tray"},
     {oid_t::type_t::psu_tray, "psu_tray"},
     {oid_t::type_t::platform, "platform"},
     {oid_t::type_t::gpio_expander, "gpio_expander"},
     {oid_t::type_t::misc_data, "misc_data"},
     {oid_t::type_t::fru, "fru"},
     {oid_t::type_t::indicator, "indicator"},
})

oid_t::oid_t(oid_t::type_t t, oid_t::value_type index)
    : m_type(t)
    , m_index(index)
{
    if ((t < 0) || (t >= oid_t::type_t::_last)) {
        std::string r("oid_t::type_t(");
        r.append(std::to_string(t))
         .append(")");
        throw std::system_error(ERANGE, std::generic_category(), r);
    }
}

oid_t::oid_t()
    : oid_t(oid_t::type_t::unspecified, 0)
{
}

oid_t::oid_t(size_t v)
    : oid_t(static_cast<oid_t::type_t>(v >> 24), (v & 0xffffff))
{
}

oid_t::operator std::string() const
{
    std::stringstream buf;
    json j = m_type;

    buf << j.get<std::string>()
        << ":"
        << std::setw(2) << std::setfill('0') << std::hex << m_type
        << std::setw(6) << m_index
        ;
    return buf.str();
}

void
to_json(json& j, const oid_t& obj)
{
    j = json{{"type", obj.obj_type()}, {"index", obj.index()}};
}

void
from_json(const json& j, oid_t& obj)
{
    j.at("type").get_to(obj.m_type);
    j.at("index").get_to(obj.m_index);
}

} // namespace bsp2
