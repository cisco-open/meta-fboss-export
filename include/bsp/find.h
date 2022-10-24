/**
 * @file bsp/find.h
 *
 * @brief find template algorithm
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */

#ifndef BSP_FIND_H_
#define BSP_FIND_H_

#include "bsp/fwd.h"

namespace bsp2 {

//! @brief Load (initialize) object database
//!
//! @returns all instantiated objects
//!
template<class C>
container<C> load(std::string);

//! @brief Find an object by their object identifier
//!
//! If the object type is specified, it must be in agreement
//! with the template class C object type
//!
//! @param[in] oid           The identifier to search for
//! @param[in] visible_only  Only return visible objects
//!
//! @returns the found object or a nullptr if not found
//!
template<class C>
pointer<C> find(const oid_t &oid,
                bool visible_only = true);

//!
//! @brief Find an object by name (possibly alias)
//!
//! @param[in] name          The name to search for
//! @param[in] visible_only  Only return visible objects
//!
//! @returns the list of found objects (possibly empty)
//!
template<class C>
container<C> find(const std::string &name = "",
                  bool visible_only = true);

} // namespace bsp2

#endif // ndef BSP_FIND_H_
