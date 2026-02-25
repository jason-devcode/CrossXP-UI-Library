#pragma once
/*
 * window_manager.hpp  –  High-level application wrapper for winxp_ui.hpp
 *
 * Hides all SDL2 boilerplate (init, window, renderer, loop, cleanup)
 * y expone una API limpia orientada a aplicaciones WXUI.
 *
 * Uso básico:
 *
 *   WXUI::Application app("Mi App", 800, 600);
 *   // ... agregar widgets a app.ctx() ...
 *   app.run();
 *
 * Con callbacks de evento y frame:
 *
 *   app.onEvent([](SDL_Event& ev, bool& running) {
 *       if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
 *           running = false;
 *   });
 *   app.onFrame([] {
 *       // lógica por frame (antes del render)
 *   });
 *   app.onResize([](int w, int h) {
 *       // relayout
 *   });
 *   app.run();
 *
 * Build (Linux):
 *   g++ -std=c++11 main.cpp -o demo $(sdl2-config --cflags --libs)
 */

#include "winxp_ui.hpp"
#include <functional>
#include <string>

namespace WXUI {

// ═══════════════════════════════════════════════════════════════════════════
//  Application  –  ventana + renderer + UIContext en un solo objeto
// ═══════════════════════════════════════════════════════════════════════════
class Application {
public:
    // ── Constructor / Destructor ─────────────────────────────────────────
    Application(const std::string& title,
                int width  = 800,
                int height = 600,
                bool resizable = true,
                bool software = true)
        : m_title(title), m_w(width), m_h(height)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            SDL_Log("SDL_Init error: %s", SDL_GetError());
            m_ok = false;
            return;
        }

        Uint32 flags = 0;
        if (resizable) flags |= SDL_WINDOW_RESIZABLE;

        m_win = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            flags
        );
        if (!m_win) {
            SDL_Log("SDL_CreateWindow error: %s", SDL_GetError());
            m_ok = false;
            return;
        }

        Uint32 renFlags = software ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED;
        m_ren = SDL_CreateRenderer(m_win, -1, renFlags);
        if (!m_ren) {
            SDL_Log("SDL_CreateRenderer error: %s", SDL_GetError());
            m_ok = false;
            return;
        }

        if (!m_ctx.init(m_ren, m_w, m_h)) {
            SDL_Log("UIContext::init error");
            m_ok = false;
            return;
        }

        m_ok = true;
    }

    ~Application() {
        m_ctx.destroy();
        if (m_ren) SDL_DestroyRenderer(m_ren);
        if (m_win) SDL_DestroyWindow(m_win);
        SDL_Quit();
    }

    // Disable copy
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // ── Accessors ────────────────────────────────────────────────────────
    UIContext&    ctx()      { return m_ctx; }
    SDL_Window*   window()   { return m_win; }
    SDL_Renderer* renderer() { return m_ren; }
    int           width()  const { return m_w; }
    int           height() const { return m_h; }
    bool          ok()     const { return m_ok; }

    // ── Shortcut: add widget directly ───────────────────────────────────
    UIComponent* add(std::unique_ptr<UIComponent> c) {
        return m_ctx.add(std::move(c));
    }

    UIComponent* find(const std::string& id) {
        return m_ctx.findById(id);
    }

    // ── Callbacks ────────────────────────────────────────────────────────
    // Se llama UNA VEZ por evento SDL crudo (antes de procesarlo el UIContext).
    // Pon `running = false` para salir.
    using EventCB  = std::function<void(SDL_Event&, bool&)>;
    // Se llama al inicio de cada frame (antes del render).
    using FrameCB  = std::function<void()>;
    // Se llama cuando la ventana cambia de tamaño; recibe nuevo (w, h).
    using ResizeCB = std::function<void(int, int)>;

    Application& onEvent (EventCB  cb) { m_onEvent  = std::move(cb); return *this; }
    Application& onFrame (FrameCB  cb) { m_onFrame  = std::move(cb); return *this; }
    Application& onResize(ResizeCB cb) { m_onResize = std::move(cb); return *this; }

    // ── run() ────────────────────────────────────────────────────────────
    // Inicia el bucle principal; regresa cuando el usuario cierra la ventana
    // o algún callback pone running = false.
    // `targetFPS` controla la velocidad del bucle (0 = sin límite).
    void run(int targetFPS = 60) {
        if (!m_ok) return;

        const Uint32 frameMs = (targetFPS > 0) ? (1000u / (Uint32)targetFPS) : 0;
        bool running = true;

        while (running) {
            Uint32 frameStart = SDL_GetTicks();

            // ── Procesar eventos ────────────────────────────────────────
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {

                if (ev.type == SDL_QUIT) {
                    running = false;
                    break;
                }

                // Resize nativo
                if (ev.type == SDL_WINDOWEVENT &&
                    (ev.window.event == SDL_WINDOWEVENT_RESIZED ||
                     ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
                {
                    m_w = ev.window.data1;
                    m_h = ev.window.data2;
                    m_ctx.resize(m_w, m_h);
                    if (m_onResize) m_onResize(m_w, m_h);
                    continue;
                }

                // Callback de usuario (puede consumir el evento)
                if (m_onEvent) {
                    m_onEvent(ev, running);
                    if (!running) break;
                }

                // UIContext procesa el resto
                m_ctx.processEvent(ev);
            }

            if (!running) break;

            // ── Lógica por frame ────────────────────────────────────────
            if (m_onFrame) m_onFrame();

            // ── Render ─────────────────────────────────────────────────
            m_ctx.render();

            // ── Frame cap ──────────────────────────────────────────────
            if (frameMs > 0) {
                Uint32 elapsed = SDL_GetTicks() - frameStart;
                if (elapsed < frameMs)
                    SDL_Delay(frameMs - elapsed);
            }
        }
    }

    // ── Helpers de layout ────────────────────────────────────────────────

    // Rect que ocupa toda la ventana (útil como contenedor raíz)
    Rect fullRect() const { return Rect(0, 0, m_w, m_h); }

    // Ajusta el rect de un widget ya registrado al nuevo tamaño de ventana
    void setWidgetRect(const std::string& id, Rect r) {
        if (auto* c = m_ctx.findById(id)) c->setRect(r);
    }

    // Cambia el título de la ventana en tiempo de ejecución
    void setTitle(const std::string& t) { SDL_SetWindowTitle(m_win, t.c_str()); }

    // Fuerza un redibujado completo en el próximo frame
    void invalidate() { m_ctx.needsRedraw = true; }

private:
    std::string   m_title;
    int           m_w, m_h;
    bool          m_ok   = false;

    SDL_Window*   m_win  = nullptr;
    SDL_Renderer* m_ren  = nullptr;
    UIContext     m_ctx;

    EventCB       m_onEvent;
    FrameCB       m_onFrame;
    ResizeCB      m_onResize;
};

} // namespace WXUI