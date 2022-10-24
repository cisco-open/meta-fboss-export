/*!
 * find.h
 *
 * Copyright (c) 2021-2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#ifndef _PRIVATE_FIND_H_
#define _PRIVATE_FIND_H_

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "bsp/fwd.h"
#include "bsp/find.h"
#include "bsp/object.h"
#include "bsp/oid.h"
#include "bsp/traits.h"

namespace bsp2 {

template<class C>
json
metadata(std::string json_data)
{
    static json cfg;
    static std::mutex m;
    std::lock_guard<std::mutex> l(m);

    if (cfg.is_null()) {
        cfg = json::parse(json_data);
    }
    return cfg[traits<C>::label];
}

template<class C>
pointer<C>
find(json &cfg)
{
    pointer<C> result;

    oid_t oid = cfg.value("oid", oid_t());
    if (oid.obj_type() != traits<C>::oid_type) {
       std::stringstream msg;
       msg << cfg["oid"];
       throw std::invalid_argument(msg.str());
    }
    std::lock_guard<std::mutex> l(object_t::db_m);

    auto it = object_t::db.find(oid);
    if (it != object_t::db.end()) {
        return std::dynamic_pointer_cast<C>(it->second);
    }
    auto obj = std::make_shared<C>(cfg);
    object_t::db[oid] = obj;
    return obj;
}

template<class C>
container<C>
find(const oid_t &from_oid,
     const oid_t &to_oid,
     bool visible_only)
{
    container<C> result;

    if ((from_oid.obj_type() != oid_t::type_t::unspecified &&
         from_oid.obj_type() != traits<C>::oid_type) ||
        (to_oid.obj_type() != oid_t::type_t::unspecified &&
         to_oid.obj_type() != traits<C>::oid_type)) {
        return result;
    }

    size_t from_index = from_oid.index();
    size_t to_index = to_oid.index();
    if (!to_index) {
        to_index = std::numeric_limits<size_t>::max();
    }
    if (to_index < from_index) {
        return result;
    }

    oid_t from(traits<C>::oid_type, from_index);
    oid_t to(traits<C>::oid_type, to_index);

    for (json j : metadata<C>("")) {
        oid_t oid = j.value("oid", oid_t());
        if (oid < from || oid > to) {
            continue;
        }
        auto obj = find<C>(j);
//      if (visible_only && !obj->is_visible()) {
//          continue;
//      }
        result.push_back(obj);
    }
    return result;
}

template<class C>
container<C>
load(std::string json_data)
{
    oid_t oid(traits<C>::oid_type, 0);
    metadata<C>(json_data);
    return find<C>(oid, oid, false);
}

template<class C>
container<C>
find(const std::string &name,
     bool visible_only)
{
    container<C> result;

    std::lock_guard<std::mutex> l(object_t::db_m);
    for (auto &it : object_t::db) {
        if ((!name.length() /* && (!visible_only || it.second->is_visible()) */ )
                    || it.second->matches_id(name)) {
            result.push_back(std::dynamic_pointer_cast<C>(it.second));
        }
    }
    return result;
}

#define INSTANTIATE_FIND(C) \
    template json metadata<C>(std::string json_data); \
    template pointer<C> find(json &cfg); \
    template container<C> load(std::string json_data); \
    template container<C> find(const oid_t &from_oid, \
                               const oid_t &to_oid, \
                               bool visible_only); \
    template container<C> find(const std::string &name, \
                               bool visible_only)

} // namespace bsp2

#endif // _PRIVATE_FIND_H_
