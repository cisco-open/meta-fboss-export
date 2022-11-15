/**
 * @file WeutilPlatform.h
 *
 * @brief Cisco-8000 implementation of weutil utility
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 *
 * Interface adheres to requirements of
 *     repository: git@github.com:facebook/fboss.git
 *     path:       fboss/platform/weutil/WeutilInterface.h
 *     (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
 * and skeleton is copied from same.
 *
 */
#pragma once

#include <memory>
#include <nlohmann/json.hpp>

#include "fboss/platform/weutil/WeutilInterface.h"

#include "bsp/idprom.h"
#include "WeutilSandia.h"
#include "WeutilLassen.h"
#include "WeutilExport.h"

using json = nlohmann::json;

namespace facebook::fboss::platform {

class WeutilCisco : public WeutilInterface
{
public:
    WeutilCisco(std::string platform_json, std::string weutil_json)
        : WeutilInterface()
    {
        select_idproms(platform_json, weutil_json);
    }
    ~WeutilCisco()
    {
    }
    json load(const std::string json) {
        return json::parse(json);
    }
    void printInfo() override {
        printInfo(std::cout);
    }
    void printInfoJson() override {
        printInfoJson(std::cout);
    }
    bool verifyOptions(void) override;
    std::vector<std::pair<std::string, std::string>> getInfo(std::string eeprom = "") override;

    void printInfo(std::ostream &s);
    void printInfoJson(std::ostream &s);
private:
    bsp2::container<bsp2::idprom_t> m_idproms;
    json root;

    void select_idproms(std::string platform_json, std::string weutil_json);
    bool idprom(const json &node, std::string &result) const;
    bool idprom(const std::string &token, std::string &result) const;
    bool idprom(const bsp2::container<bsp2::idprom_t> &idproms,
                const std::string &tag,
                size_t instance,
                std::string &result) const;
    bool process_file(const json &node,
                      const std::string &key,
                      std::string &translation) const;
    bool process_tag(const json &node,
                     const std::string &key,
                     std::string &translation) const;
    void format(const json &node,
                std::string &translation) const;
};

} // namespace facebook::fboss::platform
