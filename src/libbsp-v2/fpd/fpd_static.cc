/*!
 * fpd_setup.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <iostream>
#include <bsp/fpd.h>
#include <private/sysfs.h>

#include <fpd/aldrin.h>
#include <fpd/bmc_bios.h>
#include <fpd/cpucpld.h>
#include <fpd/nvme.h>
#include <fpd/pemcpld.h>
#include <fpd/powercpld.h>
#include <fpd/ssd.h>

#define THROW_ON_FAIL true


namespace bsp2 {

std::string getSandiafpds();
std::string getLassenfpds();

void
fpd_proxy_t::setup(pointer<fpd_t> parent)
{
    if (m_object)
        return;

    const fpd_t& cobj = *parent;
    fpd_t *lib_obj = NULL;
    const std::string& lib_symbol = libsymbol();

    if (!lib_symbol.compare("get_fpd_obj_pwrcpld")) {
        lib_obj = new Fpd_powercpld(cobj);
    } else if (!lib_symbol.compare("get_fpd_obj_cpucpld")) {
        lib_obj = new Fpd_cpucpld(cobj);
    } else if (!lib_symbol.compare("get_fpd_obj_nvme")) {
        lib_obj = new Fpd_nvme(cobj);
    } else if (!lib_symbol.compare("get_fpd_obj_ssd")) {
        lib_obj = new Fpd_ssd(cobj);
    } else if (!lib_symbol.compare("get_fpd_obj_bmc_bios")) {
        lib_obj = new Fpd_bmc_bios(cobj);
    }

    if (!lib_obj) {
        // These are implementations not available yet...
        lib_obj = new fpd_t(cobj);
    }
    m_object = lib_obj;
}
} // namespace bsp2
