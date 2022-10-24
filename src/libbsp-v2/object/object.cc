/*!
 * object.cc
 *
 * Copyright (c) 2021 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <iostream>
#include <map>
#include <mutex>

#include "bsp/oid.h"
#include "bsp/object.h"

namespace bsp2 {

std::mutex object_t::db_m;
std::map<oid_t, object_p> object_t::db;

bool
object_t::is_present() const
{
    if (m_presence.empty()) {
        return true;
    }
    std::error_code ec;

    return std::filesystem::exists(m_presence, ec);
}

bool
object_t::is_ok() const
{
    if (!is_present()) {
        return false;
    }
    if (m_ok.empty()) {
        return true;
    }
    std::error_code ec;

    return std::filesystem::exists(m_ok, ec);
}

bool
object_t::matches_id(const std::string &given_name) const
{
    const char *c = given_name.c_str();
    if (!strcasecmp(name().c_str(), c)) {
        return true;
    }
    for (const auto &it : m_aliases) {
        if (!strcasecmp(it.c_str(), c)) {
            return true;
        }
    }
    return false;
}

void
to_json(json& j, const object_t& obj)
{
    j = json{{"oid", obj.oid()},
             {"name", obj.name()},
             {"description", obj.description()},
             {"parents", obj.parents()},
             {"aliases", obj.aliases()},
             {"presence", obj.m_presence},
             {"ok", obj.m_ok}
            };
}

void
from_json(const json& j, object_t& obj)
{
    j.at("oid").get_to(obj.m_oid);
    obj.m_name = j.value("name", "");
    obj.m_description = j.value("description", "");
    if (j.contains("parents")) {
        j.at("parents").get_to(obj.m_parents);
    }
    if (j.contains("aliases")) {
        j.at("aliases").get_to(obj.m_aliases);
    }
    obj.m_presence = j.value("presence", "");
    obj.m_ok = j.value("ok", "");
}

} // namespace bsp2
