// XXX rename
// XXX recv should use a timeout, and not the flags

// XXX all routines check the handle is valid

#include "wc.h"

#define WC_PORT 9950 

int listen_sd = -1;

// --------------------------------------------------------------------------------------

int p2p_init(bool is_webcam)
{
    int                optval, sd;
    struct sockaddr_in addr;
    socklen_t          addrlen;

    // if not the webcam then just return
    if (!is_webcam) {
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

    // bind listen socket to any IP addr, and WC_PORT
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(WC_PORT);
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

// --------------------------------------------------------------------------------------

int p2p_connect(char * wc_name, char * password, int * connect_status)
{
    int rc, sd;
    struct sockaddr_in sin;

    // connect to the webcam 
    rc = getsockaddr(wc_name, WC_PORT, &sin);
    if (rc != 0) {
        ERROR("getsockaddr failed for %s, %s\n", wc_name, strerror(errno));
        return -1;
    }
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        FATAL("failed to create socket, %s\n",strerror(errno));
    }
    rc = connect(sd, (struct sockaddr *)&sin, sizeof(sin));
    if (rc < 0) {
        ERROR("connect to %s failed, %s\n", wc_name, strerror(errno));
        return -1;
    }

    // send the password, and
    // wait for ack that the password was okay

    // return the handle (sd)
    return sd;
}

int p2p_accept(void)
{
    struct sockaddr_in addr;
    socklen_t          addrlen;
    int                sd;
    char               str[100];

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

    // recv the password, and validate;
    // send ack/nak

    // return the handle (sd)
    return sd;
}

int p2p_disconnect(int handle)
{
    // xxx check handle is valid

    // close the handle
    // xxx shutdown too
    close(handle);

    // success
    return 0;
}

// --------------------------------------------------------------------------------------

int p2p_send(int handle, void * buff, int len)
{
    return do_send(handle, buff, len);
}

int p2p_recv(int handle, void * buff, int len)
{
    // XXX add timeout
    return do_recv(handle, buff, len);
}

// --------------------------------------------------------------------------------------

int p2p_get_stats(int handle, p2p_stats_t * stats)
{
    // XXX later
    return 0;
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
