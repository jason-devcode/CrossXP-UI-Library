# 04 — Framebuffer personalizado y render 3D

`UIViewport3D` es un widget que acepta un buffer de píxeles ARGB que tú generas —  
ya sea un render 3D por software, un editor de imágenes, un emulador, o cualquier  
cosa que produzca píxeles.

---

## Formato del framebuffer

```
uint32_t pixel = 0xAARRGGBB
                   ││ ││ ││
                   ││ ││ └┴── Azul   (0x00–0xFF)
                   ││ └┴───── Verde  (0x00–0xFF)
                   └┴──────── Rojo   (0x00–0xFF)
                              Alpha (ignorado para composición)
```

```cpp
uint32_t rojo    = 0xFF'FF'00'00;
uint32_t verde   = 0xFF'00'FF'00;
uint32_t blanco  = 0xFF'FF'FF'FF;
uint32_t negro   = 0xFF'00'00'00;
```

---

## Caso de uso básico: pintar píxeles

### C++

```cpp
#include "winxp_ui.hpp"
#include "window_manager.hpp"
using namespace WXUI;

int main() {
    Application app("Pixel Painter", 640, 480);
    if (!app.ok()) return 1;

    const int VW = 600, VH = 420;

    // Crear el viewport
    auto* vp = static_cast<UIViewport3D*>(
        app.add(Make::Viewport("canvas", Rect(20, 30, VW, VH), "Canvas"))
    );

    // Buffer de píxeles
    std::vector<uint32_t> fb(VW * VH, 0xFF202020);  // fondo gris oscuro

    // Dibujar un gradiente
    for (int y = 0; y < VH; y++) {
        for (int x = 0; x < VW; x++) {
            uint8_t r = (uint8_t)(255.f * x / VW);
            uint8_t g = (uint8_t)(255.f * y / VH);
            uint8_t b = 128;
            fb[y * VW + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    // Enviar al viewport (se copia internamente)
    vp->updatePixels(fb.data(), VW, VH);

    app.run(60);
}
```

### C

```c
#include "wxui_c.h"
// El viewport 3D no está expuesto en la API C pública.
// Para renders simples usa la API C++ desde wxui_c.cpp,
// o añade una función especializada al wrapper.
// Ver sección "Extender el wrapper C" más abajo.
```

---

## Render 3D: patrón completo

El patrón estándar separa **actualización**, **render** y **envío al viewport**:

```cpp
#include "winxp_ui.hpp"
#include "window_manager.hpp"
#include <cmath>
using namespace WXUI;

// ── Cámara simple ──────────────────────────────────────────────────────
struct Camera {
    float azimuth  = 0.4f;
    float elevation= 0.35f;
    float zoom     = 1.f;
    float panX = 0, panY = 0;
};

// ── Utilidades de píxeles ──────────────────────────────────────────────
static void setPixel(uint32_t* fb, int w, int h, int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    fb[y * w + x] = color;
}

static void drawLine(uint32_t* fb, int w, int h,
                     int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = abs(x1-x0), dy = abs(y1-y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (true) {
        setPixel(fb, w, h, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// ── Proyección ─────────────────────────────────────────────────────────
static void project(float wx, float wy, float wz,
                    const Camera& cam, int vpW, int vpH,
                    int& sx, int& sy)
{
    // Rotación de cámara
    float rx =  wx * cosf(cam.azimuth) + wz * sinf(cam.azimuth);
    float ry =  wy;
    float rz = -wx * sinf(cam.azimuth) + wz * cosf(cam.azimuth);

    float ox =  rx;
    float oy =  ry * cosf(cam.elevation) - rz * sinf(cam.elevation);
    float oz =  ry * sinf(cam.elevation) + rz * cosf(cam.elevation);

    // Perspectiva
    float camDist = 300.f * cam.zoom;
    float dz = oz + camDist;
    if (dz < 0.01f) dz = 0.01f;
    float f = camDist / dz;

    sx = (int)(vpW * 0.5f + cam.panX + ox * f);
    sy = (int)(vpH * 0.5f + cam.panY - oy * f);
}

// ── Render de escena ───────────────────────────────────────────────────
static void renderFrame(uint32_t* fb, int w, int h, const Camera& cam) {
    // Limpiar
    for (int i = 0; i < w * h; i++) fb[i] = 0xFF303030;

    // Rejilla de suelo
    for (int i = -5; i <= 5; i++) {
        int ax, ay, bx, by;
        project((float)i * 40, -40, -200, cam, w, h, ax, ay);
        project((float)i * 40, -40,  200, cam, w, h, bx, by);
        drawLine(fb, w, h, ax, ay, bx, by, 0xFF505050);

        project(-200, -40, (float)i * 40, cam, w, h, ax, ay);
        project( 200, -40, (float)i * 40, cam, w, h, bx, by);
        drawLine(fb, w, h, ax, ay, bx, by, 0xFF505050);
    }

    // Cubo simple (8 vértices, 12 aristas)
    float v[8][3] = {
        {-50,-50,-50},{50,-50,-50},{50,50,-50},{-50,50,-50},
        {-50,-50, 50},{50,-50, 50},{50,50, 50},{-50,50, 50}
    };
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},  // cara frontal
        {4,5},{5,6},{6,7},{7,4},  // cara trasera
        {0,4},{1,5},{2,6},{3,7}   // laterales
    };
    for (auto& e : edges) {
        int ax, ay, bx, by;
        project(v[e[0]][0], v[e[0]][1], v[e[0]][2], cam, w, h, ax, ay);
        project(v[e[1]][0], v[e[1]][1], v[e[1]][2], cam, w, h, bx, by);
        drawLine(fb, w, h, ax, ay, bx, by, 0xFF00C8FF);
    }
}

// ── main ───────────────────────────────────────────────────────────────
int main() {
    Application app("Viewer 3D", 800, 600);
    if (!app.ok()) return 1;

    const int VW = 800, VH = 540;

    auto* vp = static_cast<UIViewport3D*>(
        app.add(Make::Viewport("vp3d", Rect(0, 0, VW, VH), "Perspective"))
    );

    std::vector<uint32_t> fb(VW * VH);
    Camera cam;
    bool dirty3d = true;

    // Arrastrar para orbitar
    struct Drag { bool active = false; int lastX = 0, lastY = 0; } drag;

    app.onEvent([&](SDL_Event& ev, bool& running) {
        if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
            if (ev.button.x < VW && ev.button.y < VH) {
                drag = { true, ev.button.x, ev.button.y };
            }
        }
        if (ev.type == SDL_MOUSEBUTTONUP) drag.active = false;

        if (ev.type == SDL_MOUSEMOTION && drag.active) {
            cam.azimuth   += (ev.motion.x - drag.lastX) * 0.008f;
            cam.elevation += (ev.motion.y - drag.lastY) * 0.008f;
            cam.elevation  = std::max(-1.4f, std::min(1.4f, cam.elevation));
            drag.lastX = ev.motion.x;
            drag.lastY = ev.motion.y;
            dirty3d = true;
        }
        if (ev.type == SDL_MOUSEWHEEL) {
            cam.zoom *= (ev.wheel.y > 0) ? 1.15f : (1.f / 1.15f);
            cam.zoom  = std::max(0.1f, std::min(10.f, cam.zoom));
            dirty3d   = true;
        }
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
            running = false;
    });

    app.onFrame([&]() {
        if (!dirty3d) return;
        renderFrame(fb.data(), VW, VH, cam);
        vp->updatePixels(fb.data(), VW, VH);
        dirty3d = false;
    });

    app.run(60);
}
```

---

## Multi-viewport (4 vistas estilo 3ds Max)

```cpp
const int VW = 800, VH = 600;
const int PANEL_W = 200;
int vpW = VW - PANEL_W, vpH = VH;

// Calcular los 4 rectángulos
auto computeRects = [&](Rect out[4]) {
    int hw = vpW / 2, hh = vpH / 2;
    out[0] = Rect(0,  0,  hw, hh);   // Perspectiva
    out[1] = Rect(hw, 0,  hw, hh);   // Top
    out[2] = Rect(0,  hh, hw, hh);   // Front
    out[3] = Rect(hw, hh, hw, hh);   // Left
};

Rect rects[4];
computeRects(rects);

UIViewport3D* vps[4];
SDL_Surface*  vpSurfs[4] = {};
bool          vpDirty[4] = {true, true, true, true};

const char* labels[] = {"Perspective", "Top", "Front", "Left"};

for (int i = 0; i < 4; i++) {
    vps[i] = static_cast<UIViewport3D*>(
        app.add(Make::Viewport("vp" + std::to_string(i), rects[i], labels[i]))
    );
}

// En onFrame, renderizar solo los viewports dirty
app.onFrame([&]() {
    for (int i = 0; i < 4; i++) {
        if (!vpDirty[i]) continue;
        int w = vps[i]->rect.w, h = vps[i]->rect.h;
        if (w < 1 || h < 1) continue;

        // Crear/recrear surface si cambió de tamaño
        if (!vpSurfs[i] || vpSurfs[i]->w != w || vpSurfs[i]->h != h) {
            if (vpSurfs[i]) SDL_FreeSurface(vpSurfs[i]);
            vpSurfs[i] = SDL_CreateRGBSurface(
                0, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
            );
        }

        // Renderizar en la surface
        SDL_LockSurface(vpSurfs[i]);
        renderViewport(static_cast<uint32_t*>(vpSurfs[i]->pixels), w, h, i, scene);
        SDL_UnlockSurface(vpSurfs[i]);

        // Copiar al viewport
        std::vector<uint32_t> tmp(w * h);
        memcpy(tmp.data(), vpSurfs[i]->pixels, w * h * 4);
        vps[i]->updatePixels(tmp.data(), w, h);

        vpDirty[i] = false;
    }
});

// Limpiar al final
for (int i = 0; i < 4; i++)
    if (vpSurfs[i]) SDL_FreeSurface(vpSurfs[i]);
```

---

## Viewport maximizable (doble clic para expandir)

```cpp
int maximized = -1;   // -1 = modo cuadrícula, 0-3 = viewport maximizado

for (int i = 0; i < 4; i++) {
    int idx = i;
    vps[i]->on(EventType::DblClick, [&, idx](UIComponent*, const UIEvent&) {
        if (maximized == idx) {
            maximized = -1;   // volver a cuadrícula
        } else {
            maximized = idx;  // maximizar este viewport
        }

        Rect full(0, 0, vpW, vpH);
        Rect rects4[4];
        computeRects(rects4);

        for (int j = 0; j < 4; j++) {
            if (maximized < 0) {
                vps[j]->setRect(rects4[j]);
                vps[j]->setVisible(true);
            } else {
                vps[j]->setRect(full);
                vps[j]->setVisible(j == maximized);
            }
            vpDirty[j] = true;
        }
        app.invalidate();
    });
}
```

---

## Redimensionar viewports al cambiar el tamaño de ventana

```cpp
app.onResize([&](int nw, int nh) {
    vpW = nw - PANEL_W;
    vpH = nh;

    Rect rects4[4];
    computeRects(rects4);

    for (int i = 0; i < 4; i++) {
        vps[i]->setRect(rects4[i]);
        // Las surfaces se recrean automáticamente en onFrame
        if (vpSurfs[i]) { SDL_FreeSurface(vpSurfs[i]); vpSurfs[i] = nullptr; }
        vpDirty[i] = true;
    }
    app.invalidate();
});
```

---

## Dibujar con Draw:: sobre un UIComponent personalizado

Si necesitas un widget completamente personalizado (ej. un editor de sprites),  
hereda de `UIComponent` y sobreescribe `onDraw`:

```cpp
class SpriteEditor : public UIComponent {
public:
    std::vector<uint32_t> pixels;
    int sprW = 16, sprH = 16;
    int zoom  = 8;

    SpriteEditor(const std::string& id, Rect r)
        : UIComponent(id, r), pixels(sprW * sprH, 0xFF000000) {}

    void onDraw() override {
        // Fondo
        Draw::fillRect(surf, 0, 0, rect.w, rect.h, Pal::DARK_PANEL);

        // Dibujar cada píxel del sprite ampliado
        for (int y = 0; y < sprH; y++) {
            for (int x = 0; x < sprW; x++) {
                uint32_t c = pixels[y * sprW + x];
                uint8_t r = (c >> 16) & 0xFF;
                uint8_t g = (c >>  8) & 0xFF;
                uint8_t b =  c        & 0xFF;
                Draw::fillRect(surf, x*zoom, y*zoom, zoom, zoom, Color(r, g, b));
            }
        }

        // Rejilla
        for (int x = 0; x <= sprW; x++)
            Draw::drawVLine(surf, x*zoom, 0, sprH*zoom, Pal::DARK_BORDER);
        for (int y = 0; y <= sprH; y++)
            Draw::drawHLine(surf, 0, y*zoom, sprW*zoom, Pal::DARK_BORDER);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if (e.type == EventType::MouseDown || e.type == EventType::MouseMove) {
            if (e.mbtn == SDL_BUTTON_LEFT || e.type == EventType::MouseMove) {
                int px = (e.mx - rect.x) / zoom;
                int py = (e.my - rect.y) / zoom;
                if (px >= 0 && py >= 0 && px < sprW && py < sprH) {
                    pixels[py * sprW + px] = 0xFFFF0000;  // pintar rojo
                    markDirty();
                }
            }
        }
        return true;
    }
};

// Uso:
auto editor = std::unique_ptr<SpriteEditor>(
    new SpriteEditor("sprite_ed", Rect(10, 10, 128, 128))
);
app.add(std::move(editor));
```

---

## Rendimiento: solo re-renderizar cuando sea necesario

```cpp
bool escenaSucia = true;

// Marcar dirty solo cuando algo cambia
btn->on(EventType::Click, [&](UIComponent*, const UIEvent&) {
    // modificar escena...
    escenaSucia = true;
});

app.onFrame([&]() {
    if (!escenaSucia) return;   // sin cambios = no gastar CPU
    renderFrame(fb.data(), VW, VH, cam);
    vp->updatePixels(fb.data(), VW, VH);
    escenaSucia = false;
});
```

Con un render 3D complejo, esto puede bajar el uso de CPU de ~100% a <5% cuando la escena está estática.

---

## Extender el wrapper C para UIViewport3D

Si necesitas usar viewports desde C, añade esto a `wxui_c.h` y `wxui_c.cpp`:

**wxui_c.h:**
```c
/* Viewport (solo disponible en C++ internamente; expuesto aquí para C) */
WXWidget* wxmake_viewport(WXApp* app, const char* id,
                           int x, int y, int w, int h,
                           const char* label);

void wxviewport_update_pixels(WXWidget* vp,
                               const uint32_t* argb_data,
                               int w, int h);

void wxviewport_set_active(WXWidget* vp, int active);
int  wxviewport_is_active(const WXWidget* vp);
```

**wxui_c.cpp:**
```cpp
WXWidget* wxmake_viewport(WXApp* app, const char* id,
                           int x, int y, int w, int h, const char* label) {
    MAKE_WIDGET(WXUI::Make::Viewport(id, WXUI::Rect(x,y,w,h), label ? label : ""))
}

void wxviewport_update_pixels(WXWidget* vp, const uint32_t* data, int w, int h) {
    if (!vp || !vp->ptr) return;
    if (auto* v = dynamic_cast<WXUI::UIViewport3D*>(vp->ptr))
        v->updatePixels(data, w, h);
}

void wxviewport_set_active(WXWidget* vp, int active) {
    if (!vp || !vp->ptr) return;
    if (auto* v = dynamic_cast<WXUI::UIViewport3D*>(vp->ptr))
        v->setActive(active != 0);
}

int wxviewport_is_active(const WXWidget* vp) {
    if (!vp || !vp->ptr) return 0;
    if (auto* v = dynamic_cast<WXUI::UIViewport3D*>(vp->ptr))
        return v->isActive() ? 1 : 0;
    return 0;
}
```