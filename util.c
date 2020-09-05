#include "wc.h"

// -----------------  CONFIG READ / WRITE  -------------------------------

int config_read(char * config_path, config_t * config, int config_version)
{
    FILE * fp;
    int    i, version=0;
    char * name;
    char * value;
    char * saveptr;
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
    while (fgets(s, sizeof(s), fp) != NULL) {
        name = strtok_r(s, " \n", &saveptr);
        if (name == NULL || name[0] == '#') {
            continue;
        }

        value = strtok_r(NULL, " \n", &saveptr);
        if (value == NULL) {
            value = "";
        }

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

// -----------------  SOCKET UTILS  ---------------------------------------

int getsockaddr(char * node, int port, struct sockaddr_in * ret_addr)
{
    struct addrinfo   hints;
    struct addrinfo * result;
    char              port_str[20];
    int               ret;

    sprintf(port_str, "%d", port);

    bzero(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
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


#if 0  // XXX del
void set_sock_opts(int sfd, int reuseaddr, int sndbuf, int rcvbuf, int rcvto_us)
{
    int ret;

    if (reuseaddr != -1) {
        ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
        if (ret == -1) {
            ERROR("setsockopt SO_REUSEADDR, %s", strerror(errno));
        }
    }

    if (sndbuf != -1) {
        ret = setsockopt(sfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
        if (ret == -1) {
            ERROR("setsockopt SO_SNDBUF, %s", strerror(errno));
        }
    }

    if (rcvbuf != -1) {
        ret = setsockopt(sfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
        if (ret == -1) {
            ERROR("setsockopt SO_RCVBUF, %s", strerror(errno));
        }
    }

    if (rcvto_us != -1) {
        struct timeval rcvto = {rcvto_us/1000000, rcvto_us%1000000};
        ret = setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &rcvto, sizeof(rcvto));
        if (ret == -1) {
            ERROR("setsockopt SO_RCVBUF, %s", strerror(errno));
        }
    }
}
#endif

char * sock_to_options_str(int sfd, char * s, int slen)
{
    int reuseaddr=0, sndbuf=0, rcvbuf=0;
    struct timeval rcvto={0,0};
    socklen_t reuseaddr_len=4, sndbuf_len=4, rcvbuf_len=4, rcvto_len=sizeof(rcvto);

    getsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, &reuseaddr_len);
    getsockopt(sfd, SOL_SOCKET, SO_SNDBUF,    &sndbuf   , &sndbuf_len);
    getsockopt(sfd, SOL_SOCKET, SO_RCVBUF,    &rcvbuf   , &rcvbuf_len);
    getsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO,  &rcvto,     &rcvto_len);
    snprintf(s, slen, "REUSEADDR=%d SNDBUF=%d RCVBUF=%d RCVTIMEO=%d", 
             reuseaddr, sndbuf, rcvbuf,
             (int)(rcvto.tv_sec+1000000*rcvto.tv_usec));

    return s;
}

char * sock_addr_to_str(char * s, int slen, struct sockaddr * addr)
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

int do_recv(int sockfd, void * recv_buff, size_t len)
{
    int ret;
    size_t len_remaining = len;

    while (len_remaining) {
        ret = recv(sockfd, recv_buff, len_remaining, MSG_WAITALL);
        if (ret <= 0) {
            if (ret == 0) {
                errno = ENODATA;
            }
            return -1;
        }

        len_remaining -= ret;
        recv_buff += ret;
    }

    return len;
}

int do_send(int sockfd, void * send_buff, size_t len)
{
    int ret;
    size_t len_remaining = len;

    while (len_remaining) {
        ret = send(sockfd, send_buff, len_remaining, MSG_NOSIGNAL);
        if (ret <= 0) {
            if (ret == 0) {
                errno = ENODATA;
            }
            return -1;
        }

        len_remaining -= ret;
        send_buff += ret;
    }

    return len;
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

void printmsg(char *fmt, ...) 
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
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

void printmsg(char *fmt, ...) 
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, ap);
    va_end(ap);
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
    us = ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);

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
            if (strcmp(s, "NTP synchronized: yes\n") == 0) {
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
