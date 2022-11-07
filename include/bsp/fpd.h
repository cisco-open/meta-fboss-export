/**
 * @file fpd.h
 *
 * @brief Definitions related to field programmable devices
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */
#ifndef BSP_FPD_H_
#define BSP_FPD_H_

#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>

#include "bsp/fwd.h"
#include "bsp/object.h"

//!
//! @brief Holds all public APIS
//!
namespace bsp2 {

//!
//! @brief Field programmable device object
//!
class fpd_t : public object_t {
public:
    //!
    //! @brief Construction / destruction
    //!

    fpd_t() :
        m_version(""),              m_device_path(""),     m_activate_path(""),
        m_path(""),                 m_alt_path(""),        m_helper(),
        m_verfile(""),              m_dllpath(""),         m_dllsymbol(""),
        m_offsets(),                m_golden(false)
    {};
    fpd_t(const fpd_t& fpd) = default;
    virtual ~fpd_t() = default;

    //!
    //! @brief Factory method to simplify applications
    //!
    //! @param[in] id  The identifier for the idprom
    //! @param[in] t   The identifier type
    //!
    //! @returns A pointer to the new object
    //!
    static std::vector<std::shared_ptr<fpd_t>> factory(const std::string &id);

    //!
    //! @brief For writing string representation to stream
    //!
    //! @param[in] os  Output stream
    //! @param[in] f   Object to write
    //!
    //! @returns output stream
    //!
    friend std::ostream& operator<<(std::ostream& os, const fpd_t &f);

    //!
    //! @brief Return list of pathnames associated with FPD
    //!
    //! @returns List of pathnames associated with FPD
    //!
    const std::string &path() const { return m_path; }
    const std::string &alt_path() const { return m_alt_path; }

    //!
    //! @brief Set the given path to the upgrade image path
    //!
    //! @param[in] path The path of the image to be used for upgrade
    //!
    //! @returns true if the path is successfully set, false otherwise.
    //!
    virtual bool set_file_path(const std::string &path);

    //!
    //! @brief gets the version of FPD from pathspec
    //!
    //! @returns The running version of the FPD
    //!
    std::string get_version() const;

    //!
    //! @brief gets the expected version of FPD from pathspec
    //!
    //! @returns The expected version of the FPD
    //!
    std::string get_expected_version() const;

    //!
    //! @brief gets the i2c device info from the device pathspec
    //!
    //! @returns i2c device info from resolved path
    //!
    const std::filesystem::path &get_i2c_info() const;

    //!
    //! @brief access to activate the fpd path
    //!
    //! @returns The the activate path string
    //!
    const std::filesystem::path &get_activate_path() const;

    //!
    //! @brief set a value to activate the fpd path
    //!
    void set_activate_path_value(const std::filesystem::path &value) const;

    //!
    //! @brief Running version of FPD
    //!
    //! @returns The running version of the FPD
    //!
    virtual std::string running_version() const;

    //!
    //! @brief  Version of FPD in the configured packaged image
    //!
    //! @returns The version of the FPD in the packaged image
    //!
    virtual std::string packaged_version() const;

    //!
    //! @brief Compares two fpd versions
    //!
    //! @param[in] version1 as a string
    //! @param[in] version2 as a string
    //!
    //! @returns True is version1 is greater or same as version2, False otherwise
    //!
    bool compare_version(const std::string &version1, const std::string &version2) const;

    //!
    //! @brief Returns the list of helpers associated with the FPD
    //!
    //! @returns The list of helpers associated with the FPD
    //!
    const std::vector<std::string> &helper() const { return m_helper; }

    //!
    //! @brief Returns the version file associated with the FPD
    //!
    //! @returns The version file associated with the FPD
    //!
    const std::filesystem::path &version_file() const { return m_verfile; }

    //!
    //! @brief Returns the path of library associated with the FPD
    //!
    //! @returns The path to the FPD library
    //!
    const std::string &libpath() const { return m_dllpath; }

    //!
    //! @brief Returns the symbol of library init associated with the FPD
    //!
    //! @returns The path to the FPD library
    //!
    const std::string &libsymbol() const { return m_dllsymbol; }

    //!
    //! @brief Checks if the FPD is golden
    //!
    //! @returns True if golden fpd is requested, false otherwise
    //!
    bool is_golden_fpd() const { return m_golden; }

    //!
    //! @brief Returns offset specific to a FPD 
    //!
    //! @returns the image offset required to upgrade the FW flash,
    //!          based on the input key
    //!
    const std::string &get_fpga_offset(const std::string &key) const;

    //!
    //! @brief Determine if the object is present
    //!        In general, the object and its parent (recursively) must be present
    //!
    //! Objects should not be accessed if they are not present
    //!
    //! @returns true if the object is present
    //! @returns false if the object is not present
    //!
    bool is_present() const override;

    //!
    //! @brief Erase and Update the firmware
    //!
    //! @param[in] force Boolean to request force upgrade
    //!
    virtual void program(bool force = false) const;

    //!
    //! @brief erases the firmware location
    //!
    virtual void erase() const;

    //!
    //! @brief verify the firmware location
    //!
    virtual std::string verify() const;

    //!
    //! @brief activates the fpd after firmware upgrade
    //!
    virtual void activate() const;

    //!
    //! @brief Convert object to string representation
    //!
    operator std::string () const;

    //!
    //! @brief Dump object (including possible debug information)
    //!        to the given stream
    //!
    //! @param[in] os          The output stream
    //! @param[in] indentation The desired indentation level
    //!
    //! @returns The output stream
    //!
    std::ostream &dump(std::ostream &os, size_t indentation=0) const;

    //!
    //! @brief Invoke an external command
    //!
    //! Invokes a command given an array of arguments.  Each argument
    //! may have an embedded {PATH} which is replaced with a set of
    //! arguments that are a copy of the passed paths
    //!
    //! @param[in] cmds  The argument vector to invoke, with {PATH} placeholders
    //! @param[in] paths The paths to fill in {PATH}
    //!
    //! @returns exit status of child
    //! @throws system_error on error
    //!
    int invoke(const std::vector<std::string> &cmds,
               const std::vector<std::string> &paths) const;

    //!
    //! @brief Convert object to json
    //!
    //! @param[out] j   The json representation of object
    //! @param[in]  obj The object to convert
    //!
    friend void to_json(json &j, const fpd_t &obj);

    //!
    //! @brief Convert object from json
    //!
    //! @param[in]   j   The json representation of object
    //! @param[out]  obj The destination object
    //!
    friend void from_json(const json &j, fpd_t &obj);

private:
    std::filesystem::path m_version;                   //!< path access to retrieve version
    std::filesystem::path m_device_path;               //!< path access to retrieve device path
    std::filesystem::path m_activate_path;             //!< path access to retrieve path to activate fpd

    std::string m_path;                                //!< path to object
    std::string m_alt_path;                            //!< alt path to object (usually tam)
    std::vector<std::string> m_helper;                 //!< Configured helpers
    std::filesystem::path m_verfile;                   //!< Configured version file
    std::string m_dllpath;                             //!< FPD Library path
    std::string m_dllsymbol;                           //!< FPD symbol path
    std::map<std::string, std::string> m_offsets;      //!< FPGA address offsets 
    bool m_golden;                                     //!< Golden upgrade flag
    std::string m_expected_version;                    //!< path access to retrieve expected version

}; // class fpd_t

class fpd_proxy_t : public fpd_t {
public:

    fpd_proxy_t() = delete;

    fpd_proxy_t(const fpd_t& fpd)
                : fpd_t(fpd)
                , m_object(NULL)
                , m_handle(NULL)
    {}

    ~fpd_proxy_t();

    void program(bool force = false) const override {
        m_object->program(force);
    }

    std::string running_version() const override {
        return m_object->running_version();
    }

    std::string packaged_version() const override {
        return m_object->packaged_version();
    }

    void activate() const override {
        return m_object->activate();
    }

    void erase() const override {
        if (is_golden_fpd()) {
            std::cout << name() << ": Erase for golden not supported" << std::endl;
        }
        return m_object->erase();
    }

    bool set_file_path(const std::string &ipath) override {
        return m_object->set_file_path(ipath);
    }

    void setup(pointer<fpd_t> parent);
private:
    fpd_t *m_object;                                   //!< Object of the library implemenation
    void *m_handle;                                    //!< Handle of the dlopen to be closed
};

//!
//! @brief Output a string representation of object to stream
//!
//! @param[in] os  The output stream
//! @param[in] fpd The object to output
//!
//! @returns the output stream
//!
std::ostream& operator<<(std::ostream& os, const fpd_t &fpd);

} // namespace bsp2

#endif // ndef BSP_FPD_H_
