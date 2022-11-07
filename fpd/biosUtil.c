/*------------------------------------------------------------------
 * biosUtil.c 
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 *-----------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <errno.h>
#include "biosUtil.h"


void * 
mmap_bios_block(const char *block_name) {
#ifdef UIO_SUPPORTED
    struct uio_info_t *device_info = NULL;
    void *map_base = NULL;

    device_info = uio_find_devices_by_name(block_name);
    if (!device_info) {
        fprintf(stderr, "failed to find uio device. name: %s\n", block_name);
        return NULL;
    }

    map_base = uio_mmap(device_info, 0);
    if (!map_base) {
        fprintf(stderr, "uio mmap failed. block_name: %s\n", block_name);
        return NULL;
    }
    return map_base;
#else
	return NULL;
#endif //UIO_SUPPORTED
}

static void
munmap_bios_block(void *addr, const char *block_name) {
#ifdef UIO_SUPPORTED
    struct uio_info_t *device_info = NULL;

    device_info = uio_find_devices_by_name(block_name);
    if (!device_info) {
        fprintf(stderr, "failed to find uio device. name: %s\n", block_name);
        return;
    }

    uio_munmap(addr, device_info->maps[0].size);
#endif //UIO_SUPPORTED
}

int
bios_pci_util_write(uint32_t target, uint32_t data,
               const char *block_name) {
    uint32_t *virt_addr;
    void *map_base = NULL;

    map_base = mmap_bios_block(block_name);
    if (!map_base) {
        fprintf(stderr, "failed to mmap block %s\n", block_name);
        return -1;
    }
    virt_addr = map_base + target;
    *virt_addr = data;

    munmap_bios_block(map_base, block_name);
    return 0;
}

int 
bios_pci_util_read(uint32_t target, uint32_t *data,
              const char *block_name) {
    void *virt_addr;
    void *map_base = NULL;
    
    map_base = mmap_bios_block(block_name);
    if (!map_base) {
        fprintf(stderr, "failed to mmap block %s\n", block_name);
        return -1;
    }

    if (data) {
        virt_addr = map_base + target;
        *data = *((uint32_t *)virt_addr);
    } else {
        fprintf(stderr, "invalid parameter. data is NULL\n");
        return -1;
    }
    return 0;
}

int bios_is_golden_booted(const char *block_name) {

    int rc = 0;
    uint32_t data = 0;
    uint32_t target = BIOS_BOOT_STATUS_REG_OFFSET;
     
    // Read x86 Status Register
    rc = bios_pci_util_read(target, &data, block_name);
    if (rc != 0) {
        fprintf(stderr, "Unable to read register\n");
        return -1;
    }
    
    return (data >> 16) & 1UL;
}

int fpd_bios_switch_flash(const char *block_name) {
    int rc = 0;
    uint32_t data;
    uint32_t target = BIOS_X86_SPI_CTLSTAT_REG_OFFSET;

    rc = bios_pci_util_read(target, &data, block_name);
    if (rc != 0) {
        fprintf(stderr, "Failed to read register");
        return -1;
    }

    data = data | 0x1UL;
    rc = bios_pci_util_write(target, data, block_name);
    if (rc != 0) {
        fprintf(stderr, "Failed to write to register");
        return -1;
    }
    return 0;
}


int fpd_bios_get_active_flash(const char *block_name) {
    int rc = 0;
    uint32_t data;
    uint32_t target = BIOS_X86_SPI_CTLSTAT_REG_OFFSET;

    rc = bios_pci_util_read(target, &data, block_name);
    if (rc != 0) {
        fprintf(stderr, "Failed to read register");
        return -1;
    }

    data = (data >> 1) & 0x1UL;
    return data;
}

