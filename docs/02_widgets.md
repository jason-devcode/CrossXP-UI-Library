# 02 — Widgets

Referencia completa de todos los widgets disponibles, sus propiedades y los eventos que emiten.

---

## Tabla de widgets

| Widget | Clase C++ | Factory C++ | Función C |
|---|---|---|---|
| Panel / contenedor | `UIPanel` | `Make::Panel` | `wxmake_panel` |
| Etiqueta de texto | `UILabel` | `Make::Label` | `wxmake_label` |
| Botón | `UIButton` | `Make::Button` | `wxmake_button` |
| Casilla de verificación | `UICheckbox` | `Make::Checkbox` | `wxmake_checkbox` |
| Botón de radio | `UIRadioButton` | `Make::Radio` | — |
| Slider horizontal/vertical | `UISlider` | `Make::Slider` | `wxmake_slider` |
| Spinner numérico | `UISpinner` | `Make::Spinner` | `wxmake_spinner` |
| Campo de texto | `UITextInput` | `Make::TextInput` | `wxmake_textinput` |
| Barra de desplazamiento | `UIScrollBar` | `Make::ScrollBar` | — |
| Caja de grupo | `UIGroupBox` | `Make::GroupBox` | `wxmake_groupbox` |
| Separador | `UISeparator` | `Make::Separator` | `wxmake_separator` |
| Viewport 3D | `UIViewport3D` | `Make::Viewport` | — |
| Barra de menú | `UIMenuBar` | `Make::MenuBar` | — |
| Barra de herramientas | `UIToolbar` | `Make::Toolbar` | — |
| Ventana flotante | `UIWindow` | `Make::Window` | — |
| Panel de pestañas | `UITabPanel` | `Make::TabPanel` | — |

---

## UIPanel

Panel de fondo con borde bevel estilo WinXP.

```cpp
// Make::Panel(id, rect, dark=false)
auto* p = static_cast<UIPanel*>(
    app.add(Make::Panel("panel", Rect(10, 10, 300, 200)))
);
p->raised  = true;    // true=relieve, false=hundido
p->border  = true;    // mostrar borde
p->bgColor = Pal::DARK_PANEL;  // color de fondo personalizado
```

```c
WXWidget* p = wxmake_panel(app, "panel", 10, 10, 300, 200, 0 /*dark*/);
```

**No emite eventos** — es un contenedor puramente visual.

---

## UILabel

Etiqueta de texto no interactiva. El texto usa la fuente bitmap 8×8 integrada.

```cpp
// Make::Label(id, rect, text, dark=false)
auto* lbl = static_cast<UILabel*>(
    app.add(Make::Label("lbl", Rect(10, 10, 200, 20), "Hola mundo"))
);

lbl->setText("Nuevo texto");   // cambia el texto y marca dirty
lbl->align = 0;                // 0=izquierda, 1=centrado, 2=derecha
lbl->fgColor = Color{255, 0, 0};  // texto rojo
```

```c
WXWidget* lbl = wxmake_label(app, "lbl", 10, 10, 200, 20, "Hola mundo", 0);
wxwidget_set_text(lbl, "Nuevo texto");
```

**Eventos que escucha:**  
Ninguno por defecto. Puedes escuchar `WXEV_CLICK` si lo deseas.

---

## UIButton

Botón pulsable con efecto de presionado.

```cpp
// Make::Button(id, rect, label)
auto* btn = static_cast<UIButton*>(
    app.add(Make::Button("btn_ok", Rect(100, 200, 80, 28), "Aceptar"))
);

btn->flat = false;   // true = estilo barra de herramientas (sin relieve en reposo)

btn->on(EventType::Click, [](UIComponent*, const UIEvent&) {
    // Se dispara al soltar el botón izquierdo sobre él
    // También con ENTER o ESPACIO cuando tiene el foco
});
```

```c
WXWidget* btn = wxmake_button(app, "btn_ok", 100, 200, 80, 28, "Aceptar");
wxwidget_on(btn, WXEV_CLICK, mi_callback, mi_estado);
```

**Eventos emitidos:**
| Evento | Cuándo |
|---|---|
| `Click` | Botón izquierdo soltado encima / ENTER / ESPACIO |

**Deshabilitar un botón:**
```cpp
btn->setEnabled(false);   // gris, no responde eventos
btn->setEnabled(true);    // restaura
```

---

## UICheckbox

Casilla de verificación con estado checked/unchecked.

```cpp
// Make::Checkbox(id, rect, label, checked=false)
auto* chk = static_cast<UICheckbox*>(
    app.add(Make::Checkbox("chk_auto", Rect(10, 50, 150, 18), "Auto-guardar", false))
);

bool estado = chk->isChecked();
chk->setChecked(true);

chk->on(EventType::CheckChanged, [](UIComponent* w, const UIEvent& e) {
    bool checked = (e.ivalue != 0);
    // e.ivalue == 1 → marcado, 0 → desmarcado
});
```

```c
WXWidget* chk = wxmake_checkbox(app, "chk_auto", 10, 50, 150, 18, "Auto-guardar", 0);
wxwidget_on(chk, WXEV_CHECK_CHANGED, on_check, estado);

// Leer estado:
int checked = wxwidget_is_checked(chk);
// Cambiar estado programáticamente:
wxwidget_set_checked(chk, 1);
```

**Eventos emitidos:**
| Evento | `e.ivalue` |
|---|---|
| `CheckChanged` | `1` = marcado, `0` = desmarcado |

---

## UIRadioButton (solo C++)

Los radio buttons se agrupan por nombre de grupo. Solo uno del grupo puede estar activo.

```cpp
// Make::Radio(id, rect, label, group, checked=false)
auto* r1 = static_cast<UIRadioButton*>(
    app.add(Make::Radio("r_op1", Rect(10, 10, 120, 18), "Opción A", "grupo1", true))
);
auto* r2 = static_cast<UIRadioButton*>(
    app.add(Make::Radio("r_op2", Rect(10, 30, 120, 18), "Opción B", "grupo1", false))
);

// Leer selección
bool a_activo = r1->isChecked();

// Escuchar cambios
r1->on(EventType::CheckChanged, [](UIComponent* w, const UIEvent& e) {
    if (e.ivalue) {
        // Este radio está ahora seleccionado
    }
});
```

---

## UISlider

Deslizador con rango configurable, horizontal o vertical.

```cpp
// Make::Slider(id, rect, min, max, value)
// Horizontal si rect.w > rect.h, vertical si rect.h > rect.w
auto* sld = static_cast<UISlider*>(
    app.add(Make::Slider("vol", Rect(10, 50, 200, 16), 0.f, 100.f, 50.f))
);

float v = sld->getValue();
sld->setValue(75.f);    // no emite ValueChanged
sld->markDirty();       // forzar redibujado tras setValue manual

sld->on(EventType::ValueChanged, [](UIComponent*, const UIEvent& e) {
    float valor = e.fvalue;   // nuevo valor en [min, max]
});
```

```c
WXWidget* sld = wxmake_slider(app, "vol", 10, 50, 200, 16, 0.f, 100.f, 50.f);
wxwidget_on(sld, WXEV_VALUE_CHANGED, on_slider, estado);

float v = wxwidget_get_value(sld);
wxwidget_set_value(sld, 75.f);
```

**Controles del usuario:**
- Clic/arrastre con ratón
- Flechas del teclado (±5% del rango) cuando tiene foco

---

## UISpinner

Campo numérico con botones arriba/abajo.

```cpp
// Make::Spinner(id, rect, min, max, value, step)
auto* spn = static_cast<UISpinner*>(
    app.add(Make::Spinner("edad", Rect(80, 100, 100, 22), 0.f, 120.f, 25.f, 1.f))
);

spn->decimals = 0;       // 0 = entero, N = decimales mostrados
float v = spn->getValue();
spn->setValue(30.f);

spn->on(EventType::ValueChanged, [](UIComponent*, const UIEvent& e) {
    float valor = e.fvalue;
});
```

```c
WXWidget* spn = wxmake_spinner(app, "edad", 80, 100, 100, 22, 0.f, 120.f, 25.f, 1.f);
wxwidget_on(spn, WXEV_VALUE_CHANGED, on_spinner, estado);
```

**Controles del usuario:**
- Clic en flechas arriba/abajo
- Rueda del ratón cuando tiene foco
- Flechas de teclado

---

## UITextInput

Campo de texto editable con soporte de selección.

```cpp
// Make::TextInput(id, rect, placeholder="")
auto* ti = static_cast<UITextInput*>(
    app.add(Make::TextInput("nombre", Rect(80, 50, 200, 22), "Tu nombre..."))
);

std::string txt = ti->getText();
ti->setText("valor inicial");
ti->maxLen = 64;     // longitud máxima en caracteres

ti->on(EventType::ValueChanged, [](UIComponent*, const UIEvent& e) {
    std::string texto = e.svalue;   // contenido actual del campo
});
```

```c
WXWidget* ti = wxmake_textinput(app, "nombre", 80, 50, 200, 22, "Tu nombre...");
wxwidget_on(ti, WXEV_VALUE_CHANGED, on_text, estado);

const char* txt = wxwidget_get_text(ti);
wxwidget_set_text(ti, "valor inicial");
```

**Atajos de teclado:**
- `←` `→` — mover cursor
- `Home` / `End` — inicio/fin de línea
- `Backspace` / `Delete` — borrar
- `Ctrl+A` — seleccionar todo

---

## UIScrollBar

Barra de desplazamiento. El valor va de `0.0` a `1.0`.

```cpp
// Make::ScrollBar(id, rect, horizontal=false)
auto* sb = static_cast<UIScrollBar*>(
    app.add(Make::ScrollBar("sb_v", Rect(380, 50, 16, 300)))
);

sb->thumbRatio = 0.2f;   // tamaño del thumb como fracción del track
sb->setValue(0.5f);       // posición 0..1

sb->on(EventType::Scroll, [](UIComponent*, const UIEvent& e) {
    float pos = e.fvalue;   // 0.0 (arriba) .. 1.0 (abajo)
    // actualizar lo que se muestra
});
```

---

## UIGroupBox

Cuadro de grupo con título, para organizar controles relacionados.

```cpp
// Make::GroupBox(id, rect, title)
auto* gb = static_cast<UIGroupBox*>(
    app.add(Make::GroupBox("grp_color", Rect(10, 10, 200, 120), "Color"))
);
// Los widgets dentro del groupbox se posicionan con coordenadas absolutas
// (no son hijos en el árbol)
```

```c
WXWidget* gb = wxmake_groupbox(app, "grp_color", 10, 10, 200, 120, "Color");
```

---

## UISeparator

Línea visual horizontal o vertical.

```cpp
// Make::Separator(id, rect, horizontal=true)
app.add(Make::Separator("sep1", Rect(10, 40, 300, 4), true));   // horizontal
app.add(Make::Separator("sep2", Rect(150, 10, 4, 200), false)); // vertical
```

```c
wxmake_separator(app, "sep1", 10, 40, 300, 4, 1 /*horizontal*/);
```

---

## UIMenuBar

Barra de menús en la parte superior de la ventana.

```cpp
// Make::MenuBar(id, rect)
auto mb = Make::MenuBar("menubar", Rect(0, 0, app.width(), 20));

mb->addMenu("Archivo", {
    MenuItem("Nuevo",       "file_new"),
    MenuItem("Abrir...",    "file_open"),
    MenuItem("",            "",          true),  // separador
    MenuItem("Salir",       "file_exit"),
});
mb->addMenu("Edición", {
    MenuItem("Copiar",  "edit_copy"),
    MenuItem("Pegar",   "edit_paste"),
    MenuItem("Deshacer — deshabilitado", "edit_undo", false, true),  // disabled
});

mb->on(EventType::MenuItemClicked, [](UIComponent*, const UIEvent& e) {
    if (e.svalue == "file_exit") {
        SDL_Event q; q.type = SDL_QUIT; SDL_PushEvent(&q);
    } else if (e.svalue == "file_new") {
        // nueva acción
    }
    // e.svalue  = id del MenuItem
    // e.ivalue  = índice en el menú
});

app.add(std::move(mb));
```

---

## UIToolbar

Barra de herramientas con botones planos.

```cpp
auto tb = Make::Toolbar("toolbar", Rect(0, 20, app.width(), 26));

// addButton(id, x_offset, width, label, ctx)
UIButton* btnSel = tb->addButton("btn_sel",   2, 50, "Select", app.ctx());
UIButton* btnMov = tb->addButton("btn_move", 54, 50, "Move",   app.ctx());

// Los botones de toolbar son flat=true por defecto
// Para "activar" uno (indicar selección):
btnSel->flat = false;   // muestra relieve = activo
btnMov->flat = true;    // sin relieve = inactivo

btnSel->on(EventType::Click, [&](UIComponent*, const UIEvent&) {
    btnSel->flat = false;
    btnMov->flat = true;
    app.invalidate();
});

app.add(std::move(tb));
```

---

## UIWindow (ventana flotante)

Ventana flotante con barra de título y botón de cierre, arrastrable.

```cpp
// Make::Window(id, rect, title, ctx, dark=false)
auto win = Make::Window("prop_win", Rect(100, 100, 250, 200),
                        "Propiedades", app.ctx());

auto* pWin = static_cast<UIWindow*>(app.add(std::move(win)));

// Agregar controles al cuerpo de la ventana
pWin->addToBody(
    Make::Label("lbl_w", Rect(110, 140, 100, 20), "Nombre:"),
    app.ctx()
);

// Escuchar cierre
pWin->titleBar->on(EventType::Click, [&](UIComponent*, const UIEvent& e) {
    if (e.svalue == "close")
        pWin->setVisible(false);
});
```

---

## UIViewport3D

Zona de renderizado de framebuffer personalizado. Ver la guía completa en  
[04 — Framebuffer personalizado](04_framebuffer.md).

```cpp
auto vp = Make::Viewport("vp1", Rect(0, 0, 512, 400), "Perspective");
auto* pVP = static_cast<UIViewport3D*>(app.add(std::move(vp)));

// Alimentar píxeles ARGB
std::vector<uint32_t> pixeles(512 * 400);
// ... renderizar en pixeles ...
pVP->updatePixels(pixeles.data(), 512, 400);
```

---

## Propiedades comunes a todos los widgets

Todos los widgets heredan de `UIComponent` y comparten:

```cpp
widget->id             // std::string — identificador único
widget->rect           // Rect — posición y tamaño
widget->visible        // bool — si se renderiza
widget->enabled        // bool — si responde eventos
widget->layer          // int  — orden de render (mayor = encima)
widget->bgColor        // Color — color de fondo
widget->fgColor        // Color — color de texto/primer plano
widget->darkMode       // bool — tema oscuro

widget->setVisible(bool)
widget->setEnabled(bool)
widget->setRect(Rect)
widget->markDirty()    // fuerza redibujado en el próximo frame
```

---

## Paleta de colores (Pal::)

```cpp
Pal::BG            // fondo de ventana       {236, 233, 216}
Pal::FACE          // superficie de control  {236, 233, 216}
Pal::TEXT          // texto normal           {0, 0, 0}
Pal::DISABLED_TXT  // texto deshabilitado    {172, 168, 153}
Pal::SHADOW        // borde sombra           {172, 168, 153}
Pal::DARK_SHADOW   // borde sombra oscuro    {113, 111, 100}
Pal::SEL_BG        // fondo selección        {49, 106, 197}
Pal::SEL_TXT       // texto selección        {255, 255, 255}
Pal::TITLE_L       // degradado título (izq) {0, 84, 166}
Pal::TITLE_R       // degradado título (der) {116, 166, 241}
Pal::DARK_PANEL    // panel oscuro           {105, 105, 105}
Pal::VP_BORDER_ON  // borde viewport activo  {255, 200, 0}
```

### Crear un color personalizado

```cpp
Color rojo   = Color(255, 0, 0);
Color verde  = Color(0, 255, 0, 128);  // con transparencia
widget->fgColor = Color(200, 200, 200);
```