/*!
 * sysfs.h
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#ifndef _PRIVATE_SYSFS_H_
#define _PRIVATE_SYSFS_H_

#include <filesystem>
#include <istream>
#include <sstream>
#include <string>

namespace bsp2 {

class sysfs { 
public:
    sysfs(const std::filesystem::path &path)
        : m_path(path)
    {
    }
    std::string get_value(const std::string &default_value = "") const {
        std::string result;
        try {
            std::ifstream f(m_path);
            getline(f, result);
        } catch (...) {
            result = default_value;
        }
        return result;
    }
    std::string get_data(const std::string &default_value = "") const {
        std::string result;
        try {
            std::ifstream f(m_path);
            std::stringstream stream;
            stream << f.rdbuf();
            result = stream.str();
            return result;
        } catch (...) {
            result = default_value;
        }
        return result;
    }
    void set_value(const std::string &value) const {
        std::ofstream f(m_path);
        f << value;
    }
    bool get_bool(bool default_value = false) const {
        std::string result(get_value());
        if (!result.length()) {
            return default_value;
        }
        const std::regex false_regex("\\s*(0+|no|off|false|f|disable)\\s*",
                                     std::regex_constants::icase);
        if (std::regex_match(result, false_regex)) {
            return false;
        }
        const std::regex true_regex("\\s*(1|yes|on|true|t|enable)\\s*",
                                    std::regex_constants::icase);
        if (std::regex_match(result, true_regex)) {
            return true;
        }
        return default_value;
    }
    void set_bool(bool value) {
        std::ofstream f(m_path);
        int v = value ? 1 : 0;
        f << v;
    }
private:
    std::filesystem::path m_path;
};

} // namespace bsp2

#endif // _PRIVATE_SYSFS__H_
