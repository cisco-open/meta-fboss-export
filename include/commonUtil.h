/*
 * commonUtil.h
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#ifndef _COMMONFPD_UTIL_H_
#define _COMMONFPD_UTIL_H_

/*
 *  Existing CPA error codes that map to POSIX errors.
 */
#define CPA_STATUS_OK               0
#define CPA_STATUS_E_NOENT          ENOENT
#define CPA_STATUS_E_INVALID        EINVAL
#define CPA_STATUS_E_NOMEM          ENOMEM
#define CPA_STATUS_E_UNSUPPORTED    ENOTSUP
#define CPA_STATUS_E_IN_PROGRESS    EINPROGRESS
#define CPA_STATUS_E_EAGAIN         EAGAIN
#define CPA_STATUS_E_NOACCESS       EACCES
#define CPA_STATUS_E_FAULT          EFAULT
#define CPA_STATUS_E_NODEV          ENODEV
#define CPA_STATUS_E_BUSY           EBUSY
#define CPA_STATUS_E_IO_ERROR       EIO
#define CPA_STATUS_E_DEADLOCK       EDEADLK
#define CPA_STATUS_E_NOPERM         EPERM

#define MAX_FPGA_FLASH_OBJ_NAME_STR_LEN         28
#define MAX_FPGA_FLASH_OBJ_VERSION_STR_LEN      16
#define MAX_FPGA_FLASH_OBJ_BUILD_TIME_STR_LEN   24
#define MAX_FPGA_FLASH_OBJ_BUILD_USER_STR_LEN   12

#define FPD_METADATA_MAGIC_NUM      0xDDB2DDB3
#define FPD_METADATA_MAGIC_NUM_V2   0x66706431 /* fpd1 */
#define FPD_METADATA_MAGIC_NUM_V3   0x66706432 /* fpd2 */
#define FPD_FILE_LIST_MAGIC         0x696d6773 /* imgs */

#define MAX_FPD_NAME_LEN        17
#define MAX_FPD_VER_STR_LEN     16
#define MAX_BUILD_TIME_STR_LEN  24
#define MAX_BUILD_USER_STR_LEN  12
#define MAX_MD5_DIGEST              16

#define FPD_META_DATA_VER_1         1
#define FPD_META_DATA_VER_2         2
#define FPD_META_DATA_VER_3         3

#define ERRBUF_SIZE 300

#define SLPC_MEMORY_BASE            0x401000
#define SLPC_MEMORY_OFFSET          0x40100
#define PINPOINTER_SPI_BLOCK_ADDR   0x32000

typedef struct fpd_images_hdr_v1_ {
    uint32_t magic_num;
    uint16_t ver;
    uint16_t num_images;
    uint32_t hdr_size;
    uint32_t image_sizes[0];
} __attribute__((__packed__)) fpd_images_hdr_v1_t;

typedef struct fpd_mdata_hdr_v1_ {
    /*
     * Fixed signature defined in FPD_METADATA_MAGIC_NUM, used
     * to check for presence of the meta-data
     */
    uint32_t magic_num;

    /* Version of the meata-data format */
    uint8_t metadata_version;

    /*
     * Size of the fpd_meta_data_ structure, required to calculate
     * meta-data checksum as we are not including the whole 64-KByte allocated
     * for this, only the part that has the data
     */
    uint16_t metadata_size;
} __attribute__((__packed__)) fpd_mdata_hdr_v1_t;

typedef struct fpd_mdata_hdr_v2_ {
    /*
     * Fixed signature defined in FPD_METADATA_MAGIC_NUM_V2, used
     * to check for presence of the V2 meta-data
     */
    uint32_t magic_num;

    /* Version 2 of the meata-data format */
    uint8_t metadata_version;

    /*
     * Size of the fpd_meta_data_ structure, required to calculate
     * meta-data checksum
     */
    uint16_t metadata_size;

    /*
     * crc32 checksum value to check metadata integrity
     */
    uint32_t metadata_crc32;

} __attribute__((__packed__)) fpd_mdata_hdr_v2_t;

typedef struct fpd_mdata_hdr_v3_ {
    /*
     * Fixed signature defined in FPD_METADATA_MAGIC_NUM_V3, used
     * to check for presence of the V3 meta-data
     */
    uint32_t magic_num;

    /* Version 3 of the meta-data format */
    uint8_t metadata_version;

    /*
     * Size of the fpd_meta_data_ structure, required to calculate
     * meta-data checksum
     */
    uint16_t metadata_size;

    /*
     * crc32 checksum value to check metadata integrity
     */
    uint32_t metadata_crc32;

} __attribute__((__packed__)) fpd_mdata_hdr_v3_t;


typedef struct fpd_mdata_data_v1_ {
    /*
     * Defines the type as primary/upgrade vs golden/backup
     */
    uint8_t object_type;

    /*
     * A unique ID for each Chassis type Fixed/Distributed/Centralized.
     * Empty if Unused
     */
    uint8_t platform_id;
    /*
     * A unique ID to distinguish the various FPGA's on each board
     * Spitfire-F:
     * 0x1 = BMC FPGA
     * 0x2 = X86 FPGA
     * 0x3 = Sherman IOFPGA
     * 0x4 = Kangaroo IOFPGA
     * 0x5 = Pershing Base IOFPGA
     * 0x6 = Pershing Mezz MIFPGA
     * Spitfire-D:
     * 0x10 = BMC FPGA RP
     * 0x11 = BMC FPGA LC
     * 0x20 = Pembrey RP
     * 0x21 = Zenith RP
     * 0x30 = Exeter Gauntlet
     * 0x31 = Exeter Corsair
     * 0x40 = Kenley Gauntlet
     * 0x41 = Kenley Corsair
     * 0x50 = Warmwell (fan tray)
     * 0x60 = Filtion (Fabric)
     * Spitfire-C:
     * 0x1 = Altus FPGA
     * 0x2 = Kobler FPGA
     * 0x3 = Blackfish FPGA
     */
    uint8_t object_id;

    /* Unique instance number of FPGA on a board.
     * 0 if fpga ID is only one on the board. */
    uint8_t instance_num;

    /* Object version format as defined in fpga_flash_obj_ver_fmt_en */
    uint8_t object_version_format;

    /* Size of the object/image */
    uint32_t object_size;

    /*
     * The target (PCI device ID, card type, etc.) for which this FPGA is built.
     */
    uint32_t object_target_id;

    /*
     * Version of the object, the format of the version will be specified
     * via object_version_format field.
     */
    uint32_t object_version;

    /*
     * The date where the "raw/original" image was built in the following
     * format:
     * - Bits [31:16] = year
     * - Bits [15:8] = month
     * - Bits [7:0] = day
     */
    uint32_t object_build_date;

    /* Object name string */
    char object_name_str[MAX_FPGA_FLASH_OBJ_NAME_STR_LEN];

    /* Object version string */
    char object_version_str[MAX_FPGA_FLASH_OBJ_VERSION_STR_LEN];

    /* Object build date and time string */
    char object_build_time_str[MAX_FPGA_FLASH_OBJ_BUILD_TIME_STR_LEN];

    /* User ID that build the object */
    char object_build_user_str[MAX_FPGA_FLASH_OBJ_BUILD_USER_STR_LEN];

    /* CRC-32 value for the meta-data (with metadata_crc = 0) */
    uint32_t metadata_crc;

    /* CRC-32 for the object */
    uint32_t object_crc;

    /*
     * Secure Boot Signature size. If the object doesn't support secure
     * boot signature, then this value will be set to '0'.
     */
    uint32_t sb_signature_size;

    /* CRC-32 for the secure boot signature */
    uint32_t sb_signature_crc;

    /* Test area for FPD tool testing */
    uint32_t test_area;

} __attribute__((__packed__)) fpd_mdata_data_v1_t;

/*
 * Can define a minimum card hw version that
 * this FPD firmware allowed to be upgraded
 */
typedef struct fpd_card_version_ {
    uint16_t minor;
    uint16_t major;
} __attribute__((__packed__)) fpd_card_version_t;

/*
 * FPD firmware version
 */
typedef struct fpd_version_ {
    uint16_t major;
    uint16_t minor;
    uint16_t debug;
} __attribute__((__packed__)) fpd_version_t;

/*
 * Card info header
 */
typedef struct fpd_mdata_card_info_hdr_ {
    uint8_t ver;
    uint8_t num_card_info;
} __attribute__((__packed__)) fpd_mdata_card_info_hdr_t;

/*
 * Card info to define
 */
#define S_IDPROM_PRODUCT_ID_MAX  18  /* PID Max Length                 */
typedef struct fpd_mdata_card_info_ {
    char card_pid[S_IDPROM_PRODUCT_ID_MAX+1];
    uint32_t platform_id;
    fpd_card_version_t min_hw_ver;
    fpd_card_version_t max_hw_ver;
} __attribute__((__packed__)) fpd_mdata_card_info_t;

/*
 * FPD image firmware type
 */
typedef enum fpd_fw_type_ {
    FPD_FW_TYPE_PLAIN = 0,
    FPD_FW_TYPE_GZIP = 1,
} fpd_fw_type_en;

/*
 * FPD image firmware type
 */
typedef enum user_flags_ {
    FPD_DYNAMIC_NAME = 0x1,
    FPD_REL_SIGNED_IMG = 0x2,
    FPD_DEV_SIGNED_IMG = 0x4,
} user_flags_en;

typedef struct fpd_mdata_data_v2_ {
    /*
     * User flags to be used for various purposes
     */
    user_flags_en user_flags;
    /*
     * FPD name
     */
    char name[MAX_FPD_NAME_LEN];
    /*
     * FPD version
     */
    fpd_version_t fpd_ver;
    /*
     * FPD version string
     */
    char fpd_ver_str[MAX_FPD_VER_STR_LEN];
    /*
     * FPD image build data and time
     */
    char build_time_str[MAX_BUILD_TIME_STR_LEN];
    /*
     * FPD build by user
     */
    char build_user_str[MAX_BUILD_USER_STR_LEN];
    /*
     * FPD firmware whether plain binary or compressed
     */
    fpd_fw_type_en fw_type;
    /*
     * FPD firmware actual size after uncompression if it is compressed
     */
    uint32_t fw_size;
    /*
     * FPD firmware actual md5 checksum after uncompression if it is
     * compressed
     */
    uint8_t fw_md5[MAX_MD5_DIGEST];

    /*
     * Card info header
     */
    fpd_mdata_card_info_hdr_t card_info_hdr;

    /*
     * All the supported card info appended and upgrade of this firmware are
     * allowed only for those cards.
     */
    fpd_mdata_card_info_t card_info[0];
} __attribute__((__packed__)) fpd_mdata_data_v2_t;

/*
 * fpd name info to define
 */
typedef struct fpd_mdata_name_info_ {

    uint8_t num_fpd_names;

    /*
     * FPD names
     */
    char name[0][MAX_FPD_NAME_LEN];

} __attribute__((__packed__)) fpd_mdata_name_info_t;


typedef struct fpd_mdata_data_v3_ {
    /*
     * User flags to be used for various purposes
     */
    user_flags_en user_flags;
    /*
     * FPD version
     */
    fpd_version_t fpd_ver;
    /*
     * FPD version string
     */
    char fpd_ver_str[MAX_FPD_VER_STR_LEN];
    /*
     * FPD image build data and time
     */
    char build_time_str[MAX_BUILD_TIME_STR_LEN];
    /*
     * FPD build by user
     */
    char build_user_str[MAX_BUILD_USER_STR_LEN];
    /*
     * FPD firmware whether plain binary or compressed
     */
    fpd_fw_type_en fw_type;
    /*
     * FPD firmware actual size after uncompression if it is compressed
     */
    uint32_t fw_size;
    /*
     * FPD firmware actual md5 checksum after uncompression if it is
     * compressed
     */
    uint8_t fw_md5[MAX_MD5_DIGEST];

    /*
     * Card info header
     */
    fpd_mdata_card_info_hdr_t card_info_hdr;

    /*
     * All the supported card info appended and upgrade of this firmware are
     * allowed only for those cards.
     */
    fpd_mdata_card_info_t card_info[0];

    /*
     * All the supported FPD names appended and upgrade of this firmware
     * only name match in the list
     */
    fpd_mdata_name_info_t fpd_name_info[0];

} __attribute__((__packed__)) fpd_mdata_data_v3_t;


typedef struct fpd_mdata_hdr_ {
    union {
        fpd_mdata_hdr_v1_t v1;
        fpd_mdata_hdr_v2_t v2;
        fpd_mdata_hdr_v3_t v3;
    } u;
} fpd_mdata_hdr_t;

typedef struct fpd_mdata_data_ {
    union {
        fpd_mdata_data_v1_t v1;
        fpd_mdata_data_v2_t v2;
        fpd_mdata_data_v3_t v3;
    } u;
} fpd_mdata_data_t;

/*
 * A dedicated flash sector should be reserved to store the meta-data
 * information for each image that we need to track for FPD feature.
 */
typedef struct fpd_meta_data_ {
    fpd_mdata_hdr_t hdr;
    fpd_mdata_data_t data;
} __attribute__((__packed__)) fpd_meta_data_t;

#ifdef __cplusplus
extern "C" {
#endif
uint32_t get_metadata_size(const char *file_name);

uint32_t get_image_version(const char *file_name);

#define FPD_MDATA_MAX_LIST  16
#define PID_MATCH_FLAG      0x0001
#define NAME_MATCH_FLAG     0x0002
#define NAME2_MATCH_FLAG    0x0004
#define FULL_MATCH          (PID_MATCH_FLAG | NAME_MATCH_FLAG | NAME2_MATCH_FLAG)

typedef struct fpd_meta_info_t_ {
    uint32_t img_size;
    uint32_t img_msize;
    void *img;
    uint32_t mdata_size;
    void *mdata;
    uint32_t mver;

    uint32_t match_flags;
    uint32_t flags;
    fpd_version_t version;
    char *version_string;
    uint32_t compressed;
    uint32_t pid_size;
    char **pid_list;
    uint32_t name_size;
    char **name_list;
    uint8_t *md5;
} fpd_meta_info_t;

typedef struct fpd_imgs_ {
    uint32_t num_imgs;
    uint32_t size;
    uint32_t magic;
    const char *name;
    fpd_meta_info_t **match_list;
    uint32_t match_count;
    fpd_meta_info_t meta[0];
} fpd_imgs_t;

/*
 * If meta version is 2 or 3 validate version, else return error EBADR.
 * Check magic number, if good fill in size, pid_list, return 0
 * Else return EFAULT
 * meta_data: PID
 */
int
get_image_lists(void *mdata, uint32_t mdata_size,
                uint32_t *pid_size, char ***pid_list,
                uint32_t *name_size, char ***name_list);

uint16_t get_cpld_version(uint64_t block_offset, uint64_t version_mask,
                          uint32_t target, uint16_t reg_shift);

int get_img_data(const char *path, void **data, uint32_t *data_size,
                 char *err_msg, uint32_t msg_size);

int get_data_info(void *data, fpd_meta_info_t *fpd_meta,
                  char *err_msg, uint32_t msg_size);

int get_imgs_info(const char *name, fpd_imgs_t **imgs,
                  char *err_msg, uint32_t msg_size);

int get_img_metadata(const char *path, uint32_t *img_size, void **img,
                     uint32_t *mdata_size, void **mdata, char *err_msg,
                     uint32_t msg_size);

int img_inflate(fpd_meta_info_t *fpd_meta, void **data,
                char *err_msg, uint32_t msg_size);

int fpd_find_img(fpd_imgs_t *fpd_imgs, const char *pid, char *name, char *name2);

void fpd_print_meta_info(fpd_meta_info_t *fpd);
void fpd_print_imgs_info(fpd_imgs_t *fpd_imgs);

#ifdef __cplusplus
}
#endif

void * get_pinpointer_block_virtual_addr(int pim, uint64_t block_offset);

void * get_cyclonus_block_virtual_addr(uint64_t block_offset);

uint8_t fpd_mdata_get_fpd_version(void *mdata,
                                  fpd_version_t *fpd_version);

typedef uint32_t cpa_status_t;
cpa_status_t
cpa_get_shell_cmd_output (char      *buff,
                          size_t    buff_size,
                          size_t    *out_size,
                          char      *err_msg,
                          size_t    msg_size,
                          char      *format, ...);
#endif

