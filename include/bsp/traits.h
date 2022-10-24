/**
 * @file traits.h
 *
 * @brief Definitions related to object traits
 *
 * @copyright Copyright (c) 2021-2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */

#ifndef BSP_TRAITS_H_
#define BSP_TRAITS_H_

#include <string>

#include "bsp/oid.h"

//!
//! @brief Holds all public APIS
//!
namespace bsp2 {

template<class C>
class traits
{
public:
    static const oid_t::type_t oid_type;
    static const std::string name;
    static const std::string label;
    static const std::string metadata_path;
};

#define INSTANTIATE_TRAITS(C, Type, Name, Label, Path) \
    template<> const oid_t::type_t traits<C>::oid_type(Type); \
    template<> const std::string traits<C>::name(Name); \
    template<> const std::string traits<C>::label(Label); \
    template<> const std::string traits<C>::metadata_path(Path)

} // namespace bsp2

#endif // ndef BSP_TRAITS_H_
