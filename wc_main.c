#include "wc.h"

//
// defines
//

//
// typedefs
//

//
// variables
//

static int  debug_mode;

//
// prototypes
//

int wc_svc_webcam_init(void);
void * wc_svc_webcam(void * cx);

// -----------------  MAIN  ---------------------------------------------------------

int main(int argc, char **argv)
{
    struct rlimit  rl;
    pthread_t      thread;
    int            ret, handle;
    char           opt_char;
    pthread_attr_t thread_attr;

    // set resource limti to allow core dumps
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    ret = setrlimit(RLIMIT_CORE, &rl);
    if (ret < 0) {
        WARN("setrlimit for core dump\n");
    }

    // parse options
    while (true) {
        opt_char = getopt(argc, argv, "d");
        if (opt_char == -1) {
            break;
        }
        switch (opt_char) {
        case 'd':
            debug_mode++;
            break;
        default:
            exit(1);
        }
    }

    // initialize net connection module
    if (net_init(16) < 0) {
        FATAL("net_init failed\n");
    }

    // initialize message logging
    logmsg_init(debug_mode == 0 ? "wc_server.log" : "stderr");
    INFO("STARTING %s\n", argv[0]);

    // init thread attribute for creating threads detached
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    // initialize temperature monitor
    ret = temper_init();
    if (ret < 0) {
        ERROR("temper_init failed\n");
    }

    // init webcam service
    wc_svc_webcam_init();

    // loop forever
    while (true) {
        // accept a connection
        handle = net_accept();
        if (handle < 0) {
            ERROR("net_accept failed\n");
            sleep(1);
            continue;
        }

        // create the service thread 
        INFO("creating thread wc_svc_webcam, net_handle=%d\n", handle);
        pthread_create(&thread, &thread_attr, wc_svc_webcam, (void*)(long)handle);
    }

    INFO("TERMINATING %s\n", argv[0]);
    return 0;
}
