/**
 * @file idprom.h
 *
 * @brief Definitions related to IDPROM access
 *
 * @copyright Copyright (c) 2021-2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */
#ifndef BSP_IDPROM_H_
#define BSP_IDPROM_H_

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <bsp/fwd.h>
#include <bsp/object.h>

//!
//! @brief Holds all public APIS
//!
namespace bsp2 {

//!
//! @brief The idprom object
//!
class idprom_t : public object_t
{
public:
    //!
    //! @brief Construction / destruction
    //!
    idprom_t();
    idprom_t(const idprom_t&);
    virtual ~idprom_t() = default;

    //!
    //! @brief Factory method to simplify applications
    //!
    //! @param[in] id  The identifier for the idprom
    //! @param[in] t   The identifier type
    //!
    //! @returns A pointer to the new object
    //!
    static pointer<idprom_t> factory(const std::string &id);

    //!
    //! @brief Read the idprom
    //!
    //! @param[in] refresh If true, read from the device
    //!                    rather than returning any previously
    //!                    cached data.
    //!
    //! @returns a dictionary of key, value-list
    //!
    const std::map<std::string,std::vector<std::string>> &read(bool refresh=false);

    //!
    //! @brief Return the pathname associated with the idprom
    //!
    //! @param[in] resolved  Choose resolved or unresolved pathname
    //!
    //! @returns resolved pathname if resolve parameter is true
    //! @returns unresolved pathname if resolve parameter is false
    //!
    const std::filesystem::path path(bool resolved = false) const;

    //!
    //! @brief Return the numeric tag associated with an ascii tag name
    //!
    //! @param[in] code  The ascii tag name to process
    //!
    //! @returns the numeric tag matching the code
    //! @returns 0 if there is no matching code
    //!
    static size_t code_map(const std::string &code);

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
    //! @brief Check if the name or alias matches the given string
    //!
    //! @param[in] given   The string to match
    //!
    //! @returns true if this object matches the given string, whether
    //!          through the name or an alias
    //! @returns false if there is no name match
    bool matches_id(const std::string &given) const override;

private:
    typedef std::map<std::string,std::string> tag_value_t;


    size_t m_size;                              //!< The maximum size to read (0 is no limit)
    size_t m_offset;                            //!< The starting offset
    std::string m_format;                       //!< The format of the idprom data
    std::filesystem::path m_path;               //!< The path to the idprom

    std::map<std::string,tag_value_t> m_fallback; //!< Map of fallback content
    std::string m_fallback_algorithm;           //!< How to decide fallback
    std::filesystem::path m_fallback_status;    //!< Access to w1 status file
    std::filesystem::path m_fallback_presence;  //!< Access to w1 presence file

    class tlv;
    class descriptor;

    const size_t TLV_IDPROM_HEADER_SIZE =  4;

    std::map<std::string,std::vector<std::string>> m_decoded; //!< Data that has been read/decoded
    std::mutex m_lock;                          //!< Critical section protection

    //!
    //! @brief Read, parse/decode idprom
    //!
    //! @param[in] info  A descriptor providing access to the underlying idprom
    //!
    void parse(descriptor &info);

    //!
    //! @brief Parse according to datacenter format
    //!
    //! @param[in] info A descriptor providing access to the underlying idprom
    //!
    void dc_parse(descriptor &info);

    //!
    //! @brief Parse according to Cisco TLV format
    //!
    //! @param[in] info A descriptor providing access to the underlying idprom
    //!
    void tlv_parse(descriptor &info);

    //!
    //! @brief Fallback to pre-configured values
    //!
    void fallback();

    //!
    //! @brief Utility to save a value_pair into the given key
    //!
    //! @param[in] k          The key (tag) that the value should be stored against
    //! @param[in] value_pair A pair where the first is an indication of whether second is valid
    //!
    //! @throws eofException at end of idprom
    //!
    template<class C>
    void dc_set(const char *k, const C &value_pair);

    //!
    //! @brief Determine if the fallback routine indicates presence
    //!
    //! @returns true if the fallback method detects presence
    //! @returns false if the fallback method does not detect presencde
    //!
    bool fallback_present() const;

    //!
    //! @brief Encapsulate a computed field
    //!
    class cfield_t {
    public:
        std::string m_from;                             //!< Source field
        std::vector<size_t> m_bits;                     //!< Bits to extract
        std::string m_default;                          //!< Default if not source field
        std::map<std::string, std::string> m_values;    //!< Content mapping
    };

    //!
    //! @brief A map of computed fields
    //!
    std::map<std::string, cfield_t> m_computed;

    //!
    //! @brief Process the computed fields
    //!
    void add_computed_fields();

    //!
    //! @brief Add manufacturing location and date
    //!        (computed from serial number)
    //!
    //! @params[in] tag    The source serial number tag to use
    //! @params[in] prefix The prefix to apply to the newly
    //!                    created tags
    //!
    //! Updates m_decoded based on calculations
    //!
    void decode_serial_no(const std::string &tag,
                          const std::string &prefix);

    //!
    //! @brief Convert object to json
    //!
    //! @param[out] j   The json representation of object
    //! @param[in]  obj The object to convert
    //!
    friend void to_json(json &, const idprom_t &);
    friend void to_json(json &, const cfield_t &);

    //!
    //! @brief Convert object from json
    //!
    //! @param[in]   j   The json representation of object
    //! @param[out]  obj The destination object
    //!
    friend void from_json(const json &, idprom_t &);
    friend void from_json(const json &, cfield_t &);
};

} // namespace bsp2

#endif // ndef BSP_IDPROM_H_
