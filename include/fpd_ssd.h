/**
 * @file fpd_ssd.h
 *
 * @brief Definitions related to SSD FPD utilities 
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */
#ifndef FPD_SSD_H_
#define FPD_SSD_H_


//!
//! @brief Upgrade SSD 
//!
//! @returns Status if FPD upgrade is successful or not 
//!
extern "C" uint32_t ssd_fpd_upgrade(void);

//!
//! @brief Get the type of SSD 
//!
//! @returns type of SSD as a string 
//!
extern "C" char * fpd_get_ssd_str(void);

//!
//! @brief Get the running version of SSD 
//!
//! @returns running version of SSD 
//!
extern "C" uint32_t get_ssd_fpd_version(void);


#endif // FPD_SSD_H_
