#include "wc.h"

// -----------------  CONFIG READ / WRITE  -------------------------------

// XXX this should check for array bound
int config_read(char * config_path, config_t * config, int config_version)
{
    FILE * fp;
    int    i, version=0, len;
    char * name;
    char * value;
    char   s[100] = "";

    // open config_file and verify version, 
    // if this fails then write the config file with default values
    if ((fp = fopen(config_path, "re")) == NULL ||
        fgets(s, sizeof(s), fp) == NULL ||
        sscanf(s, "VERSION %d", &version) != 1 ||
        version != config_version)
    {
        if (fp != NULL) {
            fclose(fp);
        }
        INFO("creating default config file %s, version=%d\n", config_path, config_version);
        return config_write(config_path, config, config_version);
    }

    // read config entries
    while (memset(s, 0, sizeof(s)), fgets(s, sizeof(s), fp) != NULL) {
        // remove trailing \n
        len = strlen(s);
        if (len > 0 && s[len-1] == '\n') {
            s[len-1] = '\0';
        }

        // use strtok to get name
        name = strtok(s, " ");
        if (name == NULL || name[0] == '#') {
            continue;
        }

        // value can have embedded spaces, but leading spaces are skipped;
        // value starts after skipping spaces and extends to the end of s
        value = name + strlen(name) + 1;
        while (*value == ' ' && *value != '\0') {
            value++;
        }

        // XXX del
        INFO("name='%s' value='%s'\n", name, value);

        // search the config array for a matching name;
        // if found, save the new value associated with the name
        for (i = 0; config[i].name[0]; i++) {
            if (strcmp(name, config[i].name) == 0) {
                strcpy(config[i].value, value);
                break;
            }
        }
    }

    // close
    fclose(fp);
    return 0;
}

int config_write(char * config_path, config_t * config, int config_version)
{
    FILE * fp;
    int    i;

    // open
    fp = fopen(config_path, "we");  // mode: truncate-or-create, close-on-exec
    if (fp == NULL) {
        ERROR("failed to write config file %s, %s\n", config_path, strerror(errno));
        return -1;
    }

    // write version
    fprintf(fp, "VERSION %d\n", config_version);

    // write name/value pairs
    for (i = 0; config[i].name[0]; i++) {
        fprintf(fp, "%-20s %s\n", config[i].name, config[i].value);
    }

    // close
    fclose(fp);
    return 0;
}

// -----------------  NETWORKING  ----------------------------------------

int getsockaddr(char * node, int port, int socktype, int protocol, struct sockaddr_in * ret_addr)
{
    struct addrinfo   hints;
    struct addrinfo * result;
    char              port_str[20];
    int               ret;

    sprintf(port_str, "%d", port);

    bzero(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags    = AI_NUMERICSERV;

    ret = getaddrinfo(node, port_str, &hints, &result);
    if (ret != 0) {
        ERROR("failed to get address of %s, %s\n", node, gai_strerror(ret));
        return -1;
    }
    if (result->ai_addrlen != sizeof(*ret_addr)) {
        ERROR("getaddrinfo result addrlen=%d, expected=%d\n",
            (int)result->ai_addrlen, (int)sizeof(*ret_addr));
        return -1;
    }

    *ret_addr = *(struct sockaddr_in*)result->ai_addr;
    freeaddrinfo(result);
    return 0;
}

char *sock_addr_to_str(char * s, int slen, struct sockaddr * addr)
{
    char addr_str[100];
    int port;

    if (addr->sa_family == AF_INET) {
        inet_ntop(AF_INET,
                  &((struct sockaddr_in*)addr)->sin_addr,
                  addr_str, sizeof(addr_str));
        port = ((struct sockaddr_in*)addr)->sin_port;
    } else if (addr->sa_family == AF_INET6) {
        inet_ntop(AF_INET6,
                  &((struct sockaddr_in6*)addr)->sin6_addr,
                 addr_str, sizeof(addr_str));
        port = ((struct sockaddr_in6*)addr)->sin6_port;
    } else {
        snprintf(s,slen,"Invalid AddrFamily %d", addr->sa_family);
        return s;
    }

    snprintf(s,slen,"%s:%d",addr_str,htons(port));
    return s;
}

// -----------------  LOGGING & PRINTMSG  ---------------------------------

#ifndef ANDROID

FILE * logmsg_fp             = NULL;
FILE * logmsg_fp_old         = NULL;
size_t logmsg_file_size      = 0;
char   logmsg_file_name[100] = "stderr";
bool   logmsg_disabled       = false;
bool   logmsg_init_called    = false;

void logmsg_init(char * file_name)
{
    struct stat buf;

    // don't support calling this routine more than once
    if (logmsg_init_called) {
        FATAL("logmsg_init called multiple times\n");
    }
    logmsg_init_called = true;

    // save copy of file_name
    strcpy(logmsg_file_name, file_name);

    // determine logmsg_disabled flag, if set then return
    logmsg_disabled = (strcmp(logmsg_file_name, "none") == 0);
    if (logmsg_disabled) {
        return;
    }

    // if logmsg_file_name is stderr then set logmsg_fp to NULL and return
    if (strcmp(logmsg_file_name, "stderr") == 0) {
        logmsg_fp = NULL;
        return;
    }

    // logging is to a file:
    // - open the file
    // - determine its size
    // - set line buffering
    logmsg_fp = fopen(logmsg_file_name, "ae");   // mode: append, close-on-exec
    if (logmsg_fp == NULL) {
        FATAL("failed to create logmsg file %s, %s\n", logmsg_file_name, strerror(errno));
    }
    if (stat(logmsg_file_name, &buf) != 0) {
        FATAL("failed to stat logmsg file %s, %s\n", logmsg_file_name, strerror(errno));
    }
    logmsg_file_size = buf.st_size;
    setlinebuf(logmsg_fp);
}

void logmsg(char *lvl, const char *func, char *fmt, ...) 
{
    va_list ap;
    char    msg[1000];
    int     len, cnt;
    char    time_str[MAX_TIME_STR];

    // if disabled then 
    // - print FATAL msg to stderr
    // - return
    // endif
    if (logmsg_disabled) {
        if (strcmp(lvl, "FATAL") == 0) {
            va_start(ap, fmt);
            vfprintf(stderr, fmt, ap);
            va_end(ap);
        }
        return;
    }

    // construct msg
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // remove terminating newline
    len = strlen(msg);
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

    // check if logging to file vs stderr
    if (logmsg_fp != NULL) {
        // logging to file

        // print the preamble and msg
        cnt = fprintf(logmsg_fp, "%s %s %s: %s\n",
                      time2str(time_str, get_real_time_sec(), false),
                      lvl, func, msg);

        // keep track of file size
        logmsg_file_size += cnt;

        // if file size greater than max then rename file to file.old, and create new file
        if (logmsg_file_size > MAX_LOGMSG_FILE_SIZE) {
            char   dot_old[200];
            FILE * new_fp;

            if (logmsg_fp_old) {
                fclose(logmsg_fp_old);
            }
            logmsg_fp_old = logmsg_fp;

            sprintf(dot_old, "%s.old", logmsg_file_name);
            rename(logmsg_file_name, dot_old);

            new_fp = fopen(logmsg_file_name, "we");
            if (new_fp == NULL) {
                FATAL("failed to create logmsg file %s, %s\n", logmsg_file_name, strerror(errno));
            }
            setlinebuf(new_fp);

            logmsg_fp = new_fp;
            logmsg_file_size = 0;
        }
    } else {
        // logging to stderr
        cnt = fprintf(stderr, "%s %s %s: %s\n",
                      time2str(time_str, get_real_time_sec(), false),
                      lvl, func, msg);
    }
}

#else

#include <SDL.h>

void logmsg_init(char * file_name)
{
    // nothing to do here for Android,
    // logging is always performed
}

void logmsg(char *lvl, const char *func, char *fmt, ...) 
{
    va_list ap;
    char    msg[1000];
    int     len;
    char    time_str[MAX_TIME_STR];

    // construct msg
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // remove terminating newline
    len = strlen(msg);
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

    // log the message
    if (strcmp(lvl, "INFO") == 0) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "%s %s %s: %s\n",
                    time2str(time_str, time(NULL), false),
                    lvl, func, msg);
    } else if (strcmp(lvl, "WARN") == 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "%s %s %s: %s\n",
                    time2str(time_str, time(NULL), false),
                    lvl, func, msg);
    } else if (strcmp(lvl, "FATAL") == 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "%s %s %s: %s\n",
                        time2str(time_str, time(NULL), false),
                        lvl, func, msg);
    } else if (strcmp(lvl, "DEBUG") == 0) {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                     "%s %s %s: %s\n",
                     time2str(time_str, time(NULL), false),
                     lvl, func, msg);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s %s %s: %s\n",
                     time2str(time_str, time(NULL), false),
                     lvl, func, msg);
    }
}

#endif

// -----------------  IMAGE UTILS  ----------------------------------------

static inline unsigned char clip(int x)
{
    if (x < 0) {
        return 0;
    } else if (x > 255) {
        return 255;
    } else {
        return x;
    }
}

void convert_yuy2_to_rgb(uint8_t * yuy2, uint8_t * rgb, int pixels)
{
    int y0,u0,y1,v0,c,d,e;
    int i;

    for (i = 0;  i < pixels/2; i++) {
        y0 = yuy2[0];
        u0 = yuy2[1];
        y1 = yuy2[2];
        v0 = yuy2[3];
        yuy2 += 4;
        c = y0 - 16;
        d = u0 - 128;
        e = v0 - 128;
        rgb[0] = clip(( 298 * c + 516 * d           + 128) >> 8); // blue
        rgb[1] = clip(( 298 * c - 100 * d - 208 * e + 128) >> 8); // green
        rgb[2] = clip(( 298 * c           + 409 * e + 128) >> 8); // red
        rgb[3] = 0;
        c = y1 - 16;
        rgb[4] = clip(( 298 * c + 516 * d           + 128) >> 8); // blue
        rgb[5] = clip(( 298 * c - 100 * d - 208 * e + 128) >> 8); // green
        rgb[6] = clip(( 298 * c           + 409 * e + 128) >> 8); // red
        rgb[7] = 0;
        rgb += 8;
    }
}

void convert_yuy2_to_gs(uint8_t * yuy2, uint8_t * gs, int pixels)
{
    int y0,u0,y1,v0,c,d,e,b,g,r;
    int i;
    
    for (i = 0;  i < pixels/2; i++) {
        y0 = yuy2[0];
        u0 = yuy2[1];
        y1 = yuy2[2];
        v0 = yuy2[3];
        yuy2 += 4;

        c = y0 - 16;
        d = u0 - 128;
        e = v0 - 128;
        b = clip(( 298 * c + 516 * d           + 128) >> 8); // blue
        g = clip(( 298 * c - 100 * d - 208 * e + 128) >> 8); // green
        r = clip(( 298 * c           + 409 * e + 128) >> 8); // red
        *gs++ = (b + g + r) / 3;

        c = y1 - 16;
        b = clip(( 298 * c + 516 * d           + 128) >> 8); // blue
        g = clip(( 298 * c - 100 * d - 208 * e + 128) >> 8); // green
        r = clip(( 298 * c           + 409 * e + 128) >> 8); // red
        *gs++ = (b + g + r) / 3;
    }
}

// -----------------  TIME UTILS  -----------------------------------------

static int64_t system_clock_offset_us;

uint64_t microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
}

time_t get_real_time_sec(void)
{
    return get_real_time_us() / 1000000;
}

uint64_t get_real_time_us(void)
{
    struct timespec ts;
    uint64_t us;
    static uint64_t last_us;

    clock_gettime(CLOCK_REALTIME,&ts);
    us = ((uint64_t)ts.tv_sec * 1000000) + 
         ((uint64_t)ts.tv_nsec / 1000) +
         system_clock_offset_us;

    if (us <= last_us) {
        us = last_us + 1;
    }
    last_us = us;

    return us;
}

char * time2str(char * str, time_t time, bool gmt) 
{
    struct tm tm;

    if (gmt) {
        gmtime_r(&time, &tm);
        snprintf(str, MAX_TIME_STR,
                "%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d GMT",
                tm.tm_mon+1, tm.tm_mday, tm.tm_year%100,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else {
        localtime_r(&time, &tm);
        snprintf(str, MAX_TIME_STR,
                "%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
                tm.tm_mon+1, tm.tm_mday, tm.tm_year%100,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
    }


    return str;
}

bool ntp_synced(void)
{
    FILE * fp;
    char   s[100];
    bool   synced = false;

    // Raspberry Pi uses ntpq
    fp = popen("ntpq -c \"rv 0 stratum\" < /dev/null 2>&1", "re");
    if (fp != NULL) {
        int cnt=0, stratum=0;
        if (fgets(s, sizeof(s), fp) != NULL) {
            cnt = sscanf(s, "stratum=%d", &stratum);
            synced = (cnt == 1 && stratum > 0 && stratum < 16);
        }
        fclose(fp);
        if (synced) {
            INFO("return synced (ntpq stratum=%d okay)\n", stratum);
            return true;
        }
    }

    // Fedora 20 uses timedatectl
    fp = popen("timedatectl status < /dev/null 2>&1", "re");
    if (fp != NULL) {
        while (fgets(s, sizeof(s), fp) != NULL) {
            // check based on Fedora 20
            if (strstr(s, "NTP synchronized: yes") != NULL) {
                synced = true;
                break;
            }
            // check based on Fedora 31
            if (strstr(s, "System clock synchronized: yes") != NULL) {
                synced = true;
                break;
            }
        }
        fclose(fp);
        if (synced) {
            INFO("return synced (timedatectl okay)\n");
            return true;
        }
    }

    // return false
    INFO("return notsynced\n");
    return false;
}

void init_system_clock_offset_using_sntp(void)
{
    #define MAX_SERVER_LIST     (sizeof(server_name_list)/sizeof(char*))
    #define NTP_PORT            123
    #define VN                  4
    #define MODE                3
    #define DIFF_SEC_1900_1970  (2208988800u)

    #define TIMESTAMP_NET_TO_HOST(seconds,fraction) \
            (((uint64_t)ntohl(seconds) << 32) | ntohl(fraction))

    #define CLOCK_TO_TIMESTAMP_NET(seconds,fraction) \
            do { \
                struct timespec    ts; \
                clock_gettime(CLOCK_REALTIME,&ts); \
                (seconds) = htonl(ts.tv_sec + DIFF_SEC_1900_1970); \
                (fraction) = htonl(((uint64_t)ts.tv_nsec << 32) / 1000000000); \
            } while (0)

    #define CLOCK_TO_TIMESTAMP_HOST() \
            ( { struct timespec ts; \
                clock_gettime(CLOCK_REALTIME,&ts); \
                ts.tv_sec += DIFF_SEC_1900_1970; \
                ((uint64_t)ts.tv_sec << 32) | (((uint64_t)ts.tv_nsec << 32) / 1000000000); \
            } )

    static char      * server_name_list[5] = { "pool.ntp.org", 
                                               "0.pool.ntp.org", 
                                               "1.pool.ntp.org", 
                                               "2.pool.ntp.org", 
                                               "3.pool.ntp.org",
                                                     };

    uint32_t           request[12];
    uint32_t           response[12];
    int                sfd, i, recvlen;
    struct sockaddr_in local_addr;
    struct sockaddr_in server_addr;
    socklen_t          server_addr_len;
    struct timeval     rcvto;
    char               s[100];
    int64_t            sys_clk_off_us = 0;

    // documentation:
    //  - https://tools.ietf.org/html/rfc4330#section-5
    //  - RFC 4330    SNTPv4 for IPv4, IPv6 and OSI   January 2006

    // open socket, and bind local address
    sfd = socket(AF_INET, SOCK_DGRAM|SOCK_CLOEXEC, IPPROTO_UDP);
    if (sfd == -1) {
        ERROR("socket, %s\n",strerror(errno));
        return;
    }
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family      = AF_INET;
    local_addr.sin_port        = htons(0);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        ERROR("bind, %s\n",strerror(errno));
        close(sfd);
        return;
    }

    // loop over the list of candidate ntp servers
    // XXX should use an average
    for (i = 0; i < MAX_SERVER_LIST; i++) {
        char * server_name = server_name_list[i];

        // get server address
        if (getsockaddr(server_name, NTP_PORT, SOCK_DGRAM, IPPROTO_UDP, &server_addr) < 0) {
            ERROR("failed to get address of %s\n", server_name);
            continue;
        }
        DEBUG("address of %s is %s\n", server_name, sock_addr_to_str(s, sizeof(s), (struct sockaddr *)&server_addr));

        // format and send request
        bzero(request, sizeof(request));
        request[0] = htonl((VN << 27) | (MODE << 24));
        CLOCK_TO_TIMESTAMP_NET(request[10],request[11]);   // set Transmit Timestamp
        if (sendto(sfd, request, sizeof(request), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            ERROR("sendto, %s\n",strerror(errno));
            continue;
        }
        
        // wait for response, with 2 sec timeout
        rcvto.tv_sec  = 2;
        rcvto.tv_usec = 0;
        if (setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&rcvto, sizeof(rcvto)) < 0) {
            ERROR("setsockopt, %s\n",strerror(errno));
            continue;
        }
        server_addr_len = sizeof(server_addr);
        recvlen = recvfrom(sfd, response, sizeof(response), 0, (struct sockaddr *)&server_addr, (socklen_t *)&server_addr_len);
        if (recvlen < 0) {
            ERROR("recvfrom, %s\n",strerror(errno));
            continue;
        }

        // validate response, if not-ok then try next candidate server
        int64_t transmit_timestamp_request;
        int64_t originate_timestamp;
        int64_t receive_timestamp;
        int64_t transmit_timestamp;
        int64_t destination_timestamp;
        int32_t stratum;

        transmit_timestamp_request = TIMESTAMP_NET_TO_HOST(request[10],request[11]);
        originate_timestamp        = TIMESTAMP_NET_TO_HOST(response[6],response[7]);
        receive_timestamp          = TIMESTAMP_NET_TO_HOST(response[8],response[9]);
        transmit_timestamp         = TIMESTAMP_NET_TO_HOST(response[10],response[11]);
        destination_timestamp      = CLOCK_TO_TIMESTAMP_HOST();
        stratum                    = (ntohl(response[0]) >> 16) & 0xff;

        if (recvlen != 48 ||
            stratum == 0 ||
            originate_timestamp != transmit_timestamp_request)
        {
            ERROR("recvlen=%d, stratum=%d, originate_timestamp=%"PRId64" transmit_timestamp_request=%"PRId64"\n",
                  recvlen, stratum, originate_timestamp, transmit_timestamp_request);
            continue;
        }

        // compute round_trip_delay and sys_clk_off_us in microseconds,
        // the following is taken from the RFC 4330 ...
        //
        //   Timestamp Name          ID   When Generated
        //   ------------------------------------------------------------
        //   Originate Timestamp     T1   time request sent by client
        //   Receive Timestamp       T2   time request received by server
        //   Transmit Timestamp      T3   time reply sent by server
        //   Destination Timestamp   T4   time reply received by client
        // 
        //   The roundtrip delay d and system clock offset t are defined as:
        //
        //   d = (T4 - T1) - (T3 - T2)     t = ((T2 - T1) + (T3 - T4)) / 2.
        int64_t round_trip_delay_us;
        sys_clk_off_us = ((receive_timestamp - originate_timestamp) + 
                                  (transmit_timestamp - destination_timestamp)) / 2;
        sys_clk_off_us = sys_clk_off_us * 1000000 / 0x100000000;

        round_trip_delay_us = (destination_timestamp - originate_timestamp) - 
                              (transmit_timestamp - receive_timestamp);
        round_trip_delay_us = round_trip_delay_us * 1000000 / 0x100000000;

        DEBUG("originate_timestamp        = %"PRIu64"\n", originate_timestamp);
        DEBUG("receive_timestamp          = %"PRIu64"\n", receive_timestamp);
        DEBUG("transmit_timestamp         = %"PRIu64"\n", transmit_timestamp);
        DEBUG("destination_timestamp      = %"PRIu64"\n", destination_timestamp);
        DEBUG("sys_clk_off_us             = %"PRId64"\n", sys_clk_off_us);
        DEBUG("round_trip_delay_us        = %"PRId64"\n", round_trip_delay_us);

        // if round_trip_delay is too big we won't use this
        if (round_trip_delay_us > 500000) {
            ERROR("round_trip_delay %"PRId64" is too big\n", round_trip_delay_us);
            sys_clk_off_us = 0;
            continue;
        }

        // print success message
        INFO("success: server=%s %s, stratum=%d, sys_clk_off_us %"PRId64" us\n", 
             server_name, 
             sock_addr_to_str(s, sizeof(s), (struct sockaddr *)&server_addr),
             stratum,
             sys_clk_off_us);
        break;
    }

    // close socket
    close(sfd);

    // if failed then print error
    if (i == MAX_SERVER_LIST) {
        ERROR("failed to determine sys_clk_off_us\n");
        return;
    }

    // set system_clock_offset_us
    system_clock_offset_us = sys_clk_off_us;
}

// -----------------  FILE SYSTEM UTILS  ----------------------------------

#ifndef ANDROID
uint64_t fs_avail_bytes(char * path)
{
    int ret;
    uint64_t free_bytes;
    struct statvfs buf;

    ret = statvfs(path, &buf);
    if (ret < 0) {
        ERROR("statvfs, %s\n", strerror(errno));
        return 0;
    }

    free_bytes = (uint64_t)buf.f_bavail * buf.f_bsize;
    INFO("fs path %s, free_bytes=%"PRId64"\n", path, free_bytes);

    return free_bytes;
}
#endif

// -----------------  MISC UTILS  -----------------------------------------

char * status2str(uint32_t status)
{
    return STATUS_STR(status);
}

char * int2str(char * str, int64_t n)
{
    sprintf(str, "%"PRId64, n);
    return str;
}

