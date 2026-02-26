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
