/**
 * @file oid.h
 *
 * @brief Definitions related to object identifiers
 *
 * @copyright Copyright (c) 2021-2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */
#ifndef BSP_OID_H_
#define BSP_OID_H_

#include "bsp/fwd.h"

//!
//! @brief Holds all public APIS
//!
namespace bsp2 {

//!
//! @brief The object identifier
//!
class oid_t {
public:
    //!
    //! @brief The object type
    //!
    enum type_t {
        unspecified,
        //
        // START OF CSBSP COMPATIBILITY
        //
        // These definitions are synchronized to csbsp.h
        //
        chassis,
        module,
        thermal,
        fan,
        psu,
        led,
        sfp,
        watchdog,
        fpd,
        voltage,
        current,
        idprom,
        np,
        //
        // END OF CSBSP COMPATIBILITY
        //
        power,
        fan_tray,
        psu_tray,
        platform,
        pim,
        gpio_expander,
        misc_data,
        fru,
        indicator,
        _last,   // Should come last!
    };
    typedef std::size_t value_type;

    oid_t();

    //!
    //! @brief Constructor
    //!
    //! @param[in] t     The object type
    //! @param[in] index The object index
    //!
    oid_t(type_t t, value_type index);

    //!
    //! @brief Constructor
    //!
    //! @param[in] v  The object identifier (both type and index)
    //!
    oid_t(size_t v);

    //!
    //! @brief Get the object type
    //!
    //! @returns the object type
    //!
    type_t obj_type() const { return m_type; }

    //!
    //! @brief Get the object index
    //!
    //! @returns the object index
    //!
    value_type index() const { return m_index; };

    //!
    //! @brief prefix increment operator
    //!
    oid_t &operator++() { ++m_index; return *this; };

    //!
    //! @brief postfix increment operator
    //!
    oid_t operator++(int) { oid_t m(*this); ++m_index; return m; };

    //!
    //! @brief compare to another oid
    //!
    //! @param[in] o  The object to compare against
    //!
    //! @returns true if the current object is <= the passed object
    //!
    bool operator<=(const oid_t &o) const {
        return (m_type < o.obj_type()) ||
              (m_type == o.obj_type() && m_index <= o.index());
    }

    //!
    //! @brief compare to another oid
    //!
    //! @param[in] o  The object to compare against
    //!
    //! @returns true if the current object is > the passed object
    //!
    bool operator>(const oid_t &o) const {
        return (m_type > o.obj_type()) ||
              (m_type == o.obj_type() && m_index > o.index());
    }

    //!
    //! @brief compare to another oid
    //!
    //! @param[in] o The object to compare against
    //!
    //! @returns true if the object ids are equal
    //!
    bool operator==(const oid_t &o) const {
        return m_type == o.obj_type() && m_index == o.index();
    }

    //!
    //! @brief Convert to string representation of object
    //!
    operator std::string() const;


    //!
    //! @brief compare to another oid
    //!
    //! @param[in] o The object to compare against
    //!
    //! @returns true if the object id is less than the passed object
    //!
    bool operator<(const bsp2::oid_t &obj) const
    {
        return (m_type < obj.obj_type()) ||
               ((m_type  == obj.obj_type()) &&
                (m_index < obj.index()));
    }
private:
    type_t m_type;              //!< the object type
    value_type m_index;         //!< the object index

    //!
    //! @brief Convert object to json
    //!
    //! @param[out] j   The json representation of object
    //! @param[in]  obj The object to convert
    //!
    friend void to_json(json &, const oid_t &);

    //!
    //! @brief Convert object from json
    //!
    //! @param[in]   j   The json representation of object
    //! @param[out]  obj The destination object
    //!
    friend void from_json(const json &, oid_t &);
}; // class oid_t

} // namespace bsp2

#endif // ndef BSP_OID_H_
