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

typedef struct {
    int tbd;
} net_stats_t;

int net_init(bool is_server, int server_port);
void *net_connect(char *ipaddr, int port, char *password, int *connect_status);
void *net_accept(void);
void net_disconnect(void *handle);
int net_send(void *handle, void *buff, int len);
int net_recv(void *handle, void *buff, int len, bool non_blocking);
void net_get_stats(void *handle, net_stats_t *stats);

// -----------------  WEBCAM DEFINITIONS  ----------------------------------------------

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
#define STATUS_INFO_OK                       0
#define STATUS_INFO_STOPPED                  1
#define STATUS_INFO_GAP                      2
#define STATUS_INFO_LOADING_IMAGE            3
#define STATUS_INFO_CHANGING_RESOLUTION      4
#define STATUS_INFO_NOT_RUN                  5
#define STATUS_INFO_IN_PROGRESS              6
#define STATUS_ERR_GENERAL_FAILURE           100
#define STATUS_ERR_FRAME_FILE_OFFSET_INVLD   101
#define STATUS_ERR_FRAME_FILE_EMPTY          102
#define STATUS_ERR_FRAME_BEFORE_BOD          103
#define STATUS_ERR_FRAME_AFTER_EOD           104
#define STATUS_ERR_FRAME_NOT_FOUND_1         105
#define STATUS_ERR_FRAME_NOT_FOUND_2         106
#define STATUS_ERR_FRAME_NOT_FOUND_3         107
#define STATUS_ERR_FRAME_NOT_FOUND_4         108
#define STATUS_ERR_FRAME_HDR_READ            109
#define STATUS_ERR_FRAME_HDR_MAGIC           110
#define STATUS_ERR_FRAME_HDR_CHECKSUM        111
#define STATUS_ERR_FRAME_DATA_MEM_ALLOC      112
#define STATUS_ERR_FRAME_DATA_READ           113
#define STATUS_ERR_FRAME_TIME                114
#define STATUS_ERR_JPEG_DECODE               115
#define STATUS_ERR_DEAD                      116
#define STATUS_ERR_WEBCAM_FAILURE            117
#define STATUS_ERR_SYSTEM_CLOCK_NOT_SET      118
#define STATUS_ERR_NO_PASSWORD               120
#define STATUS_ERR_HANDLE_TOO_BIG            121
#define STATUS_ERR_GET_SERVER_ADDR           122
#define STATUS_ERR_CREATE_SOCKET             123
#define STATUS_ERR_GET_LOCAL_ADDR            124
#define STATUS_ERR_BIND_LOCAL_ADDR           125
#define STATUS_ERR_GETSOCKNAME               126
#define STATUS_ERR_SENDTO                    127
#define STATUS_ERR_RECVFROM                  128
#define STATUS_ERR_NO_RESPONSE_FROM_SERVER   129
#define STATUS_ERR_INVALID_CONNECTION_ID     130
#define STATUS_ERR_DUPLICATE_CONNECTION_ID   131
#define STATUS_ERR_TOO_MANY_CONNECTIONS      132
#define STATUS_ERR_CONNECTION_MEM_ALLOC      133
#define STATUS_ERR_NO_RESPONSE_FROM_PEER     134
#define STATUS_ERR_FAILED_CONNECT_TO_SERVER  135
#define STATUS_ERR_INVLD_RESP_FROM_SERVER    136
#define STATUS_ERR_PASSWORD_LENGTH           138
#define STATUS_ERR_PASSWORD_CHARS            140
#define STATUS_ERR_ACCESS_DENIED             141
#define STATUS_ERR_INVALID_LINK              142
#define STATUS_ERR_WC_DOES_NOT_EXIST         144
#define STATUS_ERR_WC_NOT_ONLINE             145
#define STATUS_ERR_WC_ADDR_NOT_AVAIL         146
#define STATUS_ERR_WCNAME_CHARS              150
#define STATUS_ERR_WCNAME_LENGTH             151
#define STATUS_ERR_WC_ACCESS_LIST_LENGTH     152
#define STATUS_ERR_MUST_BE_ROOT              153

#define STATUS_STR(status) \
    ((status) == STATUS_INFO_OK                       ? "OK"                             : \
     (status) == STATUS_INFO_STOPPED                  ? "STOPPED"                        : \
     (status) == STATUS_INFO_GAP                      ? "GAP"                            : \
     (status) == STATUS_INFO_LOADING_IMAGE            ? "LOADING_IMAGE"                  : \
     (status) == STATUS_INFO_CHANGING_RESOLUTION      ? "CHANGING_RESOLUTION"            : \
     (status) == STATUS_INFO_NOT_RUN                  ? "NOT_RUN"                        : \
     (status) == STATUS_INFO_IN_PROGRESS              ? "IN_PROGRESS"                    : \
     (status) == STATUS_ERR_GENERAL_FAILURE           ? "GENERAL_FAILURE"                : \
     (status) == STATUS_ERR_FRAME_FILE_OFFSET_INVLD   ? "FRAME_FILE_OFFSET_INVLD"        : \
     (status) == STATUS_ERR_FRAME_FILE_EMPTY          ? "FRAME_FILE_EMPTY"               : \
     (status) == STATUS_ERR_FRAME_BEFORE_BOD          ? "FRAME_BEFORE_BOD"               : \
     (status) == STATUS_ERR_FRAME_AFTER_EOD           ? "FRAME_AFTER_EOD"                : \
     (status) == STATUS_ERR_FRAME_NOT_FOUND_1         ? "FRAME_NOT_FOUND_1"              : \
     (status) == STATUS_ERR_FRAME_NOT_FOUND_2         ? "FRAME_NOT_FOUND_2"              : \
     (status) == STATUS_ERR_FRAME_NOT_FOUND_3         ? "FRAME_NOT_FOUND_3"              : \
     (status) == STATUS_ERR_FRAME_NOT_FOUND_4         ? "FRAME_NOT_FOUND_4"              : \
     (status) == STATUS_ERR_FRAME_HDR_READ            ? "FRAME_HDR_READ"                 : \
     (status) == STATUS_ERR_FRAME_HDR_MAGIC           ? "FRAME_HDR_MAGIC"                : \
     (status) == STATUS_ERR_FRAME_HDR_CHECKSUM        ? "FRAME_HDR_CHECKSUM"             : \
     (status) == STATUS_ERR_FRAME_DATA_MEM_ALLOC      ? "FRAME_DATA_MEM_ALLOC"           : \
     (status) == STATUS_ERR_FRAME_DATA_READ           ? "FRAME_DATA_READ"                : \
     (status) == STATUS_ERR_FRAME_TIME                ? "FRAME_TIME"                     : \
     (status) == STATUS_ERR_JPEG_DECODE               ? "JPEG_DECODE"                    : \
     (status) == STATUS_ERR_DEAD                      ? "DEAD"                           : \
     (status) == STATUS_ERR_WEBCAM_FAILURE            ? "WEBCAM_FAILURE"                 : \
     (status) == STATUS_ERR_SYSTEM_CLOCK_NOT_SET      ? "SYSTEM_CLOCK_NOT_SET"           : \
     (status) == STATUS_ERR_NO_PASSWORD               ? "NO_PASSWORD"                    : \
     (status) == STATUS_ERR_HANDLE_TOO_BIG            ? "HANDLE_TOO_BIG"                 : \
     (status) == STATUS_ERR_GET_SERVER_ADDR           ? "GET_SERVER_ADDR"                : \
     (status) == STATUS_ERR_CREATE_SOCKET             ? "CREATE_SOCKET"                  : \
     (status) == STATUS_ERR_GET_LOCAL_ADDR            ? "GET_LOCAL_ADDR"                 : \
     (status) == STATUS_ERR_BIND_LOCAL_ADDR           ? "BIND_LOCAL_ADDR"                : \
     (status) == STATUS_ERR_GETSOCKNAME               ? "GETSOCKNAME"                    : \
     (status) == STATUS_ERR_SENDTO                    ? "SENDTO"                         : \
     (status) == STATUS_ERR_RECVFROM                  ? "RECVFROM"                       : \
     (status) == STATUS_ERR_NO_RESPONSE_FROM_SERVER   ? "NO_RESPONSE_FROM_SERVER"        : \
     (status) == STATUS_ERR_INVALID_CONNECTION_ID     ? "INVALID_CONNECTION_ID"          : \
     (status) == STATUS_ERR_DUPLICATE_CONNECTION_ID   ? "DUPLICATE_CONNECTION_ID"        : \
     (status) == STATUS_ERR_TOO_MANY_CONNECTIONS      ? "TOO_MANY_CONNECTIONS"           : \
     (status) == STATUS_ERR_CONNECTION_MEM_ALLOC      ? "CONNECTION_MEM_ALLOC"           : \
     (status) == STATUS_ERR_NO_RESPONSE_FROM_PEER     ? "NO_RESPONSE_FROM_PEER"          : \
     (status) == STATUS_ERR_FAILED_CONNECT_TO_SERVER  ? "FAILED_CONNECT_TO_SERVER"       : \
     (status) == STATUS_ERR_INVLD_RESP_FROM_SERVER    ? "INVLD_RESP_FROM_SERVER"         : \
     (status) == STATUS_ERR_PASSWORD_LENGTH           ? "PASSWORD_LENGTH"                : \
     (status) == STATUS_ERR_PASSWORD_CHARS            ? "PASSWORD_CHARS"                 : \
     (status) == STATUS_ERR_ACCESS_DENIED             ? "ACCESS_DENIED"                  : \
     (status) == STATUS_ERR_INVALID_LINK              ? "INVALID_LINK"                   : \
     (status) == STATUS_ERR_WC_DOES_NOT_EXIST         ? "WC_DOES_NOT_EXIST"              : \
     (status) == STATUS_ERR_WC_NOT_ONLINE             ? "WC_NOT_ONLINE"                  : \
     (status) == STATUS_ERR_WC_ADDR_NOT_AVAIL         ? "WC_ADDR_NOT_AVAIL"              : \
     (status) == STATUS_ERR_WCNAME_CHARS              ? "WCNAME_CHARS"                   : \
     (status) == STATUS_ERR_WCNAME_LENGTH             ? "WCNAME_LENGTH"                  : \
     (status) == STATUS_ERR_WC_ACCESS_LIST_LENGTH     ? "ACCESS_LIST_LENGTH"             : \
     (status) == STATUS_ERR_MUST_BE_ROOT              ? "MUST_BE_ROOT"                   : \
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
    ((psm) == PB_SUBMODE_STOP  ? "STOPPED" : \
     (psm) == PB_SUBMODE_PAUSE ? "PAUSED"  : \
     (psm) == PB_SUBMODE_PLAY  ? "PLAYING" : \
                                 "????")

// pb_dir values
#define PB_DIR_FWD  0
#define PB_DIR_REV  1
#define PB_DIR_STR(dir) \
    ((dir) == PB_DIR_FWD  ? "FWD"  : \
     (dir) == PB_DIR_REV  ? "REV"  : \
                            "????")

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

// -----------------  JPEG_DECODE  ---------------------------------------------------

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

