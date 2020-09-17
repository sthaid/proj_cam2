#include "wc.h"

#include <SDL.h>
#include <SDL_ttf.h>

//
// button sound option definitions
//

#ifdef ENABLE_BUTTON_SOUND
    #include <SDL_mixer.h>
    #include "button_sound.h"
    #define PLAY_BUTTON_SOUND() \
        do { \
            Mix_PlayChannel(-1, button_sound, 0); \
        } while (0)
    Mix_Chunk * button_sound;
#else
    #define PLAY_BUTTON_SOUND()
#endif

//
// defines 
//

#define MAX_WEBCAM                  4
#define MAX_WC_DEF                  8     // care needed when chaning this
#define MAX_STR                     200 

#define CTL_COLS                    14
#define CTL_ROWS                    7
#define KEYBD_STR_COLS              14

#define MS                          1000
#define HIGHLIGHT_TIME_US           (2000*MS)
#define RECONNECT_TIME_US           (10000*MS)

#define CONFIG_WC_DEF(def_idx)      (config[(def_idx)].value)
#define CONFIG_WC_SELECT(sel_idx)   (config[25+(sel_idx)].value)
#define CONFIG_ZOOM                 (config[29].value[0])    // A, B, C, D, N
#define CONFIG_DEBUG                (config[30].value[0])    // N, Y
#define CONFIG_FONT_PTSZ            (config[31].value)

#ifndef ANDROID 
#define WIN_WIDTH                   1700
#define WIN_HEIGHT                  900
#define SDL_FLAGS                   SDL_WINDOW_RESIZABLE
#else
#define WIN_WIDTH                   2000   // the width/height should be ignored by SDL
#define WIN_HEIGHT                  1000   // because fullscreen requested
#define SDL_FLAGS                   SDL_WINDOW_FULLSCREEN_DESKTOP
#endif

#ifndef ANDROID
#define FONT_PATH                    "/usr/share/fonts/gnu-free/FreeMonoBold.ttf"
#define FONT_DEFAULT_PTSIZE          "50"
#else
#define FONT_PATH                    "/system/fonts/DroidSansMono.ttf"
#define FONT_DEFAULT_PTSIZE          "70"
#endif

#define PANE_COLS(p)                 ((double)(p)->w / font.char_width)
#define PANE_ROWS(p)                 ((double)(p)->h / font.char_height)

#define MAX_MOUSE_EVENT                       300
#define MOUSE_EVENT_NONE                      0   // no event
#define MOUSE_EVENT_QUIT                      1   // quit
#define MOUSE_EVENT_MODE_SELECT               2   // select live or playback
#define MOUSE_EVENT_STATUS_SELECT             3   // chosse status display (lower right corner)
#define MOUSE_EVENT_PLAYBACK_STOP             8   // playback ctrls
#define MOUSE_EVENT_PLAYBACK_PLAY             9
#define MOUSE_EVENT_PLAYBACK_PAUSE            10
#define MOUSE_EVENT_PLAYBACK_FORWARD          11
#define MOUSE_EVENT_PLAYBACK_REVERSE          12
#define MOUSE_EVENT_PLAYBACK_FASTER           13
#define MOUSE_EVENT_PLAYBACK_SLOWER           14
#define MOUSE_EVENT_PLAYBACK_HOUR_MINUS       15
#define MOUSE_EVENT_PLAYBACK_HOUR_PLUS        16
#define MOUSE_EVENT_PLAYBACK_MINUTE_MINUS     17
#define MOUSE_EVENT_PLAYBACK_MINUTE_PLUS      18
#define MOUSE_EVENT_CONFIG_KEYBD_ASCII_FIRST  32     // config ctrls
#define MOUSE_EVENT_CONFIG_KEYBD_ASCII_LAST   126
#define MOUSE_EVENT_CONFIG_MODE_ENTER         140
#define MOUSE_EVENT_CONFIG_ACCEPT             141 
#define MOUSE_EVENT_CONFIG_CANCEL             142
#define MOUSE_EVENT_CONFIG_SELECT             143       // room for 25 events (MAX_WC_DERS)
#define MOUSE_EVENT_CONFIG_FONT_PLUS          170
#define MOUSE_EVENT_CONFIG_FONT_MINUS         171
#define MOUSE_EVENT_CONFIG_KEYBD_SHIFT        172
#define MOUSE_EVENT_CONFIG_KEYBD_BS           173
#define MOUSE_EVENT_CONFIG_KEYBD_NEXT_STR     174
#define MOUSE_EVENT_CONFIG_KEYBD_PREV_STR     175
#define MOUSE_EVENT_CONFIG_KEYBD_ACCEPT       176
#define MOUSE_EVENT_CONFIG_KEYBD_CANCEL       177

#define MOUSE_EVENT_WC_NAME_LIST              180    // webcam pane ctrls, room for 100 events (4xMAX_WC_DEFS)
#define MOUSE_EVENT_WC_NAME                   280       // 4 events
#define MOUSE_EVENT_WC_RES                    285       // 4 events
#define MOUSE_EVENT_WC_ZOOM                   290       // 4 events

#define QUIT_EVENT_REASON_MOUSE               1
#define QUIT_EVENT_REASON_SDL                 2

#define STATE_NOT_CONNECTED                   0
#define STATE_CONNECTING                      1
#define STATE_CONNECTED                       2
#define STATE_CONNECTING_ERROR                3
#define STATE_CONNECTED_ERROR                 4
#define STATE_FATAL_ERROR                     5

#define STATE_STR(state) \
   ((state) == STATE_NOT_CONNECTED     ? "STATE_NOT_CONNECTED"     : \
    (state) == STATE_CONNECTING        ? "STATE_CONNECTING"        : \
    (state) == STATE_CONNECTED         ? "STATE_CONNECTED"         : \
    (state) == STATE_CONNECTING_ERROR  ? "STATE_CONNECTING_ERROR"  : \
    (state) == STATE_CONNECTED_ERROR   ? "STATE_CONNECTED_ERROR"   : \
    (state) == STATE_FATAL_ERROR       ? "STATE_FATAL_ERROR"         \
                                       : "????")

#define SET_CTL_MODE_LIVE() \
    do { \
        mode.mode = MODE_LIVE; \
        mode.pb_submode = PB_SUBMODE_STOP; \
        mode.pb_mode_entry_real_time_us = 0; \
        mode.pb_real_time_us = 0; \
        mode.pb_dir = PB_DIR_FWD; \
        mode.pb_speed = 1; \
        mode.mode_id++; \
    } while (0)

#define SET_CTL_MODE_PLAYBACK_STOP(init) \
    do { \
        mode.mode = MODE_PLAYBACK; \
        if (mode.pb_submode == PB_SUBMODE_PLAY) { \
            mode.pb_real_time_us = PB_SUBMODE_PLAY_REAL_TIME_US(&mode); \
            if (mode.pb_real_time_us > get_real_time_us()) { \
                mode.pb_real_time_us = get_real_time_us(); \
            } \
        } \
        mode.pb_mode_entry_real_time_us = get_real_time_us(); \
        if (init) { \
            mode.pb_real_time_us = get_real_time_us(); \
            mode.pb_dir = PB_DIR_FWD; \
            mode.pb_speed = 1; \
        } \
        mode.pb_submode = PB_SUBMODE_STOP; \
        mode.mode_id++; \
    } while (0)

#define SET_CTL_MODE_PLAYBACK_PAUSE(init) \
    do { \
        mode.mode = MODE_PLAYBACK; \
        if (mode.pb_submode == PB_SUBMODE_PLAY) { \
            mode.pb_real_time_us = PB_SUBMODE_PLAY_REAL_TIME_US(&mode); \
        } \
        mode.pb_mode_entry_real_time_us = get_real_time_us(); \
        if (init) { \
            mode.pb_real_time_us = get_real_time_us() - 2000000; \
            mode.pb_dir = PB_DIR_FWD; \
            mode.pb_speed = 1; \
        } \
        mode.pb_submode = PB_SUBMODE_PAUSE; \
        mode.mode_id++; \
    } while (0)

#define SET_CTL_MODE_PLAYBACK_PLAY() \
    do { \
        mode.mode = MODE_PLAYBACK; \
        if (mode.pb_submode == PB_SUBMODE_PLAY) { \
            mode.pb_real_time_us = PB_SUBMODE_PLAY_REAL_TIME_US(&mode); \
        } \
        mode.pb_mode_entry_real_time_us = get_real_time_us(); \
        mode.pb_submode = PB_SUBMODE_PLAY; \
        mode.mode_id++; \
    } while (0)

#define SET_CTL_MODE_PLAYBACK_TIME(delta_sec) \
    do { \
        uint64_t curr_real_time_us = get_real_time_us(); \
        uint64_t new_pb_real_time_us; \
        mode.mode = MODE_PLAYBACK; \
        if (mode.pb_submode == PB_SUBMODE_PLAY) { \
            new_pb_real_time_us = PB_SUBMODE_PLAY_REAL_TIME_US(&mode) + \
                                  ((int64_t)(delta_sec) * 1000000); \
        } else { \
            new_pb_real_time_us = mode.pb_real_time_us + \
                                  ((int64_t)(delta_sec) * 1000000); \
        } \
        if (new_pb_real_time_us > curr_real_time_us - 2000000) { \
            new_pb_real_time_us = curr_real_time_us - 2000000; \
        } \
        mode.pb_mode_entry_real_time_us = curr_real_time_us; \
        mode.pb_real_time_us = new_pb_real_time_us; \
        if (mode.pb_submode == PB_SUBMODE_STOP) { \
            mode.pb_submode = PB_SUBMODE_PAUSE; \
        } \
        mode.mode_id++; \
    } while (0)

#define SET_CTL_MODE_PLAYBACK_DIR(dir) \
    do { \
        mode.mode = MODE_PLAYBACK; \
        if (mode.pb_submode == PB_SUBMODE_PLAY) { \
            mode.pb_real_time_us = PB_SUBMODE_PLAY_REAL_TIME_US(&mode); \
        } \
        mode.pb_mode_entry_real_time_us = get_real_time_us(); \
        mode.pb_dir = (dir); \
        mode.mode_id++; \
    } while (0)

#define SET_CTL_MODE_PLAYBACK_SPEED(speed)  \
    do { \
        mode.mode = MODE_PLAYBACK; \
        if (mode.pb_submode == PB_SUBMODE_PLAY) { \
            mode.pb_real_time_us = PB_SUBMODE_PLAY_REAL_TIME_US(&mode); \
        } \
        mode.pb_mode_entry_real_time_us = get_real_time_us(); \
        mode.pb_speed = (speed); \
        mode.mode_id++; \
    } while (0)

#define CONFIG_WRITE() \
    do { \
        config_write(config_path, config, config_version); \
    } while (0)

#define CVT_INTERVAL_SECS_TO_DAY_HMS(dur_secs, days, hours, minutes, seconds) \
    do { \
        time_t d = (dur_secs); \
        (days) = d / 86400; \
        d -= ((days) * 86400); \
        (hours) = d / 3600; \
        d -= ((hours) * 3600); \
        (minutes) = d / 60; \
        d -= ((minutes) * 60); \
        (seconds) = d; \
    } while (0)

//
// typedefs
//

typedef struct {
    char            name[MAX_STR];
    char            ipaddr[MAX_STR];
    int             port;
    char            password[MAX_STR];
    char            def_str[MAX_STR];

    uint32_t        state;
    struct mode_s   mode;
    struct status_s status; 
    uint64_t        recvd_bytes;
    uint64_t        recvd_frames;
    uint32_t        frame_status;

    SDL_Texture   * texture;
    int             texture_w;
    int             texture_h;

    bool            change_resolution_request;
    int             change_name_request;
    bool            name_select_mode;

    pthread_mutex_t image_mutex;
    uint64_t        image_change;
    char            image_name[MAX_STR];
    char            image_res[MAX_STR];
    bool            image_display;
    bool            image_highlight;
    int             image_temperature;
    uint8_t       * image;
    int             image_w;
    int             image_h;
    char            image_notification_str1[MAX_STR];
    char            image_notification_str2[MAX_STR];
    char            image_notification_str3[MAX_STR];
} webcam_t;

typedef struct {
    int      mouse_event;
    SDL_Rect mouse_event_pos[MAX_MOUSE_EVENT];

    bool     window_resize_event;
    int      window_resize_width;
    int      window_resize_height;

    bool     window_restored_event;

    bool     window_minimized_event;

    bool     quit_event;
    int      quit_event_reason;
} event_t;

typedef struct {
    TTF_Font * font; 
    int        char_width;
    int        char_height;
} font_t;

//
// variables
//

struct mode_s    mode;

int              webcam_threads_running_count;

SDL_Window     * window;
SDL_Renderer   * renderer;
int              win_width;
int              win_height;
bool             win_minimized;

font_t           font;

webcam_t         webcam[MAX_WEBCAM];

event_t          event;

char             config_path[MAX_STR];
const int        config_version = 23;
config_t         config[] = { { "wc_define_0",  "none"           },
                              { "wc_define_1",  "none"           },
                              { "wc_define_2",  "none"           },
                              { "wc_define_3",  "none"           },
                              { "wc_define_4",  "none"           },
                              { "wc_define_5",  "none"           },
                              { "wc_define_6",  "none"           },
                              { "wc_define_7",  "none"           },
                              { "wc_define_8",  "none"           },
                              { "wc_define_9",  "none"           },
                              { "wc_define_10", "none"           },
                              { "wc_define_11", "none"           },
                              { "wc_define_12", "none"           },
                              { "wc_define_13", "none"           },
                              { "wc_define_14", "none"           },
                              { "wc_define_15", "none"           },
                              { "wc_define_16", "none"           },
                              { "wc_define_17", "none"           },
                              { "wc_define_18", "none"           },
                              { "wc_define_19", "none"           },
                              { "wc_define_20", "none"           },
                              { "wc_define_21", "none"           },
                              { "wc_define_22", "none"           },
                              { "wc_define_23", "none"           },
                              { "wc_define_24", "none"           },
                              { "wc_select_A", "7"               },
                              { "wc_select_B", "7"               },
                              { "wc_select_C", "7"               },
                              { "wc_select_D", "7"               },
                              { "zoom",      "N",                },
                              { "debug",     "Y"                 },
                              { "font_ptsz", FONT_DEFAULT_PTSIZE },
                              { "",          ""                  } };

//
// prototypes
//

void display_handler(void);
void render_text(SDL_Rect * pane, double row, double col, char * str, int mouse_event);
void render_text_ex(SDL_Rect * pane, double row, double col, char * str, int mouse_event, 
                    int field_cols, bool center);
void * webcam_thread(void * cx);

// -----------------  MAIN  ----------------------------------------------

int main(int argc, char **argv)
{
    struct rlimit    rl;
    int              ret, i, count;
    const char     * config_dir;

    // set resource limti to allow core dumps
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    ret = setrlimit(RLIMIT_CORE, &rl);
    if (ret < 0) {
        WARN("setrlimit for core dump, %s\n", strerror(errno));
    }

    // reset global variables to their default values; 
    // this is needed on Android because the Java Shim can re-invoke
    // main without resetting these
    bzero(&mode, sizeof(mode));
    webcam_threads_running_count = 0;
    window = NULL;
    renderer = NULL;
    win_width = 0;
    win_height = 0;
    win_minimized = false;
    bzero(&font, sizeof(font));
    bzero(webcam, sizeof(webcam));
    bzero(&event, sizeof(event));

    // read viewer config
#ifndef ANDROID
    config_dir = getenv("HOME");
    if (config_dir == NULL) {
        FATAL("env var HOME not set\n");
    }
#else
    config_dir = SDL_AndroidGetInternalStoragePath();
    if (config_dir == NULL) {
        FATAL("android internal storage path not set\n");
    }
#endif
    sprintf(config_path, "%s/.viewer2_config", config_dir);
    if (config_read(config_path, config, config_version) < 0) {
        FATAL("config_read failed for %s\n", config_path);
    }

    // init logging
    logmsg_init(CONFIG_DEBUG == 'Y' ? "stderr" : "none");
    INFO("STARTING %s\n", argv[0]);

    // initialize net connection modules, 
    // note: false means this is being called by viewer
    if (net_init(false,0) < 0) {
        FATAL("net_init failed\n");
    }

    // it is important that the viewer and the webcam computers have their 
    // clocks in sync; the webcam computers use ntp; unfortunately my Android tablet 
    // (which viewer runs on) does not have an option to sync to network time, so this
    // call is made to check if ntp is synced; and if it is not synced then to determine
    // a time offset which is incorporated in the time returned by calls to 
    // get_real_time_us and get_real_time_sec
    if (!ntp_synced()) {
        init_system_clock_offset_using_sntp();
    }

    // initialize to live mode
    SET_CTL_MODE_LIVE();

    // initialize Simple DirectMedia Layer  (SDL)
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
        FATAL("SDL_Init failed\n");
    }

    // create SDL Window and Renderer
    if (SDL_CreateWindowAndRenderer(WIN_WIDTH, WIN_HEIGHT, SDL_FLAGS, &window, &renderer) != 0) {
        FATAL("SDL_CreateWindowAndRenderer failed\n");
    }
    SDL_GetWindowSize(window, &win_width, &win_height);
    INFO("win_width=%d win_height=%d\n", win_width, win_height);

#ifdef ENABLE_BUTTON_SOUND
    // init button_sound
    if (Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        FATAL("Mix_OpenAudio failed\n");
    }
    button_sound = Mix_QuickLoad_WAV(button_sound_wav);
    Mix_VolumeChunk(button_sound,MIX_MAX_VOLUME/4);
    if (button_sound == NULL) {
        FATAL("Mix_QuickLoadWAV failed\n");
    }
#endif

    // initialize True Type Font
    // note - the TTF_OpenFont call is made at the begining of display_handler()
    //        to handle font_ptsz changing during runtime
    if (TTF_Init() < 0) {
        FATAL("TTF_Init failed\n");
    }

    // create webcam threads 
    for (i = 0; i < MAX_WEBCAM; i++) {
        pthread_t webcam_thread_id;
        pthread_create(&webcam_thread_id, NULL, webcam_thread, (void*)(long)i);
    }

    // wait for up to 3 second for the webcam threads to initialize
    count = 0;
    while (webcam_threads_running_count != MAX_WEBCAM && count++ < 3000/10) {
        usleep(10*MS);
    }
    if (webcam_threads_running_count != MAX_WEBCAM) {
        FATAL("webcam threads failed to start\n");
    }

    // loop: processing events and updating display
    while (!event.quit_event) {
        display_handler();
        usleep(10*MS);
    }

    // program is exitting ...

    // wait for up to 3 second for the webcam threads to terminate
    count = 0;
    while (webcam_threads_running_count != 0 && count++ < 3000/10) {
        usleep(10*MS);
    }
    if (webcam_threads_running_count != 0) {
        WARN("webcam threads failed to terminate\n");
    }

    // cleanup
#ifdef ENABLE_BUTTON_SOUND
    Mix_FreeChunk(button_sound);
    Mix_CloseAudio();
#endif

    TTF_CloseFont(font.font);
    TTF_Quit();

    SDL_PumpEvents();  // helps on Android when terminating via return
    for (i = 0; i < MAX_WEBCAM; i++) {
        if (webcam[i].texture) {
            SDL_DestroyTexture(webcam[i].texture); 
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    // terminate ...

    // if terminating due to QUIT_EVENT_REASON_MOUSE then call exit;
    // on Android this causes the App (including the java shim) to terminate
    if (event.quit_event_reason == QUIT_EVENT_REASON_MOUSE) {
        INFO("TERMINATING %s: CALL EXIT\n", argv[0]);
        exit(0);
    }

    // return 0
    // - on Android the java shim continues to run and the java shim may invoke main again
    // - for example, on Android when changing the device orientation, the SDL_QUIT event
    //   is generated causing program termination; followed by re-invoking of main
    INFO("TERMINATING %s: RETURN FROM MAIN\n", argv[0]);
    return 0;
}

// -----------------  DISPLAY HANDLER  -----------------------------------

void display_handler(void)
{
    #define CONFIG_KEYBD_MODE_INACTIVE 0

    #define INIT_POS(r,_x,_y,_w,_h) \
        do { \
            (r).x = (_x); \
            (r).y = (_y); \
            (r).w = (_w); \
            (r).h = (_h); \
        } while (0)

    #define MOUSE_AT_POS(pos) ((ev.button.x >= (pos).x - 10) && \
                               (ev.button.x < (pos).x + (pos).w + 10) && \
                               (ev.button.y >= (pos).y - 10) && \
                               (ev.button.y < (pos).y + (pos).h + 10))

    #define SDL_WINDOWEVENT_STR(x) \
       ((x) == SDL_WINDOWEVENT_SHOWN        ? "SDL_WINDOWEVENT_SHOWN"        : \
        (x) == SDL_WINDOWEVENT_HIDDEN       ? "SDL_WINDOWEVENT_HIDDEN"       : \
        (x) == SDL_WINDOWEVENT_EXPOSED      ? "SDL_WINDOWEVENT_EXPOSED"      : \
        (x) == SDL_WINDOWEVENT_MOVED        ? "SDL_WINDOWEVENT_MOVED"        : \
        (x) == SDL_WINDOWEVENT_RESIZED      ? "SDL_WINDOWEVENT_RESIZED"      : \
        (x) == SDL_WINDOWEVENT_SIZE_CHANGED ? "SDL_WINDOWEVENT_SIZE_CHANGED" : \
        (x) == SDL_WINDOWEVENT_MINIMIZED    ? "SDL_WINDOWEVENT_MINIMIZED"    : \
        (x) == SDL_WINDOWEVENT_MAXIMIZED    ? "SDL_WINDOWEVENT_MAXIMIZED"    : \
        (x) == SDL_WINDOWEVENT_RESTORED     ? "SDL_WINDOWEVENT_RESTORED"     : \
        (x) == SDL_WINDOWEVENT_ENTER        ? "SDL_WINDOWEVENT_ENTER"        : \
        (x) == SDL_WINDOWEVENT_LEAVE        ? "SDL_WINDOWEVENT_LEAVE"        : \
        (x) == SDL_WINDOWEVENT_FOCUS_GAINED ? "SDL_WINDOWEVENT_FOCUS_GAINED" : \
        (x) == SDL_WINDOWEVENT_FOCUS_LOST   ? "SDL_WINDOWEVENT_FOCUS_LOST"   : \
        (x) == SDL_WINDOWEVENT_CLOSE        ? "SDL_WINDOWEVENT_CLOSE"        : \
                                              "????")

    int         i, j, font_ptsz;
    char        str[MAX_STR];
    char        date_and_time_str[MAX_TIME_STR];
    SDL_Event   ev;
    bool        event_handled;
    uint64_t    curr_us;

    SDL_Rect    ctlpane;
    SDL_Rect    ctlbpane;
    SDL_Rect    configpane;
    SDL_Rect    wcpane[MAX_WEBCAM];
    SDL_Rect    wctitlepane[MAX_WEBCAM];
    SDL_Rect    wcimagepane[MAX_WEBCAM];

    static struct {
        bool  enabled;
        char  wc_def[MAX_WC_DEF][MAX_STR];
        bool  keybd_enabled;
        int   keybd_def_idx;
        int   keybd_str_idx;
        char  keybd_str_value[4][MAX_STR];
        char *keybd_str_prompt[4];
        bool  keybd_shift;
    } config_mode;

    static int  status_select;
    static int  last_font_ptsz;

    // ----------------------------------------------
    // ---- if CONFIG_FONT_PTSZ has changed then ----
    // ---- close and reopen font with new ptsz  ----
    // ----------------------------------------------

    sscanf(CONFIG_FONT_PTSZ, "%d", &font_ptsz);
    if (font_ptsz != last_font_ptsz) {
        TTF_CloseFont(font.font);

        font.font = TTF_OpenFont(FONT_PATH, font_ptsz);
        if (font.font == NULL) {
            FATAL("failed TTF_OpenFont %s\n", FONT_PATH);
        }

        TTF_SizeText(font.font, "X", &font.char_width, &font.char_height);

        DEBUG("font_ptsz is now %d\n", font_ptsz);
        last_font_ptsz = font_ptsz;
    }

    // ----------------------------------------
    // ---- check if an event has occurred ----
    // ----------------------------------------

    while (true) {
        // get the next event, break out of loop if no event
        if (SDL_PollEvent(&ev) == 0) {
            break;
        }

        // process the SDL event, this code sets one or more of
        // the following event indicators
        // - event.mouse_event
        // - event.quit_event
        // - event.window_resize_event
        switch (ev.type) {
        case SDL_MOUSEBUTTONDOWN: {
            int i;
            DEBUG("MOUSE DOWN which=%d button=%s state=%s x=%d y=%d\n",
                   ev.button.which,
                   (ev.button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                    ev.button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                    ev.button.button == SDL_BUTTON_RIGHT  ? "RIGHT" :
                                                            "???"),
                   (ev.button.state == SDL_PRESSED  ? "PRESSED" :
                    ev.button.state == SDL_RELEASED ? "RELEASED" :
                                                      "???"),
                   ev.button.x,
                   ev.button.y);

            if (ev.button.button != SDL_BUTTON_LEFT) {
                break;
            }

            for (i = 0; i < MAX_MOUSE_EVENT; i++) {
                if (MOUSE_AT_POS(event.mouse_event_pos[i])) {
                    if (i == MOUSE_EVENT_QUIT) {
                        DEBUG("got event MOUSE_EVENT_QUIT\n");
                        event.quit_event = true;
                        event.quit_event_reason = QUIT_EVENT_REASON_MOUSE;
                    } else {
                        DEBUG("got event MOUSE_EVENT %d\n", i);
                        event.mouse_event = i;
                    }
                    PLAY_BUTTON_SOUND();
                    break;
                }
            }
            break; }

        case SDL_KEYDOWN: {
            int  key = ev.key.keysym.sym;
            bool shift = (ev.key.keysym.mod & KMOD_SHIFT) != 0;

            if (config_mode.keybd_enabled == false) {
                break;
            }

            if (key == 8) {  // backspace
                event.mouse_event = MOUSE_EVENT_CONFIG_KEYBD_BS;
            } else if (key == '\r') {  // enter, aka carriage return
                event.mouse_event = MOUSE_EVENT_CONFIG_KEYBD_ACCEPT;
            } else if (key == '\e') {  // esc
                event.mouse_event = MOUSE_EVENT_CONFIG_KEYBD_CANCEL;
            } else if (key == SDLK_UP) {  // up arrow
                event.mouse_event = MOUSE_EVENT_CONFIG_KEYBD_PREV_STR;
            } else if (key == SDLK_DOWN || key == '\t') {  // down arrow || tab
                event.mouse_event = MOUSE_EVENT_CONFIG_KEYBD_NEXT_STR;
            } else if (!shift && ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9'))) {
                event.mouse_event = key;
            } else if (shift && (key >= 'a' && key <= 'z')) {
                event.mouse_event = 'A' + (key - 'a');
            } else if (shift && key == '-') {
                event.mouse_event = '_';
            } else if (!shift && (key == '-' || key == '.')) {
                event.mouse_event = key;
            } else {
                break;
            }
            PLAY_BUTTON_SOUND();
            break; }

       case SDL_WINDOWEVENT: {
            DEBUG("got event SDL_WINOWEVENT - %s\n", SDL_WINDOWEVENT_STR(ev.window.event));
            switch (ev.window.event)  {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                event.window_resize_width = ev.window.data1;
                event.window_resize_height = ev.window.data2;
                event.window_resize_event = true;
                PLAY_BUTTON_SOUND();
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                event.window_minimized_event = true;
                PLAY_BUTTON_SOUND();
                break;
            case SDL_WINDOWEVENT_RESTORED:
                event.window_restored_event = true;
                PLAY_BUTTON_SOUND();
                break;
            }
            break; }

        case SDL_QUIT: {
            DEBUG("got event SDL_QUIT\n");
            event.quit_event = true;
            event.quit_event_reason = QUIT_EVENT_REASON_SDL;
            PLAY_BUTTON_SOUND();
            break; }

        case SDL_MOUSEMOTION: {
            break; }

        default: {
            DEBUG("got event %d - not supported\n", ev.type);
            break; }
        }

        // break if mouse_event, window event or quit_event is set
        if (event.mouse_event != MOUSE_EVENT_NONE || 
            event.window_resize_event ||
            event.window_minimized_event ||
            event.window_restored_event ||
            event.quit_event) 
        {
            break; 
        }
    }

    // -----------------------------------------------
    // ---- handle events found by the above code ----
    // -----------------------------------------------

    // start with event_handled flag clear
    event_handled = false;

    // quit event
    if (event.quit_event) {
        return;
    }

    // window events
    if (event.window_resize_event) {
        win_width = event.window_resize_width;
        win_height = event.window_resize_height;
        event.window_resize_event = false;
        event_handled = true;
    }

    if (event.window_minimized_event) {
        win_minimized = true;
        event.window_minimized_event = false;
        event_handled = true;
    }

    if (event.window_restored_event) {
        win_minimized = false;
        event.window_restored_event = false;
        event_handled = true;
    }

    // mouse and keyboard events
    if (event.mouse_event != MOUSE_EVENT_NONE) {
        if (event.mouse_event == MOUSE_EVENT_MODE_SELECT) {
            if (mode.mode == MODE_PLAYBACK) {
                SET_CTL_MODE_LIVE();
            } else {
                SET_CTL_MODE_PLAYBACK_PAUSE(true);
            }

        } else if (event.mouse_event == MOUSE_EVENT_STATUS_SELECT) {
            status_select = (status_select + 1) % 4;

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_STOP) {
            SET_CTL_MODE_PLAYBACK_STOP(false);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_PLAY) {
            SET_CTL_MODE_PLAYBACK_PLAY();

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_PAUSE) {
            SET_CTL_MODE_PLAYBACK_PAUSE(false);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_FORWARD) {
            SET_CTL_MODE_PLAYBACK_DIR(PB_DIR_FWD);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_REVERSE) {
            SET_CTL_MODE_PLAYBACK_DIR(PB_DIR_REV);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_FASTER) {
            double speed = mode.pb_speed;
            if (speed < 1000) {
                speed *= 2;
            }
            if (speed > 0.9 && speed < 1.1) {
                speed = 1.0;
            }
            SET_CTL_MODE_PLAYBACK_SPEED(speed);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_SLOWER) {
            double speed = mode.pb_speed;
            if (speed > 0.20) {
                speed /= 2;
            }
            if (speed > 0.9 && speed < 1.1) {
                speed = 1.0;
            }
            SET_CTL_MODE_PLAYBACK_SPEED(speed);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_HOUR_MINUS) {
            struct tm tm;
            time_t    secs;
            int       delta_sec;

            secs = (mode.pb_submode == PB_SUBMODE_PLAY
                    ? PB_SUBMODE_PLAY_REAL_TIME_US(&mode) / 1000000
                    : mode.pb_real_time_us / 1000000);
            localtime_r(&secs, &tm);
            delta_sec = tm.tm_min * 60 + tm.tm_sec;
            if (tm.tm_min == 0 && tm.tm_sec <= 3) {
                delta_sec += 3600;
            }
            SET_CTL_MODE_PLAYBACK_TIME(-delta_sec);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_HOUR_PLUS) {
            struct tm tm;
            time_t    secs;
            int       delta_sec;

            secs = (mode.pb_submode == PB_SUBMODE_PLAY
                    ? PB_SUBMODE_PLAY_REAL_TIME_US(&mode) / 1000000
                    : mode.pb_real_time_us / 1000000);
            localtime_r(&secs, &tm);
            delta_sec = 3600 - (tm.tm_min * 60 + tm.tm_sec);
            if (tm.tm_min == 59 && tm.tm_sec >= 56) {
                delta_sec += 3600;
            }
            SET_CTL_MODE_PLAYBACK_TIME(delta_sec);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_MINUTE_MINUS) {
            struct tm tm;
            time_t    secs;
            int       delta_sec;

            secs = (mode.pb_submode == PB_SUBMODE_PLAY
                    ? PB_SUBMODE_PLAY_REAL_TIME_US(&mode) / 1000000
                    : mode.pb_real_time_us / 1000000);
            localtime_r(&secs, &tm);
            delta_sec = tm.tm_sec;
            if (tm.tm_sec <= 3) {
                delta_sec += 60;
            }
            SET_CTL_MODE_PLAYBACK_TIME(-delta_sec);

        } else if (event.mouse_event == MOUSE_EVENT_PLAYBACK_MINUTE_PLUS) {
            struct tm tm;
            time_t    secs;
            int       delta_sec;

            secs = (mode.pb_submode == PB_SUBMODE_PLAY
                    ? PB_SUBMODE_PLAY_REAL_TIME_US(&mode) / 1000000
                    : mode.pb_real_time_us / 1000000);
            localtime_r(&secs, &tm);
            delta_sec = 60 - tm.tm_sec;
            if (tm.tm_sec >= 56) {
                delta_sec += 60;
            }
            SET_CTL_MODE_PLAYBACK_TIME(delta_sec);

        } else if (event.mouse_event >= MOUSE_EVENT_WC_ZOOM && 
                   event.mouse_event < MOUSE_EVENT_WC_ZOOM + MAX_WEBCAM) {
            int idx = event.mouse_event - MOUSE_EVENT_WC_ZOOM;
            CONFIG_ZOOM = (CONFIG_ZOOM == 'N'     ? 'a'+idx :
                           CONFIG_ZOOM == 'a'+idx ? 'A'+idx :
                                                    'N');
            CONFIG_WRITE();

        } else if (event.mouse_event >= MOUSE_EVENT_WC_NAME &&
                   event.mouse_event < MOUSE_EVENT_WC_NAME+4) {
            int idx = event.mouse_event - MOUSE_EVENT_WC_NAME;
            webcam[idx].name_select_mode = !webcam[idx].name_select_mode;

        } else if (event.mouse_event >= MOUSE_EVENT_WC_RES && 
                   event.mouse_event < MOUSE_EVENT_WC_RES+4) {
            int idx = event.mouse_event - MOUSE_EVENT_WC_RES;
            webcam[idx].change_resolution_request = true;

        } else if (event.mouse_event >= MOUSE_EVENT_WC_NAME_LIST &&
                   event.mouse_event < MOUSE_EVENT_WC_NAME_LIST+(MAX_WEBCAM*MAX_WC_DEF)) {
            int idx = (event.mouse_event - MOUSE_EVENT_WC_NAME_LIST) / MAX_WC_DEF;
            int name_idx = (event.mouse_event - MOUSE_EVENT_WC_NAME_LIST) % MAX_WC_DEF;
            webcam[idx].change_name_request = name_idx;
            webcam[idx].name_select_mode = false;

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_MODE_ENTER) {
            int def_idx;
            memset(&config_mode, 0, sizeof(config_mode));
            config_mode.enabled = true;
            for (def_idx = 0; def_idx < MAX_WC_DEF; def_idx++) {
                strcpy(config_mode.wc_def[def_idx], CONFIG_WC_DEF(def_idx));
            }

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_ACCEPT) {
            int wc_idx, def_idx;

            // update the CONFIG_WC_DEF with the editted values from config_mode.wc_def
            for (def_idx = 0; def_idx < MAX_WC_DEF; def_idx++) {
                strcpy(CONFIG_WC_DEF(def_idx), config_mode.wc_def[def_idx]);
            }

            // loop over the webcams
            for (wc_idx = 0; wc_idx < MAX_WEBCAM; wc_idx++) {
                webcam_t * wc = &webcam[wc_idx];

                // find webcam-definition that has name watching the webcam
                for (def_idx = 0; def_idx < MAX_WC_DEF; def_idx++) {
                    char wc_def_name[MAX_STR];
                    sscanf(CONFIG_WC_DEF(def_idx), "%s", wc_def_name);
                    if (strcmp(wc->name, wc_def_name) == 0) {
                        break;
                    }
                }

                // if no match found then use "none", which should always be MAX_WC_DEF-1
                if (def_idx == MAX_WC_DEF) {
                    def_idx = MAX_WC_DEF - 1;
                }

                // update the CONFIG_WC_SELECT with the possibly new def_idx value
                sprintf(CONFIG_WC_SELECT(wc_idx), "%d", def_idx);

                // if the def has changed then
                //    request name change
                // endif
                if (strcmp(CONFIG_WC_DEF(def_idx), wc->def_str) != 0) {
                    wc->change_name_request = def_idx;
                }
            }

            // write the config to storage
            CONFIG_WRITE();

            // exit config_mode
            memset(&config_mode, 0, sizeof(config_mode));
            config_mode.enabled = false;

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_CANCEL) {
            memset(&config_mode, 0, sizeof(config_mode));
            config_mode.enabled = false;

        } else if (event.mouse_event >= MOUSE_EVENT_CONFIG_SELECT &&
                   event.mouse_event < MOUSE_EVENT_CONFIG_SELECT + MAX_WC_DEF) {
            int def_idx = event.mouse_event - MOUSE_EVENT_CONFIG_SELECT;
            char name[MAX_STR];

            // preset the 4 keybd strings to empty
            memset(config_mode.keybd_str_value, 0, sizeof(config_mode.keybd_str_value));

            // if the def_idx name is not 'none' then
            //   set the keybd_str_values to name, ipaddr, port, and passwd of def_idx, so 
            //    that they can be editted
            // else if dif_idx > 0   (name == 'none') then
            //   set the keybd_str_values to "", ipaddr, port, and passwd of the prior def_idx;
            //    the prior def_idx values are being used as a good guess for a starting
            //    point of the new values being entered
            // else  (def_idx == 0 && name == 'none')
            //   since there is no prior value to use as a starting point set 
            //    the keybd_str_values each to ""
            // endif
            sscanf(config_mode.wc_def[def_idx], "%s", name);
            if (strcmp(name, "none") != 0) {
                sscanf(config_mode.wc_def[def_idx], "%s %s %s %s",
                       config_mode.keybd_str_value[0],
                       config_mode.keybd_str_value[1],
                       config_mode.keybd_str_value[2],
                       config_mode.keybd_str_value[3]);
            } else if (def_idx > 0) {
                sscanf(config_mode.wc_def[def_idx-1], "%s %s %s %s",
                       config_mode.keybd_str_value[0],
                       config_mode.keybd_str_value[1],
                       config_mode.keybd_str_value[2],
                       config_mode.keybd_str_value[3]);
                strcpy(config_mode.keybd_str_value[0], "");
            } else {
               strcpy(config_mode.keybd_str_value[0], "");
               strcpy(config_mode.keybd_str_value[1], "");
               strcpy(config_mode.keybd_str_value[2], "");
               strcpy(config_mode.keybd_str_value[3], "");
            }

            // set the 4 prompt strings
            config_mode.keybd_str_prompt[0] = "name    ";
            config_mode.keybd_str_prompt[1] = "ipaddr  ";
            config_mode.keybd_str_prompt[2] = "port    ";
            config_mode.keybd_str_prompt[3] = "password";

            // enable config keybd mode, to edit the values
            config_mode.keybd_enabled    = true;
            config_mode.keybd_str_idx = 0;
            config_mode.keybd_def_idx = def_idx;
            config_mode.keybd_shift   = false;
        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_FONT_PLUS) {
            int font_ptsz;
            sscanf(CONFIG_FONT_PTSZ, "%d", &font_ptsz);
            sprintf(CONFIG_FONT_PTSZ, "%d", font_ptsz+1);

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_FONT_MINUS) {
            int font_ptsz;
            sscanf(CONFIG_FONT_PTSZ, "%d", &font_ptsz);
            sprintf(CONFIG_FONT_PTSZ, "%d", font_ptsz-1);

        } else if (event.mouse_event >= MOUSE_EVENT_CONFIG_KEYBD_ASCII_FIRST && 
                   event.mouse_event <= MOUSE_EVENT_CONFIG_KEYBD_ASCII_LAST) {
            char *s = config_mode.keybd_str_value[config_mode.keybd_str_idx];
            char addchar[2] = {event.mouse_event, '\0'};
            strcat(s, addchar);

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_KEYBD_SHIFT) {
            config_mode.keybd_shift = !config_mode.keybd_shift;

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_KEYBD_BS) {
            char *s = config_mode.keybd_str_value[config_mode.keybd_str_idx];
            int len = strlen(s);
            if (len > 0) {
                s[len-1] = '\0';
            }

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_KEYBD_NEXT_STR) {
            config_mode.keybd_str_idx++;
            if (config_mode.keybd_str_idx == 4) {
                config_mode.keybd_str_idx = 0;
            }

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_KEYBD_PREV_STR) {
            config_mode.keybd_str_idx--;
            if (config_mode.keybd_str_idx < 0) {
                config_mode.keybd_str_idx = 3;
            }

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_KEYBD_ACCEPT) {
            if (config_mode.keybd_def_idx == MAX_WC_DEF-1) {
                strcpy(config_mode.wc_def[config_mode.keybd_def_idx], "none");
            } else if (strcmp(config_mode.keybd_str_value[0], "none") == 0 ||
                       strcmp(config_mode.keybd_str_value[0], "") == 0)
            {
                memmove(config_mode.wc_def[config_mode.keybd_def_idx],
                        config_mode.wc_def[config_mode.keybd_def_idx+1],
                        sizeof(config_mode.wc_def[0]) * ((MAX_WC_DEF-1) - config_mode.keybd_def_idx));
            } else {
                sprintf(config_mode.wc_def[config_mode.keybd_def_idx],
                        "%s %s %s %s",
                        config_mode.keybd_str_value[0],
                        config_mode.keybd_str_value[1],
                        config_mode.keybd_str_value[2],
                        config_mode.keybd_str_value[3]);
            }

            config_mode.keybd_enabled    = false;
            config_mode.keybd_str_idx = 0;
            config_mode.keybd_def_idx = 0;
            config_mode.keybd_shift   = false;

        } else if (event.mouse_event == MOUSE_EVENT_CONFIG_KEYBD_CANCEL) {
            config_mode.keybd_enabled    = false;
            config_mode.keybd_str_idx = 0;
            config_mode.keybd_def_idx = 0;
            config_mode.keybd_shift   = false;

        } else {
            ERROR("invalid mouse_event %d\n", event.mouse_event);
        }

        event.mouse_event = MOUSE_EVENT_NONE;
        event_handled = true;
    }

    // -------------------------------------------------------------------------------------
    // ---- if playback mode and all connected webcam are eod or bod then stop playback ----
    // -------------------------------------------------------------------------------------

    if (mode.mode == MODE_PLAYBACK && mode.pb_submode == PB_SUBMODE_PLAY && mode.pb_dir == PB_DIR_FWD) {
        bool all_eod = true;
        for (i = 0; i < MAX_WEBCAM; i++) {
            webcam_t * wc = &webcam[i];
            if (wc->state == STATE_CONNECTED && wc->frame_status != STATUS_ERR_FRAME_AFTER_EOD) {
                all_eod = false;
            }
        }
        if (all_eod) {
            SET_CTL_MODE_PLAYBACK_STOP(false);
        }
        event_handled = true;
    }

    if (mode.mode == MODE_PLAYBACK && mode.pb_submode == PB_SUBMODE_PLAY && mode.pb_dir == PB_DIR_REV) {
        bool all_bod = true;
        for (i = 0; i < MAX_WEBCAM; i++) {
            webcam_t * wc = &webcam[i];
            if (wc->state == STATE_CONNECTED && wc->frame_status != STATUS_ERR_FRAME_BEFORE_BOD) {
                all_bod = false;
            }
        }
        if (all_bod) {
            SET_CTL_MODE_PLAYBACK_STOP(false);
        }
        event_handled = true;
    }

    // ------------------------------------------------
    // ---- check if display needs to be rendered, ----
    // ---- if not then return                     ----
    // ------------------------------------------------

    // create the data_and_tims_str
    time_t secs;
    if (mode.mode == MODE_LIVE) {
        secs = get_real_time_sec();
        time2str(date_and_time_str, secs, false);
    } else if (mode.mode == MODE_PLAYBACK) {
        if (mode.pb_submode == PB_SUBMODE_PLAY) {
            secs = PB_SUBMODE_PLAY_REAL_TIME_US(&mode) / 1000000;
        } else {
            secs = mode.pb_real_time_us / 1000000;
        }
        time2str(date_and_time_str, secs, false);
    } else {
        bzero(date_and_time_str, sizeof(date_and_time_str));
    }

    // the following conditions require display update
    // - event was handled
    // - an image had changed
    // - date_and_time_str has changed and last update > 100 ms ago
    // - keyboard input is in progress and last update > 100 ms ago
    // - last update > 1 sec ago
    // if none of these conditions exist then return
    static char     last_date_and_time_str[MAX_TIME_STR];
    static uint64_t last_image_change[MAX_WEBCAM];
    static uint64_t last_window_update_us;
    curr_us = microsec_timer();
    do {
        if (event_handled) {
            break;
        }

        for (i = 0; i < MAX_WEBCAM; i++) {
            if (webcam[i].image_change != last_image_change[i]) {
                break;
            }
        }
        if (i < MAX_WEBCAM) {
            break;
        }

        if (strcmp(date_and_time_str, last_date_and_time_str) != 0 &&
            curr_us - last_window_update_us > 100*MS)
        {
            break;
        }

        if (config_mode.keybd_enabled && curr_us - last_window_update_us > 100*MS) {
            break;
        }

        if (curr_us - last_window_update_us > 1000*MS) {
            break;
        }

        return;
    } while (0);
    for (i = 0; i < MAX_WEBCAM; i++) {
        last_image_change[i] = webcam[i].image_change;
    }
    strcpy(last_date_and_time_str, date_and_time_str);
    last_window_update_us = curr_us;

    // --------------------------------------------
    // ---- reinit the list of positions       ----
    // --------------------------------------------
    bzero(event.mouse_event_pos, sizeof(event.mouse_event_pos));

    #define CTL_WIDTH  (CTL_COLS * font.char_width)

    INIT_POS(ctlpane, 
             win_width-CTL_WIDTH, 0,     // x, y
             CTL_WIDTH, win_height);     // w, h
    INIT_POS(ctlbpane, 
             win_width-CTL_WIDTH, win_height-CTL_ROWS*font.char_height,
             CTL_WIDTH, CTL_ROWS*font.char_height);
    INIT_POS(configpane,
             0, 0,
             win_width, win_height);

    int small_win_count = 0;
    for (i = 0; i < MAX_WEBCAM; i++) {
        int wc_x, wc_y, wc_w, wc_h;

        if (CONFIG_ZOOM >= 'a' && CONFIG_ZOOM <= 'd') {
            int wc_zw = (double)(win_width - CTL_WIDTH) / 1.33;
            if ('a'+i == CONFIG_ZOOM) {
                wc_x = 0;
                wc_y = 0;
                wc_w = wc_zw;
                wc_h = win_height;
            } else {
                wc_x = wc_zw;
                wc_y = small_win_count * (win_height / 3);
                wc_w = win_width - CTL_WIDTH - wc_zw;
                wc_h = (win_height / 3);
                small_win_count++;
            }
        } else if (CONFIG_ZOOM >= 'A' && CONFIG_ZOOM <= 'D') {
            if ('A'+i == CONFIG_ZOOM) {
                wc_x = 0;
                wc_y = 0;
                wc_w = win_width - CTL_WIDTH;
                wc_h = win_height;
            } else {
                wc_x = 0;
                wc_y = 0;
                wc_w = 0;
                wc_h = 0;
            }
        } else {
            wc_w = (win_width - CTL_WIDTH) / 2;
            wc_h = win_height / 2;
            switch (i) {
            case 0: wc_x = 0;    wc_y = 0;    break;
            case 1: wc_x = wc_w; wc_y = 0;    break;
            case 2: wc_x = 0;    wc_y = wc_h; break;
            case 3: wc_x = wc_w; wc_y = wc_h; break;
            }
        }
        INIT_POS(wcpane[i],
                 wc_x, wc_y, 
                 wc_w, wc_h);
        INIT_POS(wctitlepane[i],
                 wc_x + 1, wc_y + 1,
                 wc_w - 2, font.char_height);
        INIT_POS(wcimagepane[i],
                 wc_x + 1, wc_y + 2 + font.char_height,
                 wc_w - 2, wc_h - font.char_height - 3);
    }

    // ---------------------------------
    // ---- clear the entire window ----
    // ---------------------------------

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // ---------------------------------
    // ---- configpane: config mode ----
    // ---------------------------------

    if (config_mode.enabled) {
        if (config_mode.keybd_enabled == false) {
            int def_idx;
            char wc_name[MAX_STR];

            for (def_idx = 0; def_idx < MAX_WC_DEF; def_idx++) {
                render_text(&configpane, 1.5*def_idx, 0, config_mode.wc_def[def_idx], MOUSE_EVENT_CONFIG_SELECT+def_idx);
                sscanf(config_mode.wc_def[def_idx], "%s", wc_name);
                if (strcmp(wc_name, "none") == 0) {
                    break;
                }
            }

            double r = PANE_ROWS(&configpane);
            double c = PANE_COLS(&configpane);

            render_text(&configpane, r-5, c-2, "F+", MOUSE_EVENT_CONFIG_FONT_PLUS);
            render_text(&configpane, r-4, c-2, CONFIG_FONT_PTSZ, MOUSE_EVENT_NONE);
            render_text(&configpane, r-3, c-2, "F-", MOUSE_EVENT_CONFIG_FONT_MINUS);

            render_text(&configpane, r-1, c-15, "ACCEPT", MOUSE_EVENT_CONFIG_ACCEPT);
            render_text(&configpane, r-1, c-6,  "CANCEL", MOUSE_EVENT_CONFIG_CANCEL);
        } else {
            static char * row_chars_unshift[4] = { "1234567890",
                                                   "qwertyuiop",
                                                   "asdfghjkl",
                                                   "zxcvbnm.-_" };
            static char * row_chars_shift[4]   = { "1234567890_",
                                                   "QWERTYUIOP",
                                                   "ASDFGHJKL",
                                                   "ZXCVBNM.-_" };
            char ** row_chars;
            double  r, c;

            row_chars = (!config_mode.keybd_shift ? row_chars_unshift : row_chars_shift);
            r = 0;
            c = 2;

            for (i = 0; i < 4; i++) {
                for (j = 0; row_chars[i][j] != '\0'; j++) {
                    str[0] = row_chars[i][j];
                    str[1] = '\0';
                    render_text_ex(&configpane, r, c+4*j, str, str[0], 1, false);
                }
                r += 1.25;
            }

            render_text_ex(&configpane, r, c+0,  "SHIFT",     MOUSE_EVENT_CONFIG_KEYBD_SHIFT, 5, false);
            render_text_ex(&configpane, r, c+10, "BACKSPACE", MOUSE_EVENT_CONFIG_KEYBD_BS,    9, false);
            r += 1.25;

            for (i = 0; i < 4; i++) {
                char s[MAX_STR];
                sprintf(s, "%s = %s", 
                        config_mode.keybd_str_prompt[i], 
                        config_mode.keybd_str_value[i]);

                if (i == config_mode.keybd_str_idx) {
                    if ((microsec_timer() % 1000000) > 500000) {
                        strcat(s, "_");
                    }
                }
                render_text_ex(&configpane, r, c, s, MOUSE_EVENT_NONE, strlen(s), false);
                r += 1.25;
            }
            
            render_text_ex(&configpane, r, c, "NEXT",   MOUSE_EVENT_CONFIG_KEYBD_NEXT_STR, 4, false);
            render_text_ex(&configpane, r, c+10, "PREV",   MOUSE_EVENT_CONFIG_KEYBD_PREV_STR, 4, false);

            r = PANE_ROWS(&configpane);
            c = PANE_COLS(&configpane);

            render_text(&configpane, r-5, c-2, "F+", MOUSE_EVENT_CONFIG_FONT_PLUS);
            render_text(&configpane, r-4, c-2, CONFIG_FONT_PTSZ, MOUSE_EVENT_NONE);
            render_text(&configpane, r-3, c-2, "F-", MOUSE_EVENT_CONFIG_FONT_MINUS);

            render_text(&configpane, r-1, c-15, "ACCEPT", MOUSE_EVENT_CONFIG_KEYBD_ACCEPT);
            render_text(&configpane, r-1, c-6, "CANCEL", MOUSE_EVENT_CONFIG_KEYBD_CANCEL);
        }

        goto render_present;
    }

    // ----------------------------------------------
    // ---- image_panes: live & playback modes   ----
    // ----------------------------------------------

    for (i = 0; i < MAX_WEBCAM; i++) {
        webcam_t * wc            = &webcam[i];
        char       win_id_str[2] = { 'A'+i, 0 };

        // if this webcam is not to be displayed, because it's pane is zero width
        // then continue
        if (wcpane[i].w == 0) {
            continue;
        }

        // acquire wc mutex
        pthread_mutex_lock(&wc->image_mutex);

        // display border 
        if (wc->image_highlight && !wc->name_select_mode) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);
        }
        SDL_RenderDrawRect(renderer, &wcpane[i]);
        SDL_RenderDrawLine(renderer,
                           wcpane[i].x, wcpane[i].y+font.char_height+1,
                           wcpane[i].x+wcpane[i].w-1, wcpane[i].y+font.char_height+1);

        // display title line
        if (PANE_COLS(&wctitlepane[i]) >= 1) {
            render_text(&wctitlepane[i], 0, 0, win_id_str, MOUSE_EVENT_NONE);
        }

        if (PANE_COLS(&wctitlepane[i]) >= 10) {
            render_text_ex(&wctitlepane[i], 
                        0, 2, 
                        wc->image_name, 
                        MOUSE_EVENT_WC_NAME+i,
                        PANE_COLS(&wctitlepane[i]) - 6, 
                        false);
            render_text_ex(&wctitlepane[i], 
                        0, PANE_COLS(&wctitlepane[i]) - 3,
                        wc->image_res, 
                        mode.mode == MODE_LIVE ? MOUSE_EVENT_WC_RES+i : MOUSE_EVENT_NONE,
                        3,
                        false);
        } else if (PANE_COLS(&wctitlepane[i]) >= 3) {
            render_text_ex(&wctitlepane[i], 
                        0, 2, 
                        wc->image_name, 
                        MOUSE_EVENT_WC_NAME+i,
                        PANE_COLS(&wctitlepane[i]) - 2, 
                        false);
        }

        // display webcam_names
        if (wc->name_select_mode) {
            // display the list of available webcam names to choose from
            char wc_name[MAX_STR];
            int def_idx;
            for (def_idx = 0; def_idx < MAX_WC_DEF; def_idx++) {
                sscanf(CONFIG_WC_DEF(def_idx), "%s", wc_name);
                render_text(&wcimagepane[i], 0.5+1.5*def_idx, 0,
                            wc_name,
                            MOUSE_EVENT_WC_NAME_LIST + MAX_WC_DEF * i + def_idx);
                if (strcmp(wc_name, "none") == 0) {
                    break;
                }
            }

            // register for the zoom event
            event.mouse_event_pos[MOUSE_EVENT_WC_ZOOM+i] = wcimagepane[i];

        // display the image
        } else if (wc->image_display) {
            // create new texture, if needed
            if (wc->texture == NULL || 
                wc->texture_w != wc->image_w || 
                wc->texture_h != wc->image_h) 
            {
                wc->texture_w = wc->image_w;
                wc->texture_h = wc->image_h;
                if (wc->texture != NULL) {
                    SDL_DestroyTexture(wc->texture);
                }
                wc->texture = SDL_CreateTexture(renderer, 
                                                SDL_PIXELFORMAT_YUY2,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                wc->texture_w,
                                                wc->texture_h);
                if (wc->texture == NULL) {
                    ERROR("SDL_CreateTexture failed\n");
                    exit(1);
                }
                DEBUG("created new texture %dx%d\n", wc->texture_w, wc->texture_h);
            }

            // update the texture with the image pixels
            SDL_UpdateTexture(wc->texture,
                              NULL,            // update entire texture
                              wc->image,       // pixels
                              wc->image_w*2);  // pitch

            // copy the texture to the render target
            SDL_RenderCopy(renderer, wc->texture, NULL, &wcimagepane[i]);

            // display the temperature
            if (wc->image_temperature != INVALID_TEMPERATURE) {
                char temper_str[MAX_STR];
                DEBUG("%s is %d F\n", wc->image_name, wc->image_temperature);
                sprintf(temper_str, "%d F", wc->image_temperature);
                render_text_ex(&wcimagepane[i],
                               0, 0,                        // row, col
                               temper_str, 
                               MOUSE_EVENT_NONE,
                               PANE_COLS(&wcimagepane[i]),  // field_cols
                               true);                       // center, 
            }

            // register for the zoom event
            event.mouse_event_pos[MOUSE_EVENT_WC_ZOOM+i] = wcimagepane[i];

        // display image notification text lines
        } else {
            double r = PANE_ROWS(&wcimagepane[i]) / 2;
            render_text_ex(&wcimagepane[i], r-1, 0, wc->image_notification_str1, MOUSE_EVENT_NONE, 
                           PANE_COLS(&wcimagepane[i]), true);
            render_text_ex(&wcimagepane[i], r,   0, wc->image_notification_str2, MOUSE_EVENT_NONE, 
                           PANE_COLS(&wcimagepane[i]), true);
            render_text_ex(&wcimagepane[i], r+1, 0, wc->image_notification_str3, MOUSE_EVENT_NONE, 
                           PANE_COLS(&wcimagepane[i]), true);

            // register for the zoom event
            event.mouse_event_pos[MOUSE_EVENT_WC_ZOOM+i] = wcimagepane[i];
        }

        // relsease wc mutex
        pthread_mutex_unlock(&wc->image_mutex);
    }

    // -------------------
    // ---- ctlpane   ----
    // -------------------

    if (mode.mode == MODE_LIVE) {
        // Live Mode Ctl Pane ...
        //
        // 00:  LIVE
        // 01:
        // 02:  06/07/58
        // 03:  11:12:13

        strcpy(str, date_and_time_str);
        str[8] = '\0';
        render_text(&ctlpane, 0, 0, "LIVE", MOUSE_EVENT_MODE_SELECT);
        render_text(&ctlpane, 2, 0, str, MOUSE_EVENT_NONE);
        render_text(&ctlpane, 3, 0, str+9, MOUSE_EVENT_NONE);
    } else if (mode.mode == MODE_PLAYBACK) {
        // Playback Mode Ctl Pane example layout ...
        //
        // PLAYBACK
        // 
        // 06/07/58
        // 11:12:13
        // PLAY F:1X
        //
        // STOP    PAUSE
        // REV     FWD
        // SLOWER  FASTER
        // HOUR-   HOUR+
        // MIN-    MIN+

        // title
        double r = 0;
        render_text(&ctlpane, r, 0, "PLAYBACK", MOUSE_EVENT_MODE_SELECT);

        // status: date and time
        strcpy(str, date_and_time_str);
        str[8] = '\0';
        r += 1.4;
        render_text(&ctlpane, r, 0, str, MOUSE_EVENT_NONE);
        r += 1;
        render_text(&ctlpane, r, 0, str+9, MOUSE_EVENT_NONE);

        // status: stop|play|pause, speed, and dir
        struct mode_s m = mode;
        if (m.pb_speed >= 1) {
            sprintf(str, "%s %s:%.0fX", PB_SUBMODE_STR(m.pb_submode), PB_DIR_STR(m.pb_dir), m.pb_speed);
        } else {
            sprintf(str, "%s %s:%.2fX", PB_SUBMODE_STR(m.pb_submode), PB_DIR_STR(m.pb_dir), m.pb_speed);
        }

        r += 1;
        render_text(&ctlpane, r, 0, str, MOUSE_EVENT_NONE);

        // control: stop,play,pause
        r += 1.4;
        if (m.pb_submode == PB_SUBMODE_STOP) {
            render_text(&ctlpane, r, 0, "PLAY", MOUSE_EVENT_PLAYBACK_PLAY);
            render_text(&ctlpane, r, 8, "PAUSE", MOUSE_EVENT_PLAYBACK_PAUSE);
        } else if (m.pb_submode == PB_SUBMODE_PLAY) {
            render_text(&ctlpane, r, 0, "STOP", MOUSE_EVENT_PLAYBACK_STOP);
            render_text(&ctlpane, r, 8, "PAUSE", MOUSE_EVENT_PLAYBACK_PAUSE);
        } else if (m.pb_submode == PB_SUBMODE_PAUSE) {
            render_text(&ctlpane, r, 0, "STOP", MOUSE_EVENT_PLAYBACK_STOP);
            render_text(&ctlpane, r, 8, "PLAY", MOUSE_EVENT_PLAYBACK_PLAY);
        } else {
            render_text(&ctlpane, r, 0, "????", MOUSE_EVENT_NONE);
            render_text(&ctlpane, r, 8, "????", MOUSE_EVENT_NONE);
        }

        // control: fwd,rev
        r += 1.4;
        render_text(&ctlpane, r, 0, "REV", MOUSE_EVENT_PLAYBACK_REVERSE);
        render_text(&ctlpane, r, 8, "FWD", MOUSE_EVENT_PLAYBACK_FORWARD);

        // control: fast,slow
        r += 1.4;
        render_text(&ctlpane, r, 0, "SLOWER", MOUSE_EVENT_PLAYBACK_SLOWER);
        render_text(&ctlpane, r, 8, "FASTER", MOUSE_EVENT_PLAYBACK_FASTER);

        // control: hour-, hour+,min-,min+
        r += 1.4;
        render_text(&ctlpane,  r, 0, "HOUR-", MOUSE_EVENT_PLAYBACK_HOUR_MINUS);
        render_text(&ctlpane,  r, 8, "HOUR+", MOUSE_EVENT_PLAYBACK_HOUR_PLUS);
        r += 1.4;
        render_text(&ctlpane, r, 0, "MIN-",  MOUSE_EVENT_PLAYBACK_MINUTE_MINUS);
        render_text(&ctlpane, r, 8, "MIN+",  MOUSE_EVENT_PLAYBACK_MINUTE_PLUS);
    }

    // ------------------------------------
    // ---- ctlbpane (ctl pane bottom) ----
    // ------------------------------------

    // Ctl Pane Bottom ...
    // 
    // STATUS_TITLE
    // A <values> 
    // B <values>
    // C <values>.
    // D <values>
    // 
    // CONFIG  QUIT    

    bool okay = false;

    if (mode.mode == MODE_LIVE) {
        okay = PANE_ROWS(&ctlpane) >= 12;
    } else if (mode.mode == MODE_PLAYBACK) {
        okay = PANE_ROWS(&ctlpane) >= 25;
    } else {
        FATAL("mode %d invalid\n", mode.mode);
    }

    if (okay) {
        // status display
        switch (status_select) {

        case 0: {  // FRAMES/SEC
            uint64_t        delta_us;

            static uint64_t last_us;
            static uint64_t last_recvd_frames[MAX_WEBCAM];
            static char     static_str[MAX_WEBCAM][MAX_STR];
            
            // if greater then 3 second since last values saved then
            //   recompute rates
            //   save last values
            // endif
            curr_us = microsec_timer();
            delta_us = curr_us - last_us;
            if (delta_us > 5000*MS) {
                for (i = 0; i < MAX_WEBCAM; i++) {
                    if (webcam[i].recvd_frames < last_recvd_frames[i]) {
                        last_recvd_frames[i] = webcam[i].recvd_frames;
                    }
                    if (webcam[i].state == STATE_CONNECTED) {
                        sprintf(static_str[i], "%c %0.1f", 
                                'A'+i,
                                (webcam[i].recvd_frames - last_recvd_frames[i]) / (delta_us / 1000000.));
                    } else {
                        sprintf(static_str[i], "%c not conn", 'A'+i);
                    }
                    last_recvd_frames[i] = webcam[i].recvd_frames;
                }
                last_us = curr_us;
            }

            // display
            render_text(&ctlbpane, 0, 0, "FRAMES/SEC", MOUSE_EVENT_STATUS_SELECT);
            for (i = 0; i < MAX_WEBCAM; i++) {
                render_text(&ctlbpane, i+1, 0, static_str[i], MOUSE_EVENT_NONE);
            }
            break; }

        case 1: {  // TOTAL MB
            render_text(&ctlbpane, 0, 0, "TOTAL MB", MOUSE_EVENT_STATUS_SELECT);
            for (i = 0; i < MAX_WEBCAM; i++) {
                if (webcam[i].state == STATE_CONNECTED) {
                    sprintf(str, "%c %0.3f", 'A'+i, (webcam[i].recvd_bytes / 1000000.));
                } else {
                    sprintf(str, "%c not conn", 'A'+i);
                }
                render_text(&ctlbpane, i+1, 0, str, MOUSE_EVENT_NONE);
            }
            break; }

        case 2: { // REC DURATION
            render_text(&ctlbpane, 0, 0, "REC DURATION", MOUSE_EVENT_STATUS_SELECT);
            for (i = 0; i < MAX_WEBCAM; i++) {
                uint32_t days, hours, minutes, seconds;

                if (webcam[i].state == STATE_CONNECTED) {
                    CVT_INTERVAL_SECS_TO_DAY_HMS(webcam[i].status.rp_duration_us/1000000,
                                                days, hours, minutes, seconds);
                    sprintf(str, "%c %d:%02d:%02d", 'A'+i, 24*days+hours, minutes, seconds);
                } else {
                    sprintf(str, "%c not conn", 'A'+i);
                }
                render_text(&ctlbpane, i+1, 0, str, MOUSE_EVENT_NONE);
            }
            break; }

        case 3: { // VERSION N.N
            sprintf(str, "VERSION %d.%d", VERSION_MAJOR, VERSION_MINOR);
            render_text(&ctlbpane, 0, 0, str, MOUSE_EVENT_STATUS_SELECT);
            for (i = 0; i < MAX_WEBCAM; i++) {
                if (webcam[i].state == STATE_CONNECTED) {
                    sprintf(str, "%c %d.%d", 'A'+i, 
                            webcam[i].status.version.major,
                            webcam[i].status.version.minor);
                } else {
                    sprintf(str, "%c not conn", 'A'+i);
                }
                render_text(&ctlbpane, i+1, 0, str, MOUSE_EVENT_NONE);
            }
            break; }

        }
    }

    // config & quit
    double r = PANE_ROWS(&ctlbpane);
    double c = PANE_COLS(&ctlbpane);
    render_text(&ctlbpane, r-1, 0, "CONFIG", MOUSE_EVENT_CONFIG_MODE_ENTER);
    render_text(&ctlbpane, r-1, c-5, "QUIT", MOUSE_EVENT_QUIT);

    // -----------------
    // ---- present ----
    // -----------------

render_present:
    SDL_RenderPresent(renderer);
}

void render_text(SDL_Rect * pane, double row, double col, char * str, int mouse_event)
{
    render_text_ex(pane, row, col, str, mouse_event, PANE_COLS(pane)-col, false);
}

void render_text_ex(SDL_Rect * pane, double row, double col, char * str, int mouse_event, 
                    int field_cols, bool center)
{
    SDL_Surface    * surface; 
    SDL_Texture    * texture; 
    SDL_Color        fg_color;
    SDL_Rect         pos;
    char             s[MAX_STR];
    int              slen;

    static SDL_Color fg_color_normal = {255,255,255}; 
    static SDL_Color fg_color_event  = {0,255,255}; 
    static SDL_Color bg_color        = {0,0,0}; 

    // if zero length string then nothing to do
    if (str[0] == '\0') {
        return;
    }

    // verify row and col are in range
    if (row < 0 || row > PANE_ROWS(pane)-1 || col < 0 || col > PANE_COLS(pane)-1) {
        return;
    }

    // verify field_cols
    if (field_cols <= 0) {
         return;
    }

    // make a copy of the str arg, and shorten if necessary
    strcpy(s, str);
    slen = strlen(s);
    if (slen > field_cols) {
        s[field_cols] = '\0';
        slen = field_cols;
    }

    // if centered then adjust col
    if (center) {
        col += (field_cols - slen) / 2;
    }

    // render the text to a surface
    fg_color = (mouse_event != MOUSE_EVENT_NONE ? fg_color_event : fg_color_normal); 
    surface = TTF_RenderText_Shaded(font.font, s, fg_color, bg_color);
    if (surface == NULL) { 
        FATAL("TTF_RenderText_Shaded returned NULL\n");
    } 

    // determine the display location
    pos.x = pane->x + col * font.char_width;
    pos.y = pane->y + row * font.char_height;
    pos.w = surface->w;
    pos.h = surface->h;

    // create texture from the surface, and render the texture
    texture = SDL_CreateTextureFromSurface(renderer, surface); 
    SDL_RenderCopy(renderer, texture, NULL, &pos); 

    // clean up
    SDL_FreeSurface(surface); 
    SDL_DestroyTexture(texture); 

    // if there is a mouse_event then save the location for the event handler
    if (mouse_event != MOUSE_EVENT_NONE) {
        event.mouse_event_pos[mouse_event] = pos;
    }
}

// -----------------  WEBCAM THREAD  -------------------------------------

void * webcam_thread(void * cx) 
{
    #define STATE_CHANGE(new_state, s1, s2, s3) \
        do { \
            if ((new_state) != wc->state) { \
                INFO("wc %c: %s -> %s '%s' '%s' %s\n", \
                    id_char, STATE_STR(wc->state), STATE_STR(new_state), s1, s2, s3); \
                wc->state = (new_state); \
                last_state_change_time_us = microsec_timer(); \
                DISPLAY_TEXT(s1,s2,s3); \
            } \
        } while (0)

    #define RESOLUTION_STR(w,h) ((w) == 640 ? "HI" : (w) == 320 ? "MED" : (w) == 160 ? "LOW" : "???")

    #define DISPLAY_IMAGE(_image, _width, _height, _motion, _temperature) \
        do { \
            pthread_mutex_lock(&wc->image_mutex); \
            if (_motion) { \
                wc->image_highlight = true; \
                last_highlight_enable_time_us = microsec_timer(); \
            } \
            strcpy(wc->image_res, RESOLUTION_STR(_width,_height)); \
            if (wc->image) { \
                free(wc->image); \
            } \
            wc->image = (_image); \
            wc->image_w = (_width); \
            wc->image_h = (_height); \
            wc->image_temperature = (_temperature); \
            wc->image_display = true;  \
            wc->image_change++; \
            pthread_mutex_unlock(&wc->image_mutex); \
        } while (0)

    #define DISPLAY_TEXT(s1,s2,s3); \
        do { \
            pthread_mutex_lock(&wc->image_mutex); \
            wc->image_highlight = false; \
            strcpy(wc->image_res, ""); \
            strcpy(wc->image_notification_str1, s1); \
            strcpy(wc->image_notification_str2, s2); \
            strcpy(wc->image_notification_str3, s3); \
            wc->image_display = false; \
            wc->image_change++; \
            pthread_mutex_unlock(&wc->image_mutex); \
            if ((s1)[0] != '\0' || (s2)[0] != '\0' || (s3)[0] != '\0') { \
                usleep(500*MS); \
            } \
        } while (0)

    #define DISPLAY_CLEAR_HIGHLIGHT() \
        do { \
            if (wc->image_highlight == false) { \
                break; \
            } \
            pthread_mutex_lock(&wc->image_mutex); \
            wc->image_highlight = false; \
            wc->image_change++; \
            pthread_mutex_unlock(&wc->image_mutex); \
        } while (0)

    #define DISPLAY_WC_NAME(dn) \
        do { \
            pthread_mutex_lock(&wc->image_mutex); \
            strcpy(wc->image_name, (dn)); \
            wc->image_change++; \
            pthread_mutex_unlock(&wc->image_mutex); \
        } while (0)

    #define OK_TO_CONNECT \
        (strcmp(wc->name, "none") != 0)

    #define DISCONNECT() \
        do { \
            if (handle != NULL) { \
                net_disconnect(handle); \
                handle = NULL; \
                bzero(&wc->status, sizeof(wc->status)); \
                wc->recvd_bytes = 0; \
                wc->recvd_frames = 0; \
            } \
        } while (0)

    int              id      = (int)(long)cx;
    char             id_char = 'A' + id;
    webcam_t       * wc      = &webcam[id];
    void           * handle  = NULL;

    uint64_t         last_state_change_time_us = microsec_timer();
    uint64_t         last_status_msg_recv_time_us = microsec_timer();
    uint64_t         last_frame_recv_time_us = microsec_timer();
    uint64_t         last_highlight_enable_time_us = microsec_timer();
    uint64_t         last_intvl_us = -1;

    INFO("THREAD %d STARTING\n", id);

    // init non-zero fields of wc
    pthread_mutex_init(&wc->image_mutex,NULL);
    wc->state = STATE_NOT_CONNECTED;
    wc->change_name_request = CONFIG_WC_SELECT(id)[0]-'0';

    // acknowledge that this thread has completed initialization
    __sync_fetch_and_add(&webcam_threads_running_count,1);

    // runtime
    while (true) {
        // handle change_name_request
        if (wc->change_name_request != -1) {
            if (wc->change_name_request < 0 || wc->change_name_request >= MAX_WC_DEF) {
                FATAL("invalid wc->change_name_request = %d\n", wc->change_name_request);
            }

            DISCONNECT();
            STATE_CHANGE(STATE_NOT_CONNECTED, "", "", "");

            sprintf(CONFIG_WC_SELECT(id), "%d", wc->change_name_request);
            CONFIG_WRITE();

            strcpy(wc->def_str, CONFIG_WC_DEF(wc->change_name_request));

            wc->name[0]     = '\0';
            wc->ipaddr[0]   = '\0';
            wc->port        = 0;
            wc->password[0] = '\0';
            sscanf(wc->def_str, "%s %s %d %s", wc->name, wc->ipaddr, &wc->port, wc->password);

            DISPLAY_WC_NAME(wc->name);

            wc->change_name_request = -1;
        }

        // state processing
        switch (wc->state) {
        case STATE_NOT_CONNECTED:
            if (OK_TO_CONNECT) {
                STATE_CHANGE(STATE_CONNECTING, "", "", "");
                break;
            }
            usleep(100*MS);
            break;

        case STATE_CONNECTING: {
            int connect_status;
            void *h;

            // display connecting message
            DISPLAY_TEXT("CONNECTING", "", "");

            // attempt to connect to wc_name
            h = net_connect(wc->ipaddr, wc->port, wc->password, &connect_status);
            if (h == NULL) {  
                STATE_CHANGE(STATE_CONNECTING_ERROR, "CONNECT ERROR", status2str(connect_status), "");
                break;
            }

            // connect succeeded; init variables for the new connection
            handle = h;
            last_status_msg_recv_time_us = microsec_timer();
            last_frame_recv_time_us = microsec_timer();
            last_highlight_enable_time_us = microsec_timer();
            last_intvl_us = -1;
            bzero(&wc->mode, sizeof(struct mode_s));
            bzero(&wc->status, sizeof(struct status_s));
            wc->frame_status = STATUS_INFO_OK;
            STATE_CHANGE(STATE_CONNECTED, "CONNECTED", "", "");
            break; }

        case STATE_CONNECTED: {
            webcam_msg_t msg;
            int          ret;
            uint32_t     data_len, width, height;
            uint8_t      data[RP_MAX_FRAME_DATA_LEN];
            uint8_t    * image;
            uint64_t     curr_us;
            char         int_str[MAX_INT_STR];
            uint64_t     intvl_us;

            // if mode has changed then 
            // - perform initialization when entering the LIVE or PLAYBACK mode
            // - send the new mode struct to the webcam
            if (wc->mode.mode_id != mode.mode_id) {
                bool entering_live_mode;
                bool entering_playback_mode;

                entering_live_mode = (mode.mode == MODE_LIVE && wc->mode.mode != mode.mode);
                entering_playback_mode = (mode.mode == MODE_PLAYBACK && wc->mode.mode != mode.mode);
                wc->mode = mode;

                if (entering_live_mode) {
                    DISPLAY_TEXT("LIVE MODE", "", "");
                    last_frame_recv_time_us = microsec_timer();
                }
                if (entering_playback_mode) {
                    DISPLAY_TEXT("PLAYBACK MODE", "", "");
                    last_frame_recv_time_us = microsec_timer();
                }

#ifdef DEBUG_PRINTS
                // issue debug for new mode
                if (wc->mode.mode != MODE_PLAYBACK) {
                    DEBUG("wc %c: mode is now: %s id=%"PRId64" \n", 
                           id_char, MODE_STR(wc->mode.mode), wc->mode.mode_id);
                } else {
                    char ts1[MAX_TIME_STR], ts2[MAX_TIME_STR];
                    DEBUG("wc %c: mode is now: %s id=%"PRId64" %s %s speed=%f mode_entry=%s play=%s\n",
                           id_char, MODE_STR(wc->mode.mode), wc->mode.mode_id,
                           PB_SUBMODE_STR(wc->mode.pb_submode),
                           PB_DIR_STR(wc->mode.pb_dir),
                           wc->mode.pb_speed,
                           time2str(ts1,wc->mode.pb_mode_entry_real_time_us/1000000,false),
                           time2str(ts2,wc->mode.pb_real_time_us/1000000,false));
                }
#endif

                // send the new mode message to webcam 
                bzero(&msg,sizeof(msg));
                msg.msg_type = MSG_TYPE_CMD_SET_MODE;   
                msg.u.mt_cmd_set_mode = wc->mode;
                if ((ret = net_send(handle, &msg, sizeof(webcam_msg_t))) < 0) {
                    STATE_CHANGE(STATE_CONNECTED_ERROR, "ERROR", "send mode msg", "");
                    break;
                }
            }

            // determine the minimum interval that viewer desires the webcam to be sending frames:
            if (win_minimized) {
                intvl_us = 1000000*MS;  // 1000 secs
            } else if (CONFIG_ZOOM == 'a'+id || CONFIG_ZOOM == 'A'+id) {
                // this pane is large or largest
                intvl_us = 0;
            } else if (CONFIG_ZOOM >= 'A' && CONFIG_ZOOM <= 'D') {
                // another pane is largest, meaning this pane is not shown
                intvl_us = 1000000*MS;  // 1000 secs
            } else if (CONFIG_ZOOM >= 'a' && CONFIG_ZOOM <= 'd') {
                // another pase is large, meaning this pane is small
                intvl_us = 200*MS;
            } else {
                // this pane must be medium
                intvl_us = 200*MS;
            }

            // if the minimum interval has changed since the last value sent to webcam then
            // send the new minimum interval
            if (intvl_us != last_intvl_us) {
                DEBUG("wc %c: send MSG_TYPE_CMD_SET_MIN_SEND_INTVL_US intvl=%"PRId64" us\n", id_char, intvl_us);
                bzero(&msg,sizeof(msg));
                msg.msg_type = MSG_TYPE_CMD_SET_MIN_SEND_INTVL_US;   
                msg.u.mt_cmd_min_send_intvl.us = intvl_us;
                if ((ret = net_send(handle, &msg, sizeof(webcam_msg_t))) < 0) {
                    STATE_CHANGE(STATE_CONNECTED_ERROR, "ERROR", "send intvl msg", "");
                    break;
                }
                last_intvl_us = intvl_us;
            }

            // process resolution change request
            if (wc->change_resolution_request) {
                if (wc->mode.mode == MODE_LIVE) {
                    bzero(&msg,sizeof(msg));
                    msg.msg_type = MSG_TYPE_CMD_LIVE_MODE_CHANGE_RES;
                    if ((ret = net_send(handle, &msg, sizeof(webcam_msg_t))) < 0) {
                        STATE_CHANGE(STATE_CONNECTED_ERROR, "ERROR", "send res msg", "");
                        break;
                    }
                    DISPLAY_TEXT(status2str(STATUS_INFO_CHANGING_RESOLUTION), "", "");
                }
                wc->change_resolution_request = false;
            }

            // clear highlight if it is currently enabled and the last time it was
            // enabled is greater than HIGHLIGHT_TIME_US
            curr_us = microsec_timer();
            if ((wc->image_highlight) &&
                (curr_us - last_highlight_enable_time_us > HIGHLIGHT_TIME_US) &&
                !(wc->mode.mode == MODE_PLAYBACK && wc->mode.pb_submode == PB_SUBMODE_PAUSE))
            {
                DISPLAY_CLEAR_HIGHLIGHT();
            }

            // if an error condition exists then display the error
            bool no_status_msg = (curr_us - last_status_msg_recv_time_us > 10000000);
            bool no_live_frame = (mode.mode == MODE_LIVE && 
                                  wc->status.cam_status == STATUS_INFO_OK &&
                                  curr_us - last_frame_recv_time_us > 10000000);
            if (win_minimized) {
                DISPLAY_TEXT("WINDOW MINIMIZED", "", "");
                last_frame_recv_time_us = curr_us;
            } else if (no_status_msg || no_live_frame) {
                char * info_str = (no_status_msg && no_live_frame ? "NOTHING RCVD" :
                                   no_status_msg                  ? "NO STATUS MSG" :
                                   no_live_frame                  ? "NO LIVE FRAME" :
                                                                    "")
                DISPLAY_TEXT(status2str(STATUS_ERR_DEAD), info_str, "");
            } else if (wc->mode.mode == MODE_LIVE && wc->status.cam_status != STATUS_INFO_OK) {
                DISPLAY_TEXT(status2str(wc->status.cam_status), "", "");
            } else if (wc->mode.mode == MODE_PLAYBACK && wc->status.rp_status != STATUS_INFO_OK) {
                DISPLAY_TEXT(status2str(wc->status.rp_status), "", "");
            }

            // receive msg header  
            // - use NON_BLOCKING_WITH_TIMEOUT because a periodic status msg shold be rcvd
            ret = net_recv(handle, &msg, sizeof(msg), NON_BLOCKING_WITH_TIMEOUT);
            if (ret == -1) {
                // error
                STATE_CHANGE(STATE_CONNECTED_ERROR, "ERROR", "recv msg hdr", "");
                usleep(MS);
                break;
            } else if (ret == 0) {
                // msg_header not available
                usleep(MS);
                break;
            }

            // the msg hdr has been received, keep track of stat for number of bytes recvd
            wc->recvd_bytes += ret;

            // process the msg
            switch (msg.msg_type) {
            case MSG_TYPE_FRAME:
                // verify data_len
                data_len = msg.u.mt_frame.data_len;
                if (data_len > RP_MAX_FRAME_DATA_LEN) {
                    int2str(int_str, data_len);
                    STATE_CHANGE(STATE_CONNECTED_ERROR, "ERROR", "invld data len", int_str);
                    break;
                }

                // receive msg data  
                if (data_len > 0) {
                    ret = net_recv(handle, data, data_len, BLOCKING_WITH_TIMEOUT);
                    if (ret < 0) {
                        STATE_CHANGE(STATE_CONNECTED_ERROR, "ERROR", "recv msg data", "");
                        break;
                    }
                    wc->recvd_bytes += ret;
                }

                // increment recvd_frames count
                wc->recvd_frames++;

                // if mode_id in received msg does not match our mode_id then discard
                if (msg.u.mt_frame.mode_id != wc->mode.mode_id) {
                    WARN("wc %c: discarding frame msg because msg mode_id %"PRId64" is not expected %"PRId64"\n",
                         id_char, msg.u.mt_frame.mode_id, wc->mode.mode_id);
                    break;
                }

                // don't display if the status for the mode we're in is not okay
                if ((wc->mode.mode == MODE_LIVE && wc->status.cam_status != STATUS_INFO_OK) ||
                    (wc->mode.mode == MODE_PLAYBACK && wc->status.rp_status != STATUS_INFO_OK))
                {
                    uint32_t status = (wc->mode.mode == MODE_LIVE ? wc->status.cam_status 
                                                                  : wc->status.rp_status);
                    WARN("wc %c: discarding frame msg because %s status is %s\n",
                         id_char, MODE_STR(wc->mode.mode), status2str(status));
                    break;
                }

#ifdef DEBUG_RECEIVED_FRAME_TIME
                // sanity check the received frame time
                if (wc->mode.mode == MODE_PLAYBACK) {
                    uint64_t time_diff_us, exp_time_us;

                    exp_time_us = PB_SUBMODE_PLAY_REAL_TIME_US(&wc->mode);
                    time_diff_us = (exp_time_us > msg.u.mt_frame.real_time_us
                                    ? exp_time_us - msg.u.mt_frame.real_time_us
                                    : msg.u.mt_frame.real_time_us - exp_time_us);
                    if (time_diff_us > 1000000) {
                        ERROR("wc %c: playback mode received frame time diff %d.%3.3d\n",
                              id_char, (int32_t)(time_diff_us / 1000000), (int32_t)(time_diff_us % 1000000) / 1000);
                    }
                }
#endif

                // if data_len is 0 that means webcam is responding with no image;
                // such as for a playback time when nothing was recorded
                if (data_len == 0) {
                    wc->frame_status = msg.u.mt_frame.status;
                    DISPLAY_TEXT(status2str(msg.u.mt_frame.status), "", "");
                    break;
                }

                // jpeg decode
                ret = jpeg_decode(id, JPEG_DECODE_MODE_YUY2,
                                  data, data_len,             // jpeg
                                  &image, &width, &height);   // pixels
                if (ret < 0) {
                    ERROR("wc %c: jpeg decode ret=%d\n", id_char, ret);
                    wc->frame_status = STATUS_ERR_JPEG_DECODE;
                    DISPLAY_TEXT(status2str(STATUS_ERR_JPEG_DECODE), "", "");
                    break;
                }

                // display the image
                wc->frame_status = STATUS_INFO_OK;
                last_frame_recv_time_us = microsec_timer();
                DISPLAY_IMAGE(image, width, height, msg.u.mt_frame.motion, msg.u.mt_frame.temperature);
                break;

            case MSG_TYPE_STATUS:
                // save status and last time status recvd
                wc->status = msg.u.mt_status;
                last_status_msg_recv_time_us = microsec_timer();
                break;

            default:
                int2str(int_str, msg.msg_type);
                STATE_CHANGE(STATE_CONNECTED_ERROR, "ERROR", "invld msg type", int_str);
                break;
            }
            break; }

        case STATE_CONNECTING_ERROR:
        case STATE_CONNECTED_ERROR:
            DISPLAY_CLEAR_HIGHLIGHT();  
            DISCONNECT();
            if (microsec_timer() - last_state_change_time_us < RECONNECT_TIME_US) {
                usleep(100*MS);
                break;
            }
            STATE_CHANGE(STATE_NOT_CONNECTED, "", "", "");
            break;

        case STATE_FATAL_ERROR:
            break;

        default: {
            char int_str[MAX_INT_STR];

            int2str(int_str, wc->state);
            STATE_CHANGE(STATE_FATAL_ERROR, "DISABLED", "invalid state", int_str);
            break; }
        }

        // if quit event or in fatal error state then exit this thread
        if (event.quit_event || wc->state == STATE_FATAL_ERROR) {
            break;
        }
    }

    // disconnect
    DISCONNECT();

    // exit thread
    __sync_fetch_and_add(&webcam_threads_running_count,-1);
    INFO("THREAD %d TERMINATING, quit_event=%d fatal_error=%d\n", 
         id, event.quit_event, wc->state == STATE_FATAL_ERROR);
    return NULL;
}
