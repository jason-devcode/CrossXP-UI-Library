#pragma once
/*
 * winxp_ui.hpp  –  Retained-mode WinXP-style UI library (SDL2, software rendering)
 * C++11, single header, Linux/Windows
 */

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

namespace WXUI {

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 1 – Embedded 8×8 Bitmap Font (ASCII 32-127, public-domain glyphs)
// ═══════════════════════════════════════════════════════════════════════════
static const uint8_t g_font8x8[96][8] = {
/* 32 ' ' */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
/* 33 '!' */ {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
/* 34 '"' */ {0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00},
/* 35 '#' */ {0x66,0x66,0xFF,0x66,0xFF,0x66,0x66,0x00},
/* 36 '$' */ {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},
/* 37 '%' */ {0x62,0x66,0x0C,0x18,0x30,0x66,0x46,0x00},
/* 38 '&' */ {0x3C,0x66,0x3C,0x38,0x67,0x66,0x3F,0x00},
/* 39 ''' */ {0x06,0x06,0x0C,0x00,0x00,0x00,0x00,0x00},
/* 40 '(' */ {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
/* 41 ')' */ {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
/* 42 '*' */ {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
/* 43 '+' */ {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
/* 44 ',' */ {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
/* 45 '-' */ {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
/* 46 '.' */ {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
/* 47 '/' */ {0x02,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
/* 48 '0' */ {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
/* 49 '1' */ {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
/* 50 '2' */ {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00},
/* 51 '3' */ {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
/* 52 '4' */ {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},
/* 53 '5' */ {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
/* 54 '6' */ {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
/* 55 '7' */ {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00},
/* 56 '8' */ {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
/* 57 '9' */ {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
/* 58 ':' */ {0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x00},
/* 59 ';' */ {0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x30},
/* 60 '<' */ {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},
/* 61 '=' */ {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
/* 62 '>' */ {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},
/* 63 '?' */ {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},
/* 64 '@' */ {0x3E,0x63,0x6F,0x69,0x6F,0x60,0x3E,0x00},
/* 65 'A' */ {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00},
/* 66 'B' */ {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
/* 67 'C' */ {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
/* 68 'D' */ {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
/* 69 'E' */ {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
/* 70 'F' */ {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
/* 71 'G' */ {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00},
/* 72 'H' */ {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
/* 73 'I' */ {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
/* 74 'J' */ {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00},
/* 75 'K' */ {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
/* 76 'L' */ {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
/* 77 'M' */ {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
/* 78 'N' */ {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},
/* 79 'O' */ {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
/* 80 'P' */ {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
/* 81 'Q' */ {0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0x00},
/* 82 'R' */ {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},
/* 83 'S' */ {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
/* 84 'T' */ {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
/* 85 'U' */ {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
/* 86 'V' */ {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
/* 87 'W' */ {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
/* 88 'X' */ {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
/* 89 'Y' */ {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
/* 90 'Z' */ {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
/* 91 '[' */ {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
/* 92 '\' */ {0x40,0x60,0x30,0x18,0x0C,0x06,0x02,0x00},
/* 93 ']' */ {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
/* 94 '^' */ {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
/* 95 '_' */ {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},
/* 96 '`' */ {0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},
/* 97 'a' */ {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},
/* 98 'b' */ {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00},
/* 99 'c' */ {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00},
/*100 'd' */ {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00},
/*101 'e' */ {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00},
/*102 'f' */ {0x1C,0x30,0x30,0x7C,0x30,0x30,0x30,0x00},
/*103 'g' */ {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C},
/*104 'h' */ {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00},
/*105 'i' */ {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
/*106 'j' */ {0x06,0x00,0x06,0x06,0x06,0x06,0x66,0x3C},
/*107 'k' */ {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00},
/*108 'l' */ {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
/*109 'm' */ {0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00},
/*110 'n' */ {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00},
/*111 'o' */ {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
/*112 'p' */ {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},
/*113 'q' */ {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},
/*114 'r' */ {0x00,0x00,0x6C,0x76,0x60,0x60,0x60,0x00},
/*115 's' */ {0x00,0x00,0x3C,0x60,0x3C,0x06,0x7C,0x00},
/*116 't' */ {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00},
/*117 'u' */ {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},
/*118 'v' */ {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00},
/*119 'w' */ {0x00,0x00,0x63,0x6B,0x7F,0x77,0x63,0x00},
/*120 'x' */ {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00},
/*121 'y' */ {0x00,0x00,0x66,0x66,0x3E,0x06,0x66,0x3C},
/*122 'z' */ {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00},
/*123 '{' */ {0x0C,0x18,0x18,0x70,0x18,0x18,0x0C,0x00},
/*124 '|' */ {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
/*125 '}' */ {0x30,0x18,0x18,0x0E,0x18,0x18,0x30,0x00},
/*126 '~' */ {0x00,0x00,0x00,0x32,0x4C,0x00,0x00,0x00},
/*127 DEL */ {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
};

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 2 – Color & Palette
// ═══════════════════════════════════════════════════════════════════════════
struct Color {
    uint8_t r,g,b,a;
    Color(uint8_t r=0,uint8_t g=0,uint8_t b=0,uint8_t a=255):r(r),g(g),b(b),a(a){}
    uint32_t pack(SDL_Surface* s) const { return SDL_MapRGBA(s->format,r,g,b,a); }
    bool operator==(const Color& o) const { return r==o.r&&g==o.g&&b==o.b&&a==o.a; }
};

namespace Pal {
    // Classic WinXP / 3ds-Max neutral palette
    static const Color BG          {236,233,216};
    static const Color FACE        {236,233,216};
    static const Color LIGHT       {255,255,255};
    static const Color HILIGHT     {255,255,255};
    static const Color SHADOW      {172,168,153};
    static const Color DARK_SHADOW {113,111,100};
    static const Color TEXT        {  0,  0,  0};
    static const Color DISABLED_TXT{172,168,153};
    static const Color EDIT_BG     {255,255,255};
    static const Color SEL_BG      { 49,106,197};
    static const Color SEL_TXT     {255,255,255};
    static const Color TITLE_L     {  0, 84,166};
    static const Color TITLE_R     {116,166,241};
    static const Color SCROLLBAR_BG{212,208,200};
    // 3ds Max dark UI  (slightly lighter than before)
    static const Color DARK_PANEL  {105,105,105};
    static const Color DARK_FACE   { 80, 80, 80};
    static const Color DARK_BORDER { 50, 50, 50};
    static const Color DARK_TEXT   {220,220,220};
    // Viewport
    static const Color VP_BG       { 80, 80, 80};
    static const Color VP_BORDER_ON{255,200,  0};
    static const Color VP_BORDER_OFF{110,110,110};
    static const Color VP_LABEL    {200,200,200};
    // Tooltip
    static const Color TIP_BG      {255,255,225};
    static const Color TIP_BORDER  {  0,  0,  0};
}

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 3 – Low-level Surface Drawing Primitives
// ═══════════════════════════════════════════════════════════════════════════
namespace Draw {

inline void setPixel(SDL_Surface* s, int x, int y, uint32_t c) {
    if(x<0||y<0||x>=s->w||y>=s->h) return;
    ((uint32_t*)s->pixels)[y*(s->pitch/4)+x] = c;
}
inline uint32_t getPixel(SDL_Surface* s, int x, int y) {
    if(x<0||y<0||x>=s->w||y>=s->h) return 0;
    return ((uint32_t*)s->pixels)[y*(s->pitch/4)+x];
}

inline void fillRect(SDL_Surface* s, int x, int y, int w, int h, Color c) {
    SDL_Rect r={x,y,w,h};
    SDL_FillRect(s,&r,c.pack(s));
}

inline void drawHLine(SDL_Surface* s, int x, int y, int len, Color c) {
    uint32_t p=c.pack(s);
    for(int i=0;i<len;i++) setPixel(s,x+i,y,p);
}
inline void drawVLine(SDL_Surface* s, int x, int y, int len, Color c) {
    uint32_t p=c.pack(s);
    for(int i=0;i<len;i++) setPixel(s,x,y+i,p);
}

inline void drawRect(SDL_Surface* s, int x, int y, int w, int h, Color c) {
    drawHLine(s,x,y,w,c);
    drawHLine(s,x,y+h-1,w,c);
    drawVLine(s,x,y,h,c);
    drawVLine(s,x+w-1,y,h,c);
}

// Classic WinXP 3D bevel  (raised=true → button normal, raised=false → sunken)
inline void drawBevel(SDL_Surface* s, int x, int y, int w, int h, bool raised) {
    Color tl = raised ? Pal::HILIGHT   : Pal::DARK_SHADOW;
    Color br = raised ? Pal::DARK_SHADOW : Pal::HILIGHT;
    Color tl2= raised ? Pal::LIGHT     : Pal::SHADOW;
    Color br2= raised ? Pal::SHADOW    : Pal::LIGHT;
    // outer
    drawHLine(s,x,    y,    w,tl);
    drawVLine(s,x,    y,    h,tl);
    drawHLine(s,x,    y+h-1,w,br);
    drawVLine(s,x+w-1,y,    h,br);
    if(w>2&&h>2){
        // inner
        drawHLine(s,x+1,y+1,  w-2,tl2);
        drawVLine(s,x+1,y+1,  h-2,tl2);
        drawHLine(s,x+1,y+h-2,w-2,br2);
        drawVLine(s,x+w-2,y+1,h-2,br2);
    }
}

// WinXP thin border (edit box / panel sunken)
inline void drawSunkenBorder(SDL_Surface* s, int x, int y, int w, int h) {
    drawHLine(s,x,y,w,    Pal::SHADOW);
    drawVLine(s,x,y,h,    Pal::SHADOW);
    drawHLine(s,x,y+h-1,w,Pal::LIGHT);
    drawVLine(s,x+w-1,y,h,Pal::LIGHT);
    if(w>2&&h>2){
        drawHLine(s,x+1,y+1,  w-2,Pal::DARK_SHADOW);
        drawVLine(s,x+1,y+1,  h-2,Pal::DARK_SHADOW);
        drawHLine(s,x+1,y+h-2,w-2,Pal::FACE);
        drawVLine(s,x+w-2,y+1,h-2,Pal::FACE);
    }
}

// Horizontal WinXP gradient (title bar)
inline void drawGradientH(SDL_Surface* s, int x, int y, int w, int h,
                           Color left, Color right) {
    for(int i=0;i<w;i++){
        float t=float(i)/float(w>1?w-1:1);
        Color c{
            uint8_t(left.r+(right.r-left.r)*t),
            uint8_t(left.g+(right.g-left.g)*t),
            uint8_t(left.b+(right.b-left.b)*t)
        };
        drawVLine(s,x+i,y,h,c);
    }
}

// 8×8 bitmap text
inline void drawChar(SDL_Surface* s, int x, int y, char ch, Color fg, Color bg,
                     bool transparent_bg=true) {
    int idx=(unsigned char)ch-32;
    if(idx<0||idx>95) idx=0;
    const uint8_t* glyph=g_font8x8[idx];
    uint32_t fgP=fg.pack(s), bgP=bg.pack(s);
    for(int row=0;row<8;row++)
        for(int col=0;col<8;col++){
            bool on=(glyph[row]>>(7-col))&1;
            if(on)            setPixel(s,x+col,y+row,fgP);
            else if(!transparent_bg) setPixel(s,x+col,y+row,bgP);
        }
}

inline void drawText(SDL_Surface* s, int x, int y, const std::string& txt,
                     Color fg, Color bg={0,0,0}, bool transparent=true) {
    int cx=x;
    for(char c: txt){
        drawChar(s,cx,y,c,fg,bg,transparent);
        cx+=8;
    }
}

// Draw text centered in a rect
inline void drawTextCentered(SDL_Surface* s, int rx, int ry, int rw, int rh,
                              const std::string& txt, Color fg) {
    int tw=(int)txt.size()*8;
    int th=8;
    int ox=rx+(rw-tw)/2;
    int oy=ry+(rh-th)/2;
    drawText(s,ox,oy,txt,fg);
}

// Draw text left-aligned with padding
inline void drawTextLeft(SDL_Surface* s, int rx, int ry, int rh,
                         const std::string& txt, Color fg, int padX=4) {
    int oy=ry+(rh-8)/2;
    drawText(s,rx+padX,oy,txt,fg);
}

// Dotted focus rect
inline void drawFocusRect(SDL_Surface* s, int x, int y, int w, int h) {
    uint32_t c = Color{0,0,0}.pack(s);
    for(int i=x;i<x+w;i+=2){ setPixel(s,i,y,c); setPixel(s,i,y+h-1,c); }
    for(int i=y;i<y+h;i+=2){ setPixel(s,x,i,c); setPixel(s,x+w-1,i,c); }
}

// Checkmark (11×8)
inline void drawCheck(SDL_Surface* s, int x, int y, Color c) {
    static const int pts[][2]={{2,4},{3,5},{4,6},{5,5},{6,4},{7,3},{8,2}};
    uint32_t p=c.pack(s);
    for(auto& pt: pts){ setPixel(s,x+pt[0],y+pt[1],p); setPixel(s,x+pt[0],y+pt[1]-1,p); }
}

// Small arrow (down)
inline void drawArrowDown(SDL_Surface* s, int x, int y, int size, Color c) {
    uint32_t p=c.pack(s);
    for(int i=0;i<size;i++) drawHLine(s,x+i,y+i,size*2-1-i*2,c);
}

inline void drawArrowUp(SDL_Surface* s, int x, int y, int size, Color c) {
    uint32_t p=c.pack(s);
    for(int i=0;i<size;i++) drawHLine(s,x+size-1-i,y+i,i*2+1,c);
}

inline void drawArrowLeft(SDL_Surface* s, int x, int y, int size, Color c) {
    for(int i=0;i<size;i++) drawVLine(s,x+i,y+size-1-i,i*2+1,c);
}
inline void drawArrowRight(SDL_Surface* s, int x, int y, int size, Color c) {
    for(int i=0;i<size;i++) drawVLine(s,x+size-1-i,y+i,i*2+1,c);
}

} // namespace Draw

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 4 – Core Types: Rect, State, Event
// ═══════════════════════════════════════════════════════════════════════════
struct Rect {
    int x,y,w,h;
    Rect(): x(0),y(0),w(0),h(0){}
    Rect(int x_,int y_,int w_,int h_): x(x_),y(y_),w(w_),h(h_){}
    SDL_Rect toSDL() const { SDL_Rect r={x,y,w,h}; return r; }
    bool contains(int px,int py) const { return px>=x&&py>=y&&px<x+w&&py<y+h; }
    Rect offset(int dx,int dy) const { return Rect(x+dx,y+dy,w,h); }
};

enum class WidgetState { Normal, Hovered, Pressed, Disabled };

enum class EventType {
    Click, DblClick,
    MouseDown, MouseUp, MouseMove, MouseEnter, MouseLeave, MouseWheel,
    KeyDown, KeyUp, TextInput,
    ValueChanged, CheckChanged,
    FocusGained, FocusLost,
    MenuItemClicked,
    ViewportFocusGained, ViewportFocusLost,
    Scroll
};

struct UIEvent {
    EventType   type;
    int         mx=0, my=0;      // mouse pos (screen)
    int         mbtn=0;          // SDL_BUTTON_*
    int         wheel=0;         // scroll delta
    SDL_Keycode key=0;
    uint16_t    mod=0;
    const char* text=nullptr;    // UTF-8 text input
    float       fvalue=0.f;      // generic float payload
    int         ivalue=0;        // generic int payload
    std::string svalue;          // generic string payload
};

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 5 – UIComponent (Base)
// ═══════════════════════════════════════════════════════════════════════════
class UIContext;

class UIComponent {
public:
    // Identity
    std::string   id;
    std::string   tooltip;

    // Layout
    Rect          rect;          // absolute screen rect
    int           layer = 0;
    bool          visible = true;
    bool          enabled = true;

    // State machine
    WidgetState   wstate = WidgetState::Normal;
    bool          focused = false;
    bool          dirty   = true;

    // Per-component retained framebuffer
    SDL_Surface*  surf = nullptr;

    // Hierarchy
    UIComponent*  parent = nullptr;
    std::vector<std::unique_ptr<UIComponent>> children;

    // Event callbacks
    using CB = std::function<void(UIComponent*, const UIEvent&)>;
    std::unordered_map<int, std::vector<CB>> cbs;

    // Style overrides (optional)
    Color bgColor = Pal::FACE;
    Color fgColor = Pal::TEXT;
    bool  darkMode = false;

    UIComponent() = default;
    UIComponent(const std::string& id_, Rect r) : id(id_), rect(r) {}
    virtual ~UIComponent() { if(surf) SDL_FreeSurface(surf); }

    // Disable copy
    UIComponent(const UIComponent&)=delete;
    UIComponent& operator=(const UIComponent&)=delete;

    // ── Callbacks ────────────────────────────────────────────────────────
    UIComponent* on(EventType t, CB cb) {
        cbs[(int)t].push_back(std::move(cb));
        return this;
    }

    void emit(const UIEvent& e) {
        auto it=cbs.find((int)e.type);
        if(it!=cbs.end()) for(auto& cb: it->second) cb(this,e);
    }

    // ── Dirty / Surface ──────────────────────────────────────────────────
    void markDirty() {
        dirty=true;
        // propagate up so container knows to re-composite
        if(parent) parent->dirty=true;
    }

    void ensureSurf() {
        if(!surf||surf->w!=rect.w||surf->h!=rect.h){
            if(surf) SDL_FreeSurface(surf);
            surf=SDL_CreateRGBSurface(0,std::max(1,rect.w),std::max(1,rect.h),32,
                0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
            dirty=true;
        }
    }

    // ── State helpers ─────────────────────────────────────────────────────
    void setState(WidgetState s) { if(wstate!=s){wstate=s;markDirty();} }
    void setEnabled(bool e) { enabled=e; setState(e?WidgetState::Normal:WidgetState::Disabled); }
    void setVisible(bool v) { if(visible!=v){visible=v;markDirty();} }
    void setRect(Rect r)    { if(r.x!=rect.x||r.y!=rect.y||r.w!=rect.w||r.h!=rect.h){rect=r;if(surf){SDL_FreeSurface(surf);surf=nullptr;}markDirty();} }

    // ── Children ─────────────────────────────────────────────────────────
    UIComponent* addChild(std::unique_ptr<UIComponent> c, UIContext& ctx);

    UIComponent* findById(const std::string& searchId) {
        if(id==searchId) return this;
        for(auto& ch: children){
            auto* r=ch->findById(searchId);
            if(r) return r;
        }
        return nullptr;
    }

    bool isAncestorOf(UIComponent* c) {
        while(c){ if(c==this) return true; c=c->parent; } return false;
    }

    // ── Virtual interface ─────────────────────────────────────────────────
    virtual void onDraw() {}          // draw self onto this->surf (ONLY when dirty)
    virtual bool hitTest(int x,int y) const { return visible&&enabled&&rect.contains(x,y); }

    // Returns true if event was consumed
    virtual bool onEvent(const UIEvent& e, UIContext& ctx) { return false; }

    // ── Render (composite children) ───────────────────────────────────────
    void render() {
        ensureSurf();
        if(dirty){
            onDraw();
            // blit children in layer order
            for(auto& ch: children){
                if(!ch->visible) continue;
                ch->render();
                SDL_Rect dst={ch->rect.x-rect.x,ch->rect.y-rect.y,ch->rect.w,ch->rect.h};
                SDL_BlitSurface(ch->surf,nullptr,surf,&dst);
            }
            dirty=false;
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 6 – UIContext (registry, focus, event dispatch, main render loop)
// ═══════════════════════════════════════════════════════════════════════════
class UIContext {
public:
    // Root surface (full window)
    SDL_Surface*  root = nullptr;
    SDL_Texture*  tex  = nullptr;
    SDL_Renderer* ren  = nullptr;
    int           winW = 800, winH = 600;

    // All top-level components (owned)
    std::vector<std::unique_ptr<UIComponent>> widgets;
    // Flat registry id→ptr (includes children)
    std::unordered_map<std::string, UIComponent*> registry;

    // Focus
    UIComponent*  focusedWidget   = nullptr;
    UIComponent*  hoveredWidget   = nullptr;
    UIComponent*  pressedWidget   = nullptr;
    UIComponent*  activeViewport  = nullptr;  // for 3D editor

    // State
    bool          needsRedraw = true;

    // Menus
    UIComponent*  openMenu = nullptr;

    UIContext() = default;

    bool init(SDL_Renderer* r, int w, int h) {
        ren=r; winW=w; winH=h;
        root=SDL_CreateRGBSurface(0,w,h,32,0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
        tex =SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h);
        return root&&tex;
    }

    void resize(int w, int h) {
        winW=w; winH=h;
        if(root) SDL_FreeSurface(root);
        if(tex)  SDL_DestroyTexture(tex);
        root=SDL_CreateRGBSurface(0,w,h,32,0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
        tex =SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,w,h);
        for(auto& w2: widgets) w2->markDirty();
        needsRedraw=true;
    }

    void destroy() {
        if(root) SDL_FreeSurface(root);
        if(tex)  SDL_DestroyTexture(tex);
    }

    // ── Component Management ─────────────────────────────────────────────
    UIComponent* add(std::unique_ptr<UIComponent> c) {
        auto* ptr=c.get();
        registerWidget(ptr);
        // sort by layer
        widgets.push_back(std::move(c));
        sortWidgets();
        needsRedraw=true;
        return ptr;
    }

    bool remove(const std::string& id) {
        for(auto it=widgets.begin();it!=widgets.end();++it){
            if((*it)->id==id){
                unregisterWidget(it->get());
                if(focusedWidget==it->get()) focusedWidget=nullptr;
                if(hoveredWidget==it->get()) hoveredWidget=nullptr;
                widgets.erase(it);
                needsRedraw=true;
                return true;
            }
        }
        return false;
    }

    UIComponent* findById(const std::string& id) {
        auto it=registry.find(id);
        return it!=registry.end()?it->second:nullptr;
    }

    // ── Focus ────────────────────────────────────────────────────────────
    void setFocus(UIComponent* c) {
        if(focusedWidget==c) return;
        if(focusedWidget){
            focusedWidget->focused=false;
            focusedWidget->markDirty();
            UIEvent e; e.type=EventType::FocusLost;
            focusedWidget->emit(e);
        }
        focusedWidget=c;
        if(c){
            c->focused=true;
            c->markDirty();
            UIEvent e; e.type=EventType::FocusGained;
            c->emit(e);
        }
        needsRedraw=true;
    }

    UIComponent* getFocused() { return focusedWidget; }
    UIComponent* getHovered() { return hoveredWidget; }
    UIComponent* getActiveViewport() { return activeViewport; }
    bool hasFocus(const std::string& id) { return focusedWidget&&focusedWidget->id==id; }

    // ── Event Processing ─────────────────────────────────────────────────
    void processEvent(const SDL_Event& sdl_ev);

    // ── Render ───────────────────────────────────────────────────────────
    void render() {
        if(!needsRedraw) {
            // check if any widget is dirty
            bool any=false;
            for(auto& w: widgets) if(w->dirty){any=true;break;}
            if(!any) return;
        }
        // Clear root
        SDL_FillRect(root,nullptr,Color(Pal::BG).pack(root));
        // Composite widgets sorted by layer
        for(auto& w: widgets){
            if(!w->visible) continue;
            w->render();
            SDL_Rect dst={w->rect.x,w->rect.y,w->rect.w,w->rect.h};
            SDL_BlitSurface(w->surf,nullptr,root,&dst);
        }
        // Upload to GPU texture
        SDL_UpdateTexture(tex,nullptr,root->pixels,root->pitch);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren,tex,nullptr,nullptr);
        SDL_RenderPresent(ren);
        needsRedraw=false;
    }

    void invalidate() { needsRedraw=true; }

    static bool layerLess(const std::unique_ptr<UIComponent>& a,
                          const std::unique_ptr<UIComponent>& b){ return a->layer<b->layer; }

private:
    void registerWidget(UIComponent* c) {
        if(!c->id.empty()) registry[c->id]=c;
        for(auto& ch: c->children) registerWidget(ch.get());
    }
    void unregisterWidget(UIComponent* c) {
        if(!c->id.empty()) registry.erase(c->id);
        for(auto& ch: c->children) unregisterWidget(ch.get());
    }
    void sortWidgets(){
        std::stable_sort(widgets.begin(),widgets.end(),layerLess);
    }
    // Find deepest hit widget (children first)
    UIComponent* hitTestAll(UIComponent* root_c, int x, int y){
        // children reverse order (top layer first)
        for(int i=(int)root_c->children.size()-1;i>=0;i--){
            auto* ch=root_c->children[i].get();
            if(!ch->visible||!ch->enabled) continue;
            auto* r=hitTestAll(ch,x,y);
            if(r) return r;
        }
        if(root_c->hitTest(x,y)) return root_c;
        return nullptr;
    }
    UIComponent* hitTest(int x,int y){
        for(int i=(int)widgets.size()-1;i>=0;i--){
            auto& w=widgets[i];
            if(!w->visible||!w->enabled) continue;
            auto* r=hitTestAll(w.get(),x,y);
            if(r) return r;
        }
        return nullptr;
    }
};

// addChild needs UIContext defined, so implement here
inline UIComponent* UIComponent::addChild(std::unique_ptr<UIComponent> c, UIContext& ctx) {
    c->parent=this;
    auto* ptr=c.get();
    // register in context
    std::function<void(UIComponent*)> reg=[&](UIComponent* cc){
        if(!cc->id.empty()) ctx.registry[cc->id]=cc;
        for(auto& ch: cc->children) reg(ch.get());
    };
    reg(ptr);
    children.push_back(std::move(c));
    std::stable_sort(children.begin(),children.end(),UIContext::layerLess);
    markDirty();
    return ptr;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 7 – Concrete Widgets
// ═══════════════════════════════════════════════════════════════════════════

// ─── UIPanel ────────────────────────────────────────────────────────────────
class UIPanel : public UIComponent {
public:
    bool border=true;
    bool raised=true;   // raised=panel, false=sunken/inset

    UIPanel(const std::string& id_, Rect r, bool dark=false) : UIComponent(id_,r) {
        darkMode=dark;
        bgColor = dark ? Pal::DARK_PANEL : Pal::FACE;
    }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        if(border) Draw::drawBevel(surf,0,0,rect.w,rect.h,raised);
    }
};

// ─── UILabel ────────────────────────────────────────────────────────────────
class UILabel : public UIComponent {
public:
    std::string text;
    int align=0; // 0=left, 1=center, 2=right

    UILabel(const std::string& id_, Rect r, const std::string& t, bool dark=false)
        : UIComponent(id_,r), text(t) {
        darkMode=dark;
        fgColor = dark ? Pal::DARK_TEXT : Pal::TEXT;
        bgColor = dark ? Pal::DARK_PANEL : Pal::FACE;
    }

    void setText(const std::string& t){ text=t; markDirty(); }

    const std::string& getText() const { return text; }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        Color fg = enabled ? fgColor : Pal::DISABLED_TXT;
        if(align==1) Draw::drawTextCentered(surf,0,0,rect.w,rect.h,text,fg);
        else if(align==0) Draw::drawTextLeft(surf,0,0,rect.h,text,fg);
        else {
            int tw=(int)text.size()*8;
            Draw::drawText(surf,rect.w-tw-4,(rect.h-8)/2,text,fg);
        }
    }
};

// ─── UIButton ───────────────────────────────────────────────────────────────
class UIButton : public UIComponent {
public:
    std::string text;
    bool flat=false; // flat toolbar-style

    UIButton(const std::string& id_, Rect r, const std::string& t)
        : UIComponent(id_,r), text(t) {}

    void onDraw() override {
        bool pressed = (wstate==WidgetState::Pressed);
        bool hovered = (wstate==WidgetState::Hovered);
        bool dis     = (wstate==WidgetState::Disabled);

        if(flat){
            Draw::fillRect(surf,0,0,rect.w,rect.h,Pal::FACE);
            if(pressed) Draw::drawBevel(surf,0,0,rect.w,rect.h,false);
            else if(hovered) Draw::drawBevel(surf,0,0,rect.w,rect.h,true);
        } else {
            Draw::fillRect(surf,0,0,rect.w,rect.h,Pal::FACE);
            Draw::drawBevel(surf,0,0,rect.w,rect.h,!pressed);
        }
        int ox=pressed?1:0, oy=pressed?1:0;
        Color fg = dis ? Pal::DISABLED_TXT : Pal::TEXT;
        int tw=(int)text.size()*8;
        Draw::drawText(surf,ox+(rect.w-tw)/2,oy+(rect.h-8)/2,text,fg);
        // focus indicator
        if(focused && !flat) Draw::drawFocusRect(surf,4,4,rect.w-8,rect.h-8);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(!enabled) return false;
        if(e.type==EventType::MouseEnter){ setState(WidgetState::Hovered); return true; }
        if(e.type==EventType::MouseLeave){ setState(WidgetState::Normal);  return true; }
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            setState(WidgetState::Pressed); ctx.setFocus(this); return true;
        }
        if(e.type==EventType::MouseUp && e.mbtn==SDL_BUTTON_LEFT){
            if(wstate==WidgetState::Pressed){
                setState(WidgetState::Hovered);
                UIEvent ce; ce.type=EventType::Click;
                ce.mx=e.mx; ce.my=e.my;
                emit(ce);
            }
            return true;
        }
        if(e.type==EventType::KeyDown && focused &&
           (e.key==SDLK_RETURN||e.key==SDLK_SPACE)){
            UIEvent ce; ce.type=EventType::Click; emit(ce);
            return true;
        }
        return false;
    }
};

// ─── UITextInput ─────────────────────────────────────────────────────────────
class UITextInput : public UIComponent {
public:
    std::string text;
    std::string placeholder;
    size_t      cursor=0;
    size_t      selStart=0, selEnd=0;
    int         scrollOff=0; // pixel horizontal scroll
    size_t      maxLen=256;

    UITextInput(const std::string& id_, Rect r, const std::string& ph="")
        : UIComponent(id_,r), placeholder(ph) {}

    const std::string& getText() const { return text; }
    void setText(const std::string& t){ text=t; cursor=t.size(); markDirty(); }

    void onDraw() override {
        // Background
        Draw::fillRect(surf,0,0,rect.w,rect.h,Pal::EDIT_BG);
        Draw::drawSunkenBorder(surf,0,0,rect.w,rect.h);
        // Clip inner area
        int px=3, py=(rect.h-8)/2;
        int innerW=rect.w-8;
        // Draw selection
        if(focused && selStart!=selEnd){
            size_t s=std::min(selStart,selEnd), en=std::max(selStart,selEnd);
            int sx=px+(int)s*8-scrollOff;
            int ew=(int)(en-s)*8;
            if(sx<px+innerW && sx+ew>px)
                Draw::fillRect(surf,std::max(px,sx),py-1,std::min(ew,innerW),10,Pal::SEL_BG);
        }
        // Draw text
        int tx=px-scrollOff;
        if(text.empty() && !focused && !placeholder.empty())
            Draw::drawText(surf,px,py,placeholder,Pal::DISABLED_TXT);
        else {
            for(size_t i=0;i<text.size();i++){
                int cx2=tx+(int)i*8;
                if(cx2+8<px||cx2>=px+innerW) continue;
                bool sel=focused&&i>=std::min(selStart,selEnd)&&i<std::max(selStart,selEnd);
                Draw::drawChar(surf,cx2,py,text[i], sel?Pal::SEL_TXT:Pal::TEXT,
                               sel?Pal::SEL_BG:Pal::EDIT_BG, !sel);
            }
        }
        // Cursor
        if(focused){
            int cx2=px+(int)cursor*8-scrollOff;
            if(cx2>=px&&cx2<px+innerW)
                Draw::drawVLine(surf,cx2,py-1,10,Color{0,0,0});
        }
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(!enabled) return false;
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            ctx.setFocus(this);
            // place cursor
            int rel=e.mx-rect.x-3+scrollOff;
            cursor=(size_t)std::max(0,std::min((int)text.size(), rel/8));
            selStart=selEnd=cursor;
            markDirty(); return true;
        }
        if(e.type==EventType::MouseEnter){ setState(WidgetState::Hovered); return true; }
        if(e.type==EventType::MouseLeave){ setState(WidgetState::Normal);  return true; }
        if(e.type==EventType::TextInput && focused && e.text){
            // Insert at cursor
            std::string ins(e.text);
            if(selStart!=selEnd){
                size_t s=std::min(selStart,selEnd), en=std::max(selStart,selEnd);
                text.erase(s,en-s); cursor=s; selStart=selEnd=cursor;
            }
            if(text.size()<maxLen){
                text.insert(cursor,ins); cursor+=ins.size(); selStart=selEnd=cursor;
                UIEvent ve; ve.type=EventType::ValueChanged; ve.svalue=text; emit(ve);
            }
            scrollToShowCursor(); markDirty(); return true;
        }
        if(e.type==EventType::KeyDown && focused){
            switch(e.key){
                case SDLK_LEFT:  if(cursor>0){cursor--;} selStart=selEnd=cursor; break;
                case SDLK_RIGHT: if(cursor<text.size()){cursor++;} selStart=selEnd=cursor; break;
                case SDLK_HOME:  cursor=0; selStart=selEnd=cursor; break;
                case SDLK_END:   cursor=text.size(); selStart=selEnd=cursor; break;
                case SDLK_BACKSPACE:
                    if(selStart!=selEnd){ size_t s=std::min(selStart,selEnd),en=std::max(selStart,selEnd); text.erase(s,en-s); cursor=s; selStart=selEnd=cursor; }
                    else if(cursor>0){ text.erase(cursor-1,1); cursor--; selStart=selEnd=cursor; }
                    { UIEvent ve; ve.type=EventType::ValueChanged; ve.svalue=text; emit(ve); }
                    break;
                case SDLK_DELETE:
                    if(cursor<text.size()){ text.erase(cursor,1); UIEvent ve; ve.type=EventType::ValueChanged; ve.svalue=text; emit(ve); }
                    break;
                case SDLK_a:
                    if(e.mod&KMOD_CTRL){ selStart=0; selEnd=cursor=text.size(); }
                    break;
                default: break;
            }
            scrollToShowCursor(); markDirty(); return true;
        }
        return false;
    }

private:
    void scrollToShowCursor(){
        int innerW=rect.w-8;
        int cx2=(int)cursor*8-scrollOff;
        if(cx2<0) scrollOff=(int)cursor*8;
        if(cx2>=innerW) scrollOff=(int)cursor*8-innerW+8;
        if(scrollOff<0) scrollOff=0;
    }
};

// ─── UICheckbox ──────────────────────────────────────────────────────────────
class UICheckbox : public UIComponent {
public:
    std::string text;
    bool checked=false;

    UICheckbox(const std::string& id_, Rect r, const std::string& t, bool c=false)
        : UIComponent(id_,r), text(t), checked(c) {}

    bool isChecked() const { return checked; }
    void setChecked(bool c){ if(checked!=c){checked=c;markDirty();} }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        int cy=(rect.h-13)/2;
        // Box
        Draw::fillRect(surf,2,cy,13,13,Pal::EDIT_BG);
        Draw::drawSunkenBorder(surf,2,cy,13,13);
        // Check
        if(checked) Draw::drawCheck(surf,3,cy+1,Pal::TEXT);
        // Focus
        if(focused) Draw::drawFocusRect(surf,1,cy-1,15,15);
        // Label
        Color fg=enabled?Pal::TEXT:Pal::DISABLED_TXT;
        Draw::drawText(surf,20,(rect.h-8)/2,text,fg);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(!enabled) return false;
        if(e.type==EventType::MouseEnter){setState(WidgetState::Hovered);return true;}
        if(e.type==EventType::MouseLeave){setState(WidgetState::Normal);return true;}
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){ctx.setFocus(this);return true;}
        if(e.type==EventType::MouseUp && e.mbtn==SDL_BUTTON_LEFT){
            checked=!checked; markDirty();
            UIEvent ce; ce.type=EventType::CheckChanged; ce.ivalue=checked?1:0; emit(ce);
            return true;
        }
        if(e.type==EventType::KeyDown && focused && e.key==SDLK_SPACE){
            checked=!checked; markDirty();
            UIEvent ce; ce.type=EventType::CheckChanged; ce.ivalue=checked?1:0; emit(ce);
            return true;
        }
        return false;
    }
};

// ─── UIRadioButton ────────────────────────────────────────────────────────────
class UIRadioButton : public UIComponent {
public:
    std::string text;
    std::string group; // radio group name (siblings with same group)
    bool checked=false;

    UIRadioButton(const std::string& id_, Rect r, const std::string& t,
                  const std::string& grp, bool c=false)
        : UIComponent(id_,r), text(t), group(grp), checked(c) {}

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        int cy=(rect.h-13)/2;
        // Draw circle (approximated)
        Draw::fillRect(surf,2,cy,13,13,Pal::EDIT_BG);
        Draw::drawRect(surf,3,cy+1,11,11,Pal::SHADOW);
        Draw::drawRect(surf,4,cy+2, 9, 9,Pal::LIGHT);
        if(checked){
            Draw::fillRect(surf,6,cy+4,5,5,Pal::TEXT);
        }
        if(focused) Draw::drawFocusRect(surf,1,cy-1,15,15);
        Color fg=enabled?Pal::TEXT:Pal::DISABLED_TXT;
        Draw::drawText(surf,20,(rect.h-8)/2,text,fg);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(!enabled) return false;
        if(e.type==EventType::MouseEnter){setState(WidgetState::Hovered);return true;}
        if(e.type==EventType::MouseLeave){setState(WidgetState::Normal);return true;}
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){ctx.setFocus(this);return true;}
        if(e.type==EventType::MouseUp && e.mbtn==SDL_BUTTON_LEFT){
            activateInGroup(ctx);
            return true;
        }
        if(e.type==EventType::KeyDown && focused && e.key==SDLK_SPACE){
            activateInGroup(ctx); return true;
        }
        return false;
    }
private:
    void activateInGroup(UIContext& ctx);
};

// ─── UISlider ────────────────────────────────────────────────────────────────
class UISlider : public UIComponent {
public:
    float value=0.f, minVal=0.f, maxVal=1.f;
    bool  horizontal=true;

    UISlider(const std::string& id_, Rect r, float mn=0.f, float mx=1.f, float v=0.f)
        : UIComponent(id_,r), value(v), minVal(mn), maxVal(mx) {}

    float getValue() const { return value; }
    void  setValue(float v){ v=std::max(minVal,std::min(maxVal,v)); if(v!=value){value=v;markDirty();} }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        // Track
        if(horizontal){
            int ty=rect.h/2-2; int tw=rect.w-16;
            Draw::fillRect(surf,8,ty,tw,4,Pal::SCROLLBAR_BG);
            Draw::drawSunkenBorder(surf,8,ty,tw,4);
            // Thumb
            float t=(maxVal>minVal)?(value-minVal)/(maxVal-minVal):0.f;
            int tx=8+(int)(t*(tw-8));
            Draw::fillRect(surf,tx,rect.h/2-6,8,12,Pal::FACE);
            Draw::drawBevel(surf,tx,rect.h/2-6,8,12,true);
        } else {
            int tx=rect.w/2-2; int th=rect.h-16;
            Draw::fillRect(surf,tx,8,4,th,Pal::SCROLLBAR_BG);
            Draw::drawSunkenBorder(surf,tx,8,4,th);
            float t=(maxVal>minVal)?1.f-(value-minVal)/(maxVal-minVal):0.f;
            int ty2=8+(int)(t*(th-8));
            Draw::fillRect(surf,rect.w/2-6,ty2,12,8,Pal::FACE);
            Draw::drawBevel(surf,rect.w/2-6,ty2,12,8,true);
        }
        if(focused) Draw::drawFocusRect(surf,1,1,rect.w-2,rect.h-2);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(!enabled) return false;
        auto move=[&](int mx2, int my2){
            float t;
            if(horizontal){ int tw=rect.w-16; int rel=mx2-rect.x-8; t=(float)rel/(tw-8); }
            else           { int th=rect.h-16; int rel=my2-rect.y-8; t=1.f-(float)rel/(th-8); }
            float v=minVal+t*(maxVal-minVal);
            float oldv=value; setValue(v);
            if(value!=oldv){ UIEvent ve; ve.type=EventType::ValueChanged; ve.fvalue=value; emit(ve); }
        };
        if(e.type==EventType::MouseEnter){setState(WidgetState::Hovered);return true;}
        if(e.type==EventType::MouseLeave){setState(WidgetState::Normal);return true;}
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            setState(WidgetState::Pressed); ctx.setFocus(this); move(e.mx,e.my); return true;
        }
        if(e.type==EventType::MouseMove && wstate==WidgetState::Pressed){ move(e.mx,e.my); return true; }
        if(e.type==EventType::MouseUp && e.mbtn==SDL_BUTTON_LEFT){ setState(WidgetState::Hovered); return true; }
        if(e.type==EventType::KeyDown && focused){
            float step=(maxVal-minVal)*0.05f;
            float oldv=value;
            if(e.key==SDLK_RIGHT||e.key==SDLK_UP)   setValue(value+step);
            if(e.key==SDLK_LEFT ||e.key==SDLK_DOWN)  setValue(value-step);
            if(value!=oldv){ UIEvent ve; ve.type=EventType::ValueChanged; ve.fvalue=value; emit(ve); return true; }
        }
        return false;
    }
};

// ─── UISpinner (numeric up/down) ──────────────────────────────────────────────
class UISpinner : public UIComponent {
public:
    float value=0.f, minVal=0.f, maxVal=100.f, step=1.f;
    int   decimals=3;

    UISpinner(const std::string& id_, Rect r, float mn=0.f, float mx=100.f,
              float v=0.f, float s=1.f)
        : UIComponent(id_,r), value(v), minVal(mn), maxVal(mx), step(s) {}

    float getValue() const { return value; }
    void  setValue(float v){ v=std::max(minVal,std::min(maxVal,v)); if(v!=value){value=v;markDirty();} }

    void onDraw() override {
        int bw=16;
        // Edit area
        Draw::fillRect(surf,0,0,rect.w-bw,rect.h,Pal::EDIT_BG);
        Draw::drawSunkenBorder(surf,0,0,rect.w-bw,rect.h);
        char buf[32]; snprintf(buf,sizeof(buf),"%.*f",decimals,value);
        Draw::drawTextLeft(surf,0,0,rect.h,buf,enabled?Pal::TEXT:Pal::DISABLED_TXT,3);
        // Buttons
        int half=rect.h/2;
        Draw::fillRect(surf,rect.w-bw,0,bw,half,Pal::FACE);
        Draw::drawBevel(surf,rect.w-bw,0,bw,half,true);
        Draw::fillRect(surf,rect.w-bw,half,bw,rect.h-half,Pal::FACE);
        Draw::drawBevel(surf,rect.w-bw,half,bw,rect.h-half,true);
        // Arrows
        Draw::drawArrowUp(surf,rect.w-bw+4,2,4,Pal::TEXT);
        Draw::drawArrowDown(surf,rect.w-bw+4,half+4,4,Pal::TEXT);
        if(focused) Draw::drawFocusRect(surf,1,1,rect.w-bw-2,rect.h-2);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(!enabled) return false;
        auto tryChange=[&](float delta){
            float oldv=value; setValue(value+delta);
            if(value!=oldv){ UIEvent ve; ve.type=EventType::ValueChanged; ve.fvalue=value; emit(ve); }
        };
        if(e.type==EventType::MouseEnter){setState(WidgetState::Hovered);return true;}
        if(e.type==EventType::MouseLeave){setState(WidgetState::Normal);return true;}
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            ctx.setFocus(this);
            int bw=16, half=rect.h/2;
            int lx=e.mx-rect.x, ly=e.my-rect.y;
            if(lx>=rect.w-bw){
                if(ly<half)  tryChange(+step);
                else         tryChange(-step);
            }
            return true;
        }
        if(e.type==EventType::MouseWheel && focused){
            tryChange(e.wheel*step); return true;
        }
        if(e.type==EventType::KeyDown && focused){
            if(e.key==SDLK_UP)   { tryChange(+step); return true; }
            if(e.key==SDLK_DOWN) { tryChange(-step); return true; }
        }
        return false;
    }
};

// ─── UIScrollBar ──────────────────────────────────────────────────────────────
class UIScrollBar : public UIComponent {
public:
    bool  horizontal=false;
    float value=0.f;        // 0..1
    float thumbRatio=0.2f;  // thumb/track ratio
    bool  dragging=false;
    int   dragStart=0;
    float dragVal=0.f;

    UIScrollBar(const std::string& id_, Rect r, bool horiz=false)
        : UIComponent(id_,r), horizontal(horiz) {}

    void setValue(float v){ v=std::max(0.f,std::min(1.f,v)); if(v!=value){value=v;markDirty();} }
    float getValue() const { return value; }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,Pal::SCROLLBAR_BG);
        int btnSz=15;
        if(horizontal){
            // Left button
            Draw::fillRect(surf,0,0,btnSz,rect.h,Pal::FACE);
            Draw::drawBevel(surf,0,0,btnSz,rect.h,true);
            Draw::drawArrowLeft(surf,4,rect.h/2-3,5,Pal::TEXT);
            // Right button
            Draw::fillRect(surf,rect.w-btnSz,0,btnSz,rect.h,Pal::FACE);
            Draw::drawBevel(surf,rect.w-btnSz,0,btnSz,rect.h,true);
            Draw::drawArrowRight(surf,rect.w-btnSz+4,rect.h/2-3,5,Pal::TEXT);
            // Thumb
            int trackW=rect.w-btnSz*2;
            int tw=std::max(15,(int)(trackW*thumbRatio));
            int tx=btnSz+(int)((trackW-tw)*value);
            Draw::fillRect(surf,tx,1,tw,rect.h-2,Pal::FACE);
            Draw::drawBevel(surf,tx,1,tw,rect.h-2,true);
        } else {
            Draw::fillRect(surf,0,0,rect.w,btnSz,Pal::FACE);
            Draw::drawBevel(surf,0,0,rect.w,btnSz,true);
            Draw::drawArrowUp(surf,rect.w/2-3,4,4,Pal::TEXT);
            Draw::fillRect(surf,0,rect.h-btnSz,rect.w,btnSz,Pal::FACE);
            Draw::drawBevel(surf,0,rect.h-btnSz,rect.w,btnSz,true);
            Draw::drawArrowDown(surf,rect.w/2-3,rect.h-btnSz+4,4,Pal::TEXT);
            int trackH=rect.h-btnSz*2;
            int th=std::max(15,(int)(trackH*thumbRatio));
            int ty2=btnSz+(int)((trackH-th)*value);
            Draw::fillRect(surf,1,ty2,rect.w-2,th,Pal::FACE);
            Draw::drawBevel(surf,1,ty2,rect.w-2,th,true);
        }
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(!enabled) return false;
        int btnSz=15;
        auto fireScroll=[&](float v){ setValue(v); UIEvent se; se.type=EventType::Scroll; se.fvalue=value; emit(se); };
        if(e.type==EventType::MouseEnter){setState(WidgetState::Hovered);return true;}
        if(e.type==EventType::MouseLeave){setState(WidgetState::Normal);dragging=false;return true;}
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            ctx.setFocus(this);
            if(horizontal){
                int lx=e.mx-rect.x;
                if(lx<btnSz) fireScroll(std::max(0.f,value-0.1f));
                else if(lx>rect.w-btnSz) fireScroll(std::min(1.f,value+0.1f));
                else { dragging=true; dragStart=lx; dragVal=value; }
            } else {
                int ly=e.my-rect.y;
                if(ly<btnSz) fireScroll(std::max(0.f,value-0.1f));
                else if(ly>rect.h-btnSz) fireScroll(std::min(1.f,value+0.1f));
                else { dragging=true; dragStart=ly; dragVal=value; }
            }
            return true;
        }
        if(e.type==EventType::MouseMove && dragging){
            int trackSz=(horizontal?rect.w:rect.h)-btnSz*2;
            int thumbSz=std::max(15,(int)(trackSz*thumbRatio));
            int travel=trackSz-thumbSz;
            if(travel>0){
                int delta=horizontal?(e.mx-rect.x-dragStart):(e.my-rect.y-dragStart);
                fireScroll(std::max(0.f,std::min(1.f,dragVal+(float)delta/travel)));
            }
            return true;
        }
        if(e.type==EventType::MouseUp && e.mbtn==SDL_BUTTON_LEFT){ dragging=false; return true; }
        if(e.type==EventType::MouseWheel){
            fireScroll(std::max(0.f,std::min(1.f,value-e.wheel*0.1f))); return true;
        }
        return false;
    }
};

// ─── UIGroupBox ───────────────────────────────────────────────────────────────
class UIGroupBox : public UIComponent {
public:
    std::string title;

    UIGroupBox(const std::string& id_, Rect r, const std::string& t)
        : UIComponent(id_,r), title(t) {}

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        int titleW=(int)title.size()*8+4;
        // Border (with gap for title)
        Draw::drawHLine(surf,10,8,titleW+4,bgColor);  // erase gap
        // Draw sunken box
        Draw::drawHLine(surf,0,8,10,Pal::SHADOW);
        Draw::drawHLine(surf,10+titleW+4,8,rect.w-(10+titleW+4),Pal::SHADOW);
        Draw::drawHLine(surf,0,rect.h-1,rect.w,Pal::SHADOW);
        Draw::drawVLine(surf,0,8,rect.h-8,Pal::SHADOW);
        Draw::drawVLine(surf,rect.w-1,8,rect.h-8,Pal::SHADOW);
        // Highlight offset
        Draw::drawHLine(surf,1,9,9,Pal::LIGHT);
        Draw::drawHLine(surf,10+titleW+4,9,rect.w-(10+titleW+5),Pal::LIGHT);
        Draw::drawHLine(surf,1,rect.h,rect.w-1,Pal::LIGHT);
        Draw::drawVLine(surf,1,9,rect.h-10,Pal::LIGHT);
        Draw::drawVLine(surf,rect.w,9,rect.h-9,Pal::LIGHT);
        // Title
        Draw::drawText(surf,14,1,title,Pal::TEXT);
    }
};

// ─── UISeparator ──────────────────────────────────────────────────────────────
class UISeparator : public UIComponent {
public:
    bool horizontal=true;
    UISeparator(const std::string& id_, Rect r, bool horiz=true)
        : UIComponent(id_,r), horizontal(horiz) {}

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        if(horizontal){
            int y=rect.h/2;
            Draw::drawHLine(surf,0,y,  rect.w,Pal::SHADOW);
            Draw::drawHLine(surf,0,y+1,rect.w,Pal::LIGHT);
        } else {
            int x=rect.w/2;
            Draw::drawVLine(surf,x,  0,rect.h,Pal::SHADOW);
            Draw::drawVLine(surf,x+1,0,rect.h,Pal::LIGHT);
        }
    }
};

// ─── UIViewport3D (software render viewport for 3D editor) ───────────────────
class UIViewport3D : public UIComponent {
public:
    std::string viewLabel = "Perspective";
    bool        vpActive  = false;  // is this the active/focused viewport?

    // User-supplied software framebuffer (must be rect.w × rect.h × 4 bytes ARGB)
    // If null, a grey default is drawn
    SDL_Surface* customFB = nullptr;

    UIViewport3D(const std::string& id_, Rect r, const std::string& label="Perspective")
        : UIComponent(id_,r), viewLabel(label) {}

    // Call this to feed your software-rendered pixel data
    void updatePixels(const uint32_t* argbData, int w, int h) {
        if(!customFB||customFB->w!=w||customFB->h!=h){
            if(customFB) SDL_FreeSurface(customFB);
            customFB=SDL_CreateRGBSurface(0,w,h,32,0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
        }
        SDL_LockSurface(customFB);
        memcpy(customFB->pixels,argbData,(size_t)w*h*4);
        SDL_UnlockSurface(customFB);
        markDirty();
    }

    void setActive(bool a){
        if(vpActive!=a){ vpActive=a; markDirty();
            UIEvent e; e.type=a?EventType::ViewportFocusGained:EventType::ViewportFocusLost;
            emit(e);
        }
    }
    bool isActive() const { return vpActive; }

    void onDraw() override {
        // Draw framebuffer or solid bg
        if(customFB){
            SDL_BlitSurface(customFB,nullptr,surf,nullptr);
        } else {
            Draw::fillRect(surf,0,0,rect.w,rect.h,Pal::VP_BG);
            // Crosshair guides
            Draw::drawHLine(surf,0,rect.h/2,rect.w,Color{90,90,90});
            Draw::drawVLine(surf,rect.w/2,0,rect.h,Color{90,90,90});
        }
        // Border
        Color bc = vpActive ? Pal::VP_BORDER_ON : Pal::VP_BORDER_OFF;
        Draw::drawRect(surf,0,0,rect.w,rect.h,bc);
        if(vpActive) Draw::drawRect(surf,1,1,rect.w-2,rect.h-2,bc);
        // Label (top-left)
        Draw::fillRect(surf,2,2,(int)viewLabel.size()*8+4,12,Color{0,0,0,128});
        Draw::drawText(surf,4,3,viewLabel,Pal::VP_LABEL);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            ctx.setFocus(this);
            if(!vpActive){
                setActive(true);
                // deactivate siblings that are UIViewport3D
                // (parent should manage this, but we notify context)
                ctx.activeViewport=this;
            }
            return true;
        }
        if(e.type==EventType::FocusLost){ /* keep vpActive; focus ≠ viewport active */ return false; }
        return false;
    }
};

// ─── UITitleBar (for floating windows) ───────────────────────────────────────
class UITitleBar : public UIComponent {
public:
    std::string title;
    bool dragging=false;
    int  dragOffX=0, dragOffY=0;
    UIComponent* window=nullptr; // the window this title bar belongs to

    UITitleBar(const std::string& id_, Rect r, const std::string& t)
        : UIComponent(id_,r), title(t) {}

    void onDraw() override {
        Draw::drawGradientH(surf,0,0,rect.w,rect.h,Pal::TITLE_L,Pal::TITLE_R);
        Draw::drawText(surf,6,(rect.h-8)/2,title,Pal::SEL_TXT);
        // Close button area
        int bx=rect.w-18, by=2, bw=14, bh=rect.h-4;
        Draw::fillRect(surf,bx,by,bw,bh,Color{200,80,80});
        Draw::drawBevel(surf,bx,by,bw,bh,true);
        Draw::drawText(surf,bx+3,by+(bh-8)/2,"x",Pal::SEL_TXT);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            int bx=rect.x+rect.w-18;
            if(e.mx>=bx){ // close
                UIEvent ce; ce.type=EventType::Click; ce.svalue="close"; emit(ce);
            } else {
                dragging=true; dragOffX=e.mx-rect.x; dragOffY=e.my-rect.y;
            }
            ctx.setFocus(this); return true;
        }
        if(e.type==EventType::MouseMove && dragging && window){
            int nx=e.mx-dragOffX, ny=e.my-dragOffY;
            Rect wr=window->rect;
            int dx=nx-wr.x, dy=ny-wr.y;
            window->rect.x+=dx; window->rect.y+=dy;
            // update all children rects
            std::function<void(UIComponent*,int,int)> moveChildren;
            moveChildren=[&](UIComponent* c, int ddx, int ddy){
                for(auto& ch: c->children){
                    ch->rect.x+=ddx; ch->rect.y+=ddy;
                    moveChildren(ch.get(),ddx,ddy);
                }
            };
            moveChildren(window,dx,dy);
            window->markDirty();
            ctx.needsRedraw=true;
            return true;
        }
        if(e.type==EventType::MouseUp && e.mbtn==SDL_BUTTON_LEFT){ dragging=false; return true; }
        return false;
    }
};

// ─── UIWindow (floating panel with title bar) ─────────────────────────────────
class UIWindow : public UIComponent {
public:
    UITitleBar* titleBar=nullptr;
    UIPanel*    body=nullptr;

    UIWindow(const std::string& id_, Rect r, const std::string& title,
             UIContext& ctx, bool dark=false)
        : UIComponent(id_,r) {
        layer=10; darkMode=dark;
        bgColor=dark?Pal::DARK_PANEL:Pal::FACE;

        auto tb=std::unique_ptr<UITitleBar>(new UITitleBar(id_+"_tb",
            Rect(r.x,r.y,r.w,20),title));
        tb->window=this;
        titleBar=static_cast<UITitleBar*>(addChild(std::move(tb),ctx));

        auto bd=std::unique_ptr<UIPanel>(new UIPanel(id_+"_body",
            Rect(r.x,r.y+20,r.w,r.h-20),dark));
        body=static_cast<UIPanel*>(addChild(std::move(bd),ctx));
    }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        Draw::drawBevel(surf,0,0,rect.w,rect.h,true);
    }

    // Convenience: add to body
    UIComponent* addToBody(std::unique_ptr<UIComponent> c, UIContext& ctx){
        return body->addChild(std::move(c),ctx);
    }
};

// ─── UIMenuBar ────────────────────────────────────────────────────────────────
struct MenuItem {
    std::string text;
    std::string id;
    bool separator=false;
    bool disabled=false;
    std::vector<MenuItem> submenu;
    MenuItem(){}
    MenuItem(const std::string& t, const std::string& i, bool sep=false, bool dis=false)
        : text(t), id(i), separator(sep), disabled(dis) {}
};

class UIMenuDropdown : public UIComponent {
public:
    std::vector<MenuItem> items;
    int hoveredIdx=-1;

    UIMenuDropdown(const std::string& id_, Rect r, std::vector<MenuItem> its)
        : UIComponent(id_,r), items(std::move(its)) { layer=100; }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,Pal::FACE);
        Draw::drawBevel(surf,0,0,rect.w,rect.h,true);
        int y=2;
        for(int i=0;i<(int)items.size();i++){
            auto& it=items[i];
            if(it.separator){
                Draw::drawHLine(surf,2,y+3,rect.w-4,Pal::SHADOW);
                Draw::drawHLine(surf,2,y+4,rect.w-4,Pal::LIGHT);
                y+=8; continue;
            }
            if(i==hoveredIdx && !it.disabled){
                Draw::fillRect(surf,2,y,rect.w-4,16,Pal::SEL_BG);
                Draw::drawText(surf,6,y+4,it.text,Pal::SEL_TXT);
            } else {
                Color fg=it.disabled?Pal::DISABLED_TXT:Pal::TEXT;
                Draw::drawText(surf,6,y+4,it.text,fg);
            }
            y+=16;
        }
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(e.type==EventType::MouseMove){
            int ly=e.my-rect.y-2, y=0;
            hoveredIdx=-1;
            for(int i=0;i<(int)items.size();i++){
                if(items[i].separator){y+=8;continue;}
                if(ly>=y&&ly<y+16){hoveredIdx=i;break;}
                y+=16;
            }
            markDirty(); return true;
        }
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            if(hoveredIdx>=0 && !items[hoveredIdx].disabled){
                UIEvent ce; ce.type=EventType::MenuItemClicked;
                ce.svalue=items[hoveredIdx].id; ce.ivalue=hoveredIdx;
                emit(ce);
            }
            return true;
        }
        return false;
    }

    static int calcHeight(const std::vector<MenuItem>& its){
        int h=4;
        for(auto& it: its) h+=it.separator?8:16;
        return h;
    }
};

class UIMenuBar : public UIComponent {
public:
    struct Menu { std::string title; std::vector<MenuItem> items; };
    std::vector<Menu> menus;
    int openIdx=-1;
    UIMenuDropdown* dropdown=nullptr;

    UIMenuBar(const std::string& id_, Rect r)
        : UIComponent(id_,r) {}

    void addMenu(const std::string& title, std::vector<MenuItem> items){
        menus.push_back({title,std::move(items)});
        markDirty();
    }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,Pal::FACE);
        Draw::drawHLine(surf,0,rect.h-1,rect.w,Pal::SHADOW);
        int x=2;
        for(int i=0;i<(int)menus.size();i++){
            int tw=(int)menus[i].title.size()*8+8;
            if(i==openIdx){
                Draw::fillRect(surf,x-1,0,tw+2,rect.h,Pal::SEL_BG);
                Draw::drawText(surf,x+3,(rect.h-8)/2,menus[i].title,Pal::SEL_TXT);
            } else {
                Draw::drawText(surf,x+3,(rect.h-8)/2,menus[i].title,Pal::TEXT);
            }
            x+=tw+4;
        }
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override;
};

// ─── UIToolbar ────────────────────────────────────────────────────────────────
class UIToolbar : public UIComponent {
public:
    UIToolbar(const std::string& id_, Rect r, bool dark=false)
        : UIComponent(id_,r) {
        darkMode=dark;
        bgColor=dark?Pal::DARK_PANEL:Pal::FACE;
    }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        Draw::drawHLine(surf,0,rect.h-1,rect.w,Pal::SHADOW);
        Draw::drawHLine(surf,0,0,rect.w,Pal::LIGHT);
    }

    // Quick helpers to add flat buttons
    UIButton* addButton(const std::string& id_, int x, int w, const std::string& label,
                        UIContext& ctx){
        auto b=std::unique_ptr<UIButton>(new UIButton(id_,Rect(rect.x+x,rect.y+2,w,rect.h-4),label));
        b->flat=true;
        return static_cast<UIButton*>(addChild(std::move(b),ctx));
    }
};

// ─── UITabPanel ───────────────────────────────────────────────────────────────
class UITabPanel : public UIComponent {
public:
    struct Tab { std::string title; std::string panelId; };
    std::vector<Tab> tabs;
    int activeTab=0;
    int tabHeight=20;

    UITabPanel(const std::string& id_, Rect r) : UIComponent(id_,r) {}

    void addTab(const std::string& title, const std::string& panelId, UIContext& ctx){
        tabs.push_back({title,panelId});
        markDirty();
    }

    void setActiveTab(int idx, UIContext& ctx){
        if(idx==activeTab||idx<0||idx>=(int)tabs.size()) return;
        activeTab=idx;
        // show/hide panels
        for(int i=0;i<(int)tabs.size();i++){
            auto* p=ctx.findById(tabs[i].panelId);
            if(p) p->setVisible(i==activeTab);
        }
        markDirty();
        UIEvent e; e.type=EventType::ValueChanged; e.ivalue=activeTab; emit(e);
    }

    void onDraw() override {
        Draw::fillRect(surf,0,0,rect.w,rect.h,bgColor);
        // Tab strip
        int x=0;
        for(int i=0;i<(int)tabs.size();i++){
            int tw=(int)tabs[i].title.size()*8+12;
            bool active=(i==activeTab);
            if(active){
                Draw::fillRect(surf,x,0,tw,tabHeight+1,Pal::FACE);
                Draw::drawHLine(surf,x,0,tw,Pal::SHADOW);
                Draw::drawVLine(surf,x,0,tabHeight,Pal::SHADOW);
                Draw::drawVLine(surf,x+tw-1,0,tabHeight,Pal::SHADOW);
                Draw::drawHLine(surf,x+1,1,tw-2,Pal::LIGHT);
            } else {
                Draw::fillRect(surf,x,2,tw,tabHeight-2,Pal::SCROLLBAR_BG);
                Draw::drawRect(surf,x,2,tw,tabHeight-1,Pal::SHADOW);
            }
            Draw::drawText(surf,x+6,(tabHeight-8)/2,tabs[i].title,Pal::TEXT);
            x+=tw;
        }
        // Bottom border
        Draw::drawHLine(surf,0,tabHeight,rect.w,Pal::SHADOW);
        Draw::drawHLine(surf,0,tabHeight+1,rect.w,Pal::LIGHT);
    }

    bool onEvent(const UIEvent& e, UIContext& ctx) override {
        if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
            int lx=e.mx-rect.x, x=0;
            for(int i=0;i<(int)tabs.size();i++){
                int tw=(int)tabs[i].title.size()*8+12;
                if(lx>=x&&lx<x+tw&&(e.my-rect.y)<tabHeight){
                    setActiveTab(i,ctx); return true;
                }
                x+=tw;
            }
        }
        return false;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 8 – Event dispatch implementation
// ═══════════════════════════════════════════════════════════════════════════

inline void UIRadioButton::activateInGroup(UIContext& ctx) {
    // Find all siblings with same group and deactivate
    auto deactivateGroup=[&](UIComponent* container){
        for(auto& ch: container->children){
            auto* rb=dynamic_cast<UIRadioButton*>(ch.get());
            if(rb && rb->group==group && rb!=this){
                rb->checked=false; rb->markDirty();
            }
        }
    };
    if(parent) deactivateGroup(parent);
    else {
        for(auto& w: ctx.widgets){
            auto* rb=dynamic_cast<UIRadioButton*>(w.get());
            if(rb && rb->group==group && rb!=this){
                rb->checked=false; rb->markDirty();
            }
        }
    }
    checked=true; markDirty();
    UIEvent ce; ce.type=EventType::CheckChanged; ce.ivalue=1; emit(ce);
}

inline bool UIMenuBar::onEvent(const UIEvent& e, UIContext& ctx) {
    if(e.type==EventType::MouseDown && e.mbtn==SDL_BUTTON_LEFT){
        int lx=e.mx-rect.x, x=2;
        for(int i=0;i<(int)menus.size();i++){
            int tw=(int)menus[i].title.size()*8+12;
            if(lx>=x&&lx<x+tw){
                if(openIdx==i){
                    openIdx=-1; if(dropdown){ctx.remove(dropdown->id);dropdown=nullptr;}
                } else {
                    if(dropdown){ctx.remove(dropdown->id);dropdown=nullptr;}
                    openIdx=i;
                    // compute screen x
                    int sx=rect.x+x-2;
                    int h=UIMenuDropdown::calcHeight(menus[i].items);
                    int maxW=120;
                    for(auto& it: menus[i].items)
                        maxW=std::max(maxW,(int)it.text.size()*8+16);
                    auto dd=std::unique_ptr<UIMenuDropdown>(new UIMenuDropdown(
                        id+"_dd",
                        Rect(sx,rect.y+rect.h,maxW,h),
                        menus[i].items));
                    dd->layer=200;
                    // wire close
                    UIMenuBar* mb=this;
                    dd->on(EventType::MenuItemClicked,[mb,&ctx,this](UIComponent* c, const UIEvent& ev){
                        emit(ev); // bubble to menubar listeners
                        openIdx=-1; dropdown=nullptr; ctx.remove(c->id);
                    });
                    dropdown=static_cast<UIMenuDropdown*>(ctx.add(std::move(dd)));
                }
                markDirty(); return true;
            }
            x+=tw+4;
        }
        // click outside menus
        if(openIdx>=0){
            openIdx=-1; if(dropdown){ctx.remove(dropdown->id);dropdown=nullptr;}
            markDirty();
        }
    }
    return false;
}

// ─── UIContext::processEvent ───────────────────────────────────────────────────
inline void UIContext::processEvent(const SDL_Event& sdl_ev) {
    needsRedraw=true;

    // Helper: dispatch to widget and its parents
    auto dispatch=[&](UIComponent* w, const UIEvent& e) -> bool {
        if(!w||!w->enabled) return false;
        return w->onEvent(e,*this);
    };

    switch(sdl_ev.type){

    case SDL_MOUSEMOTION: {
        int mx=sdl_ev.motion.x, my=sdl_ev.motion.y;
        UIComponent* hit=hitTest(mx,my);
        if(hit!=hoveredWidget){
            if(hoveredWidget){
                UIEvent e; e.type=EventType::MouseLeave; e.mx=mx; e.my=my;
                dispatch(hoveredWidget,e);
            }
            hoveredWidget=hit;
            if(hoveredWidget){
                UIEvent e; e.type=EventType::MouseEnter; e.mx=mx; e.my=my;
                dispatch(hoveredWidget,e);
            }
        }
        // Mouse move (for drags)
        if(pressedWidget){
            UIEvent e; e.type=EventType::MouseMove; e.mx=mx; e.my=my;
            dispatch(pressedWidget,e);
        } else if(hit){
            UIEvent e; e.type=EventType::MouseMove; e.mx=mx; e.my=my;
            dispatch(hit,e);
        }
    } break;

    case SDL_MOUSEBUTTONDOWN: {
        int mx=sdl_ev.button.x, my=sdl_ev.button.y;
        UIComponent* hit=hitTest(mx,my);
        pressedWidget=hit;
        // Close open menus if click is not on them
        if(openMenu && hit!=openMenu){
            UIEvent e; e.type=EventType::Click; e.svalue="close";
            openMenu->emit(e);
            openMenu=nullptr;
        }
        // Double-click
        if(sdl_ev.button.clicks >= 2){
            UIEvent e; e.type=EventType::DblClick; e.mx=mx; e.my=my;
            e.mbtn=sdl_ev.button.button;
            dispatch(hit,e);
        }
        UIEvent e; e.type=EventType::MouseDown; e.mx=mx; e.my=my;
        e.mbtn=sdl_ev.button.button;
        dispatch(hit,e);
    } break;

    case SDL_MOUSEBUTTONUP: {
        int mx=sdl_ev.button.x, my=sdl_ev.button.y;
        UIEvent e; e.type=EventType::MouseUp; e.mx=mx; e.my=my;
        e.mbtn=sdl_ev.button.button;
        if(pressedWidget) dispatch(pressedWidget,e);
        pressedWidget=nullptr;
    } break;

    case SDL_MOUSEWHEEL: {
        int mx,my; SDL_GetMouseState(&mx,&my);
        UIComponent* hit=hitTest(mx,my);
        UIEvent e; e.type=EventType::MouseWheel; e.mx=mx; e.my=my;
        e.wheel=sdl_ev.wheel.y;
        dispatch(hit,e);
    } break;

    case SDL_KEYDOWN: {
        UIEvent e; e.type=EventType::KeyDown;
        e.key=sdl_ev.key.keysym.sym;
        e.mod=(uint16_t)sdl_ev.key.keysym.mod;
        if(focusedWidget) dispatch(focusedWidget,e);
        // Tab navigation
        if(sdl_ev.key.keysym.sym==SDLK_TAB && !widgets.empty()){
            // find next focusable widget
            std::vector<UIComponent*> focusable;
            std::function<void(UIComponent*)> collect=[&](UIComponent* c){
                if(c->enabled&&c->visible) focusable.push_back(c);
                for(auto& ch: c->children) collect(ch.get());
            };
            for(auto& w: widgets) collect(w.get());
            if(!focusable.empty()){
                int ci=-1;
                for(int i=0;i<(int)focusable.size();i++)
                    if(focusable[i]==focusedWidget){ci=i;break;}
                bool shift=sdl_ev.key.keysym.mod&KMOD_SHIFT;
                ci=shift?(ci-1+(int)focusable.size()):((ci+1)%(int)focusable.size());
                setFocus(focusable[ci]);
            }
        }
    } break;

    case SDL_KEYUP: {
        UIEvent e; e.type=EventType::KeyUp;
        e.key=sdl_ev.key.keysym.sym;
        e.mod=(uint16_t)sdl_ev.key.keysym.mod;
        if(focusedWidget) dispatch(focusedWidget,e);
    } break;

    case SDL_TEXTINPUT: {
        UIEvent e; e.type=EventType::TextInput;
        e.text=sdl_ev.text.text;
        if(focusedWidget) dispatch(focusedWidget,e);
    } break;

    case SDL_WINDOWEVENT:
        if(sdl_ev.window.event==SDL_WINDOWEVENT_RESIZED)
            resize(sdl_ev.window.data1,sdl_ev.window.data2);
    break;

    default: break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  SECTION 9 – Builder helpers (fluent API)
// ═══════════════════════════════════════════════════════════════════════════

// Factory namespace so you don't have to new everything manually
namespace Make {

inline std::unique_ptr<UIPanel> Panel(const std::string& id, Rect r, bool dark=false){
    return std::unique_ptr<UIPanel>(new UIPanel(id,r,dark));
}
inline std::unique_ptr<UIButton> Button(const std::string& id, Rect r, const std::string& label){
    return std::unique_ptr<UIButton>(new UIButton(id,r,label));
}
inline std::unique_ptr<UILabel> Label(const std::string& id, Rect r, const std::string& text, bool dark=false){
    return std::unique_ptr<UILabel>(new UILabel(id,r,text,dark));
}
inline std::unique_ptr<UITextInput> TextInput(const std::string& id, Rect r, const std::string& ph=""){
    return std::unique_ptr<UITextInput>(new UITextInput(id,r,ph));
}
inline std::unique_ptr<UICheckbox> Checkbox(const std::string& id, Rect r, const std::string& label, bool checked=false){
    return std::unique_ptr<UICheckbox>(new UICheckbox(id,r,label,checked));
}
inline std::unique_ptr<UIRadioButton> Radio(const std::string& id, Rect r, const std::string& label, const std::string& group, bool checked=false){
    return std::unique_ptr<UIRadioButton>(new UIRadioButton(id,r,label,group,checked));
}
inline std::unique_ptr<UISlider> Slider(const std::string& id, Rect r, float mn=0.f, float mx=1.f, float v=0.f){
    return std::unique_ptr<UISlider>(new UISlider(id,r,mn,mx,v));
}
inline std::unique_ptr<UISpinner> Spinner(const std::string& id, Rect r, float mn=0.f, float mx=100.f, float v=0.f, float step=1.f){
    return std::unique_ptr<UISpinner>(new UISpinner(id,r,mn,mx,v,step));
}
inline std::unique_ptr<UIScrollBar> ScrollBar(const std::string& id, Rect r, bool horiz=false){
    return std::unique_ptr<UIScrollBar>(new UIScrollBar(id,r,horiz));
}
inline std::unique_ptr<UIGroupBox> GroupBox(const std::string& id, Rect r, const std::string& title){
    return std::unique_ptr<UIGroupBox>(new UIGroupBox(id,r,title));
}
inline std::unique_ptr<UISeparator> Separator(const std::string& id, Rect r, bool horiz=true){
    return std::unique_ptr<UISeparator>(new UISeparator(id,r,horiz));
}
inline std::unique_ptr<UIViewport3D> Viewport(const std::string& id, Rect r, const std::string& label="Perspective"){
    return std::unique_ptr<UIViewport3D>(new UIViewport3D(id,r,label));
}
inline std::unique_ptr<UIMenuBar> MenuBar(const std::string& id, Rect r){
    return std::unique_ptr<UIMenuBar>(new UIMenuBar(id,r));
}
inline std::unique_ptr<UIToolbar> Toolbar(const std::string& id, Rect r, bool dark=false){
    return std::unique_ptr<UIToolbar>(new UIToolbar(id,r,dark));
}
inline std::unique_ptr<UITabPanel> TabPanel(const std::string& id, Rect r){
    return std::unique_ptr<UITabPanel>(new UITabPanel(id,r));
}
inline std::unique_ptr<UIWindow> Window(const std::string& id, Rect r, const std::string& title, UIContext& ctx, bool dark=false){
    return std::unique_ptr<UIWindow>(new UIWindow(id,r,title,ctx,dark));
}

} // namespace Make

} // namespace WXUI
