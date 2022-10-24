#ifndef __SPIFLASH_UTIL_H__
#define __SPIFLASH_UTIL_H__
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Modes of SPI Flash supports
 */
typedef enum fpgalib_spi_mode_ {

  /* Set SPI Flash into 4 Byte Addressing access mode */
  FPGALIB_SPI_MODE_4BYTE_ADDRESS_SET,

  /* Exit SPI Flash from 4 Byte Addressing access mode */
  FPGALIB_SPI_MODE_4BYTE_ADDRESS_UNSET,

  /* Set SPI Flash into SPI Addressing access mode */
  FPGALIB_SPI_MODE_SET,

  /* Exit SPI Flash into SPI Addressing access mode */
  FPGALIB_SPI_MODE_UNSET,

} fpgalib_spi_mode_en;

/*
 * Common Flash Interface information that may be read from a flash memory
 * device. Software can query the installed device to determine
 * configurations, various electrical and timing parameters and functions
 * supported by the device.
 * CFI allows the vendor to specify a command set that should be used with the
 * component.
 */
typedef struct spi_cfi_ {

  /*
   * Page size that Flash can support to I/O, SPI FPGA block has limit to
   * 512 bytes.
   */
  uint16_t page_size;

  /* TODO:
   * Vendor can support different sector size within a Flash. we should add
   * functionality to address multiple types of sector for a flash.
   */

  /* Flash Sector size in Bytes. */
  uint32_t sector_size;

  /* Sector start address */
  uint32_t sector_start;

  /* Sector end address */
  uint32_t sector_end;

  /* Delay factor for operation to be completed in 100 micro seconds */
  uint8_t delay_factor;

  /* Whether required to verify the programmed data */
  uint8_t verify_flag;

  /* Whether flash supports Block erase */
  uint8_t support_erase;

  /* SPI Flash device instanaces */
  uint8_t dev_inst;

  /*
   * Retrieve the vendor specific delay factor for sleep between operations.
   */
  uint8_t spi_delay_factor;

  fpgalib_spi_mode_en mode;

  /* opcode for each operation */

  /* Read JEDEC ID Register */
  uint8_t jedec_id;

  /* Read Flash configuration */
  uint8_t read_cfg;

  /* SPI Flash Status Register */
  uint8_t status;

  /* SPI Flash Flag Status Register */
  uint8_t flag_status;

  /* read from Flash */
  uint8_t read;

  /* Block Erase */
  uint8_t erase;

  /* Block write */
  uint8_t program;

  /* Write enable */
  uint8_t write_enable;

  /* Write disable */
  uint8_t write_disable;

  /* extended address register */
  uint8_t rd_ear;

  /* extended address register */
  uint8_t wr_ear;

  /* Enter into 4-Byte Address Mode */
  uint8_t enter_4byte;

  /* Exit from 4-Byte Address Mode */
  uint8_t exit_4byte;

  /* Enter into SPI Mode */
  uint8_t enter_spi;

  /* Exit into SPI Mode */
  uint8_t exit_spi;

  uint8_t force_3byte;
} spi_cfi_t;

/*
 * Different operations SPI Flash can carry out
 */
typedef enum spiflash_oper_ {

  SPIFLASH_OPER_READ_JEDEC_ID, /* READ SPI Flash JEDEC */

  SPIFLASH_OPER_READ_MEM, /* READ SPI Flash Memory */

  SPIFLASH_OPER_PROGRAM_MEM, /* PROGRAM SPI Flash Memory */

  SPIFLASH_OPER_ERASE_MEM, /* ERASE SPI Flash Memory */

  SPIFLASH_OPER_MODE, /* SET SPI Flash Mode */

  SPIFLASH_OPER_REGISTER, /* REGISTER R/W on SPI Flash */

  SPIFLASH_OPER_MAX

} spiflash_oper_t;

/*
 * This structure is to hold the data to generate sjtag Controller operation
 * command and controll instruction for sjtag FPGA block
 */
typedef struct fpgalib_spi_cmd_ {

  /* sjtag Controller Opcode */
  uint8_t opcode;

  /* Read operation */
  uint8_t go_rd;

  /* Write operation */
  uint8_t go_wr;

  /* Set to enable write accesses the BP bits in the spi flash device */
  uint8_t wr_ena;

  /*
   * I/O Width of Address and Mode cycles associated with transaction.
   * Allowed values are 1, 2  or 4.
   * 2 - for Dual I/O Read (DIOR),
   * 4 - for Quad I/O Read (QIOR),
   * 1 - for rest
   */
  uint8_t addr_width;

  /*
   * I/O Width of data cycles associated with transaction.
   * Allowed values are 1, 2  or 4.
   * 2 - for Dual Output Read (DOR) and Dual I/O Read (DIOR),
   * 4 - for Quad Output Read (QOR) and Quad I/O Read (QIOR) and
   *     Quad Page Program (QPP),
   * 1 - for rest
   */
  uint8_t data_width;

  /* Number of dummy clock cycles associated with transaction. */
  uint8_t dummy_len;

  /*
   * Number of address bytes associated with transaction.
   * Allowed values are from 0 to 4.
   */
  uint8_t addr_len;

  /*
   * Number of command bytes associated with transaction.
   * Allowed values are 0 or 1.
   */
  uint8_t inst_len;

  /*
   * Number of mode bytes associated with transaction.
   * Allowed values are 0 or 1.
   */
  uint8_t mode_len;

} fpgalib_spi_cmd_t;

/*
 * List of SPI Flash command set
 */
/*
 * Note: These operations are extracted from Micron.
 *       In general, they are the same for all vendor;
 *       however, user is responsible to check and update in order to
 *       support other vendor.
 *       New operation will be catagorized and added to the right group
 *       for easy tracking.
 */

/* IDENTIFICATION Operations */
#define SPIFLASH_READ_SERIAL_FLASH 0x5A // Y Y Y 1 to ∞ 3
#define SPIFLASH_READ_ID_2 0x90         // READ_ID Not Micron cmd
#define SPIFLASH_READ_ID 0x9E
#define SPIFLASH_READ_ID_1 0x9F           // RDID
#define SPIFLASH_MULTIPLE_I_O_READID 0xAF // N Y Y 1 to 3 2

/* READ Operations */
#define SPIFLASH_READ 0x03      // READ
#define SPIFLASH_FAST_READ 0x0B // FAST_READ
#define SPIFLASH_DUAL_INPUT_OUTPUT_FAST_READ 0x0B
#define SPIFLASH_QUAD_INPUT_OUTPUT_FAST_READ 0x0B
#define SPIFLASH_4FAST_READ 0x0C            // 4FAST_READ new 4b addr
#define SPIFLASH_FAST_READ_DDRFR 0x0D       // DDRFR new
#define SPIFLASH_FAST_READ_4DDRFR 0x0E      // 4DDRFR new 4 bytes addr
#define SPIFLASH_4READ 0x13                 // 4READ new 4 bytes addr
#define SPIFLASH_DUAL_OUTPUT_FAST_READ 0x3B // DOR
#define SPIFLASH_DUAL_INPUT_OUTPUT_FAST_READ_1 0x3B
#define SPIFLASH_4DUAL_OUTPUT_FAST_READ 0x3C // 4DOR new
#define SPIFLASH_QUAD_OUTPUT_FAST_READ 0x6B  // QOR
#define SPIFLASH_QUAD_INPUT_OUTPUT_FAST_READ_1 0x6B
#define SPIFLASH_4QUAD_OUTPUT_FAST_READ 0x6C        // 4QOR new 4 bytes addr
#define SPIFLASH_DUAL_INPUT_OUTPUT_FAST_READ_2 0xBB // DIOR
#define SPIFLASH_DUAL_INPUT_OUTPUT_FAST_READ_3 0xBC // 4DIOR new 4 bytes addr
#define SPIFLASH_QUAD_INPUT_OUTPUT_FAST_READ_2 0xEB // Y N Y 5, 7

/* WRITE Operations */
#define SPIFLASH_WRITE_DISABLE 0x04 // WRDI
#define SPIFLASH_WRITE_ENABLE 0x06  // WREN

/* REGISTER Operations */
#define SPIFLASH_WRITE_STATUS_REGISTER 0x01      // WRR
#define SPIFLASH_READ_STATUS_REGISTER 0x05       // RDSR1
#define SPIFLASH_READ_STATUS2_REGISTER 0x07      // RDSR2 new
#define SPIFLASH_READ_ABRD_REGISTER 0x14         // ABRD new
#define SPIFLASH_WRITE_ABWR_REGISTER 0x15        // ABWR new
#define SPIFLASH_READ_BRRD_REGISTER 0x16         // BRRD new
#define SPIFLASH_WRITE_BRWR_REGISTER 0x17        // BRWR new
#define SPIFLASH_CLR_STATUS_REGISTER 0x30        // CLSR new
#define SPIFLASH_READ_CMD_RCR 0x35               // RDCR Not Micron cmd
#define SPIFLASH_CLEAR_FLAG_STATUS_REGISTER 0x50 // 0
#define SPIFLASH_WRITE_ENHANCED_VOLATILE_CONFIGURATION_REGISTER                \
  0x61 // Y Y Y 1 2, 8
#define SPIFLASH_READ_ENHANCED_VOLATILE_CONFIGURATION_REGISTER                 \
  0x65                                                      // Y Y Y 1 to ∞ 2
#define SPIFLASH_READ_FLAG_STATUS_REGISTER 0x70             // Y Y Y 1 to ∞ 2
#define SPIFLASH_WRITE_VOLATILE_CONFIGURATION_REGISTER 0x81 // 1 2, 8
#define SPIFLASH_READ_VOLATILE_CONFIGURATION_REGISTER 0x85  // Y Y Y 1 to ∞ 2
#define SPIFLASH_WRITE_NONVOLATILE_CONFIGURATION_REGISTER 0xB1 // 2, 8
#define SPIFLASH_READ_NONVOLATIL_CONFIGURATION_REGISTER 0xB5   // Y Y Y 2 2
#define SPIFLASH_ACCESS_BRAC_REGISTER 0xB9                     // BRAC new
#define SPIFLASH_WRITE_LOCK_REGISTER 0xE5                      // 1 4, 8
#define SPIFLASH_READ_LOCK_REGISTER 0xE8 // Y Y Y 1 to ∞ 4
#define SPIFLASH_READ_EXTENDED_ADDRESS_REGISTER 0xC8
#define SPIFLASH_WRITE_EXTENDED_ADDRESS_REGISTER 0xC5
#define SPIFLASH_EXIT_QSPI_MODE 0xF5

/* PROGRAM Operations */
#define SPIFLASH_PAGE_PROGRAM 0x02 // PP
#define SPIFLASH_EXTENDED_QUAD_INPUT_FAST_PROGRAM 0x02
#define SPIFLASH_EXTENDED_QUAD_INPUT_FAST_PROGRAM_2 0x12 // 4PP
#define SPIFLASH_QUAD_INPUT_FAST_PROGRAM 0x32            // QPP
#define SPIFLASH_EXTENDED_QUAD_INPUT_FAST_PROGRAM_1 0x32
#define SPIFLASH_4QUAD_INPUT_FAST_PROGRAM 0x34 // 4QPP new 4 bytes addr
#define SPIFLASH_QUAD_PAGE_PROGRAM 0x38        // QPP new
#define SPIFLASH_DUAL_INPUT_FAST_PROGRAM 0xA2  // Y Y N 1 to 256 4, 8
#define SPIFLASH_EXTENDED_DUAL_INPUT_FAST_PROGRAM_1 0xA2
#define SPIFLASH_EXTENDED_DUAL_INPUT_FAST_PROGRAM_2 0xD2 // Y Y N 4, 6, 8

/* ERASE Operations */
#define SPIFLASH_SUBSECTOR_ERASE 0x20  // P4E 4KB sector
#define SPIFLASH_SUBSECTOR_4ERASE 0x21 // 4P4E 4KB sector new
#define SPIFLASH_BULK_ERASE_1 0x60     // BE new
#define SPIFLASH_PROGRAM_ERASE_SUSPEND 0x75
#define SPIFLASH_PROGRAM_ERASE_RESUME 0x7A // Y Y Y 0 2, 8
#define SPIFLASH_BULK_ERASE 0xC7           // BE
#define SPIFLASH_SECTOR_ERASE 0xD8         // SE 64KB or 256 KB
#define SPIFLASH_4SECTOR_ERASE 0xDC // 4SE 64KB or 256 KB new 4 bytes addr

/* ONE-TIME PROGRAMMABLE (OTP) Operations */
#define SPIFLASH_PROGRAM_OTP_ARRAY 0x42 // OTPP
#define SPIFLASH_READ_OTP_ARRAY 0x4B    // OTPR

/* 4-BYTE Address Mode Operations */
#define SPIFLASH_ENTER_4BYTE_ADDRESS_MODE 0xB7 //
#define SPIFLASH_EXIT_4BYTE_ADDRESS_MODE 0xE9  //

/* Flash programming progress */
#define SPI_PROGRAM_ERASE_PERCENT 33
#define SPI_PROGRAM_DO_PERCENT 34
#define SPI_PROGRAM_VERIFY_PERCENT 33
/* ERASE+PROGRAM+VERIFY: 100% */

#define IOFPGA_SPI_MAX_SECTOR_SIZE 0x40000

#define SPI_PROGRAM_REPORT_INTERVAL 3 /* every 3% */

uint8_t spiflash_prepare_spi_command(spi_cfi_t *cfi, spiflash_oper_t oper,
                                     fpgalib_spi_cmd_t *cmd, char *err_msg,
                                     uint32_t msg_size);

char *spi_mode_str(fpgalib_spi_mode_en mode);

#endif /* __SPIFLASH_UTIL_H__ */
