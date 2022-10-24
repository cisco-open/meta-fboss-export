/*!
 * fpd.cc
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */

#include <iomanip>
#include <regex>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <wait.h>
#include <fstream>
#include <cmath>

#include <bsp/fwd.h>
#include <bsp/fpd.h>
#include <bsp/traits.h>
#include <private/sysfs.h>
#include <private/find.h>

#define THROW_ON_FAIL true

namespace bsp2 {

INSTANTIATE_TRAITS(fpd_t,
                   oid_t::type_t::fpd,
                   "fpd",
                   "fpds",
                   "/opt/cisco/etc/metadata/fpds.json");

INSTANTIATE_FIND(fpd_t);

fpd_proxy_t::~fpd_proxy_t() {
    delete m_object;
    if (m_handle) {
        dlclose(m_handle);
    }
}

std::vector<std::shared_ptr<fpd_t>>
fpd_t::factory(const std::string &ident)
{
    auto c = find<fpd_t>(ident, true);
    std::vector<std::shared_ptr<fpd_t>> objs;

    if (!c.empty()) {
        for (auto x : c) {
            pointer<fpd_proxy_t> y = std::make_shared<fpd_proxy_t>(*x);
            y->setup(x);
            objs.push_back(y);
        }
        return objs;
    }
    std::stringstream msg;
    msg << ident
        << " matches "
        << c.size()
        << " fpds";
    throw std::invalid_argument(msg.str());
}

std::string
fpd_t::get_version() const
{
    std::string line;
    if (!m_version.empty()) {
        std::ifstream file;
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        file.open(m_version);

        getline(file, line);
    }
    return line;
}

const std::filesystem::path&
fpd_t::get_i2c_info() const
{
    return m_device_path;
}

const std::filesystem::path&
fpd_t::get_activate_path() const
{
    return m_activate_path;
}

void
fpd_t::set_activate_path_value(const std::filesystem::path &value) const
{
    sysfs(m_activate_path).set_value(value);
}

const std::string&
fpd_t::get_fpga_offset(const std::string &key) const
{
    auto it = m_offsets.find(key);

    if (it == m_offsets.end()) {
        throw std::system_error(ENOENT, std::generic_category(), key);
    }
    return it->second;
}

fpd_t::operator std::string() const
{
    std::string result;
    std::string osep;

    if (!m_version.empty()) {
        result.append(osep)
              .append("version ")
              .append(m_version) ;
        osep = ", ";
    }
    if (!m_path.empty()) {
        result.append(osep)
              .append("path ")
              .append(m_path);
        osep = ", ";
    }
    if (!m_alt_path.empty()) {
        result.append(osep)
              .append("alt_path ")
              .append(m_alt_path);
        osep = ", ";
    }
    if (!m_helper.empty()) {
        result.append(osep)
              .append("helper: [");
        std::string sep;
        for (auto p : m_helper) {
            result.append(sep)
                  .append(p);
            sep = ", ";
        }
        result.append("]");
    }
    return result;
}

std::ostream &
fpd_t::dump(std::ostream &os, size_t indentation) const
{
    return os << *this;
}

std::ostream&
operator<<(std::ostream& os, const fpd_t &fpd)
{
    return os << std::string(fpd);
}

bool
fpd_t::is_present() const

{
    std::error_code ec; 

    if (m_presence.empty()) {
        return true;
    }

    return sysfs(m_presence).get_bool(false);
}

std::string
fpd_t::running_version() const
{
    std::string info(__func__);
    info.append(": ");
    throw std::system_error(ENOTSUP, std::generic_category(), info);
}

std::string
fpd_t::packaged_version()  const
{
    std::string info(__func__);
    info.append(": ");
    throw std::system_error(ENOTSUP, std::generic_category(), info);
}

void
fpd_t::program(bool force) const
{
    std::string info(__func__);
    info.append(": ");
    throw std::system_error(ENOTSUP, std::generic_category(), info);
}

void
fpd_t::erase() const
{
    std::string info(__func__);
    info.append(": ");
    throw std::system_error(ENOTSUP, std::generic_category(), info);
}

std::string
fpd_t::verify() const
{
    std::string info(__func__);
    info.append(": ");
    throw std::system_error(ENOTSUP, std::generic_category(), info);
}

void
fpd_t::activate() const
{
    std::string info(__func__);
    info.append(": ");
    throw std::system_error(ENOTSUP, std::generic_category(), info);
}

bool
fpd_t::set_file_path(const std::string &ipath)
{
    std::ifstream f(ipath);
    if (f.good()) {
        m_path = ipath;
        return true;
    }
    return false;
}

bool
fpd_t::compare_version(const std::string &ver1, const std::string &ver2) const
{
    int major1, major2, minor1, minor2;

    int pos = ver1.find('.', 0);
    if (pos == ver1.npos) {
        major1 = stoi(ver1);
        minor1 = 0;
    } else {
        major1 = stoi(ver1.substr(0,pos));
        minor1 = stoi(ver1.substr(pos+1));
    }

    pos = ver2.find('.', 0);
    if (pos == ver2.npos) {
        major2 = stoi(ver2);
        minor2 = 0;
    } else {
        major2 = stoi(ver2.substr(0,pos));
        minor2 = stoi(ver2.substr(pos+1));
    }
    return (major1 > major2 ? true : (major1 < major2 ? false : (minor1 >= minor2)));
}

int
fpd_t::invoke(const std::vector<std::string> &cmd,
              const std::vector<std::string> &paths) const
{
    pid_t pid = fork();
    int e;
    if (pid == (pid_t)-1) {
        e = errno;
        std::string info("fork:");
        std::for_each(cmd.begin(), cmd.end(),
                      [&info](const std::string &s) {
                          info.append(" ").append(s);
                      });
        throw std::system_error(e, std::generic_category(), info);
    }
    if (!pid) {
        /* Child */
        int fd = open("/dev/null", O_RDONLY);
        if (fd != -1) {
            (void)dup2(fd, 0);
            (void)close(fd);
        }
        std::vector<std::string> ncmd;
        for (const auto &s : cmd) {
            if (s.find("{PATH}") != s.npos) {
                std::regex r("\\{PATH\\}");
                for (const auto &p : paths) {
                    ncmd.push_back(std::regex_replace(s, r, p));
                }
            } else {
                ncmd.push_back(s);
            }
        }
        char **ecmd = new char*[ncmd.size() + 1];
        for (auto i = 0; i < ncmd.size(); i++) {
            ecmd[i] = const_cast<char*>(ncmd[i].c_str());
        }
        ecmd[ncmd.size()] = nullptr;
        execvp(ecmd[0], ecmd);
        e = errno;
        std::string info("execvp: ");
        std::for_each(ncmd.begin(), ncmd.end(),
                      [&info](const std::string &s) {
                          info.append(" ").append(s);
                      });
        throw std::system_error(e, std::generic_category(), info);
    }

    /* Parent */
    for(;;) {
        int status;
        e = waitpid(pid, &status, 0);
        if (e == -1) {
            e = errno;
            std::string info("waitpid: ");
            throw std::system_error(e, std::generic_category(), info.append(std::to_string(pid)));
        }
        if ((pid_t)e != pid) {
            std::string info("waitpid(");
            info.append(std::to_string(pid))
                .append(" expected ")
                .append(std::to_string(e))
                ;
            throw std::system_error(EINVAL, std::generic_category(), info);
        }

        if (WIFSTOPPED(status)) {
            std::cerr << "INFO: WIFSTOPPED (signal "
                      << WSTOPSIG(status)
                      << ") received on pid "
                      << pid
                      << std::endl;
            continue;
        } else if (WIFCONTINUED(status)) {
            std::cerr << "INFO: WIFCONTINUED received on pid "
                      << pid
                      << std::endl;
            continue;
        } else if (WIFSIGNALED(status)) {
            std::string info("WIFSIGNALED (signal ");
            info.append(std::to_string(WTERMSIG(status)))
                .append("; coredump ")
                .append(std::to_string(WCOREDUMP(status)))
                .append(") received on pid ")
                .append(std::to_string(pid));
            throw std::system_error(EINVAL, std::generic_category(), info);
        } else if (WIFEXITED(status)) {
            if (THROW_ON_FAIL) {
                std::string info("WIFEXITED (status ");
                info.append(std::to_string(WEXITSTATUS(status)))
                    .append(") received on pid ")
                    .append(std::to_string(pid));
                if (WEXITSTATUS(status)) {
                    throw std::system_error(EINVAL, std::generic_category(), info);
                }
                e = 0;
            } else {
                e = WEXITSTATUS(status);
            }
            break;
        } else {
            std::string info("waitpid(");
            info.append(std::to_string(pid))
                .append(") returned illegal status ")
                .append(std::to_string(status));
            throw std::system_error(EINVAL, std::generic_category(), info);
        }
    }
    return e;
}

void
to_json(json& j, const fpd_t& obj)
{
    const object_t& base = obj;

    to_json(j, base);
    j = json{
             {"object", base},
             {"fw_ver_path", obj.m_version},
             {"path", obj.m_path},
             {"alt_path", obj.m_alt_path},
             {"activate_path", obj.m_activate_path},
             {"device_path", obj.m_device_path},
             {"cmdline", obj.m_helper},
             {"dllpath", obj.m_dllpath},
             {"dllsymbol", obj.m_dllsymbol},
             {"golden", obj.m_golden},
             {"offsets", obj.m_offsets}
            };
}

void
from_json(const json& j, fpd_t& obj)
{
    object_t& base = obj;

    from_json(j, base);

    obj.m_path = j.value("path", "");
    obj.m_alt_path = j.value("alt_path", "");
    obj.m_version = j.value("fw_ver_path", "");
    obj.m_activate_path = j.value("activate_path", "");
    obj.m_device_path = j.value("device_path", "");

    if (j.contains("cmdline")) {
        j.at("cmdline").get_to(obj.m_helper);
    }
    obj.m_dllpath = j.value("dllpath", "");
    obj.m_dllsymbol = j.value("dllsymbol", "");
    obj.m_golden = j.value("golden", false);

    if (j.contains("offsets")) {
        j.at("offsets").get_to(obj.m_offsets);
    } else {
        if (!obj.m_offsets.empty()) {
            obj.m_offsets.clear();
        }
    }
}

} // namespace bsp2
