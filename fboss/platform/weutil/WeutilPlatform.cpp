/**
 * @file cisco_8000.cc
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

#include <errno.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "fboss/platform/weutil/WeutilPlatform.h"

#include "bsp/fwd.h"
#include "bsp/find.h"
#include "bsp/idprom.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

DEFINE_string(idproms, "",
              "comma-separated list of idproms to access");

DEFINE_bool(verbose, false,
              "Enable verbose error output");

namespace facebook::fboss::platform {

void
WeutilCisco::select_idproms(std::string platform_json, std::string weutil_json)
{
    bsp2::load<bsp2::idprom_t>(platform_json);
    root = load(weutil_json);

    if (FLAGS_idproms.empty()) {
         FLAGS_idproms = "board";
    }
    std::stringstream ss(FLAGS_idproms);
    std::string name;
    while (std::getline(ss, name, ',')) {
        if (name.empty()) {
            continue;
        }
        auto matches = bsp2::find<bsp2::idprom_t>(name);
        if (!matches.size()) {
            if (fs::exists(name)) {
                m_idproms.push_back(bsp2::idprom_t::factory(name));
            } else if (FLAGS_verbose) {
                std::cerr << "No idproms match " << name << std::endl;
            }
        } else {
            m_idproms.insert(m_idproms.end(), matches.begin(), matches.end());
        }
    }
}

void
WeutilCisco::printInfo(std::ostream &s)
{
    auto data = getInfo();
    for (auto item : data) {
        if (item.first == "Wedge EEPROM") {
              s << item.first
                << " "
                << item.second
                << ":"
                << std::endl
                ;
        } else {
              s << item.first
                << ": "
                << item.second
                << std::endl
                ;
        }
    }
}

void
WeutilCisco::printInfoJson(std::ostream &s)
{
    auto data = getInfo();
    json j;
    for (auto item : data) {
        if (item.first.compare("Wedge EEPROM")) {
            j[item.first] = item.second;
        }
    }
    s << std::setw(4) << j << std::endl;
}

std::vector<std::pair<std::string, std::string>>
WeutilCisco::getInfo()
{
    std::vector<std::pair<std::string, std::string>> ret;

    json weutil_root = root.at("weutil");
    if (m_idproms.size() && root.contains(m_idproms[0]->name())) {
        json node = root[m_idproms[0]->name()];
        if (node.is_array()) {
            weutil_root = node;
        }
    }
    if (!weutil_root.is_array()) {
        throw std::system_error(EINVAL, std::generic_category(),
                                "bad translation json input");
    }
    for (const auto &node : weutil_root) {
        const std::string key = node.value("tag", "");
        if (key.empty()) {
            continue;
        }
        bool resolved = false;
        std::string translation;
        if (node.contains("source")) {
            const json source_node = node.at("source");
            if (source_node.is_array()) {
                for (const auto &entry : source_node) {
                    if (process_tag(entry, "tag", translation) ||
                        process_file(entry, "file", translation)) {
                        resolved = true;
                        break;
                    }
               }
            }
        }
        if (!resolved &&
            !process_tag(node, "from-tag", translation) &&
            !process_file(node, "from-file", translation)) {
            translation = node.value("default", "");
        }
        ret.emplace_back(key, translation);
    }

    return ret;
}

bool
WeutilCisco::process_file(const json &node,
                          const std::string &key,
                          std::string &translation) const
{
    if (node.contains(key)) {
        std::filesystem::path filename(node.at(key));
        try {
            std::string file_contents;
            std::ifstream f(filename);
            f >> file_contents;
            translation = file_contents;
            format(node, translation);
            translation.erase(std::remove(translation.begin(),
                                          translation.end(),
                                          '\n'),
                              translation.end());
            return true;
        } catch (const std::exception &ex) {
            if (FLAGS_verbose) {
                std::cerr << filename << ": " << ex.what() << std::endl;
            }
        }
    }
    return false;
}

bool
WeutilCisco::process_tag(const json &node,
                         const std::string &key,
                         std::string &translation) const
{
    if (node.contains(key) && idprom(node.at(key), translation)) {
        format(node, translation);
        return true;
    }
    return false;
}

void
WeutilCisco::format(const json &node,
                    std::string &translation) const
{
    if (node.contains("format")) {
        const json fmt_node = node.at("format");
        if (fmt_node.is_array()) {
            std::regex r(fmt_node.at(0).get<std::string>());
            std::string f(fmt_node.at(1).get<std::string>());
            translation = std::regex_replace(translation, r, f);
        }
    }
}

bool
WeutilCisco::idprom(const json &node, std::string &translation) const
{
    if (node.is_null()) {
        return false;
    }
    if (node.is_string()) {
        const std::string tag = node.get<std::string>();
        return idprom(tag, translation);
    }
    if (!node.is_array()) {
        return false;
    }
    for (const auto &n : node) {
        if (n.is_string()) {
            const std::string tag = n.get<std::string>();
            if (idprom(tag, translation)) {
                return true;
            }
        }
    }
    return false;
}

bool
WeutilCisco::idprom(const std::string &token,
	             std::string &result) const
{
    if (token.empty() || m_idproms.empty()) {
        return false;
    }

    // Special case for internal object fields
    if (!token.compare("object::name")) {
        result = m_idproms[0]->name();
        return true;
    }
    if (!token.compare("object::path")) {
        result = m_idproms[0]->path(true);
        return true;
    }

    const std::regex r("(?:([^:]*):)?([^:0-9][^:]*)(?::(\\d{1,2}))?");
    std::smatch m;
    if (!std::regex_match(token, m, r)) {
        if (FLAGS_verbose) {
            std::cerr << "?Malformed token " << token << std::endl;
        }
        return false;
    }
    size_t instance = m.length(3) ? std::stoi(m[3]) : 0;

    if (m.length(1)) {
        return idprom(bsp2::find<bsp2::idprom_t>(m[1]), m[2], instance, result);
    }
    return idprom(m_idproms, m[2], instance, result);
}

bool
WeutilCisco::idprom(const bsp2::container<bsp2::idprom_t> &idproms,
                    const std::string &tag,
                    size_t instance,
                    std::string &result) const
{
    for (auto &c : idproms) {
        const auto &m = c->read();
        auto pos = m.find(tag);
        if (pos != m.end() && instance < pos->second.size()) {
            result = pos->second[instance];
            return true;
        }
    }
    return false;
}

} // namespace facebook::fboss::platform
