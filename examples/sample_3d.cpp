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
