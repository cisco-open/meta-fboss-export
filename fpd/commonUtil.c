/*
 * commonUtil.c
 *
 * Copyright (c) 2022 by Cisco Systems, Inc.
 * All rights reserved.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/statvfs.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stddef.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <zlib.h>
#include "commonUtil.h"

#define FPRINTF(out, ...) //fprintf(out, __VA_ARGS__)

#define PRINT_ERROR                                                            \
  do {                                                                         \
    FPRINTF(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,         \
            __FILE__, errno, strerror(errno));                                 \
    return;                                                                    \
  } while (0)


typedef uint32_t cpa_status_t;

int
get_img_data(const char *path, void **data, uint32_t *data_size,
             char *err_msg, uint32_t msg_size)
{
    struct stat st;
    int rc = 0;
    FILE *fp;
    uint32_t size;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        snprintf(err_msg, msg_size, "Failed to open file path %s", path);
        return EINVAL;
    }

    rc = stat(path, &st);
    if (rc) {
        snprintf(err_msg, msg_size, "stat failed");
        return EINVAL;
    }

    size = st.st_size;
    *data_size = size;

    /* allocate memory for metadata */
    *data = (uint8_t *)calloc(1, size);
    if (*data == NULL) {
        snprintf(err_msg, msg_size, "failed to allocate memory for metadata");
        return ENOMEM;
    }

    fseek(fp, 0, SEEK_SET);

    /* read meta data */
    rc = fread(*data, size, 1, fp);
    if (rc == 0) {
        FPRINTF(stderr, "fread %d\n", rc);
        fclose(fp);
        free(*data);
        snprintf(err_msg, msg_size, "failed to read mdata");
        return errno;
    }
    fclose(fp);

    return 0;
}

int
get_img_metadata(const char *path, uint32_t *img_size, void **img,
                 uint32_t *mdata_size, void **mdata, char *err_msg,
                 uint32_t msg_size)
{
  struct stat st;
  uint8_t *image;
  uint8_t *metadata;
  int rc;
  FILE *fp;
  uint32_t size;
  fpd_meta_data_t fpd_mdata;

  fp = fopen(path, "rb");
  if (fp == NULL) {
    snprintf(err_msg, msg_size, "Failed to open file path %s", path);
    return EINVAL;
  }

  rc = stat(path, &st);
  if (rc) {
    snprintf(err_msg, msg_size, "stat failed");
    return EINVAL;
  }

  size = st.st_size;

  rc = fread(&fpd_mdata, sizeof(fpd_meta_data_t), 1, fp);
  if (rc == 0) {
    snprintf(err_msg, msg_size, "failed to read mdata");
    fclose(fp);
    return errno;
  }

  switch (fpd_mdata.hdr.u.v1.metadata_version) {
  case FPD_META_DATA_VER_1:
    *mdata_size = fpd_mdata.hdr.u.v1.metadata_size;
    break;
  case FPD_META_DATA_VER_2:
    *mdata_size = fpd_mdata.hdr.u.v2.metadata_size;
    break;
  case FPD_META_DATA_VER_3:
    *mdata_size = fpd_mdata.hdr.u.v3.metadata_size;
    break;
  default:
    *mdata_size = fpd_mdata.hdr.u.v1.metadata_size;
    break;
  }

  FPRINTF(stderr, "mdata size is  %d\n", *mdata_size);

  if (size < *mdata_size) {
    snprintf(err_msg, msg_size, "Size of file < mdata size");
    fclose(fp);
    return EINVAL;
  }

  /* allocate memory for metadata */
  metadata = (uint8_t *)calloc(1, *mdata_size);
  if (metadata == NULL) {
    snprintf(err_msg, msg_size, "failed to allocate memory for metadata");
    return ENOMEM;
  }

  fseek(fp, 0, SEEK_SET);

  /* read meta data */
  rc = fread(metadata, *mdata_size, 1, fp);
  if (rc == 0) {
    fclose(fp);
    snprintf(err_msg, msg_size, "failed to read mdata");
    return errno;
  }
  *mdata = metadata;

  size -= *mdata_size;
  *img_size = size;

  /* allocate memory for image */
  image = (uint8_t *)calloc(size, 1);
  if (image == NULL) {
    snprintf(err_msg, msg_size, "failed to allocate memory for image buffer");
    fclose(fp);
    return -1;
  }  

  /* read image */
  rc = fread(image, sizeof(char), size, fp);
  if (rc == 0) {
    snprintf(err_msg, msg_size, "failed to read image");
    fclose(fp);
    return errno;
  }

  *img = image;

  fclose(fp);

  return 0;
}

int
get_data_info(void *data, fpd_meta_info_t *fpd_meta,
              char *err_msg, uint32_t msg_size)
{
    fpd_meta_data_t *fpd_mdata = data;
    int rc = 0;

    switch (fpd_mdata->hdr.u.v1.metadata_version) {
    case FPD_META_DATA_VER_1:
        fpd_meta->mdata = data;
        fpd_meta->mdata_size = fpd_mdata->hdr.u.v1.metadata_size;

        fpd_mdata_data_v1_t *data_v1 =
            (fpd_mdata_data_v1_t *)((uint8_t *)(data) +
                                    sizeof(fpd_mdata_hdr_v1_t));
        fpd_meta->img_msize = data_v1->object_size;
        fpd_meta->img = data + fpd_meta->mdata_size;
        fpd_meta->mver = FPD_META_DATA_VER_1;
        fpd_meta->version_string = data_v1->object_version_str;
        break;
    case FPD_META_DATA_VER_2:
        fpd_meta->mdata = data;
        fpd_meta->mdata_size = fpd_mdata->hdr.u.v2.metadata_size;

        fpd_mdata_data_v2_t *data_v2 =
            (fpd_mdata_data_v2_t *)((uint8_t *)(data) +
                                    sizeof(fpd_mdata_hdr_v2_t));
        fpd_meta->img_msize = data_v2->fw_size;
        fpd_meta->img = data + fpd_meta->mdata_size;
        fpd_meta->mver = FPD_META_DATA_VER_2;
        fpd_meta->version = data_v2->fpd_ver;
        fpd_meta->flags = data_v2->user_flags;
        fpd_meta->flags = data_v2->user_flags;
        fpd_meta->version_string = data_v2->fpd_ver_str;
        fpd_meta->md5 = data_v2->fw_md5;

        if (data_v2->fw_type == FPD_FW_TYPE_GZIP) {
            fpd_meta->compressed = 1;
        } else {
            fpd_meta->compressed = 0;
        }
        break;
    case FPD_META_DATA_VER_3:
        fpd_meta->mdata = data;
        fpd_meta->mdata_size = fpd_mdata->hdr.u.v3.metadata_size;

        fpd_mdata_data_v3_t *data_v3 =
            (fpd_mdata_data_v3_t *)((uint8_t *)(data) +
                                    sizeof(fpd_mdata_hdr_v3_t));
        fpd_meta->img_msize = data_v3->fw_size;
        fpd_meta->img = data + fpd_meta->mdata_size;
        fpd_meta->mver = FPD_META_DATA_VER_3;
        fpd_meta->version = data_v3->fpd_ver;
        fpd_meta->flags = data_v3->user_flags;
        fpd_meta->flags = data_v3->user_flags;
        fpd_meta->version_string = data_v3->fpd_ver_str;
        fpd_meta->md5 = data_v3->fw_md5;

        if (data_v3->fw_type == FPD_FW_TYPE_GZIP) {
            fpd_meta->compressed = 1;
        } else {
            fpd_meta->compressed = 0;
        }
        break;
    default:
        FPRINTF(stderr, "Only support version 3 version is %d\n",
            fpd_mdata->hdr.u.v1.metadata_version);
        return ENODATA;
        break;
    }

    rc = get_image_lists(fpd_meta->mdata, fpd_meta->mdata_size,
                         &fpd_meta->pid_size, &fpd_meta->pid_list,
                         &fpd_meta->name_size, &fpd_meta->name_list);
    if (rc) {
        FPRINTF(stderr, "Unable to get pid/name lists %d\n", rc);
        return rc;
    }
    return 0;

}

int
img_inflate(fpd_meta_info_t *fpd_meta, void **data,
              char *err_msg, uint32_t msg_size)
{
    int rc;
    z_stream d_stream;
    uint32_t size = fpd_meta->img_msize;

    if (!fpd_meta->compressed) {
        *data = fpd_meta->img;
        return 0;
    }
    *data = calloc(size, 1);

    memset(&d_stream, 0, sizeof(z_stream));
    d_stream.avail_in = fpd_meta->img_size;
    d_stream.next_in = fpd_meta->img;
    d_stream.next_out = *data;
    d_stream.avail_out = fpd_meta->img_msize;

    rc = inflateInit(&d_stream);

    if (rc) {
        FPRINTF(stderr, "inflateInit %d\n", rc);
        free(*data);
        return rc;
    }
    while (1) {
        rc = inflate(&d_stream, Z_NO_FLUSH);
        if (rc != Z_OK) {
            break;
        }
    }

    if (rc != Z_STREAM_END){
        FPRINTF(stderr, "Stream end failure %d\n", rc);
        free(*data);
        return rc;
    }
    rc = inflateEnd(&d_stream);
    if (rc) {
        FPRINTF(stderr, "inflateEnd failed %d\n", rc);
        free(*data);
    }
    return rc;
}

int
get_imgs_info(const char *name, fpd_imgs_t **imgs,
              char *err_msg, uint32_t msg_size)
{
    uint32_t *magic;
    fpd_imgs_t *fpd_imgs;
    void *data;
    uint32_t data_size;
    int rc;

    rc = get_img_data(name, &data, &data_size, err_msg, msg_size);
    if (rc) {
        FPRINTF(stderr, "get_data_info failed %d\n", rc);
        return rc;
    }
    magic = data;
    if (*magic == FPD_FILE_LIST_MAGIC) {
        fpd_images_hdr_v1_t *fip = data;
        uint32_t offset = fip->hdr_size;
        int i;
        *imgs = calloc(sizeof(fpd_imgs_t) +
             fip->num_images * sizeof(fpd_meta_info_t), 1);
        fpd_imgs = *imgs;
        fpd_imgs->magic = *magic;
        fpd_imgs->num_imgs = fip->num_images;
        for (i = 0; i < fip->num_images; i++) {
            fpd_imgs->meta[i].img_size = fip->image_sizes[i];
            fpd_imgs->meta[i].img = data + offset;
            offset += fip->image_sizes[i];
            rc = get_data_info(fpd_imgs->meta[i].img, &fpd_imgs->meta[i],
                               err_msg, msg_size);
            if (rc) {
                FPRINTF(stderr, "get_data_info failed %d\n", rc);
                free(*imgs);
                return rc;
            }
            fpd_imgs->meta[0].img_size = fip->image_sizes[i] -
                fpd_imgs->meta[0].mdata_size;
        }
    } else {
        *imgs = calloc(sizeof(fpd_imgs_t) + sizeof(fpd_meta_info_t), 1);
        fpd_imgs = *imgs;
        fpd_imgs->num_imgs = 1;
        fpd_imgs->meta[0].img = data;
        rc = get_data_info(fpd_imgs->meta[0].img, &fpd_imgs->meta[0],
                           err_msg, msg_size);
        if (rc) {
            FPRINTF(stderr, "get_data_info failed %d\n", rc);
            free(*imgs);
            return rc;
        }
        fpd_imgs->meta[0].img_size = data_size - fpd_imgs->meta[0].mdata_size;
    }
    fpd_imgs->name = name;
    fpd_imgs->size = data_size;
    *imgs = fpd_imgs;
    return rc;
}

int
fpd_find_img(fpd_imgs_t *fpd_imgs, const char *pid, char *name, char *name2)
{
    fpd_meta_info_t **match_list = calloc(sizeof(fpd_meta_info_t *),
           fpd_imgs->num_imgs);
    uint32_t match_count = 0;
    int i, j;

    for (i = 0; i < fpd_imgs->num_imgs; i++) {
        fpd_meta_info_t *info = &fpd_imgs->meta[i];
        if (pid) {
            for (j = 0; j < info->pid_size; j++) {
                if (!strncmp(info->pid_list[j], pid, S_IDPROM_PRODUCT_ID_MAX+1)) {
                    info->match_flags |= PID_MATCH_FLAG;
                    break;
                }
            }
        } else {
            info->match_flags |= PID_MATCH_FLAG;
        }
        for (j = 0; j < info->name_size; j++) {
            if (!(info->match_flags & NAME_MATCH_FLAG) && name) {
                if (!strncmp(info->name_list[j], name, MAX_FPD_NAME_LEN)) {
                    info->match_flags |= NAME_MATCH_FLAG;
                }
            } else if (!name) {
                info->match_flags |= NAME_MATCH_FLAG;
            }
            if (!(info->match_flags & NAME2_MATCH_FLAG) && name2) {
                if (!strncmp(info->name_list[j], name2, MAX_FPD_NAME_LEN)) {
                    info->match_flags |= NAME2_MATCH_FLAG;
                }
            } else if (!name2) {
                info->match_flags |= NAME2_MATCH_FLAG;
            }
        }
        if ((info->match_flags & FULL_MATCH) == FULL_MATCH) {
            match_list[match_count++] = info;
        }
        FPRINTF(stdout, "%s 0x%08x\n", info->name_list[0], info->match_flags);
    }
    fpd_imgs->match_list = match_list;
    fpd_imgs->match_count = match_count;
    return match_count;
}

uint8_t
fpd_mdata_get_fpd_version(void *mdata, fpd_version_t *fpd_version)
{
  uint8_t rc = 0;
  fpd_meta_data_t *metadata = (fpd_meta_data_t *)mdata;

  if (!mdata) {
      return -1;
  }
  FPRINTF(stderr, "%s start\n", __FUNCTION__);
  switch (metadata->hdr.u.v1.metadata_version) {
  default:
    if (metadata->hdr.u.v1.magic_num != FPD_METADATA_MAGIC_NUM) {
      return -1;
    }

    fpd_mdata_data_v1_t *data_v1 =
        (fpd_mdata_data_v1_t *)((uint8_t *)(metadata) +
                                sizeof(fpd_mdata_hdr_v1_t));

    /* Version format 0 has only major and minor num
     * <minor:16bits.major:16bits>
     * Version format 1 has major, minor and debug num
     * <debug:8bits.minor:16bits.major:8bits>
     */
    if (data_v1->object_version_format == 0) {
      fpd_version->major = (data_v1->object_version >> 0) & 0xffff;
      fpd_version->minor = (data_v1->object_version >> 16) & 0xffff;
      fpd_version->debug = 0;
    } else {
      fpd_version->major = (data_v1->object_version >> 0) & 0xff;
      fpd_version->minor = (data_v1->object_version >> 8) & 0xffff;
      fpd_version->debug = (data_v1->object_version >> 24) & 0xff;
    }

    break;

  case FPD_META_DATA_VER_2:;

    if (metadata->hdr.u.v2.magic_num != FPD_METADATA_MAGIC_NUM_V2) {
      return -1;
    }
    fpd_mdata_data_v2_t *data_v2 =
        (fpd_mdata_data_v2_t *)((uint8_t *)(metadata) +
                                sizeof(fpd_mdata_hdr_v2_t));

    fpd_version->major = data_v2->fpd_ver.major;
    fpd_version->minor = data_v2->fpd_ver.minor;
    fpd_version->debug = data_v2->fpd_ver.debug;

    break;

  case FPD_META_DATA_VER_3:
    if (metadata->hdr.u.v3.magic_num != FPD_METADATA_MAGIC_NUM_V3) {
        return -1;
    }
    fpd_mdata_data_v3_t *data_v3 =
        (fpd_mdata_data_v3_t *)((uint8_t *)(metadata) +
                                sizeof(fpd_mdata_hdr_v3_t));

    fpd_version->major = data_v3->fpd_ver.major;
    fpd_version->minor = data_v3->fpd_ver.minor;
    fpd_version->debug = data_v3->fpd_ver.debug;

    break;
  }
  FPRINTF(stderr, "%s end\n", __FUNCTION__);
  return rc;
}

uint32_t
get_metadata_size(const char *file_name)
{
  
    uint8_t *image, *mdata;
    uint32_t image_size, mdata_size;
    char err_msg[ERRBUF_SIZE] = {0};
    uint32_t msg_size = sizeof(err_msg);

    /* Extract and validate FPGA image and meta data */
    int rc =
      get_img_metadata(file_name, &image_size, (void **)&image, &mdata_size,
                       (void **)&mdata, err_msg, msg_size);
    if (rc) {
        FPRINTF(stderr, "get_img_metadata failed : [%s]\n", err_msg);
        printf("get_img_metadata failed : [%s]\n", err_msg);
    }
    
    return mdata_size;
}

uint32_t get_image_version(const char *file_name) {

  uint8_t *image, *mdata;
  uint32_t image_size, mdata_size;
  // print fpd Version
  fpd_version_t fpd_version = {0};
    uint32_t fpd_image_version;
  char err_msg[ERRBUF_SIZE] = {0};
  uint32_t msg_size = sizeof(err_msg);

  /* Extract and validate FPGA image and meta data */
  int rc =
      get_img_metadata(file_name, &image_size, (void **)&image, &mdata_size,
                       (void **)&mdata, err_msg, msg_size);
  if (rc || !mdata) {
    FPRINTF(stderr, "get_img_metadata %p failed : [%s]\n", mdata, err_msg);
    printf("get_img_metadata %p failed : [%s]\n", mdata, err_msg);
  }

  // print the fpd version
  rc = fpd_mdata_get_fpd_version((void *)mdata, &fpd_version);
  if (rc == 0) {
    FPRINTF(stderr, "major_ver:%x  minor_ver:%x\n", fpd_version.major,
            fpd_version.minor);
  }
  fpd_image_version = (fpd_version.major << 16) | fpd_version.minor;
  return fpd_image_version;
}

int
get_image_lists(void *mdata, uint32_t mdata_size, 
                uint32_t *pid_size, char ***pid_list,
                uint32_t *name_size, char ***name_list)
{
    int rc = 0;
    fpd_meta_data_t *metadata = (fpd_meta_data_t *)mdata;
    uint32_t num_pid = 0;
    char **lpid_list = NULL;
    uint32_t num_name = 0;
    char **lname_list = NULL;
    uint32_t i;

    if (!mdata) {
        FPRINTF(stderr, "%s bad mdata\n", __FUNCTION__);
        return -EFAULT;
    }
    FPRINTF(stderr, "%s start\n", __FUNCTION__);
    switch (metadata->hdr.u.v1.metadata_version) {
    default:
        FPRINTF(stderr, "V1 header not supported %s\n", __FUNCTION__);
        return -EFAULT;

    case FPD_META_DATA_VER_1:;
        break;
    case FPD_META_DATA_VER_2:;
        if (metadata->hdr.u.v2.magic_num != FPD_METADATA_MAGIC_NUM_V2) {
            FPRINTF(stderr, "Magic number failed v2 %d\n", EFAULT);
            return -EFAULT;
        }
        fpd_mdata_data_v2_t *data_v2 =
            (fpd_mdata_data_v2_t *)((uint8_t *)(metadata) +
                                    sizeof(fpd_mdata_hdr_v2_t));

        num_pid = data_v2->card_info_hdr.num_card_info;
        lpid_list = calloc(sizeof(char *), num_pid);
        for (i = 0; i < num_pid; i++) {
            lpid_list[i] = data_v2->card_info[i].card_pid;
        }
        break;

    case FPD_META_DATA_VER_3:
        if (metadata->hdr.u.v3.magic_num != FPD_METADATA_MAGIC_NUM_V3) {
            FPRINTF(stderr, "Magic number failed v3 %d\n", EFAULT);
            return -EFAULT;
        }
        fpd_mdata_data_v3_t *data_v3 =
            (fpd_mdata_data_v3_t *)((uint8_t *)(metadata) +
                                    sizeof(fpd_mdata_hdr_v3_t));
        fpd_mdata_card_info_t *card = data_v3->card_info;
        fpd_mdata_name_info_t *name;

        num_pid = data_v3->card_info_hdr.num_card_info;
        lpid_list = calloc(sizeof(char *), num_pid);
        for (i = 0; i < num_pid; i++) {
            lpid_list[i] = card->card_pid;
            card++;
        }
        // card now points to name!
        name = (fpd_mdata_name_info_t *) card;

        num_name = name->num_fpd_names;
        lname_list = calloc(sizeof(char *), num_name);
        for (i = 0; i < num_name; i++) {
            lname_list[i] = name->name[i];
        }
        break;
    }
    *pid_size = num_pid;
    *name_size = num_name;
    *pid_list = lpid_list;
    *name_list = lname_list;
    FPRINTF(stderr, "%s end\n", __FUNCTION__);
    return rc;
}

void
fpd_print_meta_info(fpd_meta_info_t *fpd)
{
    int i;

    printf("mdata ver: %d size: 0x%x image ver %d.%d.%d size: 0x%x (0x%x)\n",
        fpd->mver, fpd->mdata_size,
        fpd->version.major, fpd->version.minor, fpd->version.debug,
        fpd->img_size, fpd->img_msize);
    printf(" -- flags: 0x%08x compressed: %s\n",
        fpd->flags, fpd->compressed ? "Yes" : "No");

    printf(" MD5: ");
    for (i = 0; i < MAX_MD5_DIGEST; i++) {
        printf("%02x", fpd->md5[i]);
    }
    printf("\n");
    for (i = 0; i < fpd->pid_size; i++) {
        printf(" PID:    %s\n", fpd->pid_list[i]);
    }
    for (i = 0; i < fpd->name_size; i++) {
        printf(" NAME:   %s\n", fpd->name_list[i]);
    }
}

void
fpd_print_imgs_info(fpd_imgs_t *fpd_imgs)
{
    int i;
    printf("num_images: %d image_name: %s image_size: 0x%x\n",
        fpd_imgs->num_imgs, fpd_imgs->name, fpd_imgs->size);
    printf("===================================================\n");
    for (i = 0; i < fpd_imgs->num_imgs; i++) {
        fpd_print_meta_info(&fpd_imgs->meta[i]);
        printf("===================================================\n");
    }
}

/*
 * This API allows to get output of a shell command with proper return code
 * check.
 * In case of success, it will fill in the buffer with the output of the
 * command that executed
 * In case of failure, it will fill in the err_msg with the
 * error message displayed by the shell command.
 *
 * Input Parameters:
 * char *buff: pointer to the buffer for filling with
 *                the command output
 * size_t buff_size: size of the buffer.
 * size_t *out_size: size of the command output or error string
 * char *err_msg: pointer to the error message buffere for filling with
 *                 error string returned by the
 *                shell command.
 * size_t msg_size: size of the buffer.
 * char *format: format of the command string.
 * ...: arguments for the command string format.
 *
 * Return code: CPA_STATUS_OK in case of success.
 * Return Code: CPA_STATUS_E_INTERNAL in case of failure.
 */
cpa_status_t
cpa_get_shell_cmd_output (char      *buff,
                          size_t    buff_size,
                          size_t    *out_size,
                          char      *err_msg,
                          size_t    msg_size,
                          char      *format, ...)
{
    FILE *fp;
    int rc, retval, n;
    char cmd[256];
    char line[256];
    size_t cmd_size;
    size_t read_size = sizeof(line);
    char *redirect_output_cmd = " 2>&1";
    va_list args;

    cmd_size = sizeof(cmd);
    memset(cmd, 0, cmd_size);
    va_start(args, format);
    n = vsnprintf(cmd, cmd_size, format, args);
    va_end(args);

    *out_size = 0;
    /*
     * Append the redirection of stderr to stdout, so error message can be
     * captured with the fgets() call.
     */
    if ((n + strlen(redirect_output_cmd)) < cmd_size) {
        snprintf(cmd + n, cmd_size - n, "%s", redirect_output_cmd);
    }

    fp = popen(cmd, "r");
    if (fp == NULL) {
        /*
         * pipe creation failed errno will be set
         */
        (void)snprintf(err_msg, msg_size,
                "failed to run \"%s\" command: %s", cmd,
                 strerror(errno));
        return (-1);
    }
    /*
     * Now capture the console output
     */
    while (!feof(fp)) {

        rc = ferror(fp);
        if (rc) {
            (void)snprintf(err_msg, msg_size,
                    "output capture of \"%s\" command failed: %s",
                    cmd, strerror(rc));
            pclose(fp);
            return (-1);
        }

        if (*out_size + read_size > buff_size) {
            read_size = buff_size - *out_size;
        }

        if (fgets(line, read_size, fp)) {

            *out_size += snprintf(buff + *out_size, buff_size - *out_size,
                    "%s", line);
        } else {
            break;
        }
    }
    /* Now check for command return code */
    rc = pclose(fp);
    if (rc == -1) {
        /*
         * pclose() API failed
         */
        (void)snprintf(err_msg, msg_size,
                "failed to run \"%s\" command due to "
                "pclose() failure", cmd);
        return (-1);
    } else if (WIFEXITED(rc)) {
        /*
         * command exited 'normally', its return value can be returned
         */
        retval = WEXITSTATUS(rc);
        if (retval) {
            (void)snprintf(err_msg, msg_size,
                    "failure returned by \"%s\" command "
                    "(rc = %d) - %s", cmd, retval, line);
        } else {
            return (0);
        }
    } else {
        /*
         * Wierd, command got interrupted somehow
         */
        (void)snprintf(err_msg, msg_size, "execution of \"%s\" command got "
                 "interrupted (rc = %d)", cmd, rc);
    }
    return (-1);
}

static uint64_t
get_silverbolt_phy_address()
{
    /* Get the base address of SILVERBOLT */
    char  result[128];
    const char *cmd = "lspci -vd :017a | grep 64M | awk '{print \"0x\"$3}'";

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "popen failed");
        return -1;
    }
    if (!fgets(result, sizeof(result), fp)) {
        fprintf(stderr, "failed to read from the file");
        return -1;
    }
    result[strcspn(result, "\n")] = 0;
    pclose(fp);
    return strtoull(result, NULL, 0);
}

static uint64_t
get_cyclonus_phy_address()
{
    /* Get the base address of cyclonus */
    char  result[128];
    const char *cmd = "lspci -vd :0177 | grep 4M | awk '{print \"0x\"$3}'";

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "popen failed");
        return -1;
    }
    if (!fgets(result, sizeof(result), fp)) {
        fprintf(stderr, "failed to read from the file");
        return -1;
    }
    result[strcspn(result, "\n")] = 0;
    pclose(fp);
    return strtoull(result, NULL, 0);
}

void *
get_pinpointer_block_virtual_addr(int pim, uint64_t block_offset)
{
    void *map_addr = NULL;
    uint64_t phy_base;
    uint64_t spi_base;
    phy_base = get_silverbolt_phy_address();
    if (phy_base == -1 || phy_base == EINVAL) {
        fprintf(stderr, "Failed to get silverbolt base address. (%s)\n", strerror(errno));
        return NULL;
    }
    spi_base = phy_base + (pim - 1) * SLPC_MEMORY_BASE + SLPC_MEMORY_OFFSET + block_offset;

    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    map_addr = mmap(NULL, getpagesize(),
                    PROT_READ|PROT_WRITE, MAP_SHARED, fd,
                    spi_base & ~(getpagesize() - 1));
    if (!map_addr) {
        fprintf(stderr, "mmap failed. errno %d (%s)\n", errno, strerror(errno));
        return MAP_FAILED;
    }
    uint64_t offset = spi_base & (getpagesize() - 1);
    map_addr = map_addr + offset;

    return map_addr;
}

void *
get_cyclonus_block_virtual_addr(uint64_t block_offset)
{
    void *map_addr = NULL;
    uint64_t phy_base;
    uint64_t spi_base;
    phy_base = get_cyclonus_phy_address();
    if (phy_base == -1 || phy_base == EINVAL) {
        fprintf(stderr, "Failed to get cyclonus base address. (%s)\n", strerror(errno));
        return NULL;
    }
    spi_base = phy_base + block_offset;

    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    map_addr = mmap(NULL, getpagesize(),
                    PROT_READ|PROT_WRITE, MAP_SHARED, fd,
                    spi_base & ~(getpagesize() - 1));
    if (!map_addr) {
        fprintf(stderr, "mmap failed. errno %d (%s)\n", errno, strerror(errno));
        return MAP_FAILED;
    }
    uint64_t offset = spi_base & (getpagesize() - 1);
    map_addr = map_addr + offset;
    return map_addr;
}

uint16_t
get_cpld_version(uint64_t offset, uint64_t mask,
                 uint32_t target, uint16_t rshift)
{
    uint32_t *virt_addr;
    uint32_t data;

    void *map_base = get_cyclonus_block_virtual_addr(offset);
    if (!map_base) {
        fprintf(stderr, "failed to get base virtual address at offset %llx\n", (long long)offset);
        return -1;
    }

    virt_addr = map_base + target;
    data = *virt_addr;

    uint16_t version = (data & mask) >> rshift;
    return version & 0x3F;
}

