# 05 — Comunicación entre componentes

Cuando la aplicación crece, los widgets necesitan comunicarse entre sí sin estar  
directamente acoplados. Esta guía cubre los patrones más comunes.

---

## Patrón 1: Referencia directa (el más simple)

Cuando los widgets están en el mismo scope, usa capturas en las lambdas.  
Es directo y adecuado para apps pequeñas/medianas.

```cpp
auto* slider = static_cast<UISlider*>(
    app.add(Make::Slider("sld", Rect(10, 50, 200, 16), 0.f, 100.f, 0.f))
);
auto* spinner = static_cast<UISpinner*>(
    app.add(Make::Spinner("spn", Rect(10, 80, 100, 22), 0.f, 100.f, 0.f, 1.f))
);
auto* lbl = static_cast<UILabel*>(
    app.add(Make::Label("lbl", Rect(10, 110, 200, 20), "0"))
);

// Slider → Spinner y Label
slider->on(EventType::ValueChanged, [spinner, lbl](UIComponent*, const UIEvent& e) {
    spinner->setValue(e.fvalue);
    spinner->markDirty();
    lbl->setText(std::to_string((int)e.fvalue));
});

// Spinner → Slider y Label
spinner->on(EventType::ValueChanged, [slider, lbl](UIComponent*, const UIEvent& e) {
    slider->setValue(e.fvalue);
    slider->markDirty();
    lbl->setText(std::to_string((int)e.fvalue));
});
```

---

## Patrón 2: Estado centralizado + función sync

Más robusto: el estado es la fuente de verdad única.  
Los widgets leen del estado; los callbacks escriben al estado y llaman `sync`.

```cpp
struct AppState {
    int    brillo    = 50;
    float  contraste = 1.0f;
    bool   escalaGris = false;
};

AppState estado;

auto* sldBrillo  = static_cast<UISlider*>(...);
auto* spnContr   = static_cast<UISpinner*>(...);
auto* chkGris    = static_cast<UICheckbox*>(...);
auto* lblEstado  = static_cast<UILabel*>(...);

// Función maestra de sincronización: estado → todos los widgets
auto syncUI = [&]() {
    sldBrillo->setValue((float)estado.brillo);
    sldBrillo->markDirty();
    spnContr->setValue(estado.contraste);
    spnContr->markDirty();
    chkGris->setChecked(estado.escalaGris);
    chkGris->markDirty();

    char buf[128];
    snprintf(buf, sizeof(buf), "Brillo: %d  Contraste: %.2f  Gris: %s",
             estado.brillo, estado.contraste,
             estado.escalaGris ? "si" : "no");
    lblEstado->setText(buf);
    app.invalidate();
};

// Los callbacks SOLO modifican el estado y llaman sync
sldBrillo->on(EventType::ValueChanged, [&](UIComponent*, const UIEvent& e) {
    estado.brillo = (int)e.fvalue;
    syncUI();
});
spnContr->on(EventType::ValueChanged, [&](UIComponent*, const UIEvent& e) {
    estado.contraste = e.fvalue;
    syncUI();
});
chkGris->on(EventType::CheckChanged, [&](UIComponent*, const UIEvent& e) {
    estado.escalaGris = (e.ivalue != 0);
    syncUI();
});

syncUI();  // estado inicial
```

---

## Patrón 3: Bus de eventos (pub/sub)

Desacopla emisores de receptores. Útil cuando los componentes no se conocen entre sí.

```cpp
// ── EventBus simple ────────────────────────────────────────────────────
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

struct AppEvent {
    std::string topic;
    float       fvalue = 0.f;
    int         ivalue = 0;
    std::string svalue;
};

class EventBus {
public:
    using Handler = std::function<void(const AppEvent&)>;

    void subscribe(const std::string& topic, Handler h) {
        handlers[topic].push_back(std::move(h));
    }

    void publish(const AppEvent& ev) {
        auto it = handlers.find(ev.topic);
        if (it != handlers.end())
            for (auto& h : it->second) h(ev);
    }

private:
    std::unordered_map<std::string, std::vector<Handler>> handlers;
};

// ── Uso ────────────────────────────────────────────────────────────────
EventBus bus;

// Componente A publica
slider->on(EventType::ValueChanged, [&](UIComponent*, const UIEvent& e) {
    bus.publish({"volumen_cambiado", e.fvalue});
});

// Componente B se suscribe (no necesita saber de A)
bus.subscribe("volumen_cambiado", [&](const AppEvent& ev) {
    spinner->setValue(ev.fvalue);
    spinner->markDirty();
    lblVol->setText("Vol: " + std::to_string((int)ev.fvalue));
});

// Componente C también se suscribe al mismo topic
bus.subscribe("volumen_cambiado", [&](const AppEvent& ev) {
    // actualizar render 3D, log, etc.
    escenaDirty = true;
});
```

---

## Patrón 4: Selección + panel de propiedades

El widget seleccionado determina qué controles se muestran/activan.

```cpp
struct Objeto {
    std::string nombre;
    float x, y, z;
    float escala;
    bool  visible;
};

std::vector<Objeto> objetos;
int idSeleccionado = -1;

auto* spnX     = static_cast<UISpinner*>(app.add(Make::Spinner("spn_x",  ...)));
auto* spnY     = static_cast<UISpinner*>(app.add(Make::Spinner("spn_y",  ...)));
auto* spnZ     = static_cast<UISpinner*>(app.add(Make::Spinner("spn_z",  ...)));
auto* spnEsc   = static_cast<UISpinner*>(app.add(Make::Spinner("spn_esc",...)));
auto* chkVis   = static_cast<UICheckbox*>(app.add(Make::Checkbox("chk_vis",...,"Visible",true)));

// Cargar propiedades del objeto seleccionado en el panel
auto cargarEnPanel = [&]() {
    bool haySeleccion = (idSeleccionado >= 0);
    spnX->setEnabled(haySeleccion);
    spnY->setEnabled(haySeleccion);
    spnZ->setEnabled(haySeleccion);
    spnEsc->setEnabled(haySeleccion);
    chkVis->setEnabled(haySeleccion);

    if (!haySeleccion) return;

    const Objeto& o = objetos[idSeleccionado];
    spnX->setValue(o.x);   spnX->markDirty();
    spnY->setValue(o.y);   spnY->markDirty();
    spnZ->setValue(o.z);   spnZ->markDirty();
    spnEsc->setValue(o.escala); spnEsc->markDirty();
    chkVis->setChecked(o.visible); chkVis->markDirty();
};

// Los cambios en el panel escriben al objeto seleccionado
auto panelAObjeto = [&]() {
    if (idSeleccionado < 0) return;
    Objeto& o = objetos[idSeleccionado];
    o.x      = spnX->getValue();
    o.y      = spnY->getValue();
    o.z      = spnZ->getValue();
    o.escala = spnEsc->getValue();
    o.visible= chkVis->isChecked();
    escenaDirty = true;
};

spnX->on(EventType::ValueChanged,   [&](UIComponent*, const UIEvent&) { panelAObjeto(); });
spnY->on(EventType::ValueChanged,   [&](UIComponent*, const UIEvent&) { panelAObjeto(); });
spnZ->on(EventType::ValueChanged,   [&](UIComponent*, const UIEvent&) { panelAObjeto(); });
spnEsc->on(EventType::ValueChanged, [&](UIComponent*, const UIEvent&) { panelAObjeto(); });
chkVis->on(EventType::CheckChanged, [&](UIComponent*, const UIEvent&) { panelAObjeto(); });

// Cambiar la selección
auto seleccionar = [&](int nuevoId) {
    idSeleccionado = nuevoId;
    cargarEnPanel();
    app.invalidate();
};
```

---

## Patrón 5: Comando (Command pattern) — con undo/redo

```cpp
struct Comando {
    virtual void ejecutar() = 0;
    virtual void deshacer() = 0;
    virtual ~Comando() {}
};

class CambiarValor : public Comando {
    UISpinner* widget;
    float      valorAntes, valorDespues;
public:
    CambiarValor(UISpinner* w, float antes, float despues)
        : widget(w), valorAntes(antes), valorDespues(despues) {}

    void ejecutar() override {
        widget->setValue(valorDespues);
        widget->markDirty();
    }
    void deshacer() override {
        widget->setValue(valorAntes);
        widget->markDirty();
    }
};

// Gestor de historial
struct Historial {
    std::vector<std::unique_ptr<Comando>> pila;
    int cursor = -1;

    void ejecutar(std::unique_ptr<Comando> cmd) {
        pila.resize(cursor + 1);   // borrar redo stack
        cmd->ejecutar();
        pila.push_back(std::move(cmd));
        cursor++;
    }
    bool undo() {
        if (cursor < 0) return false;
        pila[cursor]->deshacer();
        cursor--;
        return true;
    }
    bool redo() {
        if (cursor >= (int)pila.size() - 1) return false;
        cursor++;
        pila[cursor]->ejecutar();
        return true;
    }
};

Historial hist;

// Uso: capturar valor antes/después
float valorInicial = spnX->getValue();
spnX->on(EventType::ValueChanged, [&](UIComponent*, const UIEvent& e) {
    hist.ejecutar(std::make_unique<CambiarValor>(spnX, valorInicial, e.fvalue));
    valorInicial = e.fvalue;
    panelAObjeto();
});

// Ctrl+Z / Ctrl+Y desde el teclado global
app.onEvent([&](SDL_Event& ev, bool& running) {
    if (ev.type == SDL_KEYDOWN && (ev.key.keysym.mod & KMOD_CTRL)) {
        if (ev.key.keysym.sym == SDLK_z) hist.undo();
        if (ev.key.keysym.sym == SDLK_y) hist.redo();
    }
});
```

---

## Patrón en C: callbacks encadenados con contexto compartido

```c
typedef struct {
    WXApp*    app;
    WXWidget* sld;
    WXWidget* spn;
    WXWidget* lbl;
    float     valor;
} SyncState;

static void actualizar_ui(SyncState* s) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f", s->valor);
    wxwidget_set_text(s->lbl, buf);
    wxwidget_set_value(s->sld, s->valor);
    wxwidget_set_value(s->spn, s->valor);
    wxapp_invalidate(s->app);
}

static void on_sld(WXWidget* w, const WXEvent* e, void* ud) {
    (void)w;
    SyncState* s = (SyncState*)ud;
    s->valor = e->fvalue;
    /* actualizar solo el label y el spinner, no el slider de nuevo */
    char buf[32]; snprintf(buf, sizeof(buf), "%.0f", s->valor);
    wxwidget_set_text(s->lbl, buf);
    wxwidget_set_value(s->spn, s->valor);
    wxapp_invalidate(s->app);
}

static void on_spn(WXWidget* w, const WXEvent* e, void* ud) {
    (void)w;
    SyncState* s = (SyncState*)ud;
    s->valor = e->fvalue;
    char buf[32]; snprintf(buf, sizeof(buf), "%.0f", s->valor);
    wxwidget_set_text(s->lbl, buf);
    wxwidget_set_value(s->sld, s->valor);
    wxapp_invalidate(s->app);
}

int main(void) {
    WXApp* app = wxapp_create("Sync demo", 400, 200, 1, 1);

    SyncState s;
    s.app   = app;
    s.valor = 50.f;
    s.sld   = wxmake_slider (app, "sld", 10, 10, 300, 16, 0.f, 100.f, 50.f);
    s.spn   = wxmake_spinner(app, "spn", 10, 40, 100, 22, 0.f, 100.f, 50.f, 1.f);
    s.lbl   = wxmake_label  (app, "lbl", 120, 44, 60, 18, "50", 0);

    wxwidget_on(s.sld, WXEV_VALUE_CHANGED, on_sld, &s);
    wxwidget_on(s.spn, WXEV_VALUE_CHANGED, on_spn, &s);

    wxapp_run(app, 60);
    wxapp_destroy(app);
    return 0;
}
```

---

## Visibilidad condicional de secciones de UI

```cpp
// Mostrar/ocultar un grupo de controles según una selección
auto mostrarSeccion = [&](const std::string& seccion) {
    std::vector<std::string> secciones = {"grp_esfera", "grp_caja", "grp_cilindro"};
    for (auto& id : secciones) {
        if (auto* w = app.find(id))
            w->setVisible(id == seccion);
    }
    app.invalidate();
};

// Radio buttons para seleccionar tipo
auto* rEsfera = static_cast<UIRadioButton*>(
    app.add(Make::Radio("r_esf", Rect(10,10,80,18), "Esfera", "tipo", true))
);
auto* rCaja   = static_cast<UIRadioButton*>(
    app.add(Make::Radio("r_caja", Rect(10,30,80,18), "Caja", "tipo", false))
);

rEsfera->on(EventType::CheckChanged, [&](UIComponent*, const UIEvent& e) {
    if (e.ivalue) mostrarSeccion("grp_esfera");
});
rCaja->on(EventType::CheckChanged, [&](UIComponent*, const UIEvent& e) {
    if (e.ivalue) mostrarSeccion("grp_caja");
});

mostrarSeccion("grp_esfera");  // estado inicial
```

---

## Comunicación viewport ↔ panel de propiedades

El viewport actualiza el estado al interactuar; el panel refleja ese estado.

```cpp
struct Seleccion {
    int   idObjeto = -1;
    float posX = 0, posY = 0, posZ = 0;
};

Seleccion sel;

// El viewport detecta clics → actualiza selección → el panel responde
vp->on(EventType::MouseDown, [&](UIComponent*, const UIEvent& e) {
    // Lógica de picking (ray casting, etc.)
    int nuevoId = pickObjectAt(e.mx - vp->rect.x, e.my - vp->rect.y);
    if (nuevoId != sel.idObjeto) {
        sel.idObjeto = nuevoId;
        if (nuevoId >= 0) {
            sel.posX = objetos[nuevoId].x;
            sel.posY = objetos[nuevoId].y;
            sel.posZ = objetos[nuevoId].z;
        }
        cargarEnPanel();    // panel se actualiza automáticamente
        app.invalidate();
    }
});

// Al mover en el viewport (drag) → actualiza posición → panel sigue
vp->on(EventType::MouseMove, [&](UIComponent*, const UIEvent& e) {
    if (sel.idObjeto >= 0 && modoActivo == MODO_MOVER) {
        // calcular delta...
        objetos[sel.idObjeto].x = sel.posX + delta.x;
        objetos[sel.idObjeto].y = sel.posY + delta.y;
        // Actualizar spinners del panel sin disparar un bucle de eventos
        spnX->setValue(objetos[sel.idObjeto].x);  spnX->markDirty();
        spnY->setValue(objetos[sel.idObjeto].y);  spnY->markDirty();
        escenaDirty = true;
        app.invalidate();
    }
});
```

---

## Resumen: elegir el patrón correcto

| Escenario | Patrón recomendado |
|---|---|
| 2-4 widgets simples | **Referencia directa** (lambdas con captura) |
| Panel con muchos controles interconectados | **Estado centralizado + sync** |
| Módulos independientes que no se conocen | **Bus de eventos** |
| Editor con selección de objeto | **Selección + panel de propiedades** |
| Necesitas deshacer/rehacer | **Command pattern** |
| Viewport + controles | **Estado centralizado + sync bidireccional** |