/*!
 * idprom_factory.cc
 *
 * Copyright (c) 2021-2022 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "bsp/idprom.h"
#include "bsp/traits.h"
#include "private/find.h"
#include "private/sysfs.h"

namespace bsp2 {

INSTANTIATE_TRAITS(idprom_t,
                   oid_t::type_t::idprom,
                   "idprom",
                   "idproms",
                   "/opt/cisco/etc/metadata/idproms.json");
INSTANTIATE_FIND(idprom_t);

pointer<idprom_t>
idprom_t::factory(const std::string &ident)
{
    auto c = find<idprom_t>(ident, true);

    if (!c.empty()) {
        if (c.size() != 1) {
            std::stringstream msg;
            msg << ident
                << " matches "
                << c.size()
                << " idproms";
            throw std::invalid_argument(msg.str()); 
        }
        return c[0];
    }

    json j = json{
                  {"path", ident},
                  {"oid", {
                      { "type", "idprom" },
                      { "index", 0 }
                  }}
                 };
    idprom_p obj = std::make_shared<idprom_t>(j);

    return obj;
}

} // namespace bsp2
