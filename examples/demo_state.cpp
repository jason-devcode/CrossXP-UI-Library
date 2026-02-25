/*
 * main_simple.cpp  –  Ejemplo de uso de window_manager.hpp
 *
 * Una ventana WinXP-style con:
 *   - Label de título
 *   - Botón "Incrementar" y "Decrementar"
 *   - Label que muestra el contador actual
 *   - Slider sincronizado con el contador
 *   - Status bar en la parte inferior
 *
 * Build (Linux):
 *   g++ -std=c++11 main_simple.cpp -o simple $(sdl2-config --cflags --libs)
 */

#include "winxp_ui.hpp"
#include "window_manager.hpp"

using namespace WXUI;

int main(int, char**) {

    // ── 1. Crear la aplicación ─────────────────────────────────────────────
    Application app("Contador  – WinXP UI", 400, 280);
    if (!app.ok()) return 1;

    UIContext& ctx = app.ctx();

    // ── 2. Construir la UI ─────────────────────────────────────────────────
    const int PAD = 10;
    const int W   = app.width();

    // Panel de fondo
    auto panel = Make::Panel("bg", Rect(0, 0, W, app.height()));
    app.add(std::move(panel));

    // Título
    auto title = Make::Label("lbl_title", Rect(PAD, 12, W - PAD*2, 20),
                             "  Contador de ejemplo");
    app.add(std::move(title));

    // Separador horizontal
    auto sep = Make::Separator("sep1", Rect(PAD, 38, W - PAD*2, 4), true);
    app.add(std::move(sep));

    // Label "Valor:"
    auto lblVal = Make::Label("lbl_val", Rect(PAD, 58, 60, 20), "Valor:");
    app.add(std::move(lblVal));

    // Label que muestra el número
    auto lblNum = Make::Label("lbl_num", Rect(80, 58, 80, 20), "0");
    auto* pLblNum = static_cast<UILabel*>(app.add(std::move(lblNum)));

    // Botones
    auto btnInc = Make::Button("btn_inc", Rect(PAD,       90, 90, 26), "Incrementar");
    auto btnDec = Make::Button("btn_dec", Rect(PAD + 100, 90, 90, 26), "Decrementar");
    auto btnRst = Make::Button("btn_rst", Rect(PAD + 200, 90, 80, 26), "Reiniciar");

    auto* pBtnInc = static_cast<UIButton*>(app.add(std::move(btnInc)));
    auto* pBtnDec = static_cast<UIButton*>(app.add(std::move(btnDec)));
    auto* pBtnRst = static_cast<UIButton*>(app.add(std::move(btnRst)));

    // Slider sincronizado (rango 0..100)
    auto slider = Make::Slider("sld", Rect(PAD, 130, W - PAD*2, 16), 0.f, 100.f, 0.f);
    auto* pSld  = static_cast<UISlider*>(app.add(std::move(slider)));

    // Spinner de edición directa
    auto spinner = Make::Spinner("spn", Rect(PAD, 158, 100, 22), 0.f, 100.f, 0.f, 1.f);
    auto* pSpn   = static_cast<UISpinner*>(app.add(std::move(spinner)));

    // Checkbox "Solo pares"
    auto chk = Make::Checkbox("chk_par", Rect(PAD, 192, 160, 18), "Solo valores pares", false);
    auto* pChk = static_cast<UICheckbox*>(app.add(std::move(chk)));

    // Status bar
    auto sbarPanel = Make::Panel("sbar", Rect(0, app.height() - 24, W, 24));
    static_cast<UIPanel*>(sbarPanel.get())->raised = false;
    app.add(std::move(sbarPanel));

    auto lblStatus = Make::Label("lbl_st", Rect(6, app.height() - 22, W - 12, 20),
                                 "Listo");
    auto* pStatus = static_cast<UILabel*>(app.add(std::move(lblStatus)));

    // ── 3. Estado de la aplicación ─────────────────────────────────────────
    int counter = 0;

    // Sincroniza todos los widgets con el valor actual
    auto syncAll = [&](const std::string& msg = "") {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", counter);
        pLblNum->setText(buf);
        pSld->setValue((float)counter);
        pSld->markDirty();
        pSpn->setValue((float)counter);
        pSpn->markDirty();
        pBtnDec->setEnabled(counter > 0);
        pBtnInc->setEnabled(counter < 100);
        if (!msg.empty()) pStatus->setText(msg);
        ctx.needsRedraw = true;
    };

    // ── 4. Conectar eventos ────────────────────────────────────────────────
    pBtnInc->on(EventType::Click, [&](UIComponent*, const UIEvent&) {
        int step = pChk->isChecked() ? 2 : 1;
        counter = std::min(100, counter + step);
        syncAll("Incrementado a " + std::to_string(counter));
    });

    pBtnDec->on(EventType::Click, [&](UIComponent*, const UIEvent&) {
        int step = pChk->isChecked() ? 2 : 1;
        counter = std::max(0, counter - step);
        syncAll("Decrementado a " + std::to_string(counter));
    });

    pBtnRst->on(EventType::Click, [&](UIComponent*, const UIEvent&) {
        counter = 0;
        syncAll("Contador reiniciado");
    });

    pSld->on(EventType::ValueChanged, [&](UIComponent*, const UIEvent& e) {
        counter = (int)e.fvalue;
        if (pChk->isChecked() && counter % 2 != 0) counter &= ~1;
        syncAll();
    });

    pSpn->on(EventType::ValueChanged, [&](UIComponent*, const UIEvent& e) {
        counter = (int)e.fvalue;
        if (pChk->isChecked() && counter % 2 != 0) counter &= ~1;
        syncAll();
    });

    // ── 5. Callback de teclado global ─────────────────────────────────────
    app.onEvent([&](SDL_Event& ev, bool& running) {
        if (ev.type == SDL_KEYDOWN) {
            if (ev.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            else if (ev.key.keysym.sym == SDLK_UP)
                { UIEvent ue; ue.type = EventType::Click; pBtnInc->emit(ue); }
            else if (ev.key.keysym.sym == SDLK_DOWN)
                { UIEvent ue; ue.type = EventType::Click; pBtnDec->emit(ue); }
            else if (ev.key.keysym.sym == SDLK_r)
                { UIEvent ue; ue.type = EventType::Click; pBtnRst->emit(ue); }
        }
    });

    // ── 6. Relayout al redimensionar ──────────────────────────────────────
    app.onResize([&](int nw, int nh) {
        app.setWidgetRect("bg",       Rect(0, 0, nw, nh));
        app.setWidgetRect("sep1",     Rect(PAD, 38, nw - PAD*2, 4));
        app.setWidgetRect("sld",      Rect(PAD, 130, nw - PAD*2, 16));
        app.setWidgetRect("sbar",     Rect(0, nh - 24, nw, 24));
        app.setWidgetRect("lbl_st",   Rect(6, nh - 22, nw - 12, 20));
    });

    // Estado inicial
    syncAll("Use flechas ↑↓ o botones   |   R = reiniciar   |   ESC = salir");

    // ── 7. Arrancar el bucle ───────────────────────────────────────────────
    app.run(60);

    return 0;
}