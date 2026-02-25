/*
 * demo_c.c  –  Contador en C puro usando wxui_c.h
 *
 * Equivalente al demo.cpp de C++, pero sin tocar C++ en absoluto.
 *
 * Compilación:
 *   g++ -std=c++11 -c wxui_c.cpp -o wxui_c.o $(sdl2-config --cflags)
 *   gcc -std=c99   -c demo_c.c   -o demo_c.o  $(sdl2-config --cflags)
 *   g++ wxui_c.o demo_c.o -o demo_c $(sdl2-config --libs)
 */

#include <wxui_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Estado de la aplicación ──────────────────────────────────────────── */
typedef struct {
    WXApp*    app;
    int       counter;

    WXWidget* lblNum;
    WXWidget* lblStatus;
    WXWidget* btnInc;
    WXWidget* btnDec;
    WXWidget* btnRst;
    WXWidget* sld;
    WXWidget* spn;
    WXWidget* chkPar;
} AppState;

/* ── Helpers ──────────────────────────────────────────────────────────── */

static void sync_all(AppState* state, const char* msg) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", state->counter);
    wxwidget_set_text(state->lblNum, buf);
    wxwidget_set_value(state->sld, (float)state->counter);
    wxwidget_set_value(state->spn, (float)state->counter);
    wxwidget_set_enabled(state->btnDec, state->counter > 0   ? 1 : 0);
    wxwidget_set_enabled(state->btnInc, state->counter < 100 ? 1 : 0);
    if (msg && msg[0]) wxwidget_set_text(state->lblStatus, msg);
    wxapp_invalidate(state->app);
}

static int get_step(AppState* state) {
    return wxwidget_is_checked(state->chkPar) ? 2 : 1;
}

/* ── Callbacks de botones ─────────────────────────────────────────────── */

static void on_inc(WXWidget* w, const WXEvent* ev, void* ud) {
    (void)w; (void)ev;
    AppState* state = (AppState*)ud;
    int step = get_step(state);
    state->counter += step;
    if (state->counter > 100) state->counter = 100;
    char msg[64];
    snprintf(msg, sizeof(msg), "Incrementado a %d", state->counter);
    sync_all(state, msg);
}

static void on_dec(WXWidget* w, const WXEvent* ev, void* ud) {
    (void)w; (void)ev;
    AppState* state = (AppState*)ud;
    int step = get_step(state);
    state->counter -= step;
    if (state->counter < 0) state->counter = 0;
    char msg[64];
    snprintf(msg, sizeof(msg), "Decrementado a %d", state->counter);
    sync_all(state, msg);
}

static void on_rst(WXWidget* w, const WXEvent* ev, void* ud) {
    (void)w; (void)ev;
    AppState* state = (AppState*)ud;
    state->counter = 0;
    sync_all(state, "Contador reiniciado");
}

/* ── Callbacks de slider y spinner ───────────────────────────────────── */

static void on_slider_changed(WXWidget* w, const WXEvent* ev, void* ud) {
    (void)w;
    AppState* state = (AppState*)ud;
    state->counter = (int)ev->fvalue;
    if (wxwidget_is_checked(state->chkPar) && state->counter % 2 != 0)
        state->counter &= ~1;
    sync_all(state, NULL);
}

static void on_spinner_changed(WXWidget* w, const WXEvent* ev, void* ud) {
    (void)w;
    AppState* state = (AppState*)ud;
    state->counter = (int)ev->fvalue;
    if (wxwidget_is_checked(state->chkPar) && state->counter % 2 != 0)
        state->counter &= ~1;
    sync_all(state, NULL);
}

/* ── Callback de teclado global ──────────────────────────────────────── */

static void on_raw_event(const void* sdl_event, int* running, void* ud) {
    /*
     * sdl_event es un SDL_Event* pero lo casteamos desde void* para
     * no incluir SDL2 en este .c — solo necesitamos el tipo del evento.
     * Si necesitas acceso completo al SDL_Event, incluye <SDL2/SDL.h>.
     */
    typedef struct { unsigned int type; } MinEvent;
    typedef struct { unsigned int type; int scancode, sym, mod, unused; } KeyEvent;

    const MinEvent* me = (const MinEvent*)sdl_event;
    AppState* state = (AppState*)ud;

    /* SDL_KEYDOWN = 0x300 */
    if (me->type == 0x300u) {
        const KeyEvent* ke = (const KeyEvent*)sdl_event;
        /* SDLK_ESCAPE = 27, SDLK_UP = 1073741906, SDLK_DOWN = 1073741905, r = 114 */
        if      (ke->sym == 27)         *running = 0;
        else if (ke->sym == 1073741906) wxwidget_emit(state->btnInc, WXEV_CLICK);
        else if (ke->sym == 1073741905) wxwidget_emit(state->btnDec, WXEV_CLICK);
        else if (ke->sym == 114)        wxwidget_emit(state->btnRst, WXEV_CLICK);
    }
}

/* ── Callback de resize ──────────────────────────────────────────────── */

static void on_resize(int nw, int nh, void* ud) {
    AppState* state = (AppState*)ud;
    const int PAD = 10;
    wxwidget_set_rect(wxapp_find(state->app, "bg"),   0,  0,  nw, nh);
    wxwidget_set_rect(wxapp_find(state->app, "sep1"), PAD, 38, nw - PAD*2, 4);
    wxwidget_set_rect(state->sld,                     PAD, 130, nw - PAD*2, 16);
    wxwidget_set_rect(wxapp_find(state->app, "sbar"), 0,  nh - 24, nw, 24);
    wxwidget_set_rect(state->lblStatus,               6,  nh - 22, nw - 12, 20);
}

/* ── main ────────────────────────────────────────────────────────────── */

int main(void) {
    const int W   = 400;
    const int H   = 280;
    const int PAD = 10;

    AppState state;
    memset(&state, 0, sizeof(state));

    /* 1. Crear la ventana */
    state.app = wxapp_create("Contador en C puro", W, H, 1, 1);
    if (!state.app) return 1;

    /* 2. Construir la UI */
    wxmake_panel(state.app, "bg",    0, 0, W, H, 0);
    wxmake_label(state.app, "lbl_title", PAD, 12, W - PAD*2, 20,
                 "  Contador de ejemplo (C puro)", 0);
    wxmake_separator(state.app, "sep1", PAD, 38, W - PAD*2, 4, 1);
    wxmake_label(state.app, "lbl_val", PAD, 58, 60, 20, "Valor:", 0);

    state.lblNum = wxmake_label(state.app, "lbl_num", 80, 58, 80, 20, "0", 0);

    state.btnInc = wxmake_button(state.app, "btn_inc", PAD,       90, 90, 26, "Incrementar");
    state.btnDec = wxmake_button(state.app, "btn_dec", PAD + 100, 90, 90, 26, "Decrementar");
    state.btnRst = wxmake_button(state.app, "btn_rst", PAD + 200, 90, 80, 26, "Reiniciar");

    state.sld = wxmake_slider (state.app, "sld", PAD, 130, W - PAD*2, 16, 0.f, 100.f, 0.f);
    state.spn = wxmake_spinner(state.app, "spn", PAD, 158, 100, 22, 0.f, 100.f, 0.f, 1.f);

    state.chkPar = wxmake_checkbox(state.app, "chk_par", PAD, 192, 160, 18,
                                "Solo valores pares", 0);

    /* Status bar */
    wxmake_panel(state.app, "sbar", 0, H - 24, W, 24, 0);
    state.lblStatus = wxmake_label(state.app, "lbl_st", 6, H - 22, W - 12, 20, "Listo", 0);

    /* 3. Conectar callbacks */
    wxwidget_on(state.btnInc, WXEV_CLICK,         on_inc,            &state);
    wxwidget_on(state.btnDec, WXEV_CLICK,         on_dec,            &state);
    wxwidget_on(state.btnRst, WXEV_CLICK,         on_rst,            &state);
    wxwidget_on(state.sld,    WXEV_VALUE_CHANGED,  on_slider_changed, &state);
    wxwidget_on(state.spn,    WXEV_VALUE_CHANGED,  on_spinner_changed,&state);

    wxapp_on_event (state.app, on_raw_event, &state);
    wxapp_on_resize(state.app, on_resize,    &state);

    /* 4. Estado inicial */
    sync_all(&state, "Flechas UP/DOWN  |  R = reiniciar  |  ESC = salir");

    /* 5. Arrancar */
    wxapp_run(state.app, 60);

    /* 6. Limpiar */
    wxapp_destroy(state.app);
    return 0;
}