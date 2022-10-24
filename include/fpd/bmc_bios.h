/*!
 * bmc_bios.h
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <iostream>
#include "bsp/fpd.h"

class Fpd_bmc_bios : public bsp2::fpd_t {
public:
    Fpd_bmc_bios() = delete;

    Fpd_bmc_bios(const fpd_t& fpd)
             : fpd_t(fpd){}

    void program(bool force = false) const override;

    std::string running_version() const override;

    std::string packaged_version() const override;

    std::string get_bios_version_from_spiflash() const;

    virtual ~Fpd_bmc_bios() {}

};
