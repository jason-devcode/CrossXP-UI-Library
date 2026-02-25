# 01 — Primeros pasos

## Requisitos

- Compilador C++11 o superior (`g++`, `clang++`)
- SDL2 (`libsdl2-dev` en Linux)
- Para C puro: cualquier compilador C99 (`gcc`)

```bash
# Debian/Ubuntu
sudo apt install libsdl2-dev

# macOS (Homebrew)
brew install sdl2
```

---

## Estructura base de una aplicación C++

Todo programa comienza con una instancia de `Application`.  
Esta clase encapsula SDL2, la ventana, el renderer y el `UIContext`.

```cpp
#include "winxp_ui.hpp"
#include "window_manager.hpp"
using namespace WXUI;

int main() {
    // title, width, height, resizable, software_renderer
    Application app("Mi App", 800, 600, true, true);

    if (!app.ok()) return 1;   // fallo al inicializar SDL o ventana

    // Agregar widgets aquí...

    app.run(60);   // 60 FPS máximo; bloquea hasta que el usuario cierra
    return 0;
}
```

### Parámetros del constructor

| Parámetro | Tipo | Descripción |
|---|---|---|
| `title` | `string` | Título de la ventana |
| `width` | `int` | Ancho inicial en píxeles |
| `height` | `int` | Alto inicial en píxeles |
| `resizable` | `bool` | Permite redimensionar la ventana |
| `software` | `bool` | `true` = render por CPU (compatible). `false` = GPU (más rápido, requiere driver) |

---

## Estructura base en C

```c
#include "wxui_c.h"

int main(void) {
    WXApp* app = wxapp_create("Mi App", 800, 600, 1, 1);
    if (!app) return 1;

    // Agregar widgets aquí...

    wxapp_run(app, 60);
    wxapp_destroy(app);
    return 0;
}
```

> **Nota de compilación:** El enlace final siempre debe hacerse con `g++`, incluso si tu código es C puro, porque `wxui_c.o` contiene runtime C++.

---

## El primer widget: un botón con acción

### C++

```cpp
#include "winxp_ui.hpp"
#include "window_manager.hpp"
using namespace WXUI;

int main() {
    Application app("Demo Botón", 400, 200);
    if (!app.ok()) return 1;

    int clicks = 0;

    // Crear un label para mostrar el contador
    auto* lbl = static_cast<UILabel*>(
        app.add(Make::Label("lbl", Rect(150, 80, 100, 20), "Clicks: 0"))
    );

    // Crear el botón
    auto* btn = static_cast<UIButton*>(
        app.add(Make::Button("btn", Rect(150, 110, 100, 30), "Click!"))
    );

    // Conectar el evento Click
    btn->on(EventType::Click, [&](UIComponent*, const UIEvent&) {
        ++clicks;
        lbl->setText("Clicks: " + std::to_string(clicks));
    });

    app.run(60);
    return 0;
}
```

### C

```c
#include "wxui_c.h"
#include <stdio.h>

typedef struct { WXWidget* lbl; int clicks; } State;

static void on_click(WXWidget* w, const WXEvent* e, void* ud) {
    State* s = (State*)ud;
    char buf[32];
    s->clicks++;
    snprintf(buf, sizeof(buf), "Clicks: %d", s->clicks);
    wxwidget_set_text(s->lbl, buf);
    (void)w; (void)e;
}

int main(void) {
    WXApp* app = wxapp_create("Demo Botón", 400, 200, 1, 1);
    if (!app) return 1;

    State s = { NULL, 0 };
    s.lbl = wxmake_label(app, "lbl", 150, 80, 100, 20, "Clicks: 0", 0);

    WXWidget* btn = wxmake_button(app, "btn", 150, 110, 100, 30, "Click!");
    wxwidget_on(btn, WXEV_CLICK, on_click, &s);

    wxapp_run(app, 60);
    wxapp_destroy(app);
    return 0;
}
```

---

## El sistema de coordenadas y Rect

```
(0,0) ─────────────────────────► X
  │
  │   Rect(x=50, y=30, w=120, h=40)
  │   ┌──────────────────┐  ← y=30
  │   │                  │
  │   └──────────────────┘  ← y=70
  │   ↑ x=50           ↑ x=170
  ▼
  Y
```

```cpp
Rect(50, 30, 120, 40)   // x=50, y=30, ancho=120, alto=40
```

---

## Callbacks del bucle principal

`Application` expone tres callbacks opcionales que se registran **antes** de llamar a `run()`:

```cpp
// Evento SDL crudo — intercepta antes del UIContext
app.onEvent([](SDL_Event& ev, bool& running) {
    if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
        running = false;
});

// Una vez por frame, antes del render
app.onFrame([&]() {
    // Actualiza animaciones, simulación, etc.
});

// Cuando el usuario redimensiona la ventana
app.onResize([&](int nw, int nh) {
    // Relayout: actualiza posiciones/tamaños de widgets
    app.setWidgetRect("panel_fondo", Rect(0, 0, nw, nh));
});
```

---

## Helpers de Application

```cpp
app.ctx()          // UIContext& — acceso completo al contexto
app.window()       // SDL_Window* — ventana nativa
app.renderer()     // SDL_Renderer* — renderer nativo
app.width()        // int — ancho actual
app.height()       // int — alto actual
app.ok()           // bool — inicialización correcta

app.add(widget)    // agrega widget al contexto, devuelve UIComponent*
app.find("id")     // busca widget por id, devuelve UIComponent* o null

app.setWidgetRect("id", Rect(...))  // reposiciona widget por id
app.setTitle("Nuevo título")        // cambia el título de la ventana
app.invalidate()                    // fuerza redibujado en el próximo frame
```

---

## Capas (layers)

Los widgets se renderizan en orden de capa ascendente.  
Por defecto todos están en capa `0`. Los menús desplegables usan capa `100`, las ventanas flotantes capa `10`.

```cpp
auto* panel = static_cast<UIPanel*>(app.add(Make::Panel("p", Rect(...))));
panel->layer = 5;   // se renderiza encima de los widgets en capa 0
```

En C no hay acceso directo a `layer` vía la API pública; usa el sistema para los casos comunes (UIWindow ya maneja sus propias capas).

---

## Manejo de errores básico

```cpp
Application app("Test", 800, 600);
if (!app.ok()) {
    // SDL_Log ya habrá impreso el error en stderr
    return 1;
}

// Los widgets devuelven nullptr si app.ok() == false
auto* w = app.add(Make::Button("b", Rect(10,10,80,24), "OK"));
if (!w) { /* no debería ocurrir si app.ok() == true */ }
```

---

## Compilación manual (sin compile.sh)

```bash
# Solo C++ — un paso
g++ -std=c++11 mi_app.cpp -o mi_app -I./include $(sdl2-config --cflags --libs)

# Con wrapper C — tres pasos
g++ -std=c++11 -c src/wxui_c.cpp    -o wxui_c.o    -I./include $(sdl2-config --cflags)
gcc -std=c99   -c mi_app.c          -o mi_app.o    -I./include $(sdl2-config --cflags)
g++               wxui_c.o mi_app.o -o mi_app               $(sdl2-config --libs)
```