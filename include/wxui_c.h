#ifndef WXUI_C_H
#define WXUI_C_H
/*
 * wxui_c.h  –  API en C puro para winxp_ui / window_manager
 *
 * Todos los tipos C++ se ocultan detrás de punteros opacos.
 * Incluye solo este header desde C; nunca incluyas winxp_ui.hpp.
 *
 * Compilación:
 *   - Compila wxui_c.cpp con g++ (C++11)
 *   - Compila tu código C con gcc
 *   - Enlaza ambos juntos:
 *
 *   g++ -std=c++11 -c wxui_c.cpp -o wxui_c.o $(sdl2-config --cflags)
 *   gcc -c mi_app.c -o mi_app.o $(sdl2-config --cflags)
 *   g++ wxui_c.o mi_app.o -o mi_app $(sdl2-config --libs)
 *
 * Nota: el enlazado final debe hacerse con g++ para incluir la runtime C++.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════════
 *  TIPOS OPACOS
 * ═══════════════════════════════════════════════════════════════════ */

typedef struct WXApp_s     WXApp;      /* WXUI::Application       */
typedef struct WXWidget_s  WXWidget;   /* WXUI::UIComponent*      */

/* ═══════════════════════════════════════════════════════════════════
 *  EVENTO (subconjunto útil para C)
 * ═══════════════════════════════════════════════════════════════════ */

typedef enum {
    WXEV_CLICK = 0,
    WXEV_DBL_CLICK,
    WXEV_MOUSE_DOWN,
    WXEV_MOUSE_UP,
    WXEV_MOUSE_MOVE,
    WXEV_MOUSE_ENTER,
    WXEV_MOUSE_LEAVE,
    WXEV_MOUSE_WHEEL,
    WXEV_KEY_DOWN,
    WXEV_KEY_UP,
    WXEV_TEXT_INPUT,
    WXEV_VALUE_CHANGED,
    WXEV_CHECK_CHANGED,
    WXEV_FOCUS_GAINED,
    WXEV_FOCUS_LOST,
} WXEventType;

typedef struct {
    WXEventType type;
    int         mx, my;     /* posición del mouse                  */
    int         mbtn;       /* botón: 1=izq, 2=medio, 3=der        */
    int         wheel;      /* delta scroll                        */
    int32_t     key;        /* SDL_Keycode                         */
    uint16_t    mod;        /* modificadores de teclado            */
    float       fvalue;     /* valor float (sliders, spinners)     */
    int         ivalue;     /* valor int  (checkboxes: 0/1)        */
    const char* svalue;     /* valor string (text input)           */
    const char* text;       /* SDL_TEXTINPUT                       */
} WXEvent;

/* ═══════════════════════════════════════════════════════════════════
 *  TIPOS DE CALLBACKS
 *
 *  Todos reciben un puntero `userdata` que tú defines al registrar
 *  el callback — úsalo para pasar tu estado de aplicación.
 * ═══════════════════════════════════════════════════════════════════ */

/* Callback de widget: se llama cuando ocurre un evento en un widget */
typedef void (*WXWidgetCB)(WXWidget* widget, const WXEvent* ev, void* userdata);

/* Callback de evento SDL raw: recibe el evento SDL como bytes opacos.
 * Pon *running = 0 para salir del bucle.                           */
typedef void (*WXEventCB)(const void* sdl_event, int* running, void* userdata);

/* Callback de frame: se llama una vez por frame antes del render   */
typedef void (*WXFrameCB)(void* userdata);

/* Callback de resize: recibe nuevo ancho y alto                    */
typedef void (*WXResizeCB)(int w, int h, void* userdata);

/* ═══════════════════════════════════════════════════════════════════
 *  APLICACIÓN
 * ═══════════════════════════════════════════════════════════════════ */

/* Crea la ventana. Devuelve NULL si falla.
 * resizable: 1 = ventana redimensionable
 * software:  1 = renderer software (compatible, más lento)
 *            0 = renderer acelerado por GPU                        */
WXApp* wxapp_create(const char* title, int width, int height,
                    int resizable, int software);

/* Libera todos los recursos (ventana, renderer, SDL)               */
void   wxapp_destroy(WXApp* app);

/* 1 si la inicialización fue correcta, 0 si hubo error            */
int    wxapp_ok(const WXApp* app);

int    wxapp_width (const WXApp* app);
int    wxapp_height(const WXApp* app);

/* Cambia el título de la ventana en tiempo de ejecución           */
void   wxapp_set_title(WXApp* app, const char* title);

/* Fuerza un redibujado en el próximo frame                        */
void   wxapp_invalidate(WXApp* app);

/* Registra callbacks (pasar NULL para desregistrar)               */
void   wxapp_on_event (WXApp* app, WXEventCB  cb, void* userdata);
void   wxapp_on_frame (WXApp* app, WXFrameCB  cb, void* userdata);
void   wxapp_on_resize(WXApp* app, WXResizeCB cb, void* userdata);

/* Inicia el bucle principal. Bloquea hasta que se cierra la app.
 * target_fps: fotogramas por segundo deseados (0 = sin límite)    */
void   wxapp_run(WXApp* app, int target_fps);

/* ═══════════════════════════════════════════════════════════════════
 *  GESTIÓN DE WIDGETS
 * ═══════════════════════════════════════════════════════════════════ */

/* Busca un widget por su id de cadena                              */
WXWidget* wxapp_find(WXApp* app, const char* id);

/* Registra un callback para un tipo de evento sobre un widget     */
void wxwidget_on(WXWidget* w, WXEventType type, WXWidgetCB cb, void* userdata);

/* Dispara manualmente un evento sobre un widget                   */
void wxwidget_emit(WXWidget* w, WXEventType type);

/* Activa / desactiva un widget                                    */
void wxwidget_set_enabled(WXWidget* w, int enabled);
void wxwidget_set_visible(WXWidget* w, int visible);

/* Mueve / redimensiona un widget                                  */
void wxwidget_set_rect(WXWidget* w, int x, int y, int width, int height);

/* ── Helpers específicos por tipo de widget ─────────────────────── */

/* Label / cualquier widget con texto                               */
void wxwidget_set_text(WXWidget* w, const char* text);

/* Slider y Spinner                                                 */
void  wxwidget_set_value(WXWidget* w, float value);
float wxwidget_get_value(WXWidget* w);

/* Checkbox                                                        */
void wxwidget_set_checked(WXWidget* w, int checked);
int  wxwidget_is_checked(WXWidget* w);

/* TextInput                                                       */
const char* wxwidget_get_text(WXWidget* w);

/* ═══════════════════════════════════════════════════════════════════
 *  CREACIÓN DE WIDGETS
 *
 *  Cada función crea el widget Y lo agrega a la app.
 *  Devuelve un WXWidget* que puedes guardar para conectar callbacks
 *  o modificar después.
 * ═══════════════════════════════════════════════════════════════════ */

WXWidget* wxmake_panel    (WXApp* app, const char* id,
                            int x, int y, int w, int h, int dark);

WXWidget* wxmake_label    (WXApp* app, const char* id,
                            int x, int y, int w, int h,
                            const char* text, int dark);

WXWidget* wxmake_button   (WXApp* app, const char* id,
                            int x, int y, int w, int h,
                            const char* label);

WXWidget* wxmake_checkbox (WXApp* app, const char* id,
                            int x, int y, int w, int h,
                            const char* label, int checked);

WXWidget* wxmake_slider   (WXApp* app, const char* id,
                            int x, int y, int w, int h,
                            float vmin, float vmax, float value);

WXWidget* wxmake_spinner  (WXApp* app, const char* id,
                            int x, int y, int w, int h,
                            float vmin, float vmax, float value, float step);

WXWidget* wxmake_textinput(WXApp* app, const char* id,
                            int x, int y, int w, int h,
                            const char* placeholder);

WXWidget* wxmake_separator(WXApp* app, const char* id,
                            int x, int y, int w, int h, int horizontal);

WXWidget* wxmake_groupbox (WXApp* app, const char* id,
                            int x, int y, int w, int h,
                            const char* title);

#ifdef __cplusplus
}
#endif

#endif /* WXUI_C_H */