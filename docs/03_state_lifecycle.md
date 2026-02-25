# 03 — Estado y ciclo de vida

---

## El ciclo de vida de un frame

```
┌─────────────────────────────────────────────────────────┐
│                   Bucle principal (run)                 │
│                                                         │
│  1. SDL_PollEvent() ──► onEvent callback (usuario)      │
│                    └──► ctx.processEvent()              │
│                              │                          │
│                              ▼                          │
│                    widget.onEvent()                     │
│                    widget.emit(UIEvent)                 │
│                    callbacks de usuario                 │
│                                                         │
│  2. onFrame callback (usuario)                          │
│                                                         │
│  3. ctx.render()                                        │
│       para cada widget dirty:                           │
│           widget.onDraw()  → escribe en widget.surf     │
│       compositar todos surf → root surface              │
│       SDL_UpdateTexture → GPU → pantalla                │
│                                                         │
│  4. SDL_Delay (frame cap)                               │
└─────────────────────────────────────────────────────────┘
```

---

## Dirty / Redraw — cómo funciona el render retenido

La librería usa un modelo **retained mode**: los widgets solo se redibujan cuando están marcados como "dirty".

```cpp
widget->markDirty();   // marca este widget para redibujado
ctx.needsRedraw = true; // fuerza una pasada de composición completa
```

Regla general:
- Cuando cambias el **estado visual** de un widget (texto, valor, color), llama a `markDirty()`.
- Cuando usas los métodos estándar (`setText`, `setValue`, `setEnabled`, `setChecked`), ya llaman a `markDirty()` internamente.
- Cuando actualizas el framebuffer de un `UIViewport3D` con `updatePixels()`, también llama a `markDirty()` automáticamente.
- `app.invalidate()` equivale a `ctx.needsRedraw = true` — fuerza re-composición aunque ningún widget esté dirty.

---

## Gestión de estado en C++

### Patrón recomendado: struct de estado + lambdas con captura

```cpp
#include "winxp_ui.hpp"
#include "window_manager.hpp"
using namespace WXUI;

struct AppState {
    int    contador = 0;
    float  volumen  = 0.5f;
    bool   mutear   = false;
    std::string archivo;
};

int main() {
    Application app("Demo estado", 400, 300);
    AppState estado;

    auto* lblContador = static_cast<UILabel*>(
        app.add(Make::Label("lbl_c", Rect(10,10,200,20), "0"))
    );
    auto* sldVol = static_cast<UISlider*>(
        app.add(Make::Slider("sld_vol", Rect(10,50,300,16), 0.f, 1.f, 0.5f))
    );
    auto* chkMute = static_cast<UICheckbox*>(
        app.add(Make::Checkbox("chk_mute", Rect(10,80,120,18), "Mute", false))
    );

    // Función de sincronización: estado → UI
    auto syncUI = [&]() {
        lblContador->setText(std::to_string(estado.contador));
        sldVol->setValue(estado.volumen);
        sldVol->setEnabled(!estado.mutear);
        sldVol->markDirty();
        app.invalidate();
    };

    // Botones que modifican el estado
    auto* btnInc = static_cast<UIButton*>(
        app.add(Make::Button("btn_inc", Rect(10,110,80,26), "+1"))
    );
    btnInc->on(EventType::Click, [&](UIComponent*, const UIEvent&) {
        estado.contador++;
        syncUI();
    });

    // Slider modifica el estado
    sldVol->on(EventType::ValueChanged, [&](UIComponent*, const UIEvent& e) {
        estado.volumen = e.fvalue;
        // (no llamamos syncUI para evitar bucle)
    });

    // Checkbox modifica el estado y actualiza UI
    chkMute->on(EventType::CheckChanged, [&](UIComponent*, const UIEvent& e) {
        estado.mutear = (e.ivalue != 0);
        syncUI();
    });

    syncUI();   // estado inicial
    app.run(60);
}
```

---

## Gestión de estado en C

En C, el estado vive en un struct que se pasa como `userdata` a cada callback.

```c
#include "wxui_c.h"
#include <stdio.h>

/* ── Struct de estado ─────────────────────────────────── */
typedef struct {
    WXApp*    app;
    int       contador;
    float     volumen;
    int       mutear;

    WXWidget* lblContador;
    WXWidget* sldVol;
    WXWidget* chkMute;
} AppState;

/* ── Sincroniza estado → UI ───────────────────────────── */
static void sync_ui(AppState* s) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", s->contador);
    wxwidget_set_text(s->lblContador, buf);
    wxwidget_set_value(s->sldVol, s->volumen);
    wxwidget_set_enabled(s->sldVol, !s->mutear);
    wxapp_invalidate(s->app);
}

/* ── Callbacks ────────────────────────────────────────── */
static void on_inc(WXWidget* w, const WXEvent* e, void* ud) {
    (void)w; (void)e;
    AppState* s = (AppState*)ud;
    s->contador++;
    sync_ui(s);
}

static void on_vol(WXWidget* w, const WXEvent* e, void* ud) {
    (void)w;
    AppState* s = (AppState*)ud;
    s->volumen = e->fvalue;
}

static void on_mute(WXWidget* w, const WXEvent* e, void* ud) {
    (void)w;
    AppState* s = (AppState*)ud;
    s->mutear = e->ivalue;
    sync_ui(s);
}

/* ── main ─────────────────────────────────────────────── */
int main(void) {
    AppState s = {0};
    s.app      = wxapp_create("Demo estado", 400, 300, 1, 1);
    s.volumen  = 0.5f;

    s.lblContador = wxmake_label  (s.app, "lbl_c",   10, 10, 200, 20, "0", 0);
    s.sldVol      = wxmake_slider (s.app, "sld_vol", 10, 50, 300, 16, 0.f, 1.f, 0.5f);
    s.chkMute     = wxmake_checkbox(s.app, "chk_mute",10,80, 120, 18, "Mute", 0);

    WXWidget* btnInc = wxmake_button(s.app, "btn_inc", 10, 110, 80, 26, "+1");
    wxwidget_on(btnInc,   WXEV_CLICK,          on_inc,  &s);
    wxwidget_on(s.sldVol, WXEV_VALUE_CHANGED,   on_vol,  &s);
    wxwidget_on(s.chkMute,WXEV_CHECK_CHANGED,   on_mute, &s);

    sync_ui(&s);
    wxapp_run(s.app, 60);
    wxapp_destroy(s.app);
    return 0;
}
```

---

## El sistema de eventos

### EventType (C++) / WXEventType (C)

```
Eventos de ratón:
  Click          — botón soltado sobre el widget (el más común)
  DblClick       — doble clic
  MouseDown      — botón presionado
  MouseUp        — botón soltado
  MouseMove      — movimiento (solo si el botón está presionado sobre el widget)
  MouseEnter     — ratón entra en el área del widget
  MouseLeave     — ratón sale del área del widget
  MouseWheel     — rueda del ratón

Eventos de teclado:
  KeyDown        — tecla presionada
  KeyUp          — tecla soltada
  TextInput      — carácter de texto introducido (SDL_TEXTINPUT)

Eventos de valor:
  ValueChanged   — Slider / Spinner cambian de valor   → e.fvalue
  CheckChanged   — Checkbox / RadioButton cambian      → e.ivalue (0/1)
  Scroll         — ScrollBar se mueve                  → e.fvalue (0..1)

Eventos de foco:
  FocusGained    — widget recibe el foco de teclado
  FocusLost      — widget pierde el foco de teclado

Especiales:
  MenuItemClicked — ítem de menú seleccionado          → e.svalue = id del item
  ViewportFocusGained / ViewportFocusLost
```

### Estructura UIEvent (C++)

```cpp
struct UIEvent {
    EventType   type;
    int         mx, my;      // posición del ratón en pantalla
    int         mbtn;        // SDL_BUTTON_LEFT/MIDDLE/RIGHT
    int         wheel;       // delta de rueda
    SDL_Keycode key;         // código de tecla
    uint16_t    mod;         // modificadores (KMOD_SHIFT, KMOD_CTRL, etc.)
    const char* text;        // texto UTF-8 para TextInput
    float       fvalue;      // valor float (Slider, Spinner, Scroll)
    int         ivalue;      // valor int (Check=0/1, menu index)
    std::string svalue;      // valor string (TextInput, MenuItemClicked)
};
```

### Registrar múltiples callbacks para el mismo evento

```cpp
// Se pueden agregar varios; se ejecutan en orden de registro
btn->on(EventType::Click, callback_a);
btn->on(EventType::Click, callback_b);   // también se llama
```

### Emitir eventos programáticamente

```cpp
// Simular un click
UIEvent e;
e.type = EventType::Click;
btn->emit(e);
```

```c
// En C
wxwidget_emit(btn, WXEV_CLICK);
```

---

## Ciclo de vida de un widget

```
Construcción → addChild / ctx.add() → [vida útil] → destrucción
     │               │                    │               │
     │          se registra en        se renderiza    UIContext
     │          ctx.registry          cuando dirty    libera la memoria
     │          se llama a
     │          markDirty()
     │
     ▼
  onDraw() se llama la primera vez (dirty=true desde construcción)
```

### Agregar y quitar widgets dinámicamente

```cpp
// Agregar en cualquier momento
auto* nuevo = static_cast<UILabel*>(
    app.add(Make::Label("nuevo", Rect(10, 10, 100, 20), "Dinámico"))
);

// Quitar por id (destruye el widget y libera memoria)
app.ctx().remove("nuevo");
```

### Mostrar/ocultar sin destruir

```cpp
widget->setVisible(false);   // oculto, no renderizado, no responde eventos
widget->setVisible(true);    // vuelve a mostrarse
```

---

## Lógica por frame (animaciones)

Usa `onFrame` para actualizar datos que cambian continuamente:

```cpp
float angulo = 0.f;
auto* lbl = static_cast<UILabel*>(app.add(Make::Label("lbl", Rect(10,10,200,20), "")));

app.onFrame([&]() {
    angulo += 0.05f;
    char buf[64];
    snprintf(buf, sizeof(buf), "angulo: %.2f", angulo);
    lbl->setText(buf);   // markDirty se llama internamente
});

app.run(60);
```

---

## Hijos y jerarquía

Los widgets pueden tener hijos. Los hijos se renderizan encima del padre y se posicionan con coordenadas **absolutas de ventana**.

```cpp
auto* panel = static_cast<UIPanel*>(
    app.add(Make::Panel("panel", Rect(50, 50, 300, 200)))
);

// Agregar hijo al panel (coordenadas absolutas)
auto lblHijo = Make::Label("lbl_hijo", Rect(60, 70, 100, 20), "Soy hijo");
panel->addChild(std::move(lblHijo), app.ctx());

// El hijo se puede buscar desde cualquier sitio
auto* hijo = app.find("lbl_hijo");
```

> Los hijos heredan la visibilidad del padre: si el padre está oculto, los hijos también.

---

## Focus y teclado

Solo el widget con **foco** recibe eventos de teclado.

```cpp
// Dar foco explícitamente
app.ctx().setFocus(widget);

// Verificar quién tiene el foco
UIComponent* w = app.ctx().getFocused();

// Navegar con Tab / Shift+Tab entre widgets (automático)
```

El usuario puede navegar entre widgets con `Tab` (adelante) y `Shift+Tab` (atrás).

---

## Patrones de estado avanzados

### Estado derivado (computed)

```cpp
struct Estado {
    float precio = 10.f;
    int   cantidad = 1;
    // total es derivado: no se almacena, se calcula
    float total() const { return precio * cantidad; }
};

Estado e;
auto* lblTotal = static_cast<UILabel*>(
    app.add(Make::Label("lbl_total", Rect(10, 80, 200, 20), ""))
);

auto sync = [&]() {
    char buf[64];
    snprintf(buf, sizeof(buf), "Total: %.2f", e.total());
    lblTotal->setText(buf);
};
```

### Estado con historial (undo/redo)

```cpp
std::vector<int> historial;
int posActual = -1;

auto pushEstado = [&](int valor) {
    // Truncar el futuro al desviarnos
    historial.resize(posActual + 1);
    historial.push_back(valor);
    posActual++;
};

auto undo = [&]() -> int {
    if (posActual > 0) posActual--;
    return historial[posActual];
};

auto redo = [&]() -> int {
    if (posActual < (int)historial.size() - 1) posActual++;
    return historial[posActual];
};
```