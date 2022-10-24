#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/statvfs.h>
#include <regex.h>
#include <string.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "ssd.h"

#define BUFF_SIZE               256
#define SSD_UPG_IMG_EXTRACT_DIR "/tmp"
#define SSD_METADATA_FILE       "/tmp/ssd_mdata.txt"
#define HDPARM_UPG_CMD          "/sbin/hdparm --fwdownload-modee"
#define HDPARM_ADD_ARGU "--yes-i-know-what-i-am-doing --please-destroy-my-drive"

#define  BUFF_SIZE     256

#define SSD_METADATA_FILE "/tmp/ssd_mdata.txt"
#define HDPARM_UPG_CMD    "/sbin/hdparm --fwdownload-modee"
#define HDPARM_ADD_ARGU   "--yes-i-know-what-i-am-doing --please-destroy-my-drive"

#define NAME_SIZE       16
#define MSG_SIZE        512

char *fpd_path = "/dev/sda";

#define FPRINTF(out, ...) //fprintf(out, __VA_ARGS__)

#define PRINT_ERROR                                                            \
  do {                                                                         \
    FPRINTF(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,         \
            __FILE__, errno, strerror(errno));                                 \
    return;                                                                    \
  } while (0)


typedef uint32_t cpa_status_t;

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
ssd_cpa_get_shell_cmd_output (char      *buff,
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

int ssd_get_img_metadata(const char *path, uint32_t *img_size, void **img,
                     uint32_t *mdata_size, void **mdata, char *err_msg,
                     uint32_t msg_size) {
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

#if IMAGE_METADATA
/*This function is for future use*/
uint32_t get_metadata_size(char *file_name){

    uint8_t *image, *mdata;
    uint32_t image_size, mdata_size;
    char err_msg[ERRBUF_SIZE] = {0};
    uint32_t msg_size = sizeof(err_msg);

    /* Extract and validate FPGA image and meta data */
    uint8_t rc =
      ssd_get_img_metadata(file_name, &image_size, (void **)&image, &mdata_size,
                       (void **)&mdata, err_msg, msg_size);
    if (rc) {
        FPRINTF(stderr, "ssd_get_img_metadata failed : [%s]\n", err_msg);
        printf("ssd_get_img_metadata failed : [%s]\n", err_msg);
    }

    return mdata_size;

}
#endif
  
/**
 * Extracts the SSD smartctl Device Info string to format the SSD FPD Name.
 * Input: smartctl output line for 'Device Model|Model Family'.
 * Output: Formated SSD FPD name in fpd_name buffer.
 */
static cpa_status_t
extract_ssd_fpd_name_from_device_model_info (const char * device_info,
                                             char *       fpd_name,
                                             size_t       name_sz,
                                             char *       err_msg,
                                             size_t       msg_size)
{
    /**
     * Get the vendor and model from the device info that we support
     *      Model Family: Intel S3520 Series SSDs
     *      Device Model: Micron_5300_MTFDDAV240TDS
     *      Model Family: Intel S4510/S4610/S4500/S4600 Series SSDs
     *      Model Family: Micron 5100 Pro / 5200 SSDs
     */
#define VENDOR_MODEL_REGEX_PATTERN	\
	"^(Model Family:|Device Model:)(\\s)+(Intel|Micron)([ " \
	"_])(S3520|5300|S4510|5100)([ _/])(.)+$"
#define NUM_GROUPS 7
#define VENDOR_IDX 3
#define MODEL_IDX 5
    regex_t regex;
    regmatch_t pmatch[NUM_GROUPS];
    regoff_t name_idx, pattern_idx;
    int rc;

    /**
     * Compile the regex for SSD Device Vendor Model
     */
    rc = regcomp(&regex, VENDOR_MODEL_REGEX_PATTERN, REG_EXTENDED);
    if (rc) {
        (void)regerror(rc, &regex, err_msg, msg_size);
        return (CPA_STATUS_E_UNSUPPORTED);
    }

    /* Parse str into subgroup patterns */
    rc = regexec(&regex, device_info, NUM_GROUPS, pmatch, 0);
    if (rc) {
        (void)regerror(rc, &regex, err_msg, msg_size);
        regfree(&regex);
        return (CPA_STATUS_E_UNSUPPORTED);
    }
    regfree(&regex);

    /* Check enough buffer space in name_sz */
    if ((size_t)((pmatch[VENDOR_IDX].rm_eo - pmatch[VENDOR_IDX].rm_so) +
                 (pmatch[MODEL_IDX].rm_eo - pmatch[MODEL_IDX].rm_so) + 4) >
        name_sz) {
        snprintf(err_msg,
                 msg_size,
                 "Extracted name exceeds size %zu",
                 name_sz);
        return (CPA_STATUS_E_UNSUPPORTED);
    }
    name_idx = 0;

    /* Prefix Ssd */
    fpd_name[name_idx++] = 'S';
    fpd_name[name_idx++] = 's';
    fpd_name[name_idx++] = 'd';

    /* SSD Vendor name */
    pattern_idx = pmatch[VENDOR_IDX].rm_so;
    while (pattern_idx < pmatch[VENDOR_IDX].rm_eo) {
        fpd_name[name_idx++] = device_info[pattern_idx++];
    }

    /* SSD Vendor Model */
    pattern_idx = pmatch[MODEL_IDX].rm_so;
    while (pattern_idx < pmatch[MODEL_IDX].rm_eo) {
        fpd_name[name_idx++] = device_info[pattern_idx++];
    }

    /* null terminate */
    fpd_name[name_idx++] = '\0';

    return (CPA_STATUS_OK);
}


cpa_status_t
fpd_get_ssd_fpd_name(char *      fpd_name,
                     size_t      name_sz,
                     char *      err_msg,
                     size_t      msg_size)
{
    char   buff[BUFF_SIZE];
    size_t out_size = 0;
    cpa_status_t rc;
    /**
     * The smartctl tool provides SSD Vendor and Model info either in the field
     * of 'Model Family' or 'Device Model'. Extract the info string.
     */
    rc = ssd_cpa_get_shell_cmd_output(buff,
                                  sizeof(buff),
                                  &out_size,
                                  err_msg,
                                  msg_size,
                                  "/usr/sbin/smartctl -i %s | grep -m 1 -E "
                                  "\"Model Family|Device Model\"",
                                  fpd_path);
    if ((rc != CPA_STATUS_OK) || (out_size == 0)) {
        printf("%s", err_msg);
        return (CPA_STATUS_E_UNSUPPORTED);
    }
    //printf("SSD -> %s", buff);

    rc = extract_ssd_fpd_name_from_device_model_info(
        buff, fpd_name, name_sz, err_msg, msg_size);
    //printf("buff is:%s\n", buff);
    //printf("fpd_name is:%s\n", fpd_name);

    return rc;
}

char *
fpd_get_ssd_str ()
{
    cpa_status_t rc;
    size_t       out_size = 0;
    char         err_buf[BUFF_SIZE];
    char         buff[BUFF_SIZE];
    char         *sub_str = NULL;

    memset(buff, 0, sizeof(buff));
    memset(err_buf, 0, sizeof(err_buf));

    rc = ssd_cpa_get_shell_cmd_output(
        buff,
        sizeof(buff),
        &out_size,
        err_buf,
        sizeof(err_buf),
        "/usr/sbin/smartctl -i %s | grep \"Firmware Version\"",
        fpd_path);

    if ((rc != CPA_STATUS_OK) || (out_size == 0)) {
        return NULL;
    }

    sub_str = strtok(buff, ":");
    if (sub_str == NULL) {
        return NULL;
    }

    sub_str = strtok(NULL, " ");
    if (sub_str == NULL) {
        return NULL;
    }

    return strdup(sub_str);
}

static cpa_status_t
fpd_get_ssd_run_ver (uint16_t        *major,
                     uint16_t        *minor,
                     uint16_t        *debug,
                     char            *err_msg,
                     uint32_t        msg_size)
{
    cpa_status_t rc;
    size_t       out_size = 0;
    char         err_buf[BUFF_SIZE];
    char         buff[BUFF_SIZE];
    char         *sub_str = NULL;
    char         *ssd_version;

    memset(buff, 0, sizeof(buff));
    memset(err_buf, 0, sizeof(err_buf));

    rc = ssd_cpa_get_shell_cmd_output(
        buff,
        sizeof(buff),
        &out_size,
        err_buf,
        sizeof(err_buf),
        "/usr/sbin/smartctl -i %s | grep \"Firmware Version\"",
        fpd_path);

    if ((rc != CPA_STATUS_OK) || (out_size == 0)) {
        snprintf(err_msg, msg_size,
                "fail to get ssd version info (%s)", err_buf);
        printf("%s", err_msg);
        return (CPA_STATUS_E_UNSUPPORTED);
    }

    //printf("SSD version info: %s\n", buff);

    sub_str = strtok(buff, ":");
    if (sub_str == NULL) {
        snprintf(err_msg, msg_size,"fail to get firmware details %s", buff);
        printf("%s", err_msg);
        return (CPA_STATUS_E_UNSUPPORTED);
    }

    sub_str = strtok(NULL, " ");
    if (sub_str == NULL) {
        snprintf(err_msg, msg_size,"fail to get version %s", buff);
        printf("%s", err_msg);
        return (CPA_STATUS_E_UNSUPPORTED);
    }

    ssd_version = strdup(sub_str);
    if (!ssd_version) {
        snprintf(err_msg, msg_size,"strup(%s) failed", sub_str);
        printf("%s", err_msg);
    	return CPA_STATUS_E_FAULT;
    }

    //printf("SSD version str: %s\n", ssd_version);

    /*
     * There is no unique format followed for version controling 
     * of the SSD FW image. Each model is following the specified 
     * format for the particular type. So this needs to updated
     * if we want to support any new SSD mode 
     * Example: 
     *      Intel S3520 Series SSDs, Example version  : N2010112
     *      Micron 5100 Pro / 5200 SSDs               : D0MU051
     *      Intel S4510/S4610/S4500/S4600 Series SSDs : XC311102
     *      Micron_5300_MTFDDAV240TDS                 : D3CN000    
     */
    if ((strncmp(ssd_version, "N201", strlen("N201")) == 0) ||
        (strncmp(ssd_version, "XC31", strlen("XC31")) == 0)) {
        *major = (((ssd_version[4] - 0x30) * 10) +
                   (ssd_version[5] - 0x30));
        *minor = (((ssd_version[6] - 0x30) * 10) +
                   (ssd_version[7] - 0x30));
    } else if ((strncmp(ssd_version, "D0MU", strlen("D0MU")) == 0) ||
               (strncmp(ssd_version, "D3CN", strlen("D3CN")) == 0)) {
        *major = (((ssd_version[4] - 0x30) * 10) +
                   (ssd_version[5] - 0x30));
        *minor = (ssd_version[6] - 0x30);

        /* 
         * Current Micron 5300 firmware version is D3CN000. XR only takes
         * numeric values to populate the SSD version. 0.00 will make 
         * confusion because of default value is 0.00.
         * So maping to 0.01 for D3CN000. This will be removed when new
         * version released. 
         */
        if ((*major == 0) && (*minor == 0)) {
            *minor = 1;
        }
    } else {
        snprintf(err_msg, msg_size,
                "Unknown SSD type %s", ssd_version);
	free(ssd_version);
        printf("%s", err_msg);
        return (CPA_STATUS_E_UNSUPPORTED);
    }
    free(ssd_version);

    *debug = 0;

    //printf("SSD Version: %d.%d", *major, *minor);
    return (CPA_STATUS_OK);
}


cpa_status_t
fpd_get_ssd_ver_str (char **ssd_version,
                     char            *err_msg,
                     uint32_t        msg_size)
{
    cpa_status_t rc;
    size_t       out_size = 0;
    char         err_buf[BUFF_SIZE];
    char         buff[BUFF_SIZE];
    char         *sub_str = NULL;

    memset(buff, 0, sizeof(buff));
    memset(err_buf, 0, sizeof(err_buf));
    *ssd_version = NULL;

    rc = ssd_cpa_get_shell_cmd_output(
        buff,
        sizeof(buff),
        &out_size,
        err_buf,
        sizeof(err_buf),
        "/usr/sbin/smartctl -i %s | grep \"Firmware Version\"",
        fpd_path);

    if ((rc != CPA_STATUS_OK) || (out_size == 0)) {
        snprintf(err_msg, msg_size,
                "fail to get ssd version info (%s)", err_buf);
        printf("%s", err_msg);
        return (CPA_STATUS_E_UNSUPPORTED);
    }

    //printf("SSD version info - NEW : %s\n", buff);

    sub_str = strtok(buff, ":");
    if (sub_str == NULL) {
        snprintf(err_msg, msg_size,"fail to get firmware details %s", buff);
        printf("%s", err_msg);
        return (CPA_STATUS_E_UNSUPPORTED);
    }

    sub_str = strtok(NULL, " ");
    if (sub_str == NULL) {
        snprintf(err_msg, msg_size,"fail to get version %s", buff);
        printf("%s", err_msg);
        return (CPA_STATUS_E_UNSUPPORTED);
    }

    *ssd_version = strdup(sub_str);
    return *ssd_version ? CPA_STATUS_OK : CPA_STATUS_E_FAULT; 
}

static cpa_status_t
fpd_ssd_upgrade (
                 char                   *err_msg,
                 uint32_t               msg_size)
{
    int             res            = 0;
    uint8_t         *image         = NULL;
    FILE            *upg_img_fp;
    FILE            *mdata_fp;
    char            upg_file[_POSIX_PATH_MAX];
    uint32_t        img_size = 0, mdata_sz = 0;
    size_t        out_size = 0;
    fpd_meta_data_t *meta_data = NULL;
    char            shell_op_buff[BUFF_SIZE];
    char            err_buf[BUFF_SIZE];
    char            cmd[BUFF_SIZE *4];
    char            image_path[_POSIX_PATH_MAX];
    char         *ssd_version;

    memset(shell_op_buff, 0, sizeof(shell_op_buff));
    memset(err_buf, 0, sizeof(err_buf));
    memset(cmd, 0, sizeof(cmd));
    memset(upg_file, 0, _POSIX_PATH_MAX);

    res = fpd_get_ssd_ver_str(&ssd_version, err_msg, msg_size);
    if (res) {
        printf("failed to get SSD version string\n");
	return (CPA_STATUS_E_INVALID);
    }

    //printf("SSD string:%s\n", ssd_version);
    
    if (strncmp(ssd_version, "N201", strlen("N201")) == 0) {
        snprintf(image_path, sizeof(image_path), 
                "/opt/cisco/fpd/ssd/ssdfw_intel_s3520.img");	
    } else if (strncmp(ssd_version, "XC31", strlen("XC31")) == 0) {
        snprintf(image_path, sizeof(image_path), 
                "/opt/cisco/fpd/ssd/ssdfw_intel_s4510.img");	
    } else if (strncmp(ssd_version, "D0MU", strlen("D0MU")) == 0) {
        snprintf(image_path, sizeof(image_path), 
                "/opt/cisco/fpd/ssd/ssdfw_micron_5100.img");	
    } else if (strncmp(ssd_version, "D3CN", strlen("D3CN")) == 0) {
        snprintf(image_path, sizeof(image_path), 
                "/opt/cisco/fpd/ssd/ssdfw_micron_5300.img");	
    } else {
        printf("Invalid vendor string\n");
	free(ssd_version);
        return (CPA_STATUS_E_INVALID);
    }
    free(ssd_version);

    printf("upgrading SSD Microcode\n");

    res = snprintf(upg_file, sizeof(upg_file),
             "%s/SSD_IMAGE.bin", SSD_UPG_IMG_EXTRACT_DIR);
    if ((size_t)res > sizeof(upg_file)) {
        printf("buff overflow on copy upgrade file target location info");
        return (CPA_STATUS_E_INVALID);
    }

    res = ssd_get_img_metadata(image_path,
                           &img_size, (void **)&image,
                           &mdata_sz, (void **)&meta_data,
                           err_msg, msg_size);
    if (res) {
        printf("Fail to extract and validate the SSD FW and  meta data");
        return (CPA_STATUS_E_INVALID);
    }

    /* 
     * write SSD Micro code
     */
    upg_img_fp = fopen(upg_file, "w");
    if (upg_img_fp == NULL) {
        printf("%s", err_msg);
        if(image) free(image);
        if(meta_data) free(meta_data);
        return (CPA_STATUS_E_INVALID);
    }

    res = fwrite(image, sizeof(char), img_size, upg_img_fp);

    if ((uint32_t)res != img_size){
        printf("Unable to write the image into"
                           " the upg file due to error: %s", strerror(errno));
        (void) fclose(upg_img_fp);
        if(image) free(image);
        if(meta_data) free(meta_data);
        return (CPA_STATUS_E_INVALID);
    }

    (void) fclose(upg_img_fp);
    printf("writing SSD micro code done\n");

    snprintf(cmd, sizeof(cmd), "%s %s %s %s > /dev/null",
             HDPARM_UPG_CMD, upg_file, HDPARM_ADD_ARGU,
             fpd_path);

    printf("upgrading cmd: %s\n", cmd);

    res = ssd_cpa_get_shell_cmd_output(shell_op_buff,
                                   sizeof(shell_op_buff),
                                   &out_size, err_buf,
                                   sizeof(err_buf),
                                   cmd);

    if (res != CPA_STATUS_OK) {
        snprintf(err_msg, msg_size,
                "fail to upgrade ssd (%s)", err_buf);
        printf("%s", err_msg);
        if(image) free(image);
        if(meta_data) free(meta_data);
        return (CPA_STATUS_E_FAULT);
    }

    /* 
     * write meta data
     */
    mdata_fp = fopen(SSD_METADATA_FILE, "w");
    if (mdata_fp == NULL) {
        printf("Unable to open meta data file\n");
        if(image) free(image);
        if(meta_data) free(meta_data);
        return (CPA_STATUS_E_FAULT);
    }

    res = fwrite(meta_data, sizeof(char), mdata_sz, mdata_fp);

    if ((uint32_t)res != mdata_sz){
        printf("Unable to write meta data "
                           "file due to error: %s\n", strerror(errno));
        (void) fclose(mdata_fp);
        if(image) free(image);
        if(meta_data) free(meta_data);
        return (CPA_STATUS_E_FAULT);
    }
    if(image) free(image);
    if(meta_data) free(meta_data);

    (void) fclose(mdata_fp);

    printf("writing meta data done\n");

    unlink(upg_file);

    return (CPA_STATUS_OK);
}

uint32_t get_ssd_fpd_version()
{

    char err_msg[MSG_SIZE];
    unsigned int msg_size;
    cpa_status_t rc = 0;
    uint16_t major, minor, debug;
    uint32_t version;

    major = minor = debug = 0;
    bzero(err_msg, sizeof(err_msg));
    msg_size = sizeof(err_msg);

    rc = fpd_get_ssd_run_ver(&major, &minor, &debug, err_msg, msg_size);
    if (rc) {
	printf("fail to get %s fpd info\n", err_msg);
    }
    version = ((major << 16) | minor);
    //printf("major:%d, minor:%d\n", major, minor);

    return version;
}

uint32_t ssd_fpd_upgrade()
{

    char err_msg[MSG_SIZE];
    unsigned int msg_size;
    cpa_status_t rc = 0;
    
    bzero(err_msg, sizeof(err_msg));
    msg_size = sizeof(err_msg);
    
    rc = fpd_ssd_upgrade(err_msg, msg_size);
    if (rc) {
	printf("fail to get %s fpd info\n", err_msg);
	return -1;
    }

    return 0;
}
