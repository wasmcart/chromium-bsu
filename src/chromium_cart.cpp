/*
 * chromium_cart.cpp — wasmcart entry point for Chromium B.S.U.
 *
 * Provides wc_get_info, wc_init, wc_render exports.
 * Replaces main.cpp, MainSDL, and MainGLUT.
 */

#include "wasmcart.h"
#include "Renderer.h"

#include "Config.h"
#include "Global.h"
#include "HiScore.h"
#include "MainGL.h"
#include "MenuGL.h"
#include "HeroAircraft.h"
#include "Audio.h"
#include "AudioWasmcart.h"
#include "MainToolkit.h"

#include <cstdlib>
#include <cstring>
#include <ctime>

/* ── Default / max resolution ──────────────────────────────────── */
#define DEFAULT_WIDTH  800
#define DEFAULT_HEIGHT 600
#define MAX_WIDTH      1920
#define MAX_HEIGHT     1080

/* ── Static state ────────────────────────────────────────────────── */
static wc_info_t info;
static wc_host_info_t host_info;

static uint32_t cur_width = DEFAULT_WIDTH;
static uint32_t cur_height = DEFAULT_HEIGHT;

/* Audio ring buffer: 4096 stereo frames */
#define AUDIO_CAP 4096
static float audio_ring[AUDIO_CAP * 2];
static uint32_t audio_write_cursor;

/* Input state */
static wc_pad_t pads[4];
static wc_time_t time_info;

/* Gamepad button edge detection */
static uint16_t prev_buttons = 0;

/* Frame timing */
static int frame_count = 0;

/* ── MainToolkitWC — minimal toolkit stub ────────────────────────── */
class MainToolkitWC : public MainToolkit
{
public:
    MainToolkitWC() : MainToolkit(0, 0) {}
    virtual ~MainToolkitWC() {}
    virtual bool run() { return false; }
    virtual bool checkErrors() { return false; }
    virtual bool setVideoMode() { return true; }
    virtual void grabMouse(bool, bool) {}
};

/* ── dataLoc() — resolve asset paths ─────────────────────────────── */
/* Override the original dataLoc() from main.cpp.
 * Returns path as-is since wasmcart asset paths are relative to cart root. */
static char dataLocBuf[256];

extern "C" {

const char* dataLoc(const char* filename, bool doCheck)
{
    (void)doCheck;
    int len = 0;
    while (filename[len] && len < 254) len++;
    memcpy(dataLocBuf, filename, len);
    dataLocBuf[len] = 0;
    return dataLocBuf;
}

const char* alterPathForPlatform(char* filename)
{
    return filename;
}

void printExtensions(FILE *fstream, const char* extstr_in)
{
    (void)fstream; (void)extstr_in;
}

/* ── Syscall stubs ───────────────────────────────────────────────── */
/* Prevent wasi-libc from importing syscalls we don't need. */

int __syscall_unlinkat(int dirfd, const char* path, int flags)
{
    (void)dirfd; (void)path; (void)flags;
    return -1;
}

int __syscall_rmdir(const char* path)
{
    (void)path;
    return -1;
}

} /* extern "C" */

/* ── Input handling (called each frame before render) ────────────── */
static void handle_input(void)
{
    Global *game = Global::getInstance();
    Config *config = Config::instance();
    uint16_t buttons = pads[0].buttons;
    uint16_t pressed = buttons & ~prev_buttons; /* newly pressed */

    if (game->gameMode == Global::Menu) {
        /* Menu navigation */
        if (pressed & WC_BTN_UP) {
            game->menu->keyHit(MainToolkit::KeyUp);
        }
        if (pressed & WC_BTN_DOWN) {
            game->menu->keyHit(MainToolkit::KeyDown);
        }
        if (pressed & WC_BTN_LEFT) {
            game->menu->keyHit(MainToolkit::KeyLeft);
        }
        if (pressed & WC_BTN_RIGHT) {
            game->menu->keyHit(MainToolkit::KeyRight);
        }
        if (pressed & (WC_BTN_A | WC_BTN_START)) {
            game->menu->keyHit(MainToolkit::KeyEnter);
        }
    }
    else if (game->gameMode == Global::Game) {
        /* In-game controls */
        HeroAircraft *hero = game->hero;
        if (!hero) { prev_buttons = buttons; return; }

        /* Movement — use hero->moveEvent(int dx, int dy) which is the
         * same API that mouse/keyboard input uses. It applies
         * config->movementSpeed() internally and handles clamping.
         * Values are in "pixel-like" units: ~10-20 per frame = fast. */
        int mx = 0, my = 0;

        /* Analog stick (values: -32768 to 32767) → scale to ~±8 */
        #define STICK_DEAD 3000
        #define STICK_SCALE 8
        if (pads[0].left_x > STICK_DEAD || pads[0].left_x < -STICK_DEAD) {
            mx = (int)(pads[0].left_x * STICK_SCALE / 32768);
        }
        if (pads[0].left_y > STICK_DEAD || pads[0].left_y < -STICK_DEAD) {
            my = (int)(pads[0].left_y * STICK_SCALE / 32768);
        }

        /* Dpad overrides stick */
        if (buttons & WC_BTN_LEFT)  mx = -STICK_SCALE;
        if (buttons & WC_BTN_RIGHT) mx =  STICK_SCALE;
        if (buttons & WC_BTN_UP)    my = -STICK_SCALE;
        if (buttons & WC_BTN_DOWN)  my =  STICK_SCALE;

        if (mx || my) {
            hero->moveEvent(mx, my);
        }

        /* Fire — button A: only call fireGun on edges to avoid
         * resetting gunPause cooldown every frame */
        {
            static bool firing = false;
            bool wantFire = !!(buttons & WC_BTN_A);
            if (wantFire && !firing) {
                hero->fireGun(true);
                firing = true;
            } else if (!wantFire && firing) {
                hero->fireGun(false);
                firing = false;
            }
        }

        /* Self-destruct — button B */
        if (pressed & WC_BTN_B) {
            hero->useItem(0);
        }

        /* Pause — Start button */
        if (pressed & WC_BTN_START) {
            game->game_pause = !game->game_pause;
            game->audio->pauseGameMusic(game->game_pause);
        }
    }
    else if (game->gameMode == Global::HeroDead) {
        /* After death — any button returns to menu */
        if (game->heroDeath <= -100) {
            if (pressed & (WC_BTN_A | WC_BTN_START)) {
                game->gameMode = Global::Menu;
                game->menu->startMenu();
                game->audio->setMusicMode(Audio::MusicMenu);
            }
        }
    }

    prev_buttons = buttons;
}

/* ── Audio mixing (write to ring buffer) ─────────────────────────── */
static void mix_audio_frame(void)
{
    Global *game = Global::getInstance();
    if (!game || !game->audio) return;

    AudioWasmcart *aw = (AudioWasmcart*)game->audio;

    /* Calculate samples needed from actual delta time and sample rate */
    float delta = time_info.delta_ms;
    if (delta <= 0.0f || delta > 200.0f) delta = 16.667f; /* fallback ~60fps */
    const int SAMPLE_RATE = 44100;
    int mix_frames = (int)(SAMPLE_RATE * delta / 1000.0f);
    if (mix_frames < 1) mix_frames = 1;
    if (mix_frames > 4096) mix_frames = 4096; /* cap to ring buffer size */

    float tmp[4096 * 2];
    aw->mixAudio(tmp, mix_frames);

    for (int i = 0; i < mix_frames; i++) {
        uint32_t idx = (audio_write_cursor % AUDIO_CAP) * 2;
        audio_ring[idx]     = tmp[i * 2];
        audio_ring[idx + 1] = tmp[i * 2 + 1];
        audio_write_cursor++;
    }
}

/* ── wasmcart exports ────────────────────────────────────────────── */

extern "C" {

__attribute__((export_name("wc_get_info")))
wc_info_t* wc_get_info(void)
{
    memset(&info, 0, sizeof(info));
    memset(&host_info, 0, sizeof(host_info));

    info.version = WC_ABI_VERSION;
    info.width = DEFAULT_WIDTH;
    info.height = DEFAULT_HEIGHT;
    info.fb_ptr = 0; /* GL cart — no CPU framebuffer */

    /* Audio ring buffer */
    info.audio_ptr = (uint32_t)(uintptr_t)audio_ring;
    info.audio_cap = AUDIO_CAP;
    info.audio_write_ptr = (uint32_t)(uintptr_t)&audio_write_cursor;

    /* Input */
    info.input_ptr = (uint32_t)(uintptr_t)pads;

    /* Time */
    info.time_ptr = (uint32_t)(uintptr_t)&time_info;

    /* Host info for resolution negotiation */
    info.host_info_ptr = (uint32_t)(uintptr_t)&host_info;

    info.flags = WC_FLAG_AUDIO_F32;
    info.audio_sample_rate = 44100;

    return &info;
}

__attribute__((export_name("wc_init")))
void wc_init(void)
{
    WC_LOG("wc_init: start");

    /* Read host's preferred resolution */
    if (host_info.preferred_width > 0 && host_info.preferred_height > 0) {
        cur_width = host_info.preferred_width;
        cur_height = host_info.preferred_height;
        if (cur_width > MAX_WIDTH) cur_width = MAX_WIDTH;
        if (cur_height > MAX_HEIGHT) cur_height = MAX_HEIGHT;
    }

    /* Update info with actual resolution */
    info.width = cur_width;
    info.height = cur_height;

    WC_LOG("wc_init: calling renderer_init");

    /* Initialize GL compat layer */
    renderer_init();

    WC_LOG("wc_init: renderer_init done, initializing game");

    /* Initialize game singletons */
    Config *config = Config::init();
    config->setScreenSize((int)cur_width, (int)cur_height);

    Global *game = Global::init();
    HiScore::init();

    srand(42); /* Deterministic seed */
    game->generateRandom();

    /* Create our toolkit stub */
    game->toolkit = new MainToolkitWC();

    WC_LOG("wc_init: calling createGame");

    /* Create the game */
    game->createGame();

    WC_LOG("wc_init: createGame done");

    /* Start with menu music */
    game->audio->setMusicMode(Audio::MusicMenu);

    frame_count = 0;
    prev_buttons = 0;
    audio_write_cursor = 0;

    WC_LOG("wc_init: complete");
}

__attribute__((export_name("wc_render")))
void wc_render(void)
{
    Global *game = Global::getInstance();

    if (frame_count == 0) {
        WC_LOG("wc_render: first frame");
    }

    /* Handle input every frame */
    handle_input();

    float delta = time_info.delta_ms;
    if (delta <= 0.0f || delta > 200.0f) delta = 20.0f;

    /* Real fps for HUD display */
    game->fps = 1000.0f / delta;

    /* Delta-time scaling: speedAdj = 1.0 at 50fps (20ms per frame) */
    game->speedAdj = delta / 20.0f;

    /* Update + draw */
    game->mainGL->drawGL();
    renderer_end_frame();

    if (frame_count == 0) {
        WC_LOG("wc_render: first frame done");
    }

    /* Mix audio based on real delta time */
    mix_audio_frame();

    game->frame++;
    frame_count++;
}

} /* extern "C" */
