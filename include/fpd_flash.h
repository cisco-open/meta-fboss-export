/**
 * @file fpd_flash.h
 *
 * @brief Definitions related to Flash FPDs utilities 
 *
 * @copyright Copyright (c) 2022 by Cisco Systems, Inc.
 *            All rights reserved.
 */
#ifndef FPD_FLASH_H_
#define FPD_FLASH_H_

//!
//! @brief Program FLASH FPD
//!
//! @param[in] path FPD image path
//!
extern "C" int program_iofpga(const char *image_path, uint32_t image_offset, uint32_t image_size,
                              uint32_t mdata_offset, uint32_t mdata_size, const char *uio_block_name);

//!
//! @brief Erase FLASH FPD
//!
//! @param[in] path FPD image path
//!
extern "C" int erase_iofpga(uint32_t image_offset, uint32_t image_size,
                            uint32_t mdata_offset, uint32_t mdata_size, const char *uio_block_name);

//!
//! @brief Get the running version of Flash FPD
//!
//! @returns Running version of the FPD
//!
extern "C" const char * get_iofpga_fpd_version(void);

//!
//! @brief checks if FW is booted with golden or primary
//!
//! @returns the reg value to check if its booted with golden
//!
extern "C" int is_golden_booted(const char *uio_block_name);

//!
//! @brief reads iofpga version from flash 
//!
//! @returns the major and minor version read from flash
//!
extern "C" uint16_t get_iofpga_version_from_flash(const char *block_name, uint32_t mdata_offset);

#endif // FPD_FLASH_H_
