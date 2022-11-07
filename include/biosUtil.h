/*------------------------------------------------------------------
 * biosUtil.h 
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 *-----------------------------------------------------------------
 */

#ifndef __BIOSUTIL_H__
#define __BIOSUTIL_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BIOS_VER_SIGNATURE      "$BVDT$"
#define BIOS_VER_INFO_FILE      "/tmp/bios_ver_info.txt"
#define BIOS_MTD_INFO_FILE      "/tmp/mtd_bios.txt"
#define BIOS_VER_LEN            18
#define BIOS_VER_BVDT_LEN       6
#define BIOS_SIGNATURE_OFFSET   256
#define BIOS_VERSION_OFFSET     270

#define BIOS_BOOT_STATUS_REG_OFFSET        0x38
#define BIOS_X86_SPI_CTLSTAT_REG_OFFSET    0x28

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * @brief  Api to check if bios is booted from golden
 * @return Return 0 if booted from primary, 
 *                1 if booted from golden
 *               -1 if errored
 */
int bios_is_golden_booted(const char *block_name);

/*
 * @brief  Api to switch flash bios region
 * @return Return 0 if successful,
 *               -1 if errored
 */
int fpd_bios_switch_flash(const char *block_name);

/*
 * @brief  Api to get bios active flash region
 * @return Return 0 if primary is active
 *                1 if golden is active
 *               -1 if errored
 */
int fpd_bios_get_active_flash(const char *block_name);

#ifdef __cplusplus
}
#endif

#endif // __BIOSUTIL_H__
