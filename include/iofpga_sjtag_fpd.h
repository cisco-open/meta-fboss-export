#ifndef __IOFPGA_SJTAG_FPD_H__
#define __IOFPGA_SJTAG_FPD_H__

#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <endian.h>
#include "spiflash_util.h"
#include "commonUtil.h"


#define MAX_FPGA_FLASH_OBJ_NAME_STR_LEN         28
#define MAX_FPGA_FLASH_OBJ_VERSION_STR_LEN      16
#define MAX_FPGA_FLASH_OBJ_BUILD_TIME_STR_LEN   24
#define MAX_FPGA_FLASH_OBJ_BUILD_USER_STR_LEN   12

#define MAX_FPD_NAME_LEN        17
#define MAX_FPD_VER_STR_LEN     16
#define MAX_BUILD_TIME_STR_LEN  24
#define MAX_BUILD_USER_STR_LEN  12
#define MAX_MD5_DIGEST              16

typedef enum spiflash_model_e_ {

    SPIFLASH_MODEL_UNKNOWN = 0,

    SPIFLASH_MODEL_MICRON_MT25QL256ABA8ESF_0SIT,

    SPIFLASH_MODEL_SPANSION_S25FL256S_3BYTE_64KB_SECTOR,

    SPIFLASH_MODEL_SPANSION_S25FL256S_3BYTE_256KB_SECTOR,

    SPIFLASH_MODEL_SPANSION_S25FL512S_3BYTE_256KB_SECTOR,

    SPIFLASH_MODEL_MACRONIX_MX25L12835FMI_10G,

    SPIFLASH_MODEL_MAX,
} spiflash_model_en;

typedef enum spiflash_ip_block_type_ {

  SPIFLASH_IP_BLOCK_QSPI = 0,

  SPIFLASH_IP_BLOCK_SJTAG = 1,

  SPIFLASH_IP_BLOCK_ALL = 0xFF,

} spiflash_ip_block_type_en;

typedef struct spiflash_sector_ {

  uint32_t size;

  uint32_t start;

  uint32_t end;

} spiflash_sector_t;

typedef struct spiflash_cfg_ {

#define SPIFLASH_VENDOR_NAME_MAX 63
  char vendor_name[SPIFLASH_VENDOR_NAME_MAX + 1];

  uint8_t manuf_id;

  uint8_t memory_type;

  uint8_t memory_density;

  uint8_t vendor_id0;

  uint8_t vendor_id1;

  uint16_t page_size;

#define SPIFLASH_NUM_SECTORS 4
  uint8_t num_sectors;

  spiflash_sector_t sector[SPIFLASH_NUM_SECTORS];

  uint32_t capacity;

  uint8_t verify_flag;

  uint8_t support_erase;

  uint8_t spi_delay_factor;

  /* opcode */
  uint8_t jedec_id;

  uint8_t read_cfg;

  uint8_t status;

  uint8_t flag_status;

  uint8_t read;

  uint8_t erase;

  uint8_t program;

  uint8_t write_enable;

  uint8_t write_disable;

  uint8_t rd_ear;

  uint8_t wr_ear;

  uint8_t enter_4byte;

  uint8_t exit_4byte;

  uint8_t enter_spi;

  uint8_t exit_spi;

  spiflash_ip_block_type_en ip_block;

  uint8_t force_3byte;
} spiflash_cfg_t;

struct regblk_hdr {
  uint32_t info0;                        /* 0x00 - 0x04 */
  uint32_t info1;                        /* 0x04 - 0x08 */
  uint32_t sw0;                          /* 0x08 - 0x0c */
  uint32_t sw1;                          /* 0x0c - 0x10 */
  uint32_t magicNo;                      /* 0x10 - 0x14 */
};

struct sj_spi_csrs {
    volatile uint32_t fpga_spi_control;          /*        0x0 - 0x4        */
    volatile uint32_t fpga_spi_status;           /*        0x4 - 0x8        */
    volatile uint32_t fpga_spi_rdsize;           /*        0x8 - 0xc        */
    volatile uint32_t fpga_spi_data;             /*        0xc - 0x10       */
    volatile uint32_t fpga_spi_addr_op;          /*       0x10 - 0x14       */
};

struct sj_mb_csrs {
    volatile uint32_t mbctrl;                    /*        0x0 - 0x4        */
    volatile uint32_t mbcache1;                  /*        0x4 - 0x8        */
    volatile uint32_t mbcache2;                  /*        0x8 - 0xc        */
    volatile uint32_t mbcache3;                  /*        0xc - 0x10       */
    volatile uint32_t mbsthist;                  /*       0x10 - 0x14       */
    volatile uint32_t mbsbhist;                  /*       0x14 - 0x18       */
};

struct sjtag_rf_v4 {
    struct regblk_hdr hdr;                       /*        0x0 - 0x14       */
    volatile uint32_t fit;                       /*       0x14 - 0x18       */
    volatile char pad__0[0x8];                   /*       0x18 - 0x20       */
    volatile uint32_t conf;                      /*       0x20 - 0x24       */
    volatile uint32_t stat;                      /*       0x24 - 0x28       */
    volatile uint32_t integrity_stat;            /*       0x28 - 0x2c       */
    volatile uint32_t public_key_stat;           /*       0x2c - 0x30       */
    struct sj_spi_csrs cfgspi_reg;               /*       0x30 - 0x44       */
    volatile char pad__1[0xc];                   /*       0x44 - 0x50       */
    struct sj_mb_csrs mb_sts_reg;                /*       0x50 - 0x68       */
    volatile char pad__2[0x8];                   /*       0x68 - 0x70       */
    volatile uint32_t ecid[16384];               /*       0x70 - 0x10070    */
};

typedef struct sjtag_rf_v4 fpgalib_sjtag_regs_t;

typedef struct sj_spi_csrs fpgalib_sjtag_cfgspi_reg_t;

#endif // __IOFPGA_SJTAG_FPD_H__
