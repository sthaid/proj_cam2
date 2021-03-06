#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif
#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <netdb.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <ifaddrs.h>
#include <termios.h>
#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#ifndef ANDROID
#include <sys/statvfs.h>
#endif

#ifdef ANDROID
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif
#endif

// -----------------  VERSION  ------------------------------------------------------

#define VERSION_MAJOR 2
#define VERSION_MINOR 0

#define VERSION ( { version_t v = { VERSION_MAJOR, VERSION_MINOR }; v; } );

typedef struct {
    int major;
    int minor;
} version_t;

// -----------------  NETWORK COMMUNICATION  -----------------------------------------

// defines used for net_recv mode param
#define BLOCKING_WITH_TIMEOUT       0
#define NON_BLOCKING_WITH_TIMEOUT   1
#define NON_BLOCKING_NO_TIMEOUT     2


int net_init(bool is_server, int server_port);
void *net_connect(char *ipaddr, int port, char *password, int *connect_status);
void *net_accept(void);
void net_disconnect(void *handle);
int net_send(void *handle, void *buff, int len);
int net_recv(void *handle, void *buff, int len, int mode);

// -----------------  WEBCAM COMMON DEFINITIONS  ---------------------------------------

// msg_type for messages from webcam
#define MSG_TYPE_FRAME                     1 
#define MSG_TYPE_STATUS                    2

// msg_type for messages to webcam
#define MSG_TYPE_CMD_SET_MODE              11
#define MSG_TYPE_CMD_LIVE_MODE_CHANGE_RES  12
#define MSG_TYPE_CMD_SET_MIN_SEND_INTVL_US 13

// max frame data length
#define RP_MAX_FRAME_DATA_LEN   500000

// status values
#define STATUS_INFO_OK                       0     // status okay
#define STATUS_INFO_GAP                      1
#define STATUS_INFO_CHANGING_RESOLUTION      2
#define STATUS_INFO_STOPPED                  3
#define STATUS_ERR_GENERAL_FAILURE           100   // status error
#define STATUS_ERR_GET_WC_ADDR               101
#define STATUS_ERR_CONNECT_TO_WC             102
#define STATUS_ERR_SEND_PSSWD_TO_WC          103
#define STATUS_ERR_RECV_PSSWD_RESP_FROM_WC   104
#define STATUS_ERR_INVALID_PSSWD             105
#define STATUS_ERR_DEAD                      106
#define STATUS_ERR_JPEG_DECODE               107
#define STATUS_ERR_WEBCAM_FAILURE            108
#define STATUS_ERR_SYSTEM_CLOCK_NOT_SET      109
#define STATUS_ERR_FRAME_BEFORE_BOD          110
#define STATUS_ERR_FRAME_AFTER_EOD           111
#define STATUS_ERR_FRAME_TIME                112
#define STATUS_ERR_FRAME_DATA_MEM_ALLOC      113
#define STATUS_ERR_FRAME_DATA_READ           114
#define STATUS_ERR_FRAME_FILE_OFFSET_INVLD   115
#define STATUS_ERR_FRAME_HDR_READ            116
#define STATUS_ERR_FRAME_HDR_MAGIC           117
#define STATUS_ERR_FRAME_HDR_CHECKSUM        118
#define STATUS_ERR_FRAME_FILE_EMPTY          119
#define STATUS_ERR_FRAME_NOT_FOUND_1         120
#define STATUS_ERR_FRAME_NOT_FOUND_2         121
#define STATUS_ERR_FRAME_NOT_FOUND_3         122
#define STATUS_ERR_FRAME_NOT_FOUND_4         123

#define STATUS_STR(status) \
   ((status) == STATUS_INFO_OK                     ? "OK"                       : \
    (status) == STATUS_INFO_GAP                    ? "GAP"                      : \
    (status) == STATUS_INFO_CHANGING_RESOLUTION    ? "CHANGING_RESOLUTION"      : \
    (status) == STATUS_INFO_STOPPED                ? "STOPPED"                  : \
    (status) == STATUS_ERR_GENERAL_FAILURE         ? "GENERAL_FAILURE"          : \
    (status) == STATUS_ERR_GET_WC_ADDR             ? "GET_WC_ADDR"              : \
    (status) == STATUS_ERR_CONNECT_TO_WC           ? "CONNECT_TO_WC"            : \
    (status) == STATUS_ERR_SEND_PSSWD_TO_WC        ? "SEND_PSSWD_TO_WC"         : \
    (status) == STATUS_ERR_RECV_PSSWD_RESP_FROM_WC ? "RECV_PASSWD_RESP_FROM_WC" : \
    (status) == STATUS_ERR_INVALID_PSSWD           ? "INVALID_PSSWD"            : \
    (status) == STATUS_ERR_DEAD                    ? "DEAD"                     : \
    (status) == STATUS_ERR_JPEG_DECODE             ? "JPEG_DECODE"              : \
    (status) == STATUS_ERR_WEBCAM_FAILURE          ? "WEBCAM_FAILURE"           : \
    (status) == STATUS_ERR_SYSTEM_CLOCK_NOT_SET    ? "SYSTEM_CLOCK_NOT_SET"     : \
    (status) == STATUS_ERR_FRAME_BEFORE_BOD        ? "FRAME_BEFORE_BOD"         : \
    (status) == STATUS_ERR_FRAME_AFTER_EOD         ? "FRAME_AFTER_BOD"          : \
    (status) == STATUS_ERR_FRAME_TIME              ? "FRAME_TIME"               : \
    (status) == STATUS_ERR_FRAME_DATA_MEM_ALLOC    ? "FRAME_DATE_MEM_ALLOC"     : \
    (status) == STATUS_ERR_FRAME_DATA_READ         ? "FRAME_DATE_READ"          : \
    (status) == STATUS_ERR_FRAME_FILE_OFFSET_INVLD ? "FRAME_FILE_OFFSET_INVLD"  : \
    (status) == STATUS_ERR_FRAME_HDR_READ          ? "FRAME_HDR_READ"           : \
    (status) == STATUS_ERR_FRAME_HDR_MAGIC         ? "FRAME_HDR_MAGIC"          : \
    (status) == STATUS_ERR_FRAME_HDR_CHECKSUM      ? "FRAME_HDR_CHECKSUM"       : \
    (status) == STATUS_ERR_FRAME_FILE_EMPTY        ? "FRAME_FILE_EMTPY"         : \
    (status) == STATUS_ERR_FRAME_NOT_FOUND_1       ? "FRAME_NOT_FOUND_1"        : \
    (status) == STATUS_ERR_FRAME_NOT_FOUND_2       ? "FRAME_NOT_FOUND_2"        : \
    (status) == STATUS_ERR_FRAME_NOT_FOUND_3       ? "FRAME_NOT_FOUND_3"        : \
    (status) == STATUS_ERR_FRAME_NOT_FOUND_4       ? "FRAME_NOT_FOUND_4"        : \
                                                     "????")

// mode values
#define MODE_NONE      0
#define MODE_LIVE      1
#define MODE_PLAYBACK  2
#define MODE_STR(mode) \
    ((mode) == MODE_NONE     ? "NONE"     : \
     (mode) == MODE_LIVE     ? "LIVE"     : \
     (mode) == MODE_PLAYBACK ? "PLAYBACK" : \
                               "????")

// pb_submode values
#define PB_SUBMODE_STOP  0
#define PB_SUBMODE_PAUSE 1
#define PB_SUBMODE_PLAY  2
#define PB_SUBMODE_STR(psm) \
    ((psm) == PB_SUBMODE_STOP  ? "STOP " : \
     (psm) == PB_SUBMODE_PAUSE ? "PAUSE"  : \
     (psm) == PB_SUBMODE_PLAY  ? "PLAY " : \
                                 "????")

// pb_dir values
#define PB_DIR_FWD  0
#define PB_DIR_REV  1
#define PB_DIR_STR(dir) \
    ((dir) == PB_DIR_FWD  ? "F"  : \
     (dir) == PB_DIR_REV  ? "R"  : \
                            "?")

// macro to get the real time when in PB_SUBMODE_PLAY
#define PB_SUBMODE_PLAY_REAL_TIME_US(m) \
    ((m)->pb_real_time_us + \
     ((int64_t)(get_real_time_us() - (m)->pb_mode_entry_real_time_us) * \
      ((m)->pb_dir == PB_DIR_FWD ? 1 : -1) * \
      ((m)->pb_speed)))

#pragma pack(push,1)
typedef struct {
    uint32_t msg_type;
    union {
        // messages from webcam
        struct {
            uint64_t mode_id;
            uint64_t real_time_us;
            uint32_t data_len;
            uint32_t status;
            int32_t  temperature;
            bool     motion;
        } mt_frame;
        struct status_s {
            version_t version;
            uint32_t cam_status;
            uint32_t rp_status;
            uint64_t rp_duration_us;
        } mt_status;

        // messages to webcam
        struct mode_s {
            uint32_t mode;
            uint64_t mode_id;
            uint32_t pb_submode;
            uint64_t pb_mode_entry_real_time_us;
            uint64_t pb_real_time_us;
            uint32_t pb_dir;
            double   pb_speed;
        } mt_cmd_set_mode;
        struct {
            uint32_t empty;
        } mt_cmd_change_res;
        struct {
            uint64_t us;
        } mt_cmd_min_send_intvl;
    } u;
} webcam_msg_t;
#pragma pack(pop)

// -----------------  WEBCAM SERVER  ---------------------------------------------------

int wc_svc_webcam_init(void);
void * wc_svc_webcam(void * cx);

// -----------------  JPEG DECODE  ---------------------------------------------------

#define JPEG_DECODE_MODE_GS    1
#define JPEG_DECODE_MODE_YUY2  2

int jpeg_decode(uint32_t cxid, uint32_t jpeg_decode_mode, uint8_t *jpeg, uint32_t jpeg_size,
                uint8_t **out_buf, uint32_t *width, uint32_t *height);

// -----------------  TEMPERATURE MONITOR  -------------------------------------------

#define INVALID_TEMPERATURE 999

int temper_init(void);
int temper_read(void);

// -----------------  UTILS  ---------------------------------------------------------

//#define DEBUG_PRINTS

#define INFO(fmt, args...) \
    do { \
        logmsg("INFO", __func__, fmt, ## args); \
    } while (0)
#define WARN(fmt, args...) \
    do { \
        logmsg("WARN", __func__, fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        logmsg("ERROR", __func__, fmt, ## args); \
    } while (0)
#define FATAL(fmt, args...) \
    do { \
        logmsg("FATAL", __func__, fmt, ## args); \
        exit(1); \
    } while (0)
#ifdef DEBUG_PRINTS
  #define DEBUG(fmt, args...) \
      do { \
          logmsg("DEBUG", __func__, fmt, ## args); \
      } while (0)
#else
  #define DEBUG(fmt, args...) 
#endif

#define MAX_LOGMSG_FILE_SIZE 0x100000

#define TIMESPEC_TO_US(ts) ((uint64_t)(ts)->tv_sec *1000000 + (ts)->tv_nsec / 1000)
#define TIMEVAL_TO_US(tv)  ((uint64_t)(tv)->tv_sec *1000000 + (tv)->tv_usec)

#define MAX_CONFIG_VALUE_STR 100
#define MAX_TIME_STR         100
#define MAX_INT_STR          50

typedef struct {
    const char *name;
    char        value[MAX_CONFIG_VALUE_STR];
} config_t;

int config_read(char *config_path, config_t *config, int config_version);
int config_write(char *config_path, config_t *config, int config_version);

int getsockaddr(char *node, int port, int socktype, int protocol, struct sockaddr_in *ret_addr);
char *sock_addr_to_str(char *s, int slen, struct sockaddr *addr);

void logmsg_init(char *logmsg_file);
void logmsg(char *lvl, const char *func, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

void convert_yuy2_to_rgb(uint8_t *yuy2, uint8_t *rgb, int pixels);
void convert_yuy2_to_gs(uint8_t *yuy2, uint8_t *gs, int pixels);

uint64_t microsec_timer(void);
time_t get_real_time_sec(void);
uint64_t get_real_time_us(void);
char *time2str(char *str, time_t time, bool gmt);
bool ntp_synced(void);
void init_system_clock_offset_using_sntp(void);

uint64_t fs_avail_bytes(char *path);

char *status2str(uint32_t status);
char *int2str(char *str, int64_t n);

