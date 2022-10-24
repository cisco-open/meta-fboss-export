/**
 * @file object.h
 *
 * @brief Definitions related to the base objects
 *
 * @copyright Copyright (c) 2021-2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */

#ifndef BSP_OBJECT_H_
#define BSP_OBJECT_H_

#include <filesystem>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "bsp/fwd.h"
#include "bsp/oid.h"

//!
//! @brief Holds all public APIS
//!
namespace bsp2 {

//!
//! @brief The common base object
//!
class object_t {
public:
    //!
    //! @brief Construct a default object
    //!
    object_t() = default;
    object_t(const object_t&) = default;

    //!
    //! @brief Destroy the object
    //!
    virtual ~object_t() = default;

    //!
    //! @brief Get the parent OIDs
    //!
    //! @returns the parent OIDs
    //!
    virtual const std::vector<oid_t>& parents() const {
        return m_parents;
    }

    //!
    //! @brief Get the object identifier
    //!
    //! @returns the object identifier
    //!
    const oid_t &oid() const {
        return m_oid;
    }

    //!
    //! @brief Get the object type
    //!
    //! @returns the object type
    //!
    oid_t::type_t obj_type() const {
        return m_oid.obj_type();
    }

    //!
    //! @brief Get the object index (excludes object type)
    //!
    //! @returns the object index
    //!
    std::size_t index() const {
        return m_oid.index();
    }

    //!
    //! @brief Get the object name
    //!
    //! @returns the object name
    //!
    const std::string &name() const {
        return m_name;
    }

    //!
    //! @brief Get the list of aliases
    //!
    //! @returns the list of aliases
    //!
    const std::vector<std::string> &aliases() const {
        return m_aliases;
    }

    //!
    //! @brief Get the object description
    //!
    //! @returns the object description
    //!
    const std::string &description() const {
        return m_description;
    }

    //!
    //! @brief Determine if the object is present
    //!        In general, the object and its parent (recursively) must be present
    //!
    //! Objects should not be accessed if they are not present
    //!
    //! @returns true if the object is present
    //! @returns false if the object is not present
    //!
    virtual bool is_present() const;

    //!
    //! @brief Determine if the object is ok
    //!        The definition of ok is object type specific
    //!
    //! @returns true if the object is good
    //! @returns false if the object is not good
    //!
    virtual bool is_ok() const;

    //!
    //! @brief Convert the object to a string representation
    //!
    operator std::string () const;

    //!
    //! @brief Check if the name or alias matches the given string
    //!
    //! @param[in] given   The string to match
    //!
    //! @returns true if this object matches the given string, whether
    //!          through the name or an alias
    //! @returns false if there is no name match
    virtual bool matches_id(const std::string &given) const;

    static std::map<oid_t, object_p> db; //!< Database of objects
    static std::mutex db_m;              //!< Lock for database access
private:
    oid_t m_oid;                         //!< The object identifier
    std::string m_name;                  //!< The object name
    std::string m_description;           //!< The object description
protected:
    std::vector<oid_t> m_parents;        //!< The parent objects
    std::vector<std::string> m_aliases;  //!< The list of object aliases
    std::filesystem::path m_presence;    //!< Access to presence file
private:
    std::filesystem::path m_ok;          //!< Access to ok file

    //!
    //! @brief Convert object to json
    //!
    //! @param[out] j   The json representation of object
    //! @param[in]  obj The object to convert
    //!
    friend void to_json(json &, const object_t &);

    //!
    //! @brief Convert object from json
    //!
    //! @param[in]   j   The json representation of object
    //! @param[out]  obj The destination object
    //!
    friend void from_json(const json &, object_t &);
}; // class object_t

} // namespace bsp2

#endif // ndef BSP_OBJECT_H_
