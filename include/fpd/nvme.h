/*
 * nvme.h
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <iostream>
#include "bsp/fpd.h"

class Fpd_nvme : public bsp2::fpd_t {
public:
    Fpd_nvme() = delete;

    Fpd_nvme(const fpd_t& fpd)
             : fpd_t(fpd){}

    void program(bool force = false) const override;

    std::string running_version() const override;

    std::string packaged_version() const override;

    void activate() const override;

    virtual ~Fpd_nvme() {}

private:
    std::string exec_shell_command(const char *cmd) const;
};
