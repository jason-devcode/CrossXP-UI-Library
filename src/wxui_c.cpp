/*
 * wxui_c.cpp  –  Implementación C++ del wrapper C para winxp_ui
 *
 * Compila con:
 *   g++ -std=c++11 -c wxui_c.cpp -o wxui_c.o $(sdl2-config --cflags)
 */

#include "wxui_c.h"
#include "winxp_ui.hpp"
#include "window_manager.hpp"

#include <cstring>
#include <string>
#include <functional>
#include <unordered_map>

// ═══════════════════════════════════════════════════════════════════════════
//  Conversión entre enums
// ═══════════════════════════════════════════════════════════════════════════

static WXUI::EventType toEventType(WXEventType t) {
    switch (t) {
        case WXEV_CLICK:         return WXUI::EventType::Click;
        case WXEV_DBL_CLICK:     return WXUI::EventType::DblClick;
        case WXEV_MOUSE_DOWN:    return WXUI::EventType::MouseDown;
        case WXEV_MOUSE_UP:      return WXUI::EventType::MouseUp;
        case WXEV_MOUSE_MOVE:    return WXUI::EventType::MouseMove;
        case WXEV_MOUSE_ENTER:   return WXUI::EventType::MouseEnter;
        case WXEV_MOUSE_LEAVE:   return WXUI::EventType::MouseLeave;
        case WXEV_MOUSE_WHEEL:   return WXUI::EventType::MouseWheel;
        case WXEV_KEY_DOWN:      return WXUI::EventType::KeyDown;
        case WXEV_KEY_UP:        return WXUI::EventType::KeyUp;
        case WXEV_TEXT_INPUT:    return WXUI::EventType::TextInput;
        case WXEV_VALUE_CHANGED: return WXUI::EventType::ValueChanged;
        case WXEV_CHECK_CHANGED: return WXUI::EventType::CheckChanged;
        case WXEV_FOCUS_GAINED:  return WXUI::EventType::FocusGained;
        case WXEV_FOCUS_LOST:    return WXUI::EventType::FocusLost;
        default:                 return WXUI::EventType::Click;
    }
}

static WXEventType fromEventType(WXUI::EventType t) {
    switch (t) {
        case WXUI::EventType::Click:         return WXEV_CLICK;
        case WXUI::EventType::DblClick:      return WXEV_DBL_CLICK;
        case WXUI::EventType::MouseDown:     return WXEV_MOUSE_DOWN;
        case WXUI::EventType::MouseUp:       return WXEV_MOUSE_UP;
        case WXUI::EventType::MouseMove:     return WXEV_MOUSE_MOVE;
        case WXUI::EventType::MouseEnter:    return WXEV_MOUSE_ENTER;
        case WXUI::EventType::MouseLeave:    return WXEV_MOUSE_LEAVE;
        case WXUI::EventType::MouseWheel:    return WXEV_MOUSE_WHEEL;
        case WXUI::EventType::KeyDown:       return WXEV_KEY_DOWN;
        case WXUI::EventType::KeyUp:         return WXEV_KEY_UP;
        case WXUI::EventType::TextInput:     return WXEV_TEXT_INPUT;
        case WXUI::EventType::ValueChanged:  return WXEV_VALUE_CHANGED;
        case WXUI::EventType::CheckChanged:  return WXEV_CHECK_CHANGED;
        case WXUI::EventType::FocusGained:   return WXEV_FOCUS_GAINED;
        case WXUI::EventType::FocusLost:     return WXEV_FOCUS_LOST;
        default:                             return WXEV_CLICK;
    }
}

// Convierte UIEvent C++ → WXEvent C (sin alocar memoria extra)
static WXEvent toWXEvent(const WXUI::UIEvent& e) {
    WXEvent out;
    memset(&out, 0, sizeof(out));
    out.type   = fromEventType(e.type);
    out.mx     = e.mx;
    out.my     = e.my;
    out.mbtn   = e.mbtn;
    out.wheel  = e.wheel;
    out.key    = (int32_t)e.key;
    out.mod    = e.mod;
    out.fvalue = e.fvalue;
    out.ivalue = e.ivalue;
    out.svalue = e.svalue.c_str();   // válido mientras dure el callback
    out.text   = e.text;
    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Struct interno WXApp  (completa la declaración opaca del .h)
// ═══════════════════════════════════════════════════════════════════════════

struct WXApp_s {
    WXUI::Application*  app = nullptr;

    // Callbacks C del usuario
    WXEventCB   onEventCB   = nullptr;  void* onEventUD   = nullptr;
    WXFrameCB   onFrameCB   = nullptr;  void* onFrameUD   = nullptr;
    WXResizeCB  onResizeCB  = nullptr;  void* onResizeUD  = nullptr;

    // Almacén de strings para svalue en callbacks de widget
    // (necesario porque std::string::c_str() puede invalidarse)
    std::string lastSValue;
};

// ═══════════════════════════════════════════════════════════════════════════
//  Struct interno WXWidget  (completa la declaración opaca del .h)
// ═══════════════════════════════════════════════════════════════════════════

struct WXWidget_s {
    WXUI::UIComponent* ptr = nullptr;

    // Cada widget puede tener N callbacks C por tipo de evento
    // Los guardamos para que no se destruyan
    struct CBEntry { WXWidgetCB cb; void* ud; };
    std::unordered_map<int, std::vector<CBEntry>> callbacks;

    // Cache del texto para wxwidget_get_text
    mutable std::string textCache;
};

// ═══════════════════════════════════════════════════════════════════════════
//  Registro de WXWidget* por puntero C++
//  (para encontrar el WXWidget_s desde dentro de los callbacks C++)
// ═══════════════════════════════════════════════════════════════════════════

// Un mapa global simple; en aplicaciones multivventana se extendería.
static std::unordered_map<WXUI::UIComponent*, WXWidget_s*> g_widgetMap;

static WXWidget_s* ensureWrapper(WXUI::UIComponent* ptr) {
    auto it = g_widgetMap.find(ptr);
    if (it != g_widgetMap.end()) return it->second;
    auto* w = new WXWidget_s();
    w->ptr = ptr;
    g_widgetMap[ptr] = w;
    return w;
}

// ═══════════════════════════════════════════════════════════════════════════
//  IMPLEMENTACIÓN — APLICACIÓN
// ═══════════════════════════════════════════════════════════════════════════

WXApp* wxapp_create(const char* title, int width, int height,
                    int resizable, int software)
{
    auto* h   = new WXApp_s();
    h->app    = new WXUI::Application(title, width, height,
                                      resizable != 0, software != 0);
    if (!h->app->ok()) {
        delete h->app;
        delete h;
        return nullptr;
    }
    return h;
}

void wxapp_destroy(WXApp* app) {
    if (!app) return;
    // Liberar wrappers de widgets registrados para esta app
    for (auto& kv : g_widgetMap) delete kv.second;
    g_widgetMap.clear();
    delete app->app;
    delete app;
}

int  wxapp_ok    (const WXApp* app) { return app && app->app->ok() ? 1 : 0; }
int  wxapp_width (const WXApp* app) { return app ? app->app->width()  : 0; }
int  wxapp_height(const WXApp* app) { return app ? app->app->height() : 0; }

void wxapp_set_title(WXApp* app, const char* title) {
    if (app) app->app->setTitle(title);
}

void wxapp_invalidate(WXApp* app) {
    if (app) app->app->invalidate();
}

void wxapp_on_event(WXApp* app, WXEventCB cb, void* userdata) {
    if (!app) return;
    app->onEventCB = cb;
    app->onEventUD = userdata;
}

void wxapp_on_frame(WXApp* app, WXFrameCB cb, void* userdata) {
    if (!app) return;
    app->onFrameCB = cb;
    app->onFrameUD = userdata;
}

void wxapp_on_resize(WXApp* app, WXResizeCB cb, void* userdata) {
    if (!app) return;
    app->onResizeCB = cb;
    app->onResizeUD = userdata;
}

void wxapp_run(WXApp* app, int target_fps) {
    if (!app) return;

    // Conectar callbacks C → lambdas C++
    if (app->onEventCB) {
        app->app->onEvent([app](SDL_Event& ev, bool& running) {
            int r = running ? 1 : 0;
            app->onEventCB(&ev, &r, app->onEventUD);
            running = (r != 0);
        });
    }

    if (app->onFrameCB) {
        app->app->onFrame([app]() {
            app->onFrameCB(app->onFrameUD);
        });
    }

    if (app->onResizeCB) {
        app->app->onResize([app](int w, int h) {
            app->onResizeCB(w, h, app->onResizeUD);
        });
    }

    app->app->run(target_fps);
}

// ═══════════════════════════════════════════════════════════════════════════
//  IMPLEMENTACIÓN — WIDGETS
// ═══════════════════════════════════════════════════════════════════════════

WXWidget* wxapp_find(WXApp* app, const char* id) {
    if (!app) return nullptr;
    WXUI::UIComponent* ptr = app->app->find(id);
    if (!ptr) return nullptr;
    return ensureWrapper(ptr);
}

void wxwidget_on(WXWidget* w, WXEventType type, WXWidgetCB cb, void* userdata) {
    if (!w || !w->ptr) return;

    // Guardar la entrada en nuestro mapa
    WXWidget_s::CBEntry entry{ cb, userdata };
    w->callbacks[(int)type].push_back(entry);

    // Conectar al sistema C++ (solo la primera vez por tipo para no duplicar)
    if (w->callbacks[(int)type].size() == 1) {
        WXUI::EventType et = toEventType(type);
        w->ptr->on(et, [w, type](WXUI::UIComponent*, const WXUI::UIEvent& e) {
            WXEvent ce = toWXEvent(e);
            w->textCache = e.svalue;          // mantener el string vivo
            ce.svalue    = w->textCache.c_str();
            for (auto& entry : w->callbacks[(int)type]) {
                if (entry.cb) entry.cb(w, &ce, entry.ud);
            }
        });
    }
}

void wxwidget_emit(WXWidget* w, WXEventType type) {
    if (!w || !w->ptr) return;
    WXUI::UIEvent e;
    e.type = toEventType(type);
    w->ptr->emit(e);
}

void wxwidget_set_enabled(WXWidget* w, int enabled) {
    if (w && w->ptr) w->ptr->setEnabled(enabled != 0);
}

void wxwidget_set_visible(WXWidget* w, int visible) {
    if (w && w->ptr) w->ptr->setVisible(visible != 0);
}

void wxwidget_set_rect(WXWidget* w, int x, int y, int width, int height) {
    if (w && w->ptr) w->ptr->setRect(WXUI::Rect(x, y, width, height));
}

// ── Helpers por tipo ────────────────────────────────────────────────────

void wxwidget_set_text(WXWidget* w, const char* text) {
    if (!w || !w->ptr || !text) return;
    // UILabel y UITextInput tienen setText()
    if (auto* lbl = dynamic_cast<WXUI::UILabel*>(w->ptr))
        lbl->setText(text);
    else if (auto* ti = dynamic_cast<WXUI::UITextInput*>(w->ptr))
        ti->setText(text);
}

void wxwidget_set_value(WXWidget* w, float value) {
    if (!w || !w->ptr) return;
    if (auto* s = dynamic_cast<WXUI::UISlider*>(w->ptr))
        { s->setValue(value); s->markDirty(); }
    else if (auto* sp = dynamic_cast<WXUI::UISpinner*>(w->ptr))
        { sp->setValue(value); sp->markDirty(); }
}

float wxwidget_get_value(WXWidget* w) {
    if (!w || !w->ptr) return 0.f;
    if (auto* s  = dynamic_cast<WXUI::UISlider* >(w->ptr)) return s->getValue();
    if (auto* sp = dynamic_cast<WXUI::UISpinner*>(w->ptr)) return sp->getValue();
    return 0.f;
}

void wxwidget_set_checked(WXWidget* w, int checked) {
    if (!w || !w->ptr) return;
    if (auto* cb = dynamic_cast<WXUI::UICheckbox*>(w->ptr))
        cb->setChecked(checked != 0);
}

int wxwidget_is_checked(WXWidget* w) {
    if (!w || !w->ptr) return 0;
    if (auto* cb = dynamic_cast<WXUI::UICheckbox*>(w->ptr))
        return cb->isChecked() ? 1 : 0;
    return 0;
}

const char* wxwidget_get_text(WXWidget* w) {
    if (!w || !w->ptr) return "";
    if (auto* lbl = dynamic_cast<WXUI::UILabel*>(w->ptr))
        { w->textCache = lbl->getText(); return w->textCache.c_str(); }
    if (auto* ti = dynamic_cast<WXUI::UITextInput*>(w->ptr))
        { w->textCache = ti->getText(); return w->textCache.c_str(); }
    return "";
}

// ═══════════════════════════════════════════════════════════════════════════
//  IMPLEMENTACIÓN — CREACIÓN DE WIDGETS
// ═══════════════════════════════════════════════════════════════════════════

#define MAKE_WIDGET(expr)                                \
    if (!app) return nullptr;                            \
    WXUI::UIComponent* ptr = app->app->add(expr);        \
    if (!ptr) return nullptr;                            \
    return ensureWrapper(ptr);

WXWidget* wxmake_panel(WXApp* app, const char* id,
                        int x, int y, int w, int h, int dark)
{
    MAKE_WIDGET(WXUI::Make::Panel(id, WXUI::Rect(x, y, w, h), dark != 0))
}

WXWidget* wxmake_label(WXApp* app, const char* id,
                        int x, int y, int w, int h,
                        const char* text, int dark)
{
    MAKE_WIDGET(WXUI::Make::Label(id, WXUI::Rect(x, y, w, h), text ? text : "", dark != 0))
}

WXWidget* wxmake_button(WXApp* app, const char* id,
                         int x, int y, int w, int h,
                         const char* label)
{
    MAKE_WIDGET(WXUI::Make::Button(id, WXUI::Rect(x, y, w, h), label ? label : ""))
}

WXWidget* wxmake_checkbox(WXApp* app, const char* id,
                           int x, int y, int w, int h,
                           const char* label, int checked)
{
    MAKE_WIDGET(WXUI::Make::Checkbox(id, WXUI::Rect(x, y, w, h),
                                     label ? label : "", checked != 0))
}

WXWidget* wxmake_slider(WXApp* app, const char* id,
                         int x, int y, int w, int h,
                         float vmin, float vmax, float value)
{
    MAKE_WIDGET(WXUI::Make::Slider(id, WXUI::Rect(x, y, w, h), vmin, vmax, value))
}

WXWidget* wxmake_spinner(WXApp* app, const char* id,
                          int x, int y, int w, int h,
                          float vmin, float vmax, float value, float step)
{
    MAKE_WIDGET(WXUI::Make::Spinner(id, WXUI::Rect(x, y, w, h), vmin, vmax, value, step))
}

WXWidget* wxmake_textinput(WXApp* app, const char* id,
                            int x, int y, int w, int h,
                            const char* placeholder)
{
    MAKE_WIDGET(WXUI::Make::TextInput(id, WXUI::Rect(x, y, w, h), placeholder ? placeholder : ""))
}

WXWidget* wxmake_separator(WXApp* app, const char* id,
                            int x, int y, int w, int h, int horizontal)
{
    MAKE_WIDGET(WXUI::Make::Separator(id, WXUI::Rect(x, y, w, h), horizontal != 0))
}

WXWidget* wxmake_groupbox(WXApp* app, const char* id,
                           int x, int y, int w, int h,
                           const char* title)
{
    MAKE_WIDGET(WXUI::Make::GroupBox(id, WXUI::Rect(x, y, w, h), title ? title : ""))
}