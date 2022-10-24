/*
 * pemcpld.h
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <iostream>
#include "bsp/fpd.h"

class Fpd_pemcpld : public bsp2::fpd_t {
public:
    Fpd_pemcpld() = delete;

    Fpd_pemcpld(const fpd_t& fpd)
             : fpd_t(fpd){}

    void program(bool force = false) const override;

    std::string running_version() const override;

    std::string packaged_version() const override;

    void activate() const override;

    virtual ~Fpd_pemcpld() {}
};
