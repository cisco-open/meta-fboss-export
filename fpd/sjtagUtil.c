#include "iofpga_sjtag_fpd.h"
#include "spiflash_util.h"
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <dlfcn.h>

#define DEVMEM "/dev/mem"
#define SJTAG_BLOCK_OFFSET 0x0 //0x62000
#define IOFPGA_SJTAG_TRANS_TIMEOUT 1000
#define IOFPGA_SJTAG_READY_TIMEOUT 10000

#define IOFPGA_SJTAG_RDATA_SIZE 2048
#define IOFPGA_SJTAG_WDATA_SIZE 2048
#define IOFPGA_SJTAG_PAGE_SIZE 2048
#define IOFPGA_SJTAG_MAX_DEV 2
#define FPD_VER_STR_LEN 12

#define SPIFLASH_FSR_READY 0x80 /* Ready to accept transactions */
#define SPIFLASH_SR_WIP 0x01    /* Write in progress */

#define IOFPGA_MDATA_SIZE 148
#define IOFPGA_MDATA_FPD_VERSION_OFFSET 15

#define SJTAG_BANK_DUMP 1

#define BUF_SIZE 4096
#define USER_SPI_ADDR_START 0x0
#define USE_SPI_ADDR_END 0x200000
#define SIZE_64K 0x10000
#define SIZE_4K 0x1000
#define MASK_4K 0xFFFFF000

#define SPI_DIR_TABLE_START 0x0
#define SPI_DIR_TABLE_SIZE 12
#define SPI_DIR_FPGA_VER_OFFSET 10
#define SPI_DIR_FPGA_VER_SIZE 2
#define SPIDIR_MAX_BLOCK_SIZE 4096

#define BITSTREAM_BUF_SIZE 0x3F0000

#define GOLDEN_IMAGE 1
#define UPGRADE_IMAGE 2

#define MAX_FPGA_FLASH_OBJ_NAME_STR_LEN 28
#define MAX_FPGA_FLASH_OBJ_VERSION_STR_LEN 16
#define MAX_FPGA_FLASH_OBJ_BUILD_TIME_STR_LEN 24
#define MAX_FPGA_FLASH_OBJ_BUILD_USER_STR_LEN 12

#define AIKIDO_SPIDIR_UPDATE_IMG_ADDR_OFFSET 6
#define AIKIDO_SPIDIR_UPDATE_IMG_VER_OFFSET 10
#define PCI_ADDRESS 0x0 //0x92000000
#define ERRBUF_SIZE 300

#define SJ_SPI_CSRS__FPGA_SPI_STATUS_REG__BUSY__READ(src)                      \
  (((uint32_t)(src)&0x00004000U) >> 14)

#define SJ_SPI_CSRS__FPGA_SPI_ADDR_OP_REG__ADDRESS__MASK 0x00ffffffU
#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__ADDR_MSB__MASK 0xff000000U
#define SJ_SPI_CSRS__FPGA_SPI_ADDR_OP_REG__OPCODE__SHIFT 24

#define SJ_SPI_CSRS__FPGA_SPI_STATUS_REG__DONE__READ(src)                      \
  (((uint32_t)(src)&0x00008000U) >> 15)

#define SJ_SPI_CSRS__FPGA_SPI_RDSIZE_REG__RDSIZE__MODIFY(dst, src)             \
  (dst) = ((dst) & ~0x00000fffU) | ((uint32_t)(src)&0x00000fffU)

#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__USE_DUMMY__MODIFY(dst, src)         \
  (dst) = ((dst) & ~0x00000004U) | (((uint32_t)(src) << 2) & 0x00000004U)

#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__FIFO32__MODIFY(dst, src)            \
  (dst) = ((dst) & ~0x00000800U) | (((uint32_t)(src) << 11) & 0x00000800U)

#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__USE_OPCODE__MODIFY(dst, src)        \
  (dst) = ((dst) & ~0x00000008U) | (((uint32_t)(src) << 3) & 0x00000008U)

#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__DATA_DIR__MODIFY(dst, src)          \
  (dst) = ((dst) & ~0x00000002U) | (((uint32_t)(src) << 1) & 0x00000002U)

#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__ADDR_MSB__SHIFT 24

#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__ADDR_SIZE__MODIFY(dst, src)         \
  (dst) = ((dst) & ~0x00000300U) | (((uint32_t)(src) << 8) & 0x00000300U)

#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__USE_ADDR__MODIFY(dst, src)          \
  (dst) = ((dst) & ~0x00000001U) | ((uint32_t)(src)&0x00000001U)

#define SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__CPUWR__MODIFY(dst, src)             \
  (dst) = ((dst) & ~0x00000400U) | (((uint32_t)(src) << 10) & 0x00000400U)

#define FPRINTF(out, ...)
//    fprintf(out, __VA_ARGS__)

#define PRINT_ERROR                                                            \
  do {                                                                         \
    FPRINTF(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,         \
            __FILE__, errno, strerror(errno));                                 \
    return;                                                                    \
  } while (0)

#define SLPC_MEMORY_BASE            0x401000
#define SLPC_MEMORY_OFFSET          0x40100
#define PINPOINTER_SPI_BLOCK_ADDR   0x32000

int fd;
spi_cfi_t cfi; // spi config

static uint8_t sjtag_read_fifo(uint8_t *data, uint16_t data_len, char *err_msg,
                               uint32_t msg_size);
void sjtag_dump_data(char *client, uint32_t addr, uint8_t *data, int32_t len);
uint8_t sjtag_wait_till_wip(spi_cfi_t *cfi, uint8_t delay_factor, char *err_msg,
                            uint32_t msg_size);
void sjtag_segment_range(uint32_t target_addr, uint32_t target_len,
                         uint32_t seg_size, uint32_t *start_seg,
                         uint32_t *end_seg);
void* map_base = NULL;

spiflash_cfg_t spiflash_models_cfg[SPIFLASH_MODEL_MAX] = {
    /* SPIFLASH_MODEL_UNKNOWN */
    {
        .vendor_name = "Unknown SPI Flash",

        .spi_delay_factor = 1,

        .jedec_id = 0x9F,

        .status = 0x05,

        .flag_status = 0x0,

    },
    /* SPIFLASH MT25QL256ABA8ESF-0SIT */
    {
        .vendor_name = "MICRON MT25QL256ABA8ESF-0SIT",

        .manuf_id = 0x20,

        .memory_type = 0xBA,

        .memory_density = 0x19,

        .vendor_id0 = 0xFF,

        .vendor_id1 = 0xFF,

        .page_size = 256,

        .num_sectors = 1,

        .sector[0] =
            {

                .size = 64 * 1024,

                .start = 0,

                .end = 512,

            },

        .capacity = 32 * 1024 * 1024,

        .verify_flag = 1,

        .support_erase = 1,

        .spi_delay_factor = 1,

        .jedec_id = 0x9F,

        .read_cfg = 0xB5,

        .status = 0x05,

        .flag_status = 0x70,

        .read = 0x03,

        .erase = 0xD8,

        .program = 0x02,

        .write_enable = 0x06,

        .write_disable = 0x04,

        .rd_ear = 0x0,

        .wr_ear = 0x0,

        .enter_4byte = 0xB7,

        .exit_4byte = 0xE9,

        .enter_spi = 0x0,

        .exit_spi = 0x0,

        .ip_block = SPIFLASH_IP_BLOCK_SJTAG,

        .force_3byte = 1,
    },
    /* SPANSION S25FL256SAGMFI00 3 Byte 64KB sectors */
    {
        .vendor_name = "SPANSION S25FL256SAGMFI00 3 Byte 64KB sectors",

        .manuf_id = 0x01,

        .memory_type = 0x02,

        .memory_density = 0x19,

        .vendor_id0 = 0x4d,

        .vendor_id1 = 0x01,

        .page_size = 256,

        .num_sectors = 1,

        .sector[0] =
            {

                .size = 64 * 1024,

                .start = 0,

                .end = 512,

            },

        .capacity = 32 * 1024 * 1024,

        .verify_flag = 1,

        .support_erase = 1,

        .spi_delay_factor = 1,

        .jedec_id = 0x9F,

        .read_cfg = 0xB5,

        .status = 0x05,

        .flag_status = 0x0,

        .read = 0x03,

        .erase = 0xD8,

        .program = 0x02,

        .write_enable = 0x06,

        .write_disable = 0x04,

        .rd_ear = 0x16,

        .wr_ear = 0x17,

        .enter_4byte = 0x0,

        .exit_4byte = 0x0,

        .enter_spi = 0x0,

        .exit_spi = 0x0,

        .ip_block = SPIFLASH_IP_BLOCK_SJTAG,

        .force_3byte = 1,
    },

    /* SPANSION S25FL256SAGMFI00 3 Byte 256KB Sectors */
    {
        .vendor_name = "SPANSION S25FL256SAGMFI00 3 Byte 256KB Sectors",

        .manuf_id = 0x01,

        .memory_type = 0x02,

        .memory_density = 0x19,

        .vendor_id0 = 0x4d,

        .vendor_id1 = 0x00,

        .page_size = 256,

        .num_sectors = 1,

        .sector[0] =
            {

                .size = 64 * 1024 * 4,

                .start = 0,

                .end = 128,

            },

        .capacity = 32 * 1024 * 1024,

        .verify_flag = 1,

        .support_erase = 1,

        .spi_delay_factor = 1,

        .jedec_id = 0x9F,

        .read_cfg = 0xB5,

        .status = 0x05,

        .flag_status = 0x0,

        .read = 0x03,

        .erase = 0xD8,

        .program = 0x02,

        .write_enable = 0x06,

        .write_disable = 0x04,

        .rd_ear = 0x16,

        .wr_ear = 0x17,

        .enter_4byte = 0x0,

        .exit_4byte = 0x0,

        .enter_spi = 0x0,

        .exit_spi = 0x0,

        .ip_block = SPIFLASH_IP_BLOCK_SJTAG,

        .force_3byte = 1,
    },

    /* SPANSION S25FL512SAGMFIR10 3 Byte 256KB Sectors */
    {
        .vendor_name = "SPANSION S25FL512SAGMFIR10 3 Byte 256KB Sectors",

        .manuf_id = 0x01,

        .memory_type = 0x02,

        .memory_density = 0x20,

        .vendor_id0 = 0x4d,

        .vendor_id1 = 0x00,

        .page_size = 512,

        .num_sectors = 1,

        .sector[0] =
            {

                .size = 64 * 1024 * 4,

                .start = 0,

                .end = 128,

            },

        .capacity = 32 * 1024 * 1024,

        .verify_flag = 1,

        .support_erase = 1,

        .spi_delay_factor = 1,

        .jedec_id = 0x9F,

        .read_cfg = 0xB5,

        .status = 0x05,

        .flag_status = 0x0,

        .read = 0x03,

        .erase = 0xD8,

        .program = 0x02,

        .write_enable = 0x06,

        .write_disable = 0x04,

        .rd_ear = 0x16,

        .wr_ear = 0x17,

        .enter_4byte = 0x0,

        .exit_4byte = 0x0,

        .enter_spi = 0x0,

        .exit_spi = 0x0,

        .ip_block = SPIFLASH_IP_BLOCK_SJTAG,

        .force_3byte = 1,
    },

    /* SPIFLASH MACRONIX MX25L12835FMI-10G */
    {
        .vendor_name        = "MACRONIX MX25L12835FMI-10G",

        .manuf_id           = 0xC2,

        .memory_type        = 0x20,

        .memory_density     = 0x18,

        .vendor_id0         = 0xFF,

        .vendor_id1         = 0xFF,

        .page_size          = 256,

        .num_sectors        = 1,

        .sector[0]          = {

            .size           = 64 * 1024,

            .start          = 0,

            .end            = 512,

        },

        .capacity           = 32 * 1024 * 1024,

        .verify_flag        = 1,

        .support_erase      = 1,

        .spi_delay_factor   = 1,

        .jedec_id           = 0x9F,

        .read_cfg           = 0xB5,

        .status             = 0x05,

        .flag_status        = 0x0,

        .read               = 0x03,

        .erase              = 0xD8,

        .program            = 0x02,

        .write_enable       = 0x06,

        .write_disable      = 0x04,

        .rd_ear             = 0x0,

        .wr_ear             = 0x0,

        .enter_4byte        = 0,

        .exit_4byte         = 0,

        .enter_spi          = 0x0,

        .exit_spi           = 0x0,

        .ip_block           = SPIFLASH_IP_BLOCK_SJTAG,

        .force_3byte        = 1,
    },

};

/*
 * Returns SPI Flash operation string constant to print
 */
static inline char *spiflash_oper_str(spiflash_oper_t oper) {
  switch (oper) {

  case SPIFLASH_OPER_READ_JEDEC_ID:
    return "Read JEDEC ID";

  case SPIFLASH_OPER_READ_MEM:
    return "Read Memory";

  case SPIFLASH_OPER_PROGRAM_MEM:
    return "Program Memory";

  case SPIFLASH_OPER_ERASE_MEM:
    return "Erase Memory";

  case SPIFLASH_OPER_MODE:
    return "Flash Mode";

  case SPIFLASH_OPER_REGISTER:
    return "Flash Register";

  default:
    return "Invalid SPI Flash Operation";
  }
}

/*
 * Prepares SPI FPGA block command for a given oper SPI Flash operation.
 * Known opcodes will be used if user doesn't provide opcode in the command
 */
uint8_t spiflash_prepare_spi_command(spi_cfi_t *cfi, spiflash_oper_t oper,
                                     fpgalib_spi_cmd_t *cmd, char *err_msg,
                                     uint32_t msg_size) {
  switch (oper) {

  /* Flash Vendor IDENTIFICATION Operations */
  case SPIFLASH_OPER_READ_JEDEC_ID:

    if (!cmd->opcode) {
      cmd->opcode = cfi->jedec_id;
    }

    switch (cmd->opcode) {

    case SPIFLASH_READ_ID:
    case SPIFLASH_READ_ID_1:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 0;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_READ_ID_2:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 3;
      cmd->addr_len = 0;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    default:
      break;
    }

    break;

  /* Flash memory READ Operations */
  case SPIFLASH_OPER_READ_MEM:

    if (!cmd->opcode) {
      cmd->opcode = cfi->read;
    }

    switch (cmd->opcode) {

    case SPIFLASH_READ:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 4;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_FAST_READ:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 1; /* TODO: HW_INPUT */
      cmd->addr_len = 4;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_4FAST_READ:
    case SPIFLASH_4READ:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 8;
      cmd->addr_len = 4; /* TODO: HW_INPUT */
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_4QUAD_OUTPUT_FAST_READ:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 4;
      cmd->dummy_len = 0;
      cmd->addr_len = 4; /* TODO: HW_INPUT */
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_QUAD_OUTPUT_FAST_READ:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 4;
      cmd->dummy_len = 0;
      cmd->addr_len = 4; /* TODO: HW_INPUT */
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_QUAD_INPUT_OUTPUT_FAST_READ_2:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 4;
      cmd->data_width = 4;
      cmd->dummy_len = 6; /* TODO: HW_INPUT */
      cmd->addr_len = 3;  /* TODO: HW_INPUT */
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    default:
      break;
    }

    break;

  /* Flash memory PROGRAM Operations */
  case SPIFLASH_OPER_PROGRAM_MEM:

    if (!cmd->opcode) {
      cmd->opcode = cfi->program;
    }

    switch (cmd->opcode) {

    case SPIFLASH_PAGE_PROGRAM:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 4;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_EXTENDED_QUAD_INPUT_FAST_PROGRAM_2:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 4; /* TODO: HW_INPUT */
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_QUAD_INPUT_FAST_PROGRAM:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 4;
      cmd->dummy_len = 0;
      cmd->addr_len = 3; /* TODO: HW_INPUT */
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_4QUAD_INPUT_FAST_PROGRAM:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 4;
      cmd->dummy_len = 0;
      cmd->addr_len = 4; /* TODO: HW_INPUT */
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    default:
      break;
    }

    break;

  /* Flash memory ERASE Operations */
  case SPIFLASH_OPER_ERASE_MEM:

    if (!cmd->opcode) {
      cmd->opcode = cfi->erase;
    }

    switch (cmd->opcode) {

    case SPIFLASH_SUBSECTOR_ERASE:
    case SPIFLASH_SECTOR_ERASE:
    case SPIFLASH_BULK_ERASE:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 4;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_SUBSECTOR_4ERASE:
    case SPIFLASH_BULK_ERASE_1:
    case SPIFLASH_4SECTOR_ERASE:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 4; /* TODO: HW_INPUT */
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    default:
      break;
    }

    break;

  /* Flash Write Operations */
  case SPIFLASH_OPER_MODE:

    switch (cmd->opcode) {

    /* WRITE Operations */
    case SPIFLASH_WRITE_DISABLE:
    case SPIFLASH_WRITE_ENABLE:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 0;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    /* 4-Byte Address Mode Operations */
    case SPIFLASH_ENTER_4BYTE_ADDRESS_MODE:
    case SPIFLASH_EXIT_4BYTE_ADDRESS_MODE:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 0;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    default:
      break;
    }

    break;

  /* Flash Register Operations */
  case SPIFLASH_OPER_REGISTER:

    switch (cmd->opcode) {

    case SPIFLASH_READ_STATUS_REGISTER:
    case SPIFLASH_READ_STATUS2_REGISTER:
    case SPIFLASH_READ_ABRD_REGISTER:
    case SPIFLASH_READ_BRRD_REGISTER:
    case SPIFLASH_READ_CMD_RCR:
    case SPIFLASH_READ_ENHANCED_VOLATILE_CONFIGURATION_REGISTER:
    case SPIFLASH_READ_FLAG_STATUS_REGISTER:
    case SPIFLASH_READ_VOLATILE_CONFIGURATION_REGISTER:
    case SPIFLASH_READ_NONVOLATIL_CONFIGURATION_REGISTER:
    case SPIFLASH_ACCESS_BRAC_REGISTER:
    case SPIFLASH_READ_EXTENDED_ADDRESS_REGISTER:
    case SPIFLASH_READ_LOCK_REGISTER:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 0;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_WRITE_STATUS_REGISTER:
    case SPIFLASH_WRITE_ABWR_REGISTER:
    case SPIFLASH_WRITE_BRWR_REGISTER:
    case SPIFLASH_WRITE_ENHANCED_VOLATILE_CONFIGURATION_REGISTER:
    case SPIFLASH_WRITE_VOLATILE_CONFIGURATION_REGISTER:
    case SPIFLASH_WRITE_NONVOLATILE_CONFIGURATION_REGISTER:
    case SPIFLASH_WRITE_EXTENDED_ADDRESS_REGISTER:
    case SPIFLASH_WRITE_LOCK_REGISTER:

      cmd->go_rd = 0;
      cmd->go_wr = 1;
      cmd->wr_ena = 1;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 0;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_READ_ID:
    case SPIFLASH_READ_ID_1:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 0;
      cmd->addr_len = 0;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    case SPIFLASH_READ_ID_2:

      cmd->go_rd = 1;
      cmd->go_wr = 0;
      cmd->wr_ena = 0;
      cmd->addr_width = 1;
      cmd->data_width = 1;
      cmd->dummy_len = 3;
      cmd->addr_len = 0;
      cmd->inst_len = 1;
      cmd->mode_len = 0;

      break;

    default:
      break;
    }

    break;

  default:
    snprintf(err_msg, msg_size,
             "Unsupported spiflash operation [%s] for opcode [0x%x]",
             spiflash_oper_str(oper), cmd->opcode);
    return (ENOTSUP);
    break;
  }

  if (0 == cmd->opcode || (cmd->go_rd == 0 && cmd->go_wr == 0)) {
    snprintf(err_msg, msg_size,
            "Couldn't Build the spiflash command for operation [%s] "
            "opcode [0x%x]",
            spiflash_oper_str(oper), cmd->opcode);

    return (EINVAL);
  }

  /*
   * Check if config table has force 3 byte mode. Overwrite if force
   * 3 byte enabled
   */
  if ((cfi->force_3byte) && (cmd->addr_len == 4)) {
    cmd->addr_len = 3;
  }

  return 0;
}

char *spi_mode_str(fpgalib_spi_mode_en mode) {
  switch (mode) {

  case FPGALIB_SPI_MODE_4BYTE_ADDRESS_SET:
    return "Entering SPI Flash 4Byte Addressing Mode";

  case FPGALIB_SPI_MODE_4BYTE_ADDRESS_UNSET:
    return "Exiting SPI Flash 4Byte Addressing Mode";

  case FPGALIB_SPI_MODE_SET:
    return "Entering SPI Flash QSPI Addressing Mode";

  case FPGALIB_SPI_MODE_UNSET:
    return "Exiting SPI Flash QSPI Addressing Mode";

  default:
    return "Invalid SPI Flash Mode";
  }
}

void pci_util_write(uint32_t target, uint32_t data) {
  void *virt_addr;

  if (map_base) {
    virt_addr = map_base + target;

    *((uint32_t *)virt_addr) = data;
  }
}

void pci_util_read(uint32_t target, uint32_t *data) {
  void *virt_addr;

  if (map_base) {
    virt_addr = map_base + target;
    *data = *((uint32_t *)virt_addr);
  }
}

void *
mmap_sjtag_block(const char *block_name)
{
    if (!strncmp(block_name, "IOFP-JTAG", 9) ||
        !strncmp(block_name, "IOFP-SPI0", 9)) {

#ifdef UIO_SUPPORTED
        struct uio_info_t *device_info = NULL;

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
#else
	return NULL;
#endif //UIO_SUPPORTED
    } else {
        // PINPOINTER
        int pim = atoi(block_name);
        map_base = get_pinpointer_block_virtual_addr(pim, PINPOINTER_SPI_BLOCK_ADDR);
        if (!map_base) {
            fprintf(stderr, "PIM mmap failed. PIM: %d\n", pim);
            return NULL;
        }
    }
    return map_base;
}

uint8_t iofpga_reg_read_access(char *client, uint32_t offset, uint32_t *data,
                               char *err_msg, uint32_t msg_size) {
  pci_util_read(PCI_ADDRESS + offset, data);
  FPRINTF(stderr, "     %s: reg read access: offset 0x%x "
          "err_msg(%s) msg_size(%d)\n", 
          client, offset, err_msg, msg_size);
  return 0;
}

int is_golden_booted(const char *block_name) {

    int result = 0;
    uint32_t data = 0;
    uint32_t target = 0x24; // fpga_stat
    void *map_base = NULL;
    uint32_t *virt_addr;

    map_base = mmap_sjtag_block(block_name);
    if (!map_base) {
        fprintf(stderr, "failed to mmap block %s\n", block_name);
        return -1;
    }

    // Read x86 Status Register
    virt_addr = map_base + target;
    data = *virt_addr;
    
    if (!strncmp(block_name, "IOFP-JTAG", 9)) {
        // sj_fpga_status bits 11-14
        result = (data >> 11) & 0xF; 
    } else {
        // fpga_status Bits 8-11
        result = (data >> 8) & 0xF; 
    }
    return result; 
}

uint8_t iofpga_reg_write_access(char *client, uint32_t offset, uint32_t data,
                                char *err_msg, uint32_t msg_size) {
  pci_util_write(PCI_ADDRESS + offset, data);
  FPRINTF(stderr, "     %s: reg write access: offset 0x%x "
          "err_msg(%s) msg_size(%d)\n", 
          client, offset, err_msg, msg_size);
  return 0;
}

uint8_t sjtag_controller_spi_reg_dump() {
  uint32_t ctrl_reg = 0;
  uint32_t status_reg = 0;
  uint32_t rdsize_reg = 0;
  uint32_t data_reg = 0;
  uint32_t addr_op_reg = 0;
  uint32_t value = 0;
  uint8_t rc = 0;
  char err_msg[128] = {0};
  uint8_t msg_size = 128;

  ctrl_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
             offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_control);
  value = 0;
  rc = iofpga_reg_read_access("read sjtag ctrl reg", ctrl_reg, &value, err_msg,
                              msg_size);
  FPRINTF(stderr, "\nctrl reg dump dbg 0x%x: 0x%08x \n", ctrl_reg, value);

  status_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
               offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_status);

  value = 0;
  rc = iofpga_reg_read_access("read sjtag status reg", status_reg, &value,
                              err_msg, msg_size);
  FPRINTF(stderr, "\nstatus reg dump dbg 0x%x: 0x%08x \n", status_reg, value);

  rdsize_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
               offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_rdsize);
  value = 0;
  rc = iofpga_reg_read_access("read sjtag rdsize reg dump", rdsize_reg, &value,
                              err_msg, msg_size);
  FPRINTF(stderr, "\nrd siz reg dbg 0x%x: 0x%08x \n", rdsize_reg, value);

  data_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
             offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_data);
  value = 0;
  rc = iofpga_reg_read_access("read sjtag data reg dump", data_reg, &value,
                              err_msg, msg_size);
  FPRINTF(stderr, "\ndata reg dbg   0x%x: 0x%08x \n", data_reg, value);

  addr_op_reg = SJTAG_BLOCK_OFFSET +
                offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
                offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_addr_op);
  value = 0;
  rc = iofpga_reg_read_access("read sjtag data reg dump", addr_op_reg, &value,
                              err_msg, msg_size);
  FPRINTF(stderr, "\naddr_op dbg    0x%x: 0x%08x \n", addr_op_reg, value);

  return rc;
}

static uint8_t sjtag_op_addr_reg_set(uint8_t opcode, uint32_t addr,
                                     char *err_msg, uint32_t msg_size) {
  int rc = 0;
  uint32_t msb_addr = 0;
  uint32_t ls3b_addr = 0;
  uint32_t addr_op = 0;
  uint32_t spi_ctl = 0;
  uint32_t opcode_reg = 0;
  uint32_t ctrl_reg = 0;

  FPRINTF(stderr, "sjtag_op_addr_reg_set start\n");
  ls3b_addr = (addr & SJ_SPI_CSRS__FPGA_SPI_ADDR_OP_REG__ADDRESS__MASK);
  msb_addr = (addr & SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__ADDR_MSB__MASK);
  addr_op =
      ((opcode & 0xFF) << SJ_SPI_CSRS__FPGA_SPI_ADDR_OP_REG__OPCODE__SHIFT);
  addr_op |= ls3b_addr;

  opcode_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
               offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_addr_op);

  /*
   * Write LSB address and OPCODE
   */
  rc = iofpga_reg_write_access("write sjtag addr opcode reg", opcode_reg,
                               addr_op, err_msg, msg_size);
  if (rc) {
    FPRINTF(stderr, "opcode addr reg write failed rc %d\n", rc);
    return rc;
  }

  ctrl_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
             offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_control);

  /*
   * Read SPI Controller Instruction register
   */
  rc = iofpga_reg_read_access("read sjtag ctrl reg", ctrl_reg, &spi_ctl,
                              err_msg, msg_size);
  if (rc) {
    FPRINTF(stderr, "ctrl reg write failed rc %d\n", rc);
    return rc;
  }

  spi_ctl &= SJ_SPI_CSRS__FPGA_SPI_ADDR_OP_REG__ADDRESS__MASK;
  spi_ctl |= msb_addr;

  /*
   * Write SPI Controller Instruction register for MSB addr
   */
  rc = iofpga_reg_write_access("write sjtag ctrl reg for MSB addr", ctrl_reg,
                               spi_ctl, err_msg, msg_size);
  if (rc) {
    FPRINTF(stderr, "ctrl reg write failed rc %d\n", rc);
    return rc;
  }

  usleep(10);
  FPRINTF(stderr, "sjtag_op_addr_reg_set end\n");
  return 0;
}

static uint8_t sjtag_controller_check_for_ready(uint8_t delay_factor,
                                                char *err_msg,
                                                uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t value = 0;
  int count = 0;
  uint32_t status_reg = 0;

  FPRINTF(stderr, "sjtag_controller_check_for_ready start\n");
  status_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
               offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_status);

  /*
   * Bit14: Busy : SPI Core is currently performing an operation
   *               if set to 1. Wait till it gets cleared
   */
  do {
    rc = iofpga_reg_read_access("read sjtag status reg", status_reg, &value,
                                err_msg, msg_size);

    if (rc) {
      FPRINTF(stderr, "status reg read failed rc %d\n", rc);
      return rc;
    }

    count++;
    usleep(100 * delay_factor);

    if (count > IOFPGA_SJTAG_TRANS_TIMEOUT) {
      break;
    }
    FPRINTF(stderr, "read val: %d\n",
            (SJ_SPI_CSRS__FPGA_SPI_STATUS_REG__BUSY__READ(value)));
  } while (SJ_SPI_CSRS__FPGA_SPI_STATUS_REG__BUSY__READ(value));

  value = 0;
  /*
   * Bit15: Done : Last Operation is completed if set to 1.
   *               Write 1 to clear it before starting next operation
   */
  rc = iofpga_reg_read_access("read sjtag status reg", status_reg, &value,
                              err_msg, msg_size);

  if (rc) {
    FPRINTF(stderr, "status reg read failed rc %d\n", rc);
    return rc;
  }

  if (SJ_SPI_CSRS__FPGA_SPI_STATUS_REG__DONE__READ(value)) {
    rc = iofpga_reg_write_access("write sjtag status reg", status_reg, value,
                                 err_msg, msg_size);
    if (rc) {
      FPRINTF(stderr, "status reg write failed rc %d\n", rc);
      return rc;
    }
  }

  usleep(10 * delay_factor);

  FPRINTF(stderr, "sjtag_controller_check_for_ready end\n");
  return (0);
}

/*
 * Service routine to block until the current SPI Flash Controller transation
 * to complete or timeout occurs after FPGALIB_SJTAG_TRANS_TIMEOUT periods.
 *
 * It checks every 100 mirco seconds by default and platform can define
 * a vendor specific delay factor to oprimize the read / write performance.
 *
 * Returns 0 when SPI Flash transation is success,
 *         ETIME when transaction times out.
 *         Otherwise:
 *            Error code with message
 */
static uint8_t sjtag_wait_until_transaction_complete(uint8_t delay_factor,
                                                     char *err_msg,
                                                     uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t value = 0;
  int count = 0;
  uint32_t status_reg = 0;

  FPRINTF(stderr, "sjtag_wait_until_transaction_complete start %d\n", delay_factor);
  status_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
               offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_status);

  /*
   * Bit14: Busy : SPI Core is currently performing an operation
   *               if set to 1. Wait till it gets cleared
   */
  do {
    rc = iofpga_reg_read_access("read sjtag status reg", status_reg, &value,
                                err_msg, msg_size);
    if (rc) {
      FPRINTF(stderr, "status reg read failed rc %d\n", rc);
      return rc;
    }

    count++;
    usleep(1);

    if (count > IOFPGA_SJTAG_TRANS_TIMEOUT) {
      break;
    }
  } while (SJ_SPI_CSRS__FPGA_SPI_STATUS_REG__BUSY__READ(value));

  value = 0;
  count = 0;

  /*
   * Bit15: Done : Last Operation is completed if set to 1.
   *               Write 1 to clear it before starting next operation
   */
  rc = iofpga_reg_read_access("read sjtag status reg", status_reg, &value,
                              err_msg, msg_size);

  if (rc) {
    FPRINTF(stderr, "status reg read failed rc %d\n", rc);
    return rc;
  }

  /*
   * Bit15: Done : Operation is completed if set to 1.
   *               Ensure Operation is done
   */
  FPRINTF(stderr, "Ensure bit set to 1\n");
  while (!(SJ_SPI_CSRS__FPGA_SPI_STATUS_REG__DONE__READ(value))) {
    rc = iofpga_reg_read_access("read sjtag status reg", status_reg, &value,
                                err_msg, msg_size);

    if (rc) {
      FPRINTF(stderr, "status reg read failed rc %d\n", rc);
      return rc;
    }

    count++;
    usleep(1);

    if (count > IOFPGA_SJTAG_TRANS_TIMEOUT) {
      break;
    }
  }

  FPRINTF(stderr, "sjtag_wait_until_transaction_complete end\n");
  return 0;
}

/*
 * Service routing to Set FPGA sjtag block controll instruction register
 *
 * Returns 0 on Success
 *         otherwise - Error code with message
 */
static uint8_t sjtag_ctl_reg_set(spi_cfi_t *cfi, fpgalib_spi_cmd_t *cmd,
                                 uint16_t data_len, char *err_msg,
                                 uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t data = 0;
  uint32_t rdsize_reg = 0;
  uint32_t ctrl_reg = 0;
  uint8_t go_rd_wr = 0;

  FPRINTF(stderr, "sjtag_ctl_reg_set start\n");
  if (cfi == NULL) {
    return EINVAL;
  }
  if (cmd == NULL) {
    return EINVAL;
  }

  /*
   * Ensure SPI Controller is Ready to initiate the Command
   */
  rc = sjtag_controller_check_for_ready(cfi->delay_factor, err_msg, msg_size);

  SJ_SPI_CSRS__FPGA_SPI_RDSIZE_REG__RDSIZE__MODIFY(data, data_len);

  rdsize_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
               offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_rdsize);

  /*
   * Write Data Size value to SPI Read Size register
   */
  rc = iofpga_reg_write_access("write sjtag rdsize reg", rdsize_reg, data,
                               err_msg, msg_size);
  if (rc) {
    FPRINTF(stderr, "rdsize reg write failed rc %d\n", rc);
    return rc;
  }

  ctrl_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
             offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_control);

  data = 0;
  /*
   * Read SPI Controller Instruction register
   */
  rc = iofpga_reg_read_access("read sjtag ctrl reg", ctrl_reg, &data, err_msg,
                              msg_size);

  if (rc) {
    FPRINTF(stderr, "ctrl reg read failed rc %d\n", rc);
    return rc;
  }

  if (cmd->go_rd) {
    go_rd_wr = 0;
  } else if (cmd->go_wr) {
    go_rd_wr = 1;
  }

  /*
   * Retain the MSB Address field: 31:24
   */
  data &= SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__ADDR_MSB__MASK;

  /*
   * Number of ticks in the dummy field
   */
  SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__USE_DUMMY__MODIFY(data, cmd->dummy_len);

  /*
   * FIFO32: Data FIFO accessed 32-bits at a time
   */
  SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__FIFO32__MODIFY(data, 1);

  /*
   * Use OpCode Field
   */
  SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__USE_OPCODE__MODIFY(data, 1);

  /*
   * Write:1 Read:0
   */
  SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__DATA_DIR__MODIFY(data, go_rd_wr);

  switch (cmd->opcode) {
  case SPIFLASH_PAGE_PROGRAM:
  case SPIFLASH_READ:
  case SPIFLASH_SECTOR_ERASE:

    if (cfi->force_3byte) {
      cmd->addr_len = 3;
    }

    if (data >> (SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__ADDR_MSB__SHIFT + 1)) {
      /*
       * Address Size: 4 Bytes
       */
      SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__ADDR_SIZE__MODIFY(data, 3);
    } else {
      SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__ADDR_SIZE__MODIFY(data,
                                                           (cmd->addr_len - 1));
    }

    /*
     * Use Address Field
     */
    SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__USE_ADDR__MODIFY(data, 1);
    break;

  case SPIFLASH_WRITE_BRWR_REGISTER:
    /*
     * Disable FIFO32 bit; FIFO accessed 8-bits at a time
     * for Bank Write Register access
     */
    SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__FIFO32__MODIFY(data, 0);
    break;

  default:
    /*
     * This is already considered prior to switch statement
     * Data field is already pre-initialized for all the
     * operations.
     */
    break;
  }

  /*
   * Write SPI Controller Instruction register
   */
  rc = iofpga_reg_write_access("write sjtag ctrl reg", ctrl_reg, data, err_msg,
                               msg_size);
  if (rc) {
    FPRINTF(stderr, "ctrl reg write failed rc %d\n", rc);
    return rc;
  }

  /*
   * SET SPI transfer
   */
  SJ_SPI_CSRS__FPGA_SPI_CONTROL_REG__CPUWR__MODIFY(data, 1);

#if DEBUG
  sjtag_controller_spi_reg_dump();
#endif

  /*
   * Initiate Command for SPI transfer Write SPI Controller Instruction register
   */
  rc = iofpga_reg_write_access("write sjtag ctrl reg for spi transfer",
                               ctrl_reg, data, err_msg, msg_size);
  if (rc) {
    FPRINTF(stderr, "ctrl reg write failed rc %d\n", rc);
    return rc;
  }

  usleep(1 * cfi->delay_factor);

  /*
   * Ensure SPI Controller has successfully completed the given Command
   */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction\n");
    return rc;
  }

  FPRINTF(stderr, "sjtag_ctl_reg_set end\n");
  return (0);
}

uint8_t sjtag_spi_read_bank_addr(spi_cfi_t *cfi, uint8_t exp_bank_addr,
                                 char *err_msg, uint32_t msg_size) {
  uint8_t rc = 0;
  fpgalib_spi_cmd_t cmd = {0};
  uint8_t bank_addr[1] = {0};

  FPRINTF(stderr, "%s start\n", __FUNCTION__);

  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag write has invalid "
             "device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "[%s]\n", err_msg);
    return (ENODEV);
  }

  cmd.opcode = SPIFLASH_READ_BRRD_REGISTER;
  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_REGISTER, &cmd, err_msg,
                                    msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command [%s]\n", err_msg);
    return rc;
  }

  /* Set opcode and register address */
  rc = sjtag_op_addr_reg_set(cmd.opcode, 0, /* SPI Flash Addr */
                             err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set opcode and register [%s]\n", err_msg);
    return rc;
  }

  /* Set control register */
  rc = sjtag_ctl_reg_set(cfi, &cmd, 0, /* 1 Byte read */
                         err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set controller instruction %s", err_msg);
    return rc;
  }

  /* Check for sjtag FPGA Block transaction complete */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction %s\n",
            err_msg);
    return rc;
  }

  rc = sjtag_read_fifo(bank_addr, 1, /* 1 Byte read from buffer */
                       err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't read rfifo register [%s]\n", err_msg);
  }

  if (bank_addr[0] != exp_bank_addr) {
    snprintf(err_msg, msg_size, "bank addr mismatch actual 0x%x expected 0x%x",
             bank_addr[0], exp_bank_addr);
    FPRINTF(stderr, "[%s]\n", err_msg);
    return EINVAL;
  }

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return rc;
}
/*
 * Service routine to Read SPI Flash Flag Status Register value
 *
 * INPUT:
 *  cfi       - SPI Common Flash Interface data
 *  status    - pointer to get status value
 *  err_msg   - buffer to error message
 *  msg_size  - err_msg buffer size
 *
 * OUTPUT:
 *  register value in status
 *
 * Returns 0 on Success
 *         otherwise - Error code with message
 */
static uint8_t sjtag_flash_read_fsr(spi_cfi_t *cfi, uint32_t *status,
                                    char *err_msg, uint32_t msg_size) {

  uint8_t rc = 0;
  uint8_t fsr;
  fpgalib_spi_cmd_t cmd = {0};

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
    return EINVAL;
  }

  cmd.opcode = cfi->flag_status;
  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_REGISTER, &cmd, err_msg,
                                    msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command: [%s]\n", err_msg);
    return rc;
  }

  /* Set opcode and register address */
  rc = sjtag_op_addr_reg_set(cmd.opcode, 0, /* SPI Flash Addr */
                             err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set opcode and register: [%s]\n", err_msg);
    return rc;
  }

  /* Set control register */
  rc = sjtag_ctl_reg_set(cfi, &cmd, 1, /* 1 Byte data */
                         err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set controller instruction: [%s]\n", err_msg);
    return rc;
  }

  /* Check for sjtag FPGA Block transaction complete */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction: [%s]\n",
            err_msg);
    return rc;
  }

  /*
   * Get spi flash status register value
   */
  rc = sjtag_read_fifo(&fsr, 1, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Couldn't read rfifo register: [%s]\n", err_msg);
    return rc;
  }

  *status = fsr;

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return 0;
}

/*
 * Service routine to Read SPI Flash Flag Status Register value
 *
 * INPUT:
 *  cfi       - SPI Common Flash Interface data
 *  status    - pointer to get status value
 *  err_msg   - buffer to error message
 *  msg_size  - err_msg buffer size
 *
 * OUTPUT:
 *  register value in status
 *
 * Returns 0 on Success
 *         otherwise - Error code with message
 */
static uint8_t sjtag_wait_till_fsr_ready(spi_cfi_t *cfi, uint8_t delay_factor,
                                         char *err_msg, uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t ii;
  uint32_t fsr;

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  /*
   * Retrieve the vendor specific delay factor for sleep between operations.
   * Preventin step in case of undefined from platform
   */
  if (!delay_factor) {
    delay_factor = 1;
  }

  /*
   * Return EOK if spiflash doesn't support flag status register
   */
  if (cfi->flag_status == 0) {
    return 0;
  }

  /*
   * Check for SPI Flash Flag Status Register for ready to accept transations
   */
  for (ii = 0; ii < IOFPGA_SJTAG_READY_TIMEOUT; ii++) {
    rc = sjtag_flash_read_fsr(cfi, &fsr, err_msg, msg_size);

    if (rc != 0) {
      FPRINTF(stderr, "Failed to get SPI Flash Flag Status: [%s]\n", err_msg);
      return rc;
    }

    if (!(fsr & SPIFLASH_FSR_READY)) {
      usleep(100 * delay_factor); // Do not change here
    } else {
      return (0);
    }
  }

  snprintf(err_msg, msg_size,
           "Timed out waiting for SPI Flash device to be ready, "
           "FSR value 0x%x",
           fsr);
  FPRINTF(stderr, "%s\n", err_msg);

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return (ETIME);
}

/*
 * Service routine to Enable / Disable write operation on SPI Flash.
 *
 * Returns 0 when device is Enable / Disable success
 *   otherwise - error code with message
 */
static uint8_t sjtag_flash_wr_ena_dis(spi_cfi_t *cfi, uint8_t ena_dis,
                                      char *err_msg, uint32_t msg_size) {
  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  uint8_t rc = 0;
  fpgalib_spi_cmd_t cmd = {0};

  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag write %s has invalid "
             "device instance %d(%d)",
             ena_dis ? "Enable" : "Disable", cfi->dev_inst,
             IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "[%s]\n", err_msg);
    return (ENODEV);
  }

  /* Wait for SPI device to be ready */
  rc = sjtag_wait_till_fsr_ready(cfi, cfi->spi_delay_factor, err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "SPI Flash device is not ready to %s Write operation %s",
            ena_dis ? "Enable" : "Disable\n", err_msg);
    return rc;
  }

  if (ena_dis) {
    cmd.opcode = cfi->write_enable;
  } else {
    cmd.opcode = cfi->write_disable;
  }

  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_MODE, &cmd, err_msg,
                                    msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command [%s]\n", err_msg);
    return rc;
  }

  /* Set opcode and register address */
  rc = sjtag_op_addr_reg_set(cmd.opcode, 0, /* SPI Flash Addr */
                             err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set opcode and register [%s]\n", err_msg);
    return rc;
  }

  /* Set control register */
  rc = sjtag_ctl_reg_set(cfi, &cmd, 0, /* No data */
                         err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set controller instruction [%s]\n", err_msg);
    return rc;
  }

  /* Check for sjtag FPGA Block transaction complete */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction %s\n",
            err_msg);
    return rc;
  }

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return 0;
}

static int sjtag_spi_write_bank_addr(spi_cfi_t *cfi, int bank_data,
                                     char *err_msg, uint32_t msg_size) {
  int rc = 0;
  uint32_t data = 0;
  uint32_t ctrl_reg = 0;
  uint32_t data_reg = 0;
  fpgalib_spi_cmd_t cmd = {0};

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
    return EINVAL;
  }

  /*
   * SPI Flash write enable
   */
  rc = sjtag_flash_wr_ena_dis(cfi, 1, err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to Enable SPI Flash write [%s]\n", err_msg);
    return rc;
  }

  rc = sjtag_wait_till_wip(cfi, cfi->spi_delay_factor, err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr,
            "SPI Flash device is not ready to "
            "READ SPI Flash Memory %s\n",
            err_msg);
    return rc;
  }

  /*
   * Configure Control Register for
   * 1. Disable 32-bit access     (bit15 : 0)
   * 2. Do NOT start SPI transfer (bit10 : 0)
   * 3. Enable Use_OpCode bit     (bit3  : 1)
   * 4. Operation = WRITE         (bit1  : 1)
   */
  data = 0x00A;
  ctrl_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
             offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_control);
  rc = iofpga_reg_write_access("write ctrl reg", ctrl_reg, data, err_msg,
                               msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "fail to write ctrl reg [%s]\n", err_msg);
    return rc;
  }

  cmd.opcode = cfi->wr_ear;
  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_REGISTER, &cmd, err_msg,
                                    msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command [%s]\n", err_msg);
    return rc;
  }

  /* Set opcode and register address */
  rc = sjtag_op_addr_reg_set(cmd.opcode, 0, /* 1 Byte write */
                             err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set opcode and register [%s]\n", err_msg);
    return rc;
  }

  /*
   * Bank Write Register value should be:
   * 0    - for Address range 23:0 i.e 3 Byte Address field MSByte (31:24) = 0
   * 1    - for Address range with Bit24 = 1
   * 0x80 - for remaining 4 byte address range i.e Extended Address Enabled
   */
  data = bank_data;
  if (bank_data > 1) {
    FPRINTF(stderr, "Extended Address enabled bank_data: 0x%x\n", bank_data);
    data = 0x80;
  }

  data_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
             offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_data);
  rc = iofpga_reg_write_access("sjtag data reg write", data_reg, data, err_msg,
                               msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "fail to write data reg [%s]\n", err_msg);
    return rc;
  }

  /* Set control register */
  rc = sjtag_ctl_reg_set(cfi, &cmd, 0, err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to set controller instruction [%s]\n", err_msg);
    return rc;
  }

  /* Check for operation done */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction [%s]\n",
            err_msg);
    return rc;
  }

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return (rc);
}

/*
 * Service routine to READ page data from SPI Flash memory.
 * INPUT:
 *  cfi  - SPI Common Flash Interface Data
 *  addr     - SPI Flash memory Address
 *  data     - Pointer to get SPI Flash data that read
 *  data_len - How many bytes need to read from given addr
 *  err_msg  - Error message buffer
 *  msg_size - Error message buffer size
 *
 * Returns 0 when read from SPI Flash
 *  otherwise - error code with message
 */
static uint8_t sjtag_page_read(spi_cfi_t *cfi, uint32_t addr, uint8_t *data,
                               uint16_t data_len, char *err_msg,
                               uint32_t msg_size) {
  uint8_t rc = 0;
  fpgalib_spi_cmd_t cmd = {0};

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
  }

  FPRINTF(stderr,
          "Read SPI Flash Memory "
          "addr 0x%08x data_len 0x%x\n",
          addr, data_len);

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag READ PAGE has invalid device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    return (ENODEV);
  }

  /* check sjtag FPGA Block read capabality */
  if (data_len > IOFPGA_SJTAG_RDATA_SIZE) {
    snprintf(err_msg, msg_size,
             "SJTAG couldn't read more than %d expected %d Bytes",
             IOFPGA_SJTAG_RDATA_SIZE, data_len);
    return (EINVAL);
  }

  rc = sjtag_wait_till_wip(cfi, cfi->spi_delay_factor, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr,
            "SPI Flash device is not ready to "
            "READ SPI Flash Memory [%s]\n",
            err_msg);
    return rc;
  }

  /*
   * Check whether Bank Read/Write Register opcodes are valid and
   * non-zero values. If yes, proceed with Bank Address Write
   * Register otherwise skip writing to Bank Address Register
   * (addr >> 24) - write 4 byte value to Bank register and
   * 3 bytes to addr opcode register
   */
  if (cfi->wr_ear && cfi->rd_ear) {
    rc = sjtag_spi_write_bank_addr(cfi, (addr >> 24), err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "fail to set bank addr [%s]\n", err_msg);
      return rc;
    }

    rc = sjtag_spi_read_bank_addr(cfi, (addr >> 24), err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "bank addr mismatch, [%s]\n", err_msg);
      return rc;
    }
  }

  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_READ_MEM, &cmd, err_msg,
                                    msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command [%s]\n", err_msg);
    return rc;
  }

  /* Set opcode and register address */
  rc = sjtag_op_addr_reg_set(cmd.opcode, addr, /* SPI Flash Addr */
                             err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set opcode and register [%s]\n", err_msg);
    return rc;
  }

  /* Set control register */
  rc = sjtag_ctl_reg_set(cfi, &cmd, data_len, /* data length in bytes*/
                         err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to set controller instruction [%s]\n", err_msg);
    return rc;
  }

  /* Check for sjtag FPGA Block transaction complete */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction: [%s]\n",
            err_msg);
    return rc;
  }

  /* Extract data from sjtag FPGA Block rfifo */
  rc = sjtag_read_fifo(data, data_len, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed read from sjtag FPGA Block rfifo [%s]\n", err_msg);
    return rc;
  }

#ifdef DEBUG
  sjtag_dump_data("SPI Flash READ PAGE", addr, data, data_len);
#endif

  FPRINTF(stderr,
          "Success reading page SPI Flash Memory address 0x%x of len 0x%x\n",
          addr, data_len);

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return 0;
}

/*
 * Service routine to READ data from SPI Flash memory.
 * INPUT:
 *  cfi  - SPI Common Flash Interface Data
 *  addr     - SPI Flash memory Address
 *  data     - Pointer to get SPI Flash data that read
 *  data_len - How many bytes need to read from given addr
 *  err_msg  - Error message buffer
 *  msg_size - Error message buffer size
 *
 * Returns 0 when read from SPI Flash
 *  otherwise - error code with message
 */
static uint8_t sjtag_read(spi_cfi_t *cfi, uint32_t addr, uint8_t *data,
                          uint32_t data_len, char *err_msg, uint32_t msg_size) {
  uint8_t rc = 0;
  uint16_t page_size;
  uint32_t ii;
  uint32_t jj;
  uint32_t kk;
  uint32_t max_byte;
  uint32_t start_pg;
  uint32_t end_pg;
  uint32_t start_rd_addr;
  uint32_t start_wr_addr;
  uint8_t *data_ptr = NULL;
  uint8_t *this_data;

  FPRINTF(stderr, "Read SPI Flash Memory addr 0x%x data_len 0x%x\n", addr,
          data_len);

  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null ptr");
    return EINVAL;
  }
  if (!data) {
    snprintf(err_msg, msg_size, "data - Null ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size, "sjtag read has invalid device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    return (ENODEV);
  }

  page_size = cfi->page_size;

  if (page_size == 0 || page_size > IOFPGA_SJTAG_PAGE_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW can't handle page_size %d "
             "bigger than expected %d",
             page_size, IOFPGA_SJTAG_PAGE_SIZE);
    return (EINVAL);
  }

  /* A segment of data_len, start at addr are within these pages */
  sjtag_segment_range(addr, data_len, page_size, &start_pg, &end_pg);
  data_ptr = malloc(page_size);
  if (data_ptr == NULL) {
    snprintf(err_msg, msg_size, "failed to allocate memory %s",
             strerror(errno));
    return ENOMEM;
  }

  /*
   * NOTE: SPI FLASH is operated on page alignment
   * The request "addr" may be in the middle of the first page
   * The request "data_len" may be ended in the middle of the last page
   * First page and last page can be the same page
   * This function will read more than the request "data_len";
   * adjustment is needed, so the returned result is exactly the request
   */

  /*
   * Read 1 page at a time then transfer it to data
   */
  this_data = (uint8_t *)data;
  kk = 0;

  for (ii = start_pg; ii < end_pg; ii++) {
    start_rd_addr = ii * page_size; /* Top of the page, and <= addr */
    memset(data_ptr, 0, sizeof(page_size));

    rc = sjtag_page_read(cfi, start_rd_addr, data_ptr, page_size, err_msg,
                         msg_size);

    if (rc != 0) {
      FPRINTF(stderr,
              "Failed to the read page %d of "
              "start_pg %d end_pg %d size %d %s\n",
              ii, start_pg, end_pg, page_size, err_msg);
      goto clean_exit;
    }

    if (ii == start_pg) { /* 1st page read */
      /* Max num of capture bytes */
      max_byte = start_rd_addr + page_size - addr;

      /* Start point to capture */
      start_wr_addr = page_size - max_byte;
    } else {
      /* Max num of capture bytes */
      max_byte = page_size; /* Max num of capture bytes */

      start_wr_addr = 0; /* Start point to capture */
    }

    /* Case of last page */
    if ((kk + max_byte) > data_len) {
      max_byte = data_len - kk;
    }

    /* Store into data */
    for (jj = 0; jj < max_byte; jj++) {
      this_data[kk] = *(data_ptr + start_wr_addr);
      start_wr_addr++;
      kk++;
    }
  }

  FPRINTF(stderr, "Success Reading SPI Flash addr 0x%x data_len 0x%x\n", addr,
          data_len);

clean_exit:

  if (data_ptr) {
    free(data_ptr);
  }

  return rc;
}

/*
 * Service routine to Reads SPI Flash Data from FPGA sjtag block rfifo register
 *
 * Returns 0 on Success
 *         otherwise - Error code with message
 */
static uint8_t sjtag_read_fifo(uint8_t *data, uint16_t data_len, char *err_msg,
                               uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t ii = 0, jj = 0;
  uint32_t rfifo_reg = 0;
  uint16_t max_dwords = 0;
  uint16_t max_bytes = 0;
  uint32_t value = 0;

  FPRINTF(stderr, "sjtag_read_fifo start\n");
  if (data == NULL) {
    snprintf(err_msg, msg_size, "data - Null Ptr");
    return EINVAL;
  }

  /* check fpga lib sjtag read capabality */
  if (data_len > IOFPGA_SJTAG_RDATA_SIZE) {
    snprintf(err_msg, msg_size,
             "SJTAG couldn't read more than %d expected %d Bytes",
             IOFPGA_SJTAG_RDATA_SIZE, data_len);
    FPRINTF(stderr, "SJTAG couldn't read more than %d expected %d Bytes\n",
            IOFPGA_SJTAG_RDATA_SIZE, data_len);
    return (EINVAL);
  }

  max_dwords = data_len / 4;
  max_bytes = data_len % 4;

  rfifo_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
              offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_data);

  /*
   * Get first n-1 dwords
   */
  for (ii = 0; ii < max_dwords; ii++) {
    value = 0;
    rc = iofpga_reg_read_access("sjtag read fifo", rfifo_reg, &value, err_msg,
                                msg_size);

    if (rc != 0) {
      FPRINTF(stderr, "Failed to read rfifo register : [%s]\n", err_msg);
      return rc;
    }

    /* Endian-ness conversion requirement:
     * SJ SPI interface needs Endian-ness conversion
     * and hence the need for new Register Access functions for
     * reading and programming SPI flash memory.
     * Use register access functions appropriately while adding
     * support for new SJ SPI interface as all SJ devices may not
     * need swapping.
     */
    value = htobe32(value);

    for (jj = 0; jj < 4; jj++) {
      *(data + (ii * 4) + jj) = ((value >> (jj * 8)) & 0xFF);
    }
  }

  /*
   * Get the last bytes
   */
  if (max_bytes) {
    value = 0;
    rc = iofpga_reg_read_access("sjtag read fifo", rfifo_reg, &value, err_msg,
                                msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "Failed to read rfifo register: [%s]\n", err_msg);
      return rc;
    }

    value = htobe32(value);

    for (jj = 0; jj < max_bytes; jj++) {
      *(data + (ii * 4) + jj) = ((value >> (jj * 8)) & 0xFF);
    }
  }

  FPRINTF(stderr, "sjtag_read_fifo end\n");
  return 0;
}

/*
 * Service routine to Read SPI Flash Flag Status Register value
 */
static uint8_t sjtag_flash_read_sr(spi_cfi_t *cfi, uint32_t *status,
                                   char *err_msg, uint32_t msg_size) {
  uint8_t rc = 0;
  uint8_t fsr;
  fpgalib_spi_cmd_t cmd = {0};

  FPRINTF(stderr, "sjtag_flash_read_sr start\n");
  if (cfi == NULL) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
    return (EINVAL);
  }

  cmd.opcode = cfi->status;

  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_REGISTER, &cmd, err_msg,
                                    msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command : [%s]\n", err_msg);
    return rc;
  }

  /* Set opcode and register address */
  rc = sjtag_op_addr_reg_set(cmd.opcode, 0 /* SPI Flash Addr */, err_msg,
                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to set opcode and registers: [%s]\n", err_msg);
    return rc;
  }

  /* Set control register */
  rc = sjtag_ctl_reg_set(cfi, &cmd, 1 /* 1 Byte data */, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set controller instruction: [%s]\n", err_msg);
    return rc;
  }

  /* Check for sjtag FPGA Block transaction complete */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction : [%s]\n",
            err_msg);
    return rc;
  }

  /*
   * Get spi flash status register value
   */
  rc = sjtag_read_fifo(&fsr, 1, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Couldn't read rfifo register : [%s]\n", err_msg);
    return rc;
  }

  *status = fsr;

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return 0;
}

uint8_t sjtag_wait_till_wip(spi_cfi_t *cfi, uint8_t delay_factor, char *err_msg,
                            uint32_t msg_size) {

  uint8_t rc = 0; // SUCCESS
  uint32_t sr;    // status Register
  uint32_t i;

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
    return EINVAL;
  }

  /*
   * Retrieve the vendor specific delay factor for sleep between operations.
   * Preventin step in case of undefined from platform
   */
  if (!delay_factor) {
    delay_factor = 1;
  }

  /*
   * Check for SPI Flash Flag Status Register for ready to accept transations
   */
  for (i = 0; i < IOFPGA_SJTAG_READY_TIMEOUT; i++) {
    rc = sjtag_flash_read_sr(cfi, &sr, err_msg, msg_size);

    if (rc != 0) {
      FPRINTF(stderr, "Failed to get SPI Flash Flag Status : [%s]\n", err_msg);
      return rc;
    }

    if (sr & SPIFLASH_SR_WIP) {
      usleep(100 * delay_factor);
    } else {
      return 0;
    }
  }

  snprintf(err_msg, msg_size,
           "Timed out waiting for SPI Flash device to be ready, "
           "SR value 0x%x\n",
           sr);

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return (ETIMEDOUT);
}

/* Read the jedec_id from the SPI and based on the same, fill in the spi cfg */
#define JEDEC_ID_LEN 20
uint8_t iofpga_read_spi_jedec_id(uint8_t *data, uint16_t data_len,
                                 spi_cfi_t *cfi, char *err_msg,
                                 uint32_t msg_size) {
  uint8_t rc = 0;              // Success
  fpgalib_spi_cmd_t cmd = {0}; // spi command

  FPRINTF(stderr, "iofpga_read_spi_jedec_id start \n");
  if (data_len > IOFPGA_SJTAG_RDATA_SIZE) {
    snprintf(err_msg, msg_size, "Couldnt read id more than %d(%d) Bytes\n",
             IOFPGA_SJTAG_RDATA_SIZE, data_len);
    return EINVAL;
  }

  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_READ_JEDEC_ID, &cmd,
                                    err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command: [%s]\n", err_msg);
    return rc;
  }

  sjtag_wait_till_wip(cfi, cfi->spi_delay_factor, err_msg, msg_size);

  /* Set opcode and register address */
  rc = sjtag_op_addr_reg_set(cmd.opcode, 0 /* SPI Flash Addr */, err_msg,
                             msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set opcode and register [%s]\n", err_msg);
    goto clean_exit;
  }

  /* Set control register */
  rc = sjtag_ctl_reg_set(cfi, &cmd, data_len /* data length in Bytes */,
                         err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to set controller instruction: [%s]\n", err_msg);
    goto clean_exit;
  }

  /* Check for operation done */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction: [%s]\n",
            err_msg);
    goto clean_exit;
  }

  /*
   * Get SPI Flash JEDEC ID
   */
  rc = sjtag_read_fifo(data, data_len, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Couldn't read rfifo register: [%s]\n", err_msg);
    goto clean_exit;
  }

  FPRINTF(stderr,
          "READ SUCCESS: "
          "SJTAG Read value=0x%02x%02x%02x%02x\n",
          data[3], data[2], data[1], data[0]);

clean_exit:
  FPRINTF(stderr, "iofpga_read_spi_jedec_id end\n");
  return rc;
}

spiflash_cfg_t *get_spiflash_cfg(uint8_t *jedec_id) {
  FPRINTF(stderr, "get_spiflash_cfg start\n");
  size_t ii;

  for (ii = 1; ii < sizeof(spiflash_models_cfg) / sizeof(spiflash_cfg_t);
       ii++) {
    if ((spiflash_models_cfg[ii].manuf_id == jedec_id[0]) &&
        (spiflash_models_cfg[ii].memory_type == jedec_id[1]) &&
        (spiflash_models_cfg[ii].memory_density == jedec_id[2]) &&
        ((spiflash_models_cfg[ii].vendor_id1 == jedec_id[4]) ||
         (spiflash_models_cfg[ii].vendor_id1 == 0xFF))) {
      FPRINTF(stderr, "Found flash cfg match : vendor_name(%s) - index %zu \n",
              spiflash_models_cfg[ii].vendor_name, ii);
      return &spiflash_models_cfg[ii];
    }
  }
  FPRINTF(stderr, "get_spiflash_cfg end\n");
  return NULL;
}

uint8_t spiflash_cfi_data(spiflash_cfg_t *config, spi_cfi_t *cfi) {
  FPRINTF(stderr, "spiflash_cfi_data start\n");

  cfi->dev_inst = 0;

  cfi->spi_delay_factor = config->spi_delay_factor;

  cfi->verify_flag = config->verify_flag;

  cfi->support_erase = config->support_erase;

  cfi->page_size = config->page_size;

  /*
   * TODO: Now we are treating SPI Flash as flat sectors, need to add
   * functionality when wanted to support multiple sector types in a SPI
   * Flash.
   */
  if (config->num_sectors > 1) {
    return (ENOTSUP);
  }

  cfi->sector_size = config->sector[0].size;

  cfi->sector_start = config->sector[0].start;

  cfi->sector_end = config->sector[0].end;

  cfi->jedec_id = config->jedec_id;

  cfi->read_cfg = config->read_cfg;

  cfi->status = config->status;

  cfi->flag_status = config->flag_status;

  cfi->read = config->read;

  cfi->erase = config->erase;

  cfi->program = config->program;

  cfi->write_enable = config->write_enable;

  cfi->write_disable = config->write_disable;

  cfi->rd_ear = config->rd_ear;

  cfi->wr_ear = config->wr_ear;

  cfi->enter_4byte = config->enter_4byte;

  cfi->exit_4byte = config->exit_4byte;

  cfi->enter_spi = config->enter_spi;

  cfi->exit_spi = config->exit_spi;

  cfi->force_3byte = config->force_3byte;

  /*
   * Set SPI Flash into 4 Byte addressing mode if the capacity is more that
   * 16MB
   */
  if (cfi->enter_4byte) {
    cfi->mode = FPGALIB_SPI_MODE_4BYTE_ADDRESS_SET;
  }

  FPRINTF(stderr, "spiflash_cfi_data end\n");
  return 0;
}

int iofpga_get_spi_cfg(char *err_msg, uint32_t msg_size) {
  FPRINTF(stderr, "iofpga_get_spi_cfg start\n");
  spiflash_cfg_t *spiflash_cfg = &spiflash_models_cfg[0];

  uint8_t jedec_id[JEDEC_ID_LEN] = {0};
  uint8_t rc = 0;

  // set up cfi with defaults.
  spiflash_cfi_data(spiflash_cfg, &cfi);

  rc = iofpga_read_spi_jedec_id(jedec_id, sizeof(jedec_id), &cfi, err_msg,
                                msg_size);
  if (rc) {
    printf("Failed to get jedec_id from SPI Flash! "
            "rc %d error '%s'\n", rc, err_msg);
    return -1;
  }

  memset(&spiflash_cfg, 0, sizeof(spiflash_cfg));
  spiflash_cfg = get_spiflash_cfg(jedec_id);
  if (spiflash_cfg == NULL) {
      printf("failed to get spiflash cfg\n");
      return -1;
  }

  spiflash_cfi_data(spiflash_cfg, &cfi);

  FPRINTF(stderr, "iofpga_get_spi_cfg end\n");
  return 0;
}

/*
 * SPI FPGA block can read / write maximum of IOFPGA_SJTAG_RDATA_SIZE /
 * FPGALIB_SJTAG_WDATA_SIZE at a time. Hence need to chunk the bigger data
 * into multiple segments to read and write.
 *
 * Service routine to find segment range of size seg_size for a given
 * target_addr and target_len
 */
void sjtag_segment_range(uint32_t target_addr, uint32_t target_len,
                         uint32_t seg_size, uint32_t *start_seg,
                         uint32_t *end_seg) {
  *start_seg = target_addr / seg_size;
  *end_seg = ((target_addr + target_len - 1) / seg_size) + 1;

  FPRINTF(stderr,
          "SPI flash target Addr 0x%x and len 0x%x with segment size 0x%x "
          "is in start_seg %d to end_seg %d\n",
          target_addr, target_len, seg_size, *start_seg, *end_seg);
}

void sjtag_dump_data(char *client, uint32_t addr, uint8_t *data, int32_t len) {
#define BUF_CHUNK_SIZE 256

  int ii = 0, jj = 0;
  char *buf = NULL;
  uint32_t buf_size = 0;
  uint32_t avail = 0;

  for (ii = 0; ii < len; ii++) {

    if (avail < 32) {
      buf = realloc(buf, buf_size + BUF_CHUNK_SIZE);

      if (!buf) {
        FPRINTF(stderr, "%s: Unable to allocate memory size %d\n", client,
                buf_size + BUF_CHUNK_SIZE);
        return;
      }
      buf_size += BUF_CHUNK_SIZE;
      avail += BUF_CHUNK_SIZE;
    }

    if (!(ii % 16)) {
      avail -= snprintf(buf - avail + buf_size, avail, " \n%06X: ", addr + ii);
    }

    avail -= snprintf(buf - avail + buf_size, avail, "%02X ", data[ii]);

    if (!((ii + 1) % 16)) {

      avail -= snprintf(buf - avail + buf_size, avail, "      ");

      for (jj = 15; jj >= 0; jj--) {

        if (data[ii - jj] >= '0' && data[ii - jj] <= 'Z') {
          avail -= snprintf(buf - avail + buf_size, avail, "%c", data[ii - jj]);
        } else {
          avail -= snprintf(buf - avail + buf_size, avail, "-");
        }
      }
    }
  }

  avail -= snprintf(buf - avail + buf_size, avail, "\n");
  buf[buf_size - avail] = '\0';

  FPRINTF(stderr, "%s - SPI Data: %s\n", client, buf);

  free(buf);

  return;
}

/*
 * API to Read SPI Flash Memory
 *
 * INPUT:
 *  addr     - SPI Flash Memory Address
 *  data     - pointer to buffer to hold data
 *  data_len - Length of data in bytes to read
 *
 *  OUTPUT:
 *   data    - SPI Flash data value when read success
 *   err_msg - in case there are errors in processing
 *
 * Returns 0 when SPI Flash Memory read success
 *  otherwise - error code with error message
 */
uint8_t iofpga_sjtag_flash_read_operation(spi_cfi_t *cfi, uint32_t addr,
                                                 uint8_t *data,
                                                 uint32_t data_len,
                                                 char *err_msg,
                                                 uint32_t msg_size) {
  uint8_t rc = 0;
  uint16_t page_size;
  uint32_t sector_size;
  uint32_t start_sec;
  uint32_t end_sec;

  FPRINTF(stderr, "Read SPI Flash Memory addr 0x%x data_len 0x%x\n", addr,
          data_len);

  if (!data) {
    snprintf(err_msg, msg_size, "data - Null ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag memory read has invalid "
             "device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    return (ENODEV);
  }

  sector_size = cfi->sector_size;

  if (sector_size == 0 || sector_size > IOFPGA_SPI_MAX_SECTOR_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW cann't handle sector_size %d "
             "bigger than expected %d",
             sector_size, IOFPGA_SPI_MAX_SECTOR_SIZE);
    return (EINVAL);
  }

  /* A segment of size data_len, start at addr are within these sectors */
  sjtag_segment_range(addr, data_len, sector_size, &start_sec, &end_sec);

  page_size = cfi->page_size;

  if (page_size == 0 || page_size > IOFPGA_SJTAG_PAGE_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW can't handle page_size %d "
             "bigger than expected %d",
             page_size, IOFPGA_SJTAG_PAGE_SIZE);
    return (EINVAL);
  }

  if (err_msg && msg_size) {
      *err_msg = 0;
  }
  rc = sjtag_read(cfi, addr, data, data_len, err_msg, msg_size);

  if (rc != 0) {
    /* Can't use err_msg here as a parameter that writes into err_msg */
    /*
    snprintf(err_msg, msg_size,
             "Failed to Read SPI Flash Memory addr 0x%x "
             "data_len %d [%s]",
             addr, data_len, err_msg);
    */
    
    if (err_msg && msg_size && !*err_msg) {
        snprintf(err_msg, msg_size,
                 "Failed to Read SPI Flash Memory addr 0x%x "
                 "data_len %d",
                 addr, data_len);
    }
    return rc;
  }

  FPRINTF(stderr, "Success to Read SPI Flash Memory addr 0x%08x data_len %d\n",
          addr, data_len);
#ifdef DEBUG
  sjtag_dump_data("SPI Flash Memory read", addr, data, data_len);
#endif

  return rc;
}


void fpd_version_str (fpd_version_t *ver,
                 char          *str,
                 size_t        str_len)
{
    if (str == NULL) {
        return;
    }
    if (str_len < FPD_VER_STR_LEN) {
        *str = '\0';
        return;
    }

    str_len = snprintf(str, FPD_VER_STR_LEN-1, "%d.%d",
                           ver->major,
                           ver->minor);
    str[str_len] = '\0';
}

/*
 * Service routine to Sets SPI Flash Data into FPGA sjtag block rfifo register
 *
 * Returns 0 on Success
 *         otherwise - Error code with message
 */
static uint8_t sjtag_write_fifo(uint8_t *data, uint16_t data_len, char *err_msg,
                                uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t ii, jj;
  uint32_t wfifo_reg;
  uint16_t max_dwords = 0;
  uint16_t max_bytes = 0;
  uint32_t value = 0;

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  if (!data) {
    snprintf(err_msg, msg_size, "data - Null Ptr");
    return EINVAL;
  }
  /* check fpga lib sjtag write capabality */
  if (data_len > IOFPGA_SJTAG_WDATA_SIZE) {
    snprintf(err_msg, msg_size, "couldn't write more than %d expected %d Bytes",
             IOFPGA_SJTAG_WDATA_SIZE, data_len);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  max_dwords = data_len / 4;
  max_bytes = data_len % 4;

  wfifo_reg = SJTAG_BLOCK_OFFSET + offsetof(fpgalib_sjtag_regs_t, cfgspi_reg) +
              offsetof(fpgalib_sjtag_cfgspi_reg_t, fpga_spi_data);

  /*
   * Set first n-1 dwords
   */
  for (ii = 0; ii < max_dwords; ii++) {
    value = 0;
    for (jj = 0; jj < 4; jj++) {
      value |= (*(data + (ii * 4) + jj) << (jj * 8));
    }

    /* Endian-ness conversion requirement:
     * SJ SPI interface needs Endian-ness conversion
     * and hence the need for new Register Access functions for
     * reading and programming SPI flash memory.
     * Use register access functions appropriately while adding
     * support for new SJ SPI interface as all SJ devices may not
     * need swapping.
     */
    value = htobe32(value);

    rc = iofpga_reg_write_access("sjtag write fifo", wfifo_reg, value, err_msg,
                                 msg_size);

    if (rc != 0) {
      snprintf(err_msg, msg_size, "Failed to write wfifo register %d", rc);
      FPRINTF(stderr, "%s\n", err_msg);
      return rc;
    }
  }

  /*
   * Get the last bytes
   */
  if (max_bytes) {
    value = 0;
    for (jj = 0; jj < 4; jj++) {
      value |= (*(data + (ii * 4) + jj) << (jj * 8));
    }

    value = htobe32(value);

    rc = iofpga_reg_write_access("sjtag write fifo", wfifo_reg, value, err_msg,
                                 msg_size);
    if (rc != 0) {
      snprintf(err_msg, msg_size, "Failed to write wfifo register %d", rc);
      FPRINTF(stderr, "%s\n", err_msg);
      return rc;
    }
  }

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return (0);
}

/*
 * Service routine to WRITE data into SPI Flash memory.
 * INPUT:
 *  cfi       - SPI Common Flash Interface Data
 *  addr      - SPI Flash memory Address
 *  data      - Pointer to data that to be written into SPI Flash Memory
 *  data_len  - How many bytes need to be written into given addr
 *  err_msg   - Error message buffer
 *  msg_size  - Error message buffer size
 *
 * Returns 0 when read from SPI Flash
 *  otherwise - error code with message
 */
static uint8_t sjtag_page_write(spi_cfi_t *cfi, uint32_t addr, uint8_t *data,
                                uint16_t data_len, char *err_msg,
                                uint32_t msg_size) {
  uint8_t rc = 0;
  fpgalib_spi_cmd_t cmd = {0};
  uint8_t flash_wr_ena = 0;

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
    return EINVAL;
  }

  if (!data) {
    snprintf(err_msg, msg_size, "data - Null Ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag WRITE PAGE has invalid device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "%s\n", err_msg);
    return (ENODEV);
  }

  /* check sjtag FPGA Block write capabality */
  if (data_len > IOFPGA_SJTAG_WDATA_SIZE) {
    snprintf(err_msg, msg_size,
             "sjtag couldn't write more than %d expected %d Bytes",
             IOFPGA_SJTAG_RDATA_SIZE, data_len);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  /*
   * Check whether Bank Read/Write Register opcodes are valid and
   * non-zero values. If yes, proceed with Bank Address Write
   * Register otherwise skip writing to Bank Address Register
   * (addr >> 24) - write 4 byte value to Bank register and
   * 3 bytes to addr opcode register
   */
  if (cfi->wr_ear && cfi->rd_ear) {
    rc = sjtag_spi_write_bank_addr(cfi, (addr >> 24), err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "fail to set bank addr: [%s]\n", err_msg);
      return rc;
    }

    rc = sjtag_spi_read_bank_addr(cfi, (addr >> 24), err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "bank addr mismatch: [%s]\n", err_msg);
      return rc;
    }
  }

  /* SPI Flash write enable */
  rc = sjtag_flash_wr_ena_dis(cfi, 1, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to Enable SPI Flash write: [%s]\n", err_msg);
    return rc;
  }

  /*
   * Now onwards need to disable SPI Flash write before leaving the function
   */
  flash_wr_ena = 1;

  /* Wait for SPI device to be ready */
  rc = sjtag_wait_till_fsr_ready(cfi, cfi->spi_delay_factor, err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr,
            "SPI Flash device is not ready to "
            "READ SPI Flash Memory %s\n",
            err_msg);
    return rc;
  }

  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_PROGRAM_MEM, &cmd,
                                    err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command: [%s]\n", err_msg);
    goto clean_exit;
  }
#ifdef DEBUG
  sjtag_dump_data("SPI Flash WRITE PAGE", addr, data, data_len);
#endif

  /* Write data into sjtag FPGA Block wfifo */
  rc = sjtag_write_fifo(data, data_len, err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to write into sjtag FPGA Block wfifo: [%s]\n",
            err_msg);
    goto clean_exit;
  }

  /* Set opcode and register address */
  rc = sjtag_op_addr_reg_set(cmd.opcode, addr, /* SPI Flash Addr */
                             err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to set opcode and register: [%s]\n", err_msg);
    goto clean_exit;
  }

  /* Set control register */
  rc = sjtag_ctl_reg_set(cfi, &cmd, data_len, /* data length in bytes */
                         err_msg, msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to set controller instruction [%s]\n", err_msg);
    goto clean_exit;
  }

  /* Check for sjtag FPGA Block transaction complete */
  rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                             msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Couldn't complete the SPI Flash transaction: [%s]\n",
            err_msg);
    goto clean_exit;
  }

  FPRINTF(stderr,
          "Success writing into SPI Flash Memory address 0x%x of len 0x%x\n",
          addr, data_len);

  rc = sjtag_wait_till_wip(cfi, cfi->spi_delay_factor, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr,
            "SPI Flash device is not ready to READ SPI Flash Memory [%s]\n",
            err_msg);
    return rc;
  }

clean_exit:

  /* SPI Flash write disable */
  if (flash_wr_ena) {
    char tmp_buf[128];
    uint8_t rc1;

    /* SPI Flash write enable */
    rc1 = sjtag_flash_wr_ena_dis(cfi, 0, tmp_buf, sizeof(tmp_buf));
    if (rc1 != 0) {
      FPRINTF(stderr, "Failed to Disable SPI Flash write: [%s]\n", tmp_buf);
    }
  }
  FPRINTF(stderr, "%s end\n", __FUNCTION__);

  return 0;
}

/*
 * Service routine to restore the pages erased as part of unaligned sector
 *
 * INPUT:
 *  cfi             - SPI Common Flash Interface Data
 *  addr            - SPI Flash memory Address
 *  data_len        - Length in bytes
 *  first_sec_data  - first sector data
 *  last_sec_data   - last sector data
 *  erase_size_on_first_sec - Data length erase on 1st sector
 *  erase_size_on_last_sec  - Data length erase on last sector
 *  err_msg         - Error message buffer
 *  msg_size        - Error message buffer size
 *
 * Returns 0 when fail to restore the erased data
 *  otherwise - error code with message
 */
static uint8_t sjtag_flash_restore_unalign_sector_erase(
    spi_cfi_t *cfi, uint32_t addr, uint32_t data_len, uint8_t *first_sec_data,
    uint8_t *last_sec_data, uint32_t erase_size_on_first_sec,
    uint32_t erase_size_on_last_sec, char *err_msg, uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t page_size = 0;
  uint32_t start_pg = 0;
  uint32_t end_pg = 0;
  uint32_t ii = 0;
  uint8_t *this_data = NULL;
  uint32_t start_wr_addr = 0;
  uint32_t start_sec = 0;
  uint32_t end_sec = 0;

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  page_size = cfi->page_size;
  if (page_size == 0 || page_size > IOFPGA_SJTAG_PAGE_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW cann't handle page_size %d "
             "bigger than FPGALIB_SJTAG_PAGE_SIZE %d or Zero",
             page_size, IOFPGA_SJTAG_PAGE_SIZE);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  /*
   * Get the start end sector details
   */
  sjtag_segment_range(addr, data_len, cfi->sector_size, &start_sec, &end_sec);

  /*
   * Get the number of pages per sector
   */
  sjtag_segment_range(0, cfi->sector_size, page_size, &start_pg, &end_pg);

  for (ii = start_pg; ((ii < end_pg) && erase_size_on_first_sec); ii++) {

    start_wr_addr = (start_sec * cfi->sector_size) + (ii * page_size);

    /*
     * Skip writing the pages part of erase in the first sector
     */
    if ((start_wr_addr >= addr) &&
        ((start_wr_addr + page_size) <= (addr + data_len))) {
      continue;
    }

    /* Start point of written data */
    this_data = &first_sec_data[ii * page_size];

    rc = sjtag_page_write(cfi, start_wr_addr, this_data, page_size, err_msg,
                          msg_size);

    if (rc != 0) {
      FPRINTF(stderr,
              "Failed to program do 1st sector %d, sector_size"
              " %d, page %d, page_size %d [%s]",
              start_sec, cfi->sector_size, ii, page_size, err_msg);
      return rc;
    }
  }

  for (ii = start_pg; ((ii < end_pg) && erase_size_on_last_sec); ii++) {

    start_wr_addr = ((end_sec - 1) * cfi->sector_size) + (ii * page_size);

    /*
     * Skip writing the pages part of erase in the last sector
     */
    if ((start_wr_addr >= addr) &&
        ((start_wr_addr + page_size) <= (addr + data_len))) {
      continue;
    }

    /* Start point of written data */
    this_data = &last_sec_data[ii * page_size];

    rc = sjtag_page_write(cfi, start_wr_addr, this_data, page_size, err_msg,
                          msg_size);

    if (rc != 0) {
      FPRINTF(stderr,
              "Failed to program do 1st sector %d, sector_size"
              " %d, page %d, page_size %d [%s]",
              start_sec, cfi->sector_size, ii, page_size, err_msg);
      return rc;
    }
  }

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return rc;
}

/*
 * Service routine to read and save the first and last sector of
 * segment("addr" to "addr" + "data_len") of SPI Flash memory for later use.
 *
 * INPUT:
 *  cfi        - SPI Common Flash Interface Data
 *  addr       - SPI Flash memory Address
 *  data_len   - Length in bytes
 *  save_data0 - pointer to save first secotor data
 *  save_data1 - pointer to save last secotor data
 *  err_msg    - Error message buffer
 *  msg_size   - Error message buffer size
 *
 * Returns 0 when read from SPI Flash
 *  otherwise - error code with message
 */
static uint8_t sjtag_flash_program_init(spi_cfi_t *cfi, uint32_t addr,
                                        uint32_t data_len, uint8_t *save_data0,
                                        uint8_t *save_data1, char *err_msg,
                                        uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t sector_size;
  uint32_t start_sec;
  uint32_t end_sec;
  uint32_t start_rd_addr;

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - NULL ptr");
    return EINVAL;
  }
  if (!save_data0) {
    snprintf(err_msg, msg_size, "save_data0 - NULL ptr");
    return EINVAL;
  }
  if (!save_data1) {
    snprintf(err_msg, msg_size, "save_data1 - NULL ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag program init has invalid device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "%s\n", err_msg);
    return (ENODEV);
  }

  sector_size = cfi->sector_size;

  if (sector_size == 0 || sector_size > IOFPGA_SPI_MAX_SECTOR_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW cann't handle sector_size %d "
             "bigger than expected %d",
             sector_size, IOFPGA_SPI_MAX_SECTOR_SIZE);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  /*
   * A segment of size data_len, start at addr are within these sectors
   */
  sjtag_segment_range(addr, data_len, sector_size, &start_sec, &end_sec);

  /*
   * NOTE: SPIFLASH is operated on sector and page alignment
   * Erase operation is needed before any program
   * The request "addr" may be in the middle of the first sector
   * The request "data_len" may be ended in the mid of the last sector
   * First sector and last sector can be the same sector
   * This function will erase more than the request "data_len";
   * adjustment is needed, so the returned result is exactly the request
   */

  /*
   * Read 1st and last sector and save it for later use (sector alignment).
   * These parts must be restored: (1st and last sector can be the same)
   * - Top part before "addr" of 1st sector
   * - Bottom part after "addr"+"data_len" of last sector
   */
  start_rd_addr = start_sec * sector_size; /* sector alignment */

  /*
   * READ 1st sector
   */
  rc = sjtag_read(cfi, start_rd_addr, save_data0, sector_size, err_msg,
                  msg_size);

  if (rc != 0) {
    FPRINTF(stderr,
            "Failed to read 1st sector start_sec %d "
            "sector_size %d [%s]\n",
            start_sec, sector_size, err_msg);
    return rc;
  }

  /* READ last sector only if applicable */
  if ((end_sec - start_sec) > 1) {
    start_rd_addr = (end_sec - 1) * sector_size; /* sector alignment */

    rc = sjtag_read(cfi, start_rd_addr, save_data1, sector_size, err_msg,
                    msg_size);
    if (rc != 0) {
      FPRINTF(stderr,
              "Failed to read last sector start_sec %d "
              "sector_size %d [%s]\n",
              end_sec - 1, sector_size, err_msg);
      return rc;
    }
  }

  FPRINTF(stderr,
          "Success initialize SPI Flash program "
          "addr 0x%x data_len 0x%x\n",
          addr, data_len);
  return (0);
}
/*
 * Service routine to ERASE data from SPI Flash memory.
 * INPUT:
 *  cfi       - SPI Common Flash Interface Data
 *  addr      - SPI Flash memory Address
 *  data_len  - How many bytes need to be erased
 *  err_msg   - Error message buffer
 *  msg_size  - Error message buffer size
 *
 * Returns 0 when read from SPI Flash
 *  otherwise - error code with message
 */
static uint8_t sjtag_erase(spi_cfi_t *cfi, uint32_t addr, uint32_t data_len,
                           char *err_msg, uint32_t msg_size) {
  uint8_t rc = 0;
  uint8_t flash_wr_ena = 0;
  uint32_t ii;
  uint32_t sector_size;
  uint32_t start_sec;
  uint32_t end_sec;
  fpgalib_spi_cmd_t cmd = {0};

  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - Null Ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag erase has invalid "
             "device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "%s\n", err_msg);
    return (ENODEV);
  }

  /* Check SPI FLASH supports erase operation or not */
  if (!cfi->support_erase) {
    snprintf(err_msg, msg_size, "SPI Flash ERASE operation is not supported");
    FPRINTF(stderr, "%s\n", err_msg);
    return 0;
  }

  /*
   * Erase addr and size should be inter sector aligned
   */
  if (addr % cfi->sector_size || (addr + data_len) % cfi->sector_size) {
    snprintf(err_msg, msg_size,
             "SPI Flash ERASE operation for unaligned sector "
             "addr 0x%x data_len %d sector_size %d",
             addr, data_len, cfi->sector_size);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  sector_size = cfi->sector_size;

  /* A segment of data_len, start at addr are within these sectors */
  sjtag_segment_range(addr, data_len, sector_size, &start_sec, &end_sec);

  /*
   * Check whether Bank Read/Write Register opcodes are valid and
   * non-zero values. If yes, proceed with Bank Address Write
   * Register otherwise skip writing to Bank Address Register
   * (addr >> 24) - write 4 byte value to Bank register and
   * 3 bytes to addr opcode register
   */
  if (cfi->wr_ear && cfi->rd_ear) {
    rc = sjtag_spi_write_bank_addr(cfi, (addr >> 24), err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "fail to set bank addr: [%s]\n", err_msg);
      return rc;
    }

    rc = sjtag_spi_read_bank_addr(cfi, (addr >> 24), err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "bank addr mismatch, [%s]\n", err_msg);
      return rc;
    }
  }

  rc = spiflash_prepare_spi_command(cfi, SPIFLASH_OPER_ERASE_MEM, &cmd, err_msg,
                                    msg_size);
  if (rc != 0) {
    FPRINTF(stderr, "Failed to prepare command [%s]\n", err_msg);
    return rc;
  }

  for (ii = start_sec; ii < end_sec; ii++) {
    /* SPI Flash write enable */
    rc = sjtag_flash_wr_ena_dis(cfi, 1, err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "Failed to Enable SPI Flash write [%s]\n", err_msg);
      goto clean_exit;
    }

    /*
     * Now onwards need to disable SPI Flash write before leaving the function
     */
    flash_wr_ena = 1;

    /* Wait for SPI device to be ready */
    rc = sjtag_wait_till_fsr_ready(cfi, cfi->spi_delay_factor, err_msg,
                                   msg_size);

    if (rc != 0) {
      FPRINTF(stderr,
              "SPI Flash device is not ready to "
              "READ SPI Flash Memory [%s]\n",
              err_msg);
      return rc;
    }

    /* Set opcode and register address */
    rc = sjtag_op_addr_reg_set(cmd.opcode,
                               (ii * sector_size), /* SPI Flash Addr */
                               err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "Failed to set opcode and register [%s]\n", err_msg);
      goto clean_exit;
    }

    /* Set control register */
    rc = sjtag_ctl_reg_set(cfi, &cmd, 0, /* No data */
                           err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "Failed to set controller instruction [%s]\n", err_msg);
      goto clean_exit;
    }

    /* Check for sjtag FPGA Block transaction complete */
    rc = sjtag_wait_until_transaction_complete(cfi->spi_delay_factor, err_msg,
                                               msg_size);

    if (rc != 0) {
      FPRINTF(stderr, "Couldn't complete the SPI Flash transaction [%s]\n",
              err_msg);
      goto clean_exit;
    }

    rc = sjtag_wait_till_wip(cfi, cfi->spi_delay_factor, err_msg, msg_size);

    if (rc != 0) {
      FPRINTF(stderr,
              "SPI Flash device is not ready to READ SPI Flash Memory [%s]\n",
              err_msg);
      return rc;
    }

    FPRINTF(stderr, "Success ERASE SPI Flash Sector %d\n", ii);
  }

  FPRINTF(stderr, "Success ERASE SPI Flash Memory addr 0x%x data_len 0x%x\n",
          addr, data_len);

clean_exit:

  /* SPI Flash write disable */
  if (flash_wr_ena) {
    char tmp_buf[128];
    uint8_t rc1;

    /* SPI Flash write enable */
    rc1 = sjtag_flash_wr_ena_dis(cfi, 0, tmp_buf, sizeof(tmp_buf));
    if (rc1 != 0) {
      FPRINTF(stderr, "Failed to Disable SPI Flash write [%s]\n", tmp_buf);
    }
  }

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return 0;
}

/*
 * Service routine to erase the sectors of segment("addr" to "addr"+"data_len")
 * of SPI Flash memory.
 *
 * INPUT:
 *  cfi       - SPI Common Flash Interface Data
 *  addr      - SPI Flash memory Address
 *  data_len  - Length in bytes
 *  cb_ctx    - context for program progress callback
 *  program_progress_cb - callback function pointer for program progress
 *  err_msg   - Error message buffer
 *  msg_size  - Error message buffer size
 *
 * Returns 0 when SPI Flash memory erased
 *  otherwise - error code with message
 */
static uint8_t sjtag_flash_program_erase(
    spi_cfi_t *cfi, uint32_t addr, uint32_t data_len, void *cb_ctx,
    void (*program_progress_cb)(void *cb_ctx, uint8_t percent), char *err_msg,
    uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t sector_size;
  uint32_t start_sec;
  uint32_t end_sec;
  uint32_t start_wr_addr;
  uint32_t sum_err;
  uint8_t wdata[IOFPGA_SPI_MAX_SECTOR_SIZE];
  uint32_t ii, jj;
  uint32_t num_sec_report_prog;
  uint8_t *first_sec_data = NULL;
  uint8_t *last_sec_data = NULL;
  uint32_t aligned_offset = 0;
  uint32_t aligned_offset_size = 0;
  uint32_t erase_size_on_first_sec = 0;
  uint32_t erase_size_on_last_sec = 0;
  uint32_t idx = 0;
  uint8_t unaligned_sector = 0;

  printf("Start erase SPI flash all sectors...\n");
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi- NULL ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag program erase has invalid device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "%s\n", err_msg);
    return (ENODEV);
  }

  sector_size = cfi->sector_size;

  if (sector_size == 0 || sector_size > IOFPGA_SPI_MAX_SECTOR_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW cann't handle sector_size %d "
             "bigger than expected %d",
             sector_size, IOFPGA_SPI_MAX_SECTOR_SIZE);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  /*
   * A segment of size data_len, start at addr are within these sectors
   */
  sjtag_segment_range(addr, data_len, sector_size, &start_sec, &end_sec);

  num_sec_report_prog = (end_sec - start_sec) / (SPI_PROGRAM_ERASE_PERCENT /
                                                 SPI_PROGRAM_REPORT_INTERVAL);
  if (((end_sec - start_sec) %
       (SPI_PROGRAM_ERASE_PERCENT / SPI_PROGRAM_REPORT_INTERVAL)) != 0) {
    num_sec_report_prog++;
  }

  /*
   * Erase addr and size should be inter sector aligned
   */
  if (addr % cfi->sector_size || (addr + data_len) % cfi->sector_size) {

    /*
     * Get the nearest aligned offset and data erase on first sec
     * and last sec
     */
    aligned_offset = ((addr) - (addr % sector_size));
    aligned_offset_size = ((aligned_offset + sector_size) - addr);
    erase_size_on_first_sec =
        (data_len > aligned_offset_size) ? aligned_offset_size : data_len;
    erase_size_on_last_sec = (data_len - erase_size_on_first_sec) % sector_size;

    /*
     * If complete sector erased then we don't need to re-write.
     * 1st sector. So re-calculate
     */
    erase_size_on_first_sec = erase_size_on_first_sec % sector_size;

    FPRINTF(stderr,
            "unalign sector erase, addr 0x%08x data_len 0x%x "
            "aligned offset 0x%08x erase size on first sector 0x%x "
            "erase size on last sec 0x%x\n",
            addr, data_len, aligned_offset, erase_size_on_first_sec,
            erase_size_on_last_sec);

    /*
     * Allocate the memory to store the first and last sector data
     */
    first_sec_data = malloc(cfi->sector_size);
    if (first_sec_data == NULL) {
      snprintf(err_msg, msg_size, "sjtag failed to allocate memory '%s'",
               strerror(errno));
      FPRINTF(stderr, "%s\n", err_msg);
      rc = ENOMEM;
      goto clean_exit;
    }

    last_sec_data = malloc(cfi->sector_size);
    if (last_sec_data == NULL) {
      snprintf(err_msg, msg_size, "sjtag failed to allocate memory '%s'",
               strerror(errno));
      FPRINTF(stderr, "%s\n", err_msg);
      rc = ENOMEM;
      goto clean_exit;
    }

    /*
     * Save the 1st and last sector data before erasing it,
     * So it can merging with new data to restore
     */
    rc = sjtag_flash_program_init(cfi, addr, data_len, first_sec_data,
                                  last_sec_data, err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr,
              "addr 0x%08x data_len 0x%x"
              "Failed to read 1st and last secotor data to "
              "re-store the data erased as part of "
              " unaligned sector [%s]\n",
              addr, data_len, err_msg);
      goto clean_exit;
    }

    /*
     * Fill the 0xFF on the erased location on the stored buffer
     */
    for (idx = 0; idx < erase_size_on_first_sec; idx++) {
      first_sec_data[(addr - aligned_offset) + idx] = 0xFF;
    }

    for (idx = 0; idx < erase_size_on_last_sec; idx++) {
      last_sec_data[idx] = 0xFF;
    }

    unaligned_sector = 1;
  }

  /* Erase and verify all sectors. Stop on error */
  for (ii = start_sec; ii < end_sec; ii++) {
    start_wr_addr = ii * sector_size;

    rc = sjtag_erase(cfi, start_wr_addr, sector_size, err_msg, msg_size);

    if (rc != 0) {
      FPRINTF(stderr, "Failed to Erase sector %d sector_size %d [%s]\n", ii,
              sector_size, err_msg);
      goto clean_exit;
    }

    rc = sjtag_read(cfi, start_wr_addr, wdata, sector_size, err_msg, msg_size);

    if (rc != 0) {
      FPRINTF(stderr, "Failed to Read sector %d sector_size %d [%s]\n", ii,
              sector_size, err_msg);
      goto clean_exit;
    }

#ifdef DEBUG
    sjtag_dump_data("AFTER ERASE: SPI Flash Memory read", addr, wdata,
                    sector_size);
#endif

    /* Verification--Read back and compare to FF */
    for (jj = 0, sum_err = 0; jj < sector_size; jj++) {
      if (wdata[jj] != 0xFF) {
        sum_err++;
      }
    }

    if (sum_err) {
      snprintf(err_msg, msg_size,
               "Could not ERASE sector %d sector_size 0x%x - there are "
               "%d errors",
               ii, sector_size, sum_err);
      FPRINTF(stderr, "%s\n", err_msg);
      rc = EFAULT;
      goto clean_exit;
    }

    if (program_progress_cb) {

      if (ii == (end_sec - 1)) {
        program_progress_cb(cb_ctx, SPI_PROGRAM_ERASE_PERCENT);

      } else {
        if ((ii > start_sec) && ((ii - start_sec) % num_sec_report_prog) == 0) {
          uint8_t sec_blk = (ii - start_sec) / num_sec_report_prog;
          program_progress_cb(cb_ctx, SPI_PROGRAM_REPORT_INTERVAL * sec_blk);
        }
      }
    }
  }

  printf("Success to erase SPI flash "
         "addr 0x%x data_len 0x%x\n",
          addr, data_len);

  if (unaligned_sector == 1) {
    rc = sjtag_flash_restore_unalign_sector_erase(
        cfi, addr, data_len, first_sec_data, last_sec_data,
        erase_size_on_first_sec, erase_size_on_last_sec, err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr, "Failed to Read sector %d, sector_size %d [%s]\n", ii,
              sector_size, err_msg);
      goto clean_exit;
    }
  }

clean_exit:

  if (first_sec_data) {
    free(first_sec_data);
  }

  if (last_sec_data) {
    free(last_sec_data);
  }

  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return rc;
}

/*
 * Service routine to merge the previously stored data with new data then
 * program the sectors of segment("addr" to "addr" + "data_len") of
 * SPI Flash memory.
 *
 * INPUT:
 *  cfi        - SPI Common Flash Interface Data
 *  addr       - SPI Flash memory Address
 *  data       - new data
 *  data_len   - Length in bytes
 *  save_data0 - pointer to saved first secotor data
 *  save_data1 - pointer to saved last secotor data
 *  cb_ctx     - context for program progress callback
 *  program_progress_cb - callback function pointer for program progress
 *  err_msg    - Error message buffer
 *  msg_size   - Error message buffer size
 *
 * Returns 0 when SPI Flash memory programmed
 *  otherwise - error code with message
 */
static uint8_t sjtag_flash_program_do(
    spi_cfi_t *cfi, uint32_t addr, uint8_t *data, uint32_t data_len,
    uint8_t *save_data0, uint8_t *save_data1, void *cb_ctx,
    void (*program_progress_cb)(void *cb_ctx, uint8_t percent), char *err_msg,
    uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t sector_size;
  uint32_t start_sec;
  uint32_t end_sec;
  uint32_t start_wr_addr;
  uint32_t page_size;
  uint32_t start_pg;
  uint32_t end_pg;
  uint32_t i_start_idx = 0;
  uint32_t i_align_idx = 0;
  uint32_t w_start_idx = 0;
  uint32_t w_end_idx = 0;
  uint8_t wdata[IOFPGA_SPI_MAX_SECTOR_SIZE];
  uint8_t *this_data;
  uint32_t ii, jj, max_ii;
  uint32_t num_pg_report_prog, curr_pg;
  uint8_t pg_blk;

  printf("SPI flash memory program start\n");
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - NULL ptr");
    return EINVAL;
  }
  if (!data) {
    snprintf(err_msg, msg_size, "data - NULL ptr");
    return EINVAL;
  }
  if (!save_data0) {
    snprintf(err_msg, msg_size, "save_data0 - NULL ptr");
    return EINVAL;
  }
  if (!save_data1) {
    snprintf(err_msg, msg_size, "save_data1 - NULL ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag program do has invalid device instance %d(%d) ",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "%s\n", err_msg);
    return (ENODEV);
  }

  sector_size = cfi->sector_size;

  if (sector_size == 0 || sector_size > IOFPGA_SPI_MAX_SECTOR_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW cann't handle sector_size %d "
             "bigger than expected %d",
             sector_size, IOFPGA_SPI_MAX_SECTOR_SIZE);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  /* A segment of size data_len, start at addr are within these sectors */
  sjtag_segment_range(addr, data_len, sector_size, &start_sec, &end_sec);

  page_size = cfi->page_size;

  if (page_size == 0 || page_size > IOFPGA_SJTAG_PAGE_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW can't handle page_size %d "
             "bigger than expected %d",
             page_size, IOFPGA_SJTAG_PAGE_SIZE);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  /* Number of pages per sector */
  sjtag_segment_range(0, sector_size, page_size, &start_pg, &end_pg);

  /*
   * Handle first sector
   * Prepare wdata[] for 1st sector
   */
  this_data = (uint8_t *)data; // Original input buffer

  /*
   * Copy over the original data, so the untouched data can be restored.
   * The untouched data in this case, can be both top part and
   * bottom part of this sector.
   */
  for (ii = 0; ii < sector_size; ii++) {
    wdata[ii] = save_data0[ii];
  }

  /*
   * Keep the untouched data (top part and/or bottom part of the sector).
   * We need to fill in new data for location
   * [addr to (addr + data_len) or the end of this sector
   */
  max_ii = (addr % sector_size) + data_len;

  if (max_ii > sector_size) {
    max_ii = sector_size;
  }

  i_start_idx = 0;
  /* i_end_idx   = max_ii - (addr % sector_size); */
  w_start_idx = (addr % sector_size);
  w_end_idx = max_ii;

  i_align_idx = w_end_idx - w_start_idx; // track for alignment in input buf

  for (ii = w_start_idx; ii < w_end_idx; ii++) {
    wdata[ii] = this_data[ii - (addr % sector_size)];
  }

  /* Write the whole sector, 1 page at a time */
  this_data = &wdata[0];

#ifdef DEBUG
  sjtag_dump_data("Input data dump", addr, this_data, data_len);
#endif

  start_wr_addr = start_sec * sector_size;

  num_pg_report_prog = (end_sec - start_sec) * (end_pg - start_pg) /
                       (SPI_PROGRAM_DO_PERCENT / SPI_PROGRAM_REPORT_INTERVAL);
  if (((end_sec - start_sec) * (end_pg - start_pg) %
       (SPI_PROGRAM_DO_PERCENT / SPI_PROGRAM_REPORT_INTERVAL)) != 0) {
    num_pg_report_prog++;
  }

  for (ii = start_pg; ii < end_pg; ii++) {
    /* Actual location */
    start_wr_addr = start_sec * sector_size + ii * page_size;

    /* Start point of written data */
    this_data = &wdata[ii * page_size];

    rc = sjtag_page_write(cfi, start_wr_addr, this_data, page_size, err_msg,
                          msg_size);

    if (rc != 0) {
      FPRINTF(stderr,
              "Failed to program do 1st sector %d "
              "sector_size %d page %d page_size %d %s\n",
              start_sec, sector_size, ii, page_size, err_msg);
      return rc;
    }

    if (program_progress_cb) {

      curr_pg = (ii - start_pg);
      if (curr_pg > 0 && (curr_pg % num_pg_report_prog) == 0) {
        pg_blk = curr_pg / num_pg_report_prog;
        program_progress_cb(cb_ctx, SPI_PROGRAM_REPORT_INTERVAL * pg_blk +
                                        SPI_PROGRAM_ERASE_PERCENT);
      }
    }
  }
  printf("First sector done\n");

  /** DONE -- Handle first sector **/

  /** Special case: "addr"+"data_len" is in only 1 sector **/
  if ((end_sec - start_sec) == 1) {
    rc = 0;
    goto done;
  }

  printf("Start programming middle sectors... (will take about 90 seconds)\n");
  /* Handle all middle sectors */
  for (jj = (start_sec + 1); jj < (end_sec - 1); jj++) {
    /* Prepare wdata[] for all middle sectors */
    this_data = (uint8_t *)data; // Original input buffer

    i_start_idx = (jj - start_sec - 1) * sector_size + i_align_idx;
    /* i_end_idx   = i_start_idx + sector_size; */

    w_start_idx = 0;
    w_end_idx = sector_size;

    for (ii = w_start_idx; ii < w_end_idx; ii++) {
      wdata[ii] = this_data[i_start_idx + ii];
    }

    /* Write the whole sector, 1 page at a time */
    this_data = &wdata[0];

    for (ii = start_pg; ii < end_pg; ii++) {
      /* Actual location */
      start_wr_addr = jj * sector_size + ii * page_size;
      this_data = &wdata[ii * page_size]; /* Starting write addr */

      rc = sjtag_page_write(cfi, start_wr_addr, this_data, page_size, err_msg,
                            msg_size);

      if (rc != 0) {
        FPRINTF(stderr,
                "Failed to program do middle sector "
                "%d sector_size %d page %d page_size %d [%s]\n",
                jj, sector_size, ii, page_size, err_msg);

        return rc;
      }

      if (program_progress_cb) {

        curr_pg = (jj - start_sec) * (end_pg - start_pg) + (ii - start_pg);
        if (curr_pg > 0 && (curr_pg % num_pg_report_prog) == 0) {
          pg_blk = curr_pg / num_pg_report_prog;
          program_progress_cb(cb_ctx, SPI_PROGRAM_REPORT_INTERVAL * pg_blk +
                                          SPI_PROGRAM_ERASE_PERCENT);
        }
      }
    }
  }
  /* DONE -- Handle all middle sectors */
  printf("All middle sectors done\n");

  /*
   * Handle last sector
   * Prepare wdata[] for last sector
   */
  this_data = (uint8_t *)data;

  /*
   * Copy over the untouched data, so it can be restored.
   * Keep the untouched data (bottom part of the sector).
   * We need to fill in new data (from inpu_buf) for location
   * [0 to (addr+data_len)%sector_size)
   */
  for (ii = 0; ii < sector_size; ii++) {
    wdata[ii] = save_data1[ii];
  }

  /*
   * The last sector to program is either whole sector or less a sector
   * Middle-sector-programming doesn't cover this last sector
   * even it's a whole sector size data to program
   */
  max_ii = (addr + data_len) % sector_size;

  if (max_ii == 0) { // Case of whole last sector used
    max_ii = sector_size;
  }

  i_start_idx = (end_sec - start_sec - 2) * sector_size + i_align_idx;
  /* i_end_idx   = i_start_idx + max_ii; */

  w_start_idx = 0;
  w_end_idx = max_ii;

  for (ii = w_start_idx; ii < w_end_idx; ii++) {
    wdata[ii] = this_data[i_start_idx + ii];
  }

  /*
   * Write the whole sector, 1 page at a time
   */
  this_data = &wdata[0];

  for (ii = start_pg; ii < end_pg; ii++) {
    start_wr_addr = (end_sec - 1) * sector_size + ii * page_size;
    this_data = &wdata[ii * page_size]; /* Start point of written data */

    rc = sjtag_page_write(cfi, start_wr_addr, this_data, page_size, err_msg,
                          msg_size);

    if (rc != 0) {
      FPRINTF(stderr,
              "Failed to program do last sector %d "
              "sector_size %d page %d page_size %d [%s]\n",
              end_sec - 1, sector_size, ii, page_size, err_msg);

      return rc;
    }

    if (program_progress_cb) {

      if (ii == end_pg - 1) {
        program_progress_cb(cb_ctx,
                            SPI_PROGRAM_DO_PERCENT + SPI_PROGRAM_ERASE_PERCENT);
      } else {
        curr_pg =
            (end_sec - start_sec - 1) * (end_pg - start_pg) + (ii - start_pg);
        if (curr_pg > 0 && (curr_pg % num_pg_report_prog) == 0) {
          pg_blk = curr_pg / num_pg_report_prog;
          program_progress_cb(cb_ctx, SPI_PROGRAM_REPORT_INTERVAL * pg_blk +
                                          SPI_PROGRAM_ERASE_PERCENT);
        }
      }
    }
  }
  /** DONE -- Handle last sector **/

done:
  printf("Success to SPI flash program all sectors "
         "addr 0x%x data_len 0x%x\n",
          addr, data_len);

  return 0;
}

/*
 * Service routine to verify the data written into SPI Flash memory
 * to program the sectors of segment("addr" to "addr" + "data_len") of
 * SPI Flash memory.
 *
 * INPUT:
 *  cfi       - SPI Common Flash Interface Data
 *  addr      - SPI Flash memory Address
 *  data      - pointer to data buffer
 *  data_len  - Length in bytes
 *  cb_ctx    - context for program progress callback
 *  program_progress_cb - callback function pointer for program progress
 *  err_msg   - Error message buffer
 *  msg_size  - Error message buffer size
 *
 * Returns 0 when SPI Flash verification correct
 *  otherwise - error code with message
 */
static uint8_t sjtag_flash_program_verify(
    spi_cfi_t *cfi, uint32_t addr, uint8_t *data, uint32_t data_len,
    void *cb_ctx, void (*program_progress_cb)(void *cb_ctx, uint8_t percent),
    char *err_msg, uint32_t msg_size) {
  uint8_t rc = 0;
  uint32_t sector_size;
  uint32_t start_sec;
  uint32_t end_sec;
  uint32_t start_rd_addr;
  uint32_t page_size;
  uint32_t start_idx = 0;
  uint32_t end_idx = 0;
  uint32_t i_align_idx = 0;
  uint8_t wdata[IOFPGA_SPI_MAX_SECTOR_SIZE];
  uint8_t *this_data;
  uint32_t this_size;
  uint32_t start_this;
  uint32_t end_this;
  uint32_t ii, jj;
  uint32_t sum_err;
  uint32_t num_sec_report_prog;

  printf("SPI flash verification start...\n");
  if (!cfi) {
    snprintf(err_msg, msg_size, "cfi - NULL ptr");
    return EINVAL;
  }
  if (!data) {
    snprintf(err_msg, msg_size, "data - NULL ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag program verify has invalid "
             "device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "%s\n", err_msg);
    return (ENODEV);
  }

  sector_size = cfi->sector_size;

  if (sector_size == 0 || sector_size > IOFPGA_SPI_MAX_SECTOR_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW cann't handle sector_size %d "
             "bigger than expected %d",
             sector_size, IOFPGA_SPI_MAX_SECTOR_SIZE);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  /* A segment of size data_len, start at addr are within these sectors */
  sjtag_segment_range(addr, data_len, sector_size, &start_sec, &end_sec);

  page_size = cfi->page_size;

  if (page_size == 0 || page_size > IOFPGA_SJTAG_PAGE_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW can't handle page_size %d "
             "bigger than expected %d",
             page_size, IOFPGA_SJTAG_PAGE_SIZE);
    FPRINTF(stderr, "%s\n", err_msg);
    return (EINVAL);
  }

  /* Need to verify all sectors */
  this_size = sector_size;
  start_this = start_sec;
  end_this = end_sec;

  num_sec_report_prog = (end_sec - start_sec) / (SPI_PROGRAM_VERIFY_PERCENT /
                                                 SPI_PROGRAM_REPORT_INTERVAL);
  if (((end_sec - start_sec) %
       (SPI_PROGRAM_VERIFY_PERCENT / SPI_PROGRAM_REPORT_INTERVAL)) != 0) {
    num_sec_report_prog++;
  }

  /*
   * Read and verify all target segments (pages or sectors). Stop on error
   */
  this_data = (uint8_t *)data; // Original input buffer

  for (ii = start_this; ii < end_this; ii++) {
    start_rd_addr = ii * this_size;

    rc = sjtag_read(cfi, start_rd_addr, wdata, this_size, err_msg, msg_size);

    if (rc != 0) {
      FPRINTF(stderr, "Failed to Read sector %d sector_size %d [%s]\n", ii,
              sector_size, err_msg);
      return rc;
    }

    /*
     * Verification -- Read back and compare with original input
     */

    /* Handle first segment */
    if (ii == start_this) {
      start_idx = (addr % this_size);
      end_idx = (addr % this_size) + data_len;

      if (end_idx > this_size) {
        end_idx = this_size;
      }

      i_align_idx = end_idx - start_idx; // track for alignment

      for (jj = start_idx, sum_err = 0; jj < end_idx; jj++) {
        if (wdata[jj] != this_data[jj - start_idx]) {
          sum_err++;
          FPRINTF(stderr,
                  "Verification failed at first "
                  "sector %d size %d idx %d Flash data 0x%x != "
                  "Actual data 0x%x\n",
                  ii, this_size, jj, wdata[jj], this_data[jj - start_idx]);
          break;
        }
      }

      if (sum_err) {
        snprintf(err_msg, msg_size,
                 "Program verification failed at first sector %d "
                 "sector_size %d - there are %d errors",
                 ii, sector_size, sum_err);
        FPRINTF(stderr, "%s\n", err_msg);
        return (EFAULT);
      }

      /* Special case: "addr"+"data_len" is in only 1 segment */
      if ((end_this - start_this) == 1) {
        rc = 0;
        goto done;
      }

    } else if (ii == (end_this - 1)) { /** Handle last sector **/
      start_idx = 0;
      end_idx = (addr + data_len) % this_size;

      for (jj = start_idx, sum_err = 0; jj < end_idx; jj++) {
        if (wdata[jj] != this_data[(end_this - start_this - 2) * this_size +
                                   i_align_idx + jj]) {
          sum_err++;
          FPRINTF(stderr,
                  "Verification failed at "
                  "last sector %d size %d idx %d Flash data 0x%x != "
                  "Actual data 0x%x\n",
                  ii, this_size, jj, wdata[jj],
                  this_data[(end_this - start_this - 2) * this_size +
                            i_align_idx + jj]);
          break;
        }
      }
      if (sum_err) {
        snprintf(err_msg, msg_size,
                 "Program verification failed at last sector %d "
                 "sector_size %d - there are %d errors",
                 ii, sector_size, sum_err);
        FPRINTF(stderr, "%s\n", err_msg);
        return (EFAULT);
      }
    } else { /** Handle all middle sectors **/
      start_idx = 0;
      end_idx = this_size;
      for (jj = start_idx, sum_err = 0; jj < end_idx; jj++) {
        if (wdata[jj] !=
            this_data[(ii - start_this - 1) * this_size + i_align_idx + jj]) {
          sum_err++;
          FPRINTF(
              stderr,
              "Verification failed at middle"
              " sector %d size %d idx %d Flash data 0x%x != "
              "Actual data 0x%x\n",
              ii, this_size, jj, wdata[jj],
              this_data[(ii - start_this - 1) * this_size + i_align_idx + jj]);
          break;
        }
      }

      if (sum_err) {
        snprintf(err_msg, msg_size,
                 "Program verification failed at middle sector %d "
                 "sector_size %d - there are %d errors",
                 ii, sector_size, sum_err);
        FPRINTF(stderr, "%s\n", err_msg);
        return (EFAULT);
      }
    }

    if (program_progress_cb) {

      if (ii == (end_sec - 1)) {
        program_progress_cb(cb_ctx, SPI_PROGRAM_ERASE_PERCENT +
                                        SPI_PROGRAM_DO_PERCENT +
                                        SPI_PROGRAM_VERIFY_PERCENT);

      } else {
        if ((ii > start_sec) && ((ii - start_sec) % num_sec_report_prog) == 0) {
          uint8_t sec_blk = (ii - start_sec) / num_sec_report_prog;
          program_progress_cb(
              cb_ctx, SPI_PROGRAM_ERASE_PERCENT + SPI_PROGRAM_DO_PERCENT +
                          SPI_PROGRAM_REPORT_INTERVAL * sec_blk);
        }
      }
    }
  }

done:
  printf("Success to SPI flash verify all sectors "
         "addr 0x%x data_len 0x%x\n",
          addr, data_len);
  return 0;
}

/*
 * API to Program SPI Flash Memory
 *
 * INPUT:
 *  ctx      - opaque sjtag FPGA IP Block context
 *  addr     - SPI Flash Memory Address
 *  data     - pointer to data that need to be written info SPI Flash
 *  data_len - Length of data in bytes to read
 *  cb_ctx   - context for program progress callback
 *  program_progress_cb - callback function pointer for program progress
 *  err_msg  - Error message buffer
 *  msg_size - Error message buffer size
 *
 *  OUTPUT:
 *   err_msg - Error message when failed
 *
 * Returns 0 when SPI Flash Memory Program success
 *  otherwise - error code with message
 */
static uint8_t fpgalib_sjtag_flash_program_operation(
    spi_cfi_t *cfi, uint32_t addr, uint8_t *data, uint32_t data_len,
    void *cb_ctx, void (*program_progress_cb)(void *cb_ctx, uint8_t percent),
    char *err_msg, uint32_t msg_size) {
  uint8_t rc = 0;

  /* Hold 1st original sector data */
  uint8_t *save_data0_ptr = NULL;

  /* Hold last original sector data */
  uint8_t *save_data1_ptr = NULL;

  uint32_t sector_size;

  FPRINTF(stderr,
          "Program SPI Flash Memory "
          "addr 0x%x data_len 0x%x\n",
          addr, data_len);

  if (!data) {
    snprintf(err_msg, msg_size, "data - NULL ptr");
    return EINVAL;
  }

  /* max spi devices on sjtag controller */
  if (cfi->dev_inst > IOFPGA_SJTAG_MAX_DEV) {
    snprintf(err_msg, msg_size,
             "sjtag Program Flash Memory has invalid "
             "device instance %d(%d)",
             cfi->dev_inst, IOFPGA_SJTAG_MAX_DEV);
    FPRINTF(stderr, "%s\n", err_msg);
    return (ENODEV);
  }

  sector_size = cfi->sector_size;

  if (sector_size == 0 || sector_size > IOFPGA_SPI_MAX_SECTOR_SIZE) {
    snprintf(err_msg, msg_size,
             "Internal SW cann't handle sector_size %d "
             "bigger than expected %d",
             sector_size, IOFPGA_SPI_MAX_SECTOR_SIZE);
    printf("%s\n", err_msg);
    return (EINVAL);
  }

  save_data0_ptr = malloc(sector_size);
  if (save_data0_ptr == NULL) {
    snprintf(err_msg, msg_size,
             "SPI Flash Memory PROGRAM operation "
             "failed to allocate memory [%s]",
             strerror(errno));
    FPRINTF(stderr, "%s\n", err_msg);
    rc = ENOMEM;
    goto clean_exit;
  }

  save_data1_ptr = malloc(sector_size);
  if (save_data1_ptr == NULL) {
    snprintf(err_msg, msg_size,
             "SPI Flash Memory PROGRAM operation "
             "failed to allocate memory [%s]",
             strerror(errno));
    FPRINTF(stderr, "%s\n", err_msg);
    rc = ENOMEM;
    goto clean_exit;
  }

  /*
   * Save the 1st and last sector data before erasing it,
   * So it can merging with new data to restore
   */
  rc = sjtag_flash_program_init(cfi, addr, data_len, save_data0_ptr,
                                save_data1_ptr, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr,
            "Failed to Initialize 1st and last secotor"
            " data to program SPI Flash Memory %s\n",
            err_msg);
    goto clean_exit;
  }

  /*
   * Erase and verify all sectors. Stop on error
   */
  rc = sjtag_flash_program_erase(cfi, addr, data_len, cb_ctx,
                                 program_progress_cb, err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to Erase SPI Flash Memory to rogram %s\n", err_msg);
    goto clean_exit;
  }

  /*
   * Program SPI Flash Memory after merging new data with buffered data
   */
  rc = sjtag_flash_program_do(cfi, addr, data, data_len, save_data0_ptr,
                              save_data1_ptr, cb_ctx, program_progress_cb,
                              err_msg, msg_size);

  if (rc != 0) {
    FPRINTF(stderr, "Failed to Program new data into SPI Flash Memory %s\n",
            err_msg);
    goto clean_exit;
  }

  /*
   * Verify the new data
   */
  if (cfi->verify_flag) {
    rc = sjtag_flash_program_verify(cfi, addr, data, data_len, cb_ctx,
                                    program_progress_cb, err_msg, msg_size);
    if (rc != 0) {
      FPRINTF(stderr,
              "Verification failed to program "
              "new data into SPI Flash Memory %s\n",
              err_msg);
      goto clean_exit;
    }
  }

  FPRINTF(stderr,
          "Success to Program SPI Flash Memory addr 0x%x data_len 0x%x\n", addr,
          data_len);

clean_exit:

  if (save_data0_ptr) {
    free(save_data0_ptr);
  }

  if (save_data1_ptr) {
    free(save_data1_ptr);
  }

  return rc;
}

int iofpga_image_write(char *image_path, uint32_t fpga_image_offset, uint32_t fpga_image_size,
                       uint32_t metadata_offset, uint32_t metadata_size,
                       char *err_msg, uint32_t msg_size) {
  uint8_t *image, *mdata;
  uint32_t image_size, mdata_size;
  // print fpd Version
  fpd_version_t fpd_version = {0};

  printf("Iofpga image write started...\n");

  /* Extract and validate FPGA image and meta data */
  uint8_t rc =
      get_img_metadata(image_path, &image_size, (void **)&image, &mdata_size,
                       (void **)&mdata, err_msg, msg_size);
  if (rc) {
    printf("get_img_metadata failed : [%s]\n", err_msg);
    return -1;
  }

  // print the fpd version
  rc = fpd_mdata_get_fpd_version((void *)mdata, &fpd_version);
  if (rc == 0) {
    printf("major_ver:%x  minor_ver:%x\n", fpd_version.major,
            fpd_version.minor);
  }

  FPRINTF(stderr, "details :: moff %x, imgoff %x, msize %d, image-size %d\n",
          metadata_offset, fpga_image_offset, mdata_size,
          image_size);

  /* erase metadata */
  printf("Erase meta-data at offset: 0x%x\n", metadata_offset);
  // erase meta data
  rc = sjtag_flash_program_erase(&cfi, metadata_offset, metadata_size, NULL, NULL, err_msg, msg_size);
  if (rc) {
    printf("Failed to erase spi flash at offset: 0x%x. err_msg %s\n", metadata_offset, err_msg);
    return -1;
  }

  printf("Program image...\n");

  // program the image
  rc = fpgalib_sjtag_flash_program_operation(&cfi, fpga_image_offset, image,
                                             image_size, NULL, NULL, err_msg, msg_size);
  if (rc) {
    printf("Failed to program flash at offset: 0x%x. err_msg %s\n", fpga_image_offset, err_msg);
    return -1;
  }
  printf("Program image done\n");

  printf("Program meta data...\n");

  //program the meta_data
  fpgalib_sjtag_flash_program_operation(&cfi, metadata_offset, mdata,
                                        mdata_size, NULL, NULL, err_msg,
                                        msg_size);
  if (rc) {
    printf("Failed to program flash at offset: 0x%x. err_msg %s\n", metadata_offset, err_msg);
    return -1;
  }
  printf("Program meta data done\n");

  printf("Iofpga image write done\n");
  return 0;
}

uint16_t
get_iofpga_version_from_flash(const char *block_name, uint32_t mdata_offset)
{
    char err_msg[ERRBUF_SIZE] = {0};
    uint32_t msg_size = sizeof(err_msg);
    uint8_t data[IOFPGA_MDATA_SIZE] = {0};
    int rc;

    map_base = mmap_sjtag_block(block_name);
    if (!map_base) {
        fprintf(stderr, "failed to mmap block %s\n", block_name);
        return -1;
    }

    // read jedec_id and get cfi data
    rc = iofpga_get_spi_cfg(err_msg, msg_size);
    if (rc != 0) {
        printf("Failed to get spi flash config\n");
        return -1;
    }

    rc = iofpga_sjtag_flash_read_operation(&cfi, mdata_offset, data, IOFPGA_MDATA_SIZE,
                                           err_msg, msg_size);
    if (rc != 0) {
        printf("Failed to read mdata from flash. err_msg %s\n", err_msg);
        return -1;
    }

    uint8_t major = data[IOFPGA_MDATA_FPD_VERSION_OFFSET];
    uint8_t minor = data[IOFPGA_MDATA_FPD_VERSION_OFFSET + 2];
    return (major << 8) | minor;
}

int
program_iofpga(char *image_path, uint32_t image_offset, uint32_t image_size,
               uint32_t mdata_offset, uint32_t mdata_size, const char *block_name)
{
    char err_msg[ERRBUF_SIZE] = {0};
    uint32_t msg_size = sizeof(err_msg);
    int rc = 0;

    printf("fpga_image_offset = 0x%x\n"
           "fpga_image_size   = 0x%x\n"
           "fpga_mdata_offset = 0x%x\n"
           "fpga_mdata_size   = 0x%x\n"
           "block_name        = %s\n",
            image_offset, image_size,
            mdata_offset, mdata_size,
            block_name);

    map_base = mmap_sjtag_block(block_name);
    if (!map_base) {
        fprintf(stderr, "failed to mmap block %s\n", block_name);
        return -1;
    }

    // read jedec_id
    rc = iofpga_get_spi_cfg(err_msg, msg_size);
    if (rc != 0) {
        printf("Failed to get spi flash config\n");
        return -1;
    }

    // write image
    rc = iofpga_image_write(image_path, image_offset, image_size,
                            mdata_offset, mdata_size, err_msg, msg_size);
    if (rc != 0) {
        printf("Failed to write to the SPI flash\n");
        return -1;
    }

    return 0;
}

int
erase_iofpga(uint32_t image_offset, uint32_t image_size,
             uint32_t mdata_offset, uint32_t mdata_size, const char *block_name)
{
    char err_msg[ERRBUF_SIZE] = {0};
    uint32_t msg_size = sizeof(err_msg);
    int rc = 0;

    printf("fpga_image_offset = 0x%x\n"
           "fpga_image_size   = 0x%x\n"
           "fpga_mdata_offset = 0x%x\n"
           "fpga_mdata_size   = 0x%x\n"
           "block_name        = %s\n",
            image_offset, image_size,
            mdata_offset, mdata_size,
            block_name);

    map_base = mmap_sjtag_block(block_name);
    if (!map_base) {
        fprintf(stderr, "failed to mmap block %s\n", block_name);
        return -1;
    }

    // read jedec_id
    rc = iofpga_get_spi_cfg(err_msg, msg_size);
    if (rc != 0) {
        printf("Failed to get spi flash config\n");
        return -1;
    }

    /* erase metadata */
    printf("Erase meta-data at offset: 0x%x\n", mdata_offset);
    rc = sjtag_flash_program_erase(&cfi, mdata_offset, mdata_size, NULL, NULL, err_msg, msg_size);
    if (rc) {
        printf("Failed to erase spi flash at offset: 0x%x. err_msg %s\n", mdata_offset, err_msg);
        return -1;
    }

    /* erase image */
    printf("Erase image at offset: 0x%x\n", image_offset);
    rc = sjtag_flash_program_erase(&cfi, image_offset, image_size, NULL, NULL, err_msg, msg_size);
    if (rc) {
        printf("Failed to erase spi flash at offset: 0x%x. err_msg %s\n", image_offset, err_msg);
        return -1;
    }
    return 0;
}
