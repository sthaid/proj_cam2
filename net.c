#include "wc.h"

//
// defines
//

#define TIMEOUT_US 10000000   // 10 secs

//
// typedefs
//

typedef struct {
    int   sd;
    void *nb_recv_buff;
    int   nb_recv_len;
    int   nb_recv_xfered;
} net_con_t;

//
// variables
//

static int listen_sd = -1;

//
// prototypes
//

static int getsockaddr(char * node, int port, struct sockaddr_in * ret_addr);
static char *sock_addr_to_str(char * s, int slen, struct sockaddr * addr);
static void set_sockopts(net_con_t *con);
static char *sprintf_sockopts(int sfd, char *s, int slen);

// -----------------  INIT  -------------------------------------------------------------

int net_init(bool is_server, int server_port)
{
    int                optval, sd;
    struct sockaddr_in addr;
    socklen_t          addrlen;

    // if not being called by the server then return
    if (!is_server) {
        return 0;
    }

    // create listen socket
    listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sd == -1) {
        FATAL("socket listen_sd\n");
    }

    // enable socket option to reuse the address
    optval = 1;
    if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        FATAL("setsockopt SO_REUSEADDR, %s", strerror(errno));
    }

    // bind listen socket to any IP addr, and on server_port
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(server_port);
    if (bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        FATAL("bind listen_sd\n");
    }

    // listen for a connect request
    if (listen(listen_sd, 10) == -1) {
        FATAL("listen\n");
    }

    // success
    return 0;
}

// -----------------  ESTABLISH CONNECTION  ---------------------------------------------

void *net_connect(char *ipaddr, int port, char *password, int *connect_status)
{
    int                rc, sd;
    struct sockaddr_in sin;
    net_con_t         *net_con;
    char               str[100];

    // XXX tbd
    *connect_status = STATUS_INFO_OK;

    // connect to the server 
    rc = getsockaddr(ipaddr, port, &sin);
    if (rc != 0) {
        ERROR("getsockaddr failed for %s, %s\n", ipaddr, strerror(errno));
        return NULL;
    }
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        FATAL("failed to create socket, %s\n",strerror(errno));
    }
    rc = connect(sd, (struct sockaddr *)&sin, sizeof(sin));
    if (rc < 0) {
        ERROR("connect to %s:%d failed, %s\n", ipaddr, port, strerror(errno));
        return NULL;
    }

    // allocate the net_con_t, and 
    // set snd and rcv timeouts
    net_con = calloc(1, sizeof(net_con_t));
    net_con->sd = sd;
    set_sockopts(net_con);
    INFO("sd=%d opts=%s\n", sd, sprintf_sockopts(sd, str, sizeof(str)));

    // send the password, and
    // wait for ack that the password was okay
    // XXX later

    // return the handle (net_con)
    return net_con;
}

void *net_accept(void)
{
    struct sockaddr_in addr;
    socklen_t          addrlen;
    int                sd;
    char               str[100];
    net_con_t        * net_con;

    // if listen_sd is invalid then fatal error
    if (listen_sd == -1) {
        FATAL("listen_sd is invalid\n");
    }

    // wait for a connect request, and accept the connection
    addrlen = sizeof(addr);
    sd = accept(listen_sd, (struct sockaddr *)&addr, &addrlen);
    if (sd == -1) {
        FATAL("accept, %s\n", strerror(errno));
    }
    INFO("accepted connection from %s\n",
         sock_addr_to_str(str, sizeof(str), (struct sockaddr*)&addr));

    // allocate the net_con_t, and 
    // set snd and rcv timeouts
    net_con = calloc(1, sizeof(net_con_t));
    net_con->sd = sd;
    set_sockopts(net_con);
    INFO("sd=%d opts=%s\n", sd, sprintf_sockopts(sd, str, sizeof(str)));

    // recv the password, and validate;
    // send ack/nak
    // XXX

    // return the handle
    return net_con;
}

void net_disconnect(void *handle)
{
    net_con_t *net_con = handle;

    // sanity check arg
    if (net_con == NULL) {
        ERROR("handle is NULL\n");
        return;
    }

    // shutdown and close
    shutdown(net_con->sd, SHUT_RDWR);
    close(net_con->sd);

    // free it
    free(net_con);
}

static int getsockaddr(char * node, int port, struct sockaddr_in * ret_addr)
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

static char *sock_addr_to_str(char * s, int slen, struct sockaddr * addr)
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

static void set_sockopts(net_con_t *con)
{
    struct timeval tout = {TIMEOUT_US/1000000, TIMEOUT_US%1000000};
    int    ret;

    ret = setsockopt(con->sd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout));
    if (ret == -1) {
        ERROR("setsockopt SO_RCVBUF, %s", strerror(errno));
    }

    ret = setsockopt(con->sd, SOL_SOCKET, SO_SNDTIMEO, &tout, sizeof(tout));
    if (ret == -1) {
        ERROR("setsockopt SO_SNDBUF, %s", strerror(errno));
    }
}

static char *sprintf_sockopts(int sfd, char *s, int slen)
{
    int reuseaddr=0, sndbuf=0, rcvbuf=0;
    struct timeval rcvto={0,0};
    struct timeval sndto={0,0};
    socklen_t reuseaddr_len=4, sndbuf_len=4, rcvbuf_len=4;
    socklen_t rcvto_len=sizeof(rcvto);
    socklen_t sndto_len=sizeof(sndto);

    getsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, &reuseaddr_len);
    getsockopt(sfd, SOL_SOCKET, SO_SNDBUF,    &sndbuf   , &sndbuf_len);
    getsockopt(sfd, SOL_SOCKET, SO_RCVBUF,    &rcvbuf   , &rcvbuf_len);
    getsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO,  &rcvto,     &rcvto_len);
    getsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO,  &sndto,     &sndto_len);
    snprintf(s, slen, "REUSEADDR=%d SNDBUF=%d RCVBUF=%d RCVTIMEO=%d SNDTIMEO", 
             reuseaddr, sndbuf, rcvbuf,
             (int)(rcvto.tv_sec+1000000*rcvto.tv_usec),
             (int)(sndto.tv_sec+1000000*sndto.tv_usec));
    return s;
}

// -----------------  SEND AND RECEIVE  -------------------------------------------------

// return:
// - len: success
// - -1:  error
int net_send(void *handle, void *buff, int len)
{
    int        ret;
    uint64_t   start_us;
    net_con_t *net_con = handle;
    size_t     len_remaining = len;

    if (net_con == NULL) {
        ERROR("handle is NULL\n");
        return -1;
    }

    start_us = microsec_timer();
    while (len_remaining) {
        ret = send(net_con->sd, buff, len_remaining, MSG_NOSIGNAL);
        if (ret <= 0) {
            if (ret == 0) errno = ENODATA;
            return -1;
        }
        if (microsec_timer() - start_us > (TIMEOUT_US-100000)) {
            errno = ETIMEDOUT;
            return -1;
        }

        len_remaining -= ret;
        buff += ret;
    }

    return len;
}

// XXX more comments on non blocking recv

// return:
// - len: success
// - 0:   when non-blocking is requested, and len bytes are not available
// - -1:  error
int net_recv(void *handle, void *buff, int len, bool non_blocking)
{
    int        ret;
    uint64_t   start_us;
    net_con_t *net_con = handle;
    size_t     len_remaining = len;

    // sanity check
    if (net_con == NULL) {
        ERROR("handle is NULL\n");
        return -1;
    }

    // sanity checks
    if (non_blocking == true && net_con->nb_recv_buff != NULL) {
        if (buff != net_con->nb_recv_buff || len != net_con->nb_recv_len) {
            FATAL("non_blocking invalid buff or len\n");
        }
    }
    if (non_blocking == false && net_con->nb_recv_buff != NULL) {
        FATAL("blocking call made when non_blocking buff is set\n");
    }

    // XXX tout for non blocking
    // - the viewer should be receiving a status msg once per sec, so it could have a tout
    // - the wc does not recv a periodic msg, so no tout for it

    // check for, and handle non_blocking recv request
    if (non_blocking) {
        // if a non blocking read was not in progress then
        // remember the caller's buff and len because the following 
        // call to recv may just just return a partially filled buffer
        if (net_con->nb_recv_buff == NULL) {
            net_con->nb_recv_buff   = buff;
            net_con->nb_recv_len    = len;
            net_con->nb_recv_xfered = 0;
        }

        // recv the data, return
        //  -1 - on a error
        //   0 - if the caller's buffer has not yet been filled
        // len - if the caller's buffer has been filled
        ret = recv(net_con->sd, 
                   net_con->nb_recv_buff + net_con->nb_recv_xfered,
                   net_con->nb_recv_len - net_con->nb_recv_xfered,
                   MSG_DONTWAIT);
        if (ret < 0) {
            return (errno == EWOULDBLOCK ? 0 : -1);
        } else {
            net_con->nb_recv_xfered += ret;
            if (net_con->nb_recv_xfered == len) {
                net_con->nb_recv_buff   = NULL;
                net_con->nb_recv_len    = 0;
                net_con->nb_recv_xfered = 0;
                return len;
            } else {
                return 0;
            }
        }
    }

    // blocking recv request ...

    // do the recv
    start_us = microsec_timer();
    while (len_remaining) {
        ret = recv(net_con->sd, buff, len_remaining, MSG_WAITALL);
        if (ret <= 0) {
            if (ret == 0) errno = ENODATA;
            return -1;
        }
        if (microsec_timer() - start_us > (TIMEOUT_US-100000)) {
            errno = ETIMEDOUT;
            return -1;
        }

        len_remaining -= ret;
        buff += ret;
    }
    return len;
}

// -----------------  GET STATS  --------------------------------------------------------

void net_get_stats(void *handle, net_stats_t *stats)
{
    net_con_t *net_con = handle;

    if (net_con == NULL) {
        ERROR("handle is NULL\n");
        return;
    }

    // XXX later
}
