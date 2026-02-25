/*
 * 3d_editor.cpp  –  3d multi-object editor  (winxp_ui.hpp)
 *
 * Features:
 *  - 9 primitive types: Sphere, Box, Cylinder, Cone, Torus, Plane,
 *                       GeoSphere, Tube, Pyramid
 *  - Multiple objects in scene, each with independent transform
 *  - Click to select (yellow wireframe on selected object)
 *  - Select / Move / Rotate / Scale per object
 *  - Per-viewport wireframe toggle via RMB menu
 *  - Mouse-wheel zoom, MMB pan, LMB orbit (persp), dbl-click maximize
 *  - Window resize updates all framebuffers
 *
 * Build (Linux):
 *   g++ -std=c++11 3d_editor.cpp -o demo $(sdl2-config --cflags --libs) -lm
 */
#include "winxp_ui.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>

using namespace WXUI;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ═══════════════════════════════════════════════════════════════════
//  ENUMS
// ═══════════════════════════════════════════════════════════════════
enum class ToolMode { Select, Move, Rotate, Scale };
enum class ViewType { Perspective, Top, Front, Back, Left, Right };
enum class PrimType { Sphere, Box, Cylinder, Cone, Torus, Plane, GeoSphere, Tube, Pyramid };

static const char* viewTypeName(ViewType v){
    switch(v){
        case ViewType::Perspective:return"Perspective";case ViewType::Top:return"Top";
        case ViewType::Front:return"Front";case ViewType::Back:return"Back";
        case ViewType::Left:return"Left";case ViewType::Right:return"Right";
    }return"Perspective";
}
static bool isOrtho(ViewType v){return v!=ViewType::Perspective;}

// ═══════════════════════════════════════════════════════════════════
//  SCENE OBJECT
// ═══════════════════════════════════════════════════════════════════
static int g_objCounter=0;

struct SceneObject {
    int       id;
    std::string name;
    PrimType  type;
    // Transform
    float     x=0,y=0,z=0;
    float     rotY=0;
    float     scale=1.f;
    // Shared params
    float     radius=50.f;
    float     height=100.f;
    float     radius2=30.f;   // tube inner / torus tube-radius
    int       segsU=8;        // lat / height segs / plane segsX
    int       segsV=8;        // lon / cap segs  / plane segsZ
    bool      smooth=true;

    SceneObject(PrimType t, const std::string& nm)
        :id(++g_objCounter),name(nm),type(t){}
};

// ═══════════════════════════════════════════════════════════════════
//  PER-VIEWPORT CAMERA
// ═══════════════════════════════════════════════════════════════════
struct VPCamera {
    float az=0.4f, el=0.35f, zoom=1.f, panX=0, panY=0;
    ViewType viewType=ViewType::Perspective;
};

// ═══════════════════════════════════════════════════════════════════
//  APP STATE
// ═══════════════════════════════════════════════════════════════════
struct AppState {
    std::vector<std::unique_ptr<SceneObject>> objects;
    int       selectedId=-1;   // -1 = none
    // pending type for next Create
    PrimType  pendingType=PrimType::Sphere;
    std::string pendingName;

    ToolMode  tool=ToolMode::Select;
    int       maximised=-1;
    bool      wireframe[4]={false,true,true,true};
    VPCamera  cam[4];
    std::string statusMsg="Click a primitive type, set params, press Create";

    AppState(){
        cam[0].viewType=ViewType::Perspective;
        cam[1].viewType=ViewType::Top;
        cam[2].viewType=ViewType::Front;
        cam[3].viewType=ViewType::Left;
    }
    SceneObject* selected(){
        for(auto& o:objects) if(o->id==selectedId) return o.get();
        return nullptr;
    }
    SceneObject* findClosest(float wx,float wy,float wz,float threshold){
        SceneObject* best=nullptr; float bestD=threshold*threshold;
        for(auto& o:objects){
            float dx=o->x-wx,dy=o->y-wy,dz=o->z-wz;
            float d2=dx*dx+dy*dy+dz*dz;
            if(d2<bestD){bestD=d2;best=o.get();}
        }
        return best;
    }
};

// ═══════════════════════════════════════════════════════════════════
//  CONTEXT MENU (view-type popup)
// ═══════════════════════════════════════════════════════════════════
struct CtxMenu{
    bool open=false; int x=0,y=0,forVP=-1,hovered=-1;
    static const int IH=16,MW=110;
    struct Item{const char* label;ViewType view;};
    Item items[6]={{"Perspective",ViewType::Perspective},{"Top",ViewType::Top},
                   {"Front",ViewType::Front},{"Back",ViewType::Back},
                   {"Left",ViewType::Left},{"Right",ViewType::Right}};
    int n=6;
    int menuH()const{return n*IH+4;}
    bool contains(int px,int py)const{return open&&px>=x&&py>=y&&px<x+MW&&py<y+menuH();}
    int itemAt(int px,int py)const{if(!contains(px,py))return -1;return(py-y-2)/IH;}
    void draw(SDL_Surface* s)const{
        if(!open)return;
        SDL_Rect r2={x,y,MW,menuH()};
        SDL_FillRect(s,&r2,SDL_MapRGB(s->format,230,228,220));
        Draw::drawRect(s,x,y,MW,menuH(),Pal::DARK_SHADOW);
        for(int i=0;i<n;i++){
            int iy=y+2+i*IH;
            if(i==hovered){SDL_Rect sr={x+1,iy,MW-2,IH};SDL_FillRect(s,&sr,SDL_MapRGB(s->format,49,106,197));Draw::drawText(s,x+6,iy+4,items[i].label,Pal::SEL_TXT);}
            else Draw::drawText(s,x+6,iy+4,items[i].label,Pal::TEXT);
        }
    }
};

// ═══════════════════════════════════════════════════════════════════
//  PIXEL HELPERS
// ═══════════════════════════════════════════════════════════════════
static inline void spx(SDL_Surface* s,int x,int y,uint8_t r,uint8_t g,uint8_t b){
    if(x<0||y<0||x>=s->w||y>=s->h)return;
    ((uint32_t*)s->pixels)[y*(s->pitch/4)+x]=(0xFFu<<24)|((uint32_t)r<<16)|((uint32_t)g<<8)|b;
}
static void shline(SDL_Surface* s,int x0,int x1,int y,uint8_t r,uint8_t g,uint8_t b){
    if(x0>x1)std::swap(x0,x1);for(int x=x0;x<=x1&&x<s->w;x++)spx(s,x,y,r,g,b);
}
static void svline(SDL_Surface* s,int x,int y0,int y1,uint8_t r,uint8_t g,uint8_t b){
    if(y0>y1)std::swap(y0,y1);for(int y=y0;y<=y1&&y<s->h;y++)spx(s,x,y,r,g,b);
}
static void sbline(SDL_Surface* s,int x0,int y0,int x1,int y1,uint8_t r,uint8_t g,uint8_t b){
    int dx=abs(x1-x0),dy=abs(y1-y0),sx=x0<x1?1:-1,sy=y0<y1?1:-1,e=dx-dy;
    while(true){spx(s,x0,y0,r,g,b);if(x0==x1&&y0==y1)break;
        int e2=2*e;if(e2>-dy){e-=dy;x0+=sx;}if(e2<dx){e+=dx;y0+=sy;}}
}
static void scirc(SDL_Surface* s,int cx,int cy,int rad,uint8_t r,uint8_t g,uint8_t b){
    if(rad<=0)return;int x=rad,y=0,e=0;
    while(x>=y){
        spx(s,cx+x,cy+y,r,g,b);spx(s,cx-x,cy+y,r,g,b);
        spx(s,cx+x,cy-y,r,g,b);spx(s,cx-x,cy-y,r,g,b);
        spx(s,cx+y,cy+x,r,g,b);spx(s,cx-y,cy+x,r,g,b);
        spx(s,cx+y,cy-x,r,g,b);spx(s,cx-y,cy-x,r,g,b);
        if(e<=0){y++;e+=2*y+1;}else{x--;e-=2*x+1;}
    }
}

static void drawBG(SDL_Surface* s,float px,float py){
    int W=s->w,H=s->h;
    SDL_Rect all={0,0,W,H};SDL_FillRect(s,&all,(0xFFu<<24)|(72u<<16)|(72u<<8)|72u);
    int step=std::max(8,W/8);
    int cx=W/2+(int)px,cy=H/2+(int)py;
    for(int x=((cx%step)+step)%step;x<W;x+=step)svline(s,x,0,H-1,84,84,84);
    for(int y=((cy%step)+step)%step;y<H;y+=step)shline(s,0,W-1,y,84,84,84);
    if(cx>=0&&cx<W)svline(s,cx,0,H-1,100,100,100);
    if(cy>=0&&cy<H)shline(s,0,W-1,cy,100,100,100);
}

// ═══════════════════════════════════════════════════════════════════
//  PROJECT WORLD POSITION TO SCREEN
// ═══════════════════════════════════════════════════════════════════
// Given camera state, returns screen position of a world point (wx,wy,wz)
// baseCX/baseCY = viewport center (with pan for ortho)
// worldScale = pixels per world-unit; camDist for perspective
static void worldToScreen(float wx,float wy,float wz,
    ViewType vt,float az,float el,float worldScale,
    float camDist,  // only used for perspective
    float baseCX,float baseCY,
    float& sx,float& sy)
{
    bool ortho=isOrtho(vt);
    if(!ortho){
        // Camera rotation: azimuth Y, elevation X
        float rx= wx*cosf(az)+wz*sinf(az);
        float ry= wy;
        float rz=-wx*sinf(az)+wz*cosf(az);
        float ox=rx;
        float oy=ry*cosf(el)-rz*sinf(el);
        float oz=ry*sinf(el)+rz*cosf(el);
        float dz=oz+camDist; if(dz<0.01f)dz=0.01f;
        float f=camDist/dz;
        sx=baseCX+ox*f;
        sy=baseCY-oy*f;
    } else {
        switch(vt){
            case ViewType::Top:   sx=baseCX+wx*worldScale; sy=baseCY+wz*worldScale; break;
            case ViewType::Front: sx=baseCX+wx*worldScale; sy=baseCY-wy*worldScale; break;
            case ViewType::Back:  sx=baseCX-wx*worldScale; sy=baseCY-wy*worldScale; break;
            case ViewType::Left:  sx=baseCX+wz*worldScale; sy=baseCY-wy*worldScale; break;
            case ViewType::Right: sx=baseCX-wz*worldScale; sy=baseCY-wy*worldScale; break;
            default:              sx=baseCX+wx*worldScale; sy=baseCY-wy*worldScale; break;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  OBJECT RENDERER  (software rasterizer per primitive)
// ═══════════════════════════════════════════════════════════════════
struct RenderCtx {
    SDL_Surface* s;
    ViewType vt;
    bool ortho,wire,selected;
    float az,el,worldScale,camDist;
    float cx,cy; // object screen center
    float rs;    // "reference size" in pixels (= radius in screen pixels)
};

// Transform a local vertex by object rotation then camera:
static void transformVert(float lx,float ly,float lz,float rotY,
    const RenderCtx& rc, float& ox,float& oy,float& oz)
{
    // Object Y rotation
    float rx= lx*cosf(rotY)+lz*sinf(rotY);
    float ry= ly;
    float rz=-lx*sinf(rotY)+lz*cosf(rotY);
    if(!rc.ortho){
        float ax= rx*cosf(rc.az)+rz*sinf(rc.az);
        float ay= ry;
        float az2=-rx*sinf(rc.az)+rz*cosf(rc.az);
        ox=ax;
        oy=ay*cosf(rc.el)-az2*sinf(rc.el);
        oz=ay*sinf(rc.el)+az2*cosf(rc.el);
    } else {
        ox=rx;oy=ry;oz=rz;
    }
}

// Project rotated vertex → screen (relative to object center cx,cy)
static void projectVert(float ox,float oy,float oz,const RenderCtx& rc,int& sx,int& sy){
    if(!rc.ortho){
        float dz=oz+rc.camDist; if(dz<0.01f)dz=0.01f;
        float f=rc.camDist/dz;
        sx=(int)(rc.cx+ox*f); sy=(int)(rc.cy-oy*f);
    } else {
        switch(rc.vt){
            case ViewType::Top:   sx=(int)(rc.cx+ox*rc.worldScale); sy=(int)(rc.cy+oz*rc.worldScale); break;
            case ViewType::Front: sx=(int)(rc.cx+ox*rc.worldScale); sy=(int)(rc.cy-oy*rc.worldScale); break;
            case ViewType::Back:  sx=(int)(rc.cx-ox*rc.worldScale); sy=(int)(rc.cy-oy*rc.worldScale); break;
            case ViewType::Left:  sx=(int)(rc.cx+oz*rc.worldScale); sy=(int)(rc.cy-oy*rc.worldScale); break;
            case ViewType::Right: sx=(int)(rc.cx-oz*rc.worldScale); sy=(int)(rc.cy-oy*rc.worldScale); break;
            default:              sx=(int)(rc.cx+ox*rc.worldScale); sy=(int)(rc.cy-oy*rc.worldScale); break;
        }
    }
}

// Line with back-face culling for perspective (oz>0 means behind camera)
static void wireLine(const RenderCtx& rc,
    float ox0,float oy0,float oz0,
    float ox1,float oy1,float oz1,
    uint8_t r,uint8_t g,uint8_t b)
{
    if(!rc.ortho&&oz0>0&&oz1>0)return;
    int sx0,sy0,sx1,sy1;
    projectVert(ox0,oy0,oz0,rc,sx0,sy0);
    projectVert(ox1,oy1,oz1,rc,sx1,sy1);
    sbline(rc.s,sx0,sy0,sx1,sy1,r,g,b);
}

// Shaded fill – approximation for all shapes (draw shaded ellipse at screen)
// For ortho we just do a flat tinted circle/rect; for perspective full Phong
static void drawShadeCircle(const RenderCtx& rc,float lx,float ly,float lz,bool smooth){
    int W=rc.s->w,H=rc.s->h;
    float cx=rc.cx,cy=rc.cy,rs=rc.rs;
    if(!rc.ortho){
        for(int py=0;py<H;py++) for(int px=0;px<W;px++){
            float ddx=(px-cx)/rs,ddy=(py-cy)/rs;
            float d2=ddx*ddx+ddy*ddy;
            if(d2<1.f){
                float nz=sqrtf(1.f-d2);
                float diff=std::max(0.f,ddx*lx+ddy*ly+nz*lz);
                float spec=smooth?powf(std::max(0.f,nz),32.f)*0.8f:0.f;
                spx(rc.s,px,py,
                    (uint8_t)std::min(255.f,180*diff+200*spec),
                    (uint8_t)std::min(255.f, 50*diff+200*spec),
                    (uint8_t)std::min(255.f, 50*diff+200*spec));
            }
        }
    } else {
        for(int py=0;py<H;py++) for(int px=0;px<W;px++){
            float ddx=(px-cx)/rs,ddy=(py-cy)/rs;
            float d2=ddx*ddx+ddy*ddy;
            if(d2<1.f){
                float lf=0.3f+0.5f*(1.f-sqrtf(d2));
                uint8_t v=(uint8_t)(170*lf);
                spx(rc.s,px,py,v,(uint8_t)(v/4),(uint8_t)(v/4));
            }
        }
    }
}

// ── SPHERE ────────────────────────────────────────────────────────
static void renderSphere(const RenderCtx& rc,const SceneObject& o,uint8_t wr,uint8_t wg,uint8_t wb){
    float rs=rc.rs;
    int nLat=o.segsU,nLon=o.segsV;
    int steps=std::max(48,std::max(nLat,nLon)*8);
    // Lat rings
    for(int i=1;i<nLat;i++){
        float lat=(float)M_PI*i/nLat-(float)M_PI*0.5f;
        float yr=rs*sinf(lat),r2=rs*cosf(lat);
        int px0=-1,py0=-1; float opx=0,opy=0,opz=0;
        for(int j=0;j<=steps;j++){
            float lon=2*(float)M_PI*j/steps;
            float ox,oy,oz;transformVert(r2*cosf(lon),yr,r2*sinf(lon),o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;(void)opx;(void)opy;(void)opz;opx=ox;opy=oy;opz=oz;
        }
    }
    // Meridians
    for(int i=0;i<nLon;i++){
        float lon=2*(float)M_PI*i/nLon;
        int px0=-1,py0=-1;
        for(int j=0;j<=steps;j++){
            float lat=(float)M_PI*j/steps-(float)M_PI*0.5f;
            float lx2=rs*cosf(lat)*cosf(lon),ly2=rs*sinf(lat),lz2=rs*cosf(lat)*sinf(lon);
            float ox,oy,oz;transformVert(lx2,ly2,lz2,o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;
        }
    }
}

// ── BOX ───────────────────────────────────────────────────────────
static void renderBox(const RenderCtx& rc,const SceneObject& o,uint8_t wr,uint8_t wg,uint8_t wb){
    float hw=o.radius,hh=o.height*0.5f,hd=o.radius;
    // 8 corners
    float vx[8]={-hw,-hw,-hw,-hw,hw,hw,hw,hw};
    float vy[8]={-hh,-hh,hh,hh,-hh,-hh,hh,hh};
    float vz[8]={-hd,hd,-hd,hd,-hd,hd,-hd,hd};
    int edges[12][2]={{0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7}};
    // Subdivide each edge
    int sub=std::max(1,o.segsU);
    for(auto& e:edges){
        int a=e[0],b=e[1];
        int px0=-1,py0=-1;float opx=0,opy=0,opz=0;(void)opx;(void)opy;(void)opz;
        for(int k=0;k<=sub;k++){
            float t=(float)k/sub;
            float lx2=vx[a]+(vx[b]-vx[a])*t;
            float ly2=vy[a]+(vy[b]-vy[a])*t;
            float lz2=vz[a]+(vz[b]-vz[a])*t;
            float ox,oy,oz;transformVert(lx2,ly2,lz2,o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;
        }
    }
    // Draw grid on each face
    for(int s2=1;s2<sub;s2++){
        float t=(float)s2/sub;
        // Bottom/top face X grid
        auto faceX=[&](float fy){ // y=-hh or +hh
            float lx2=-hw+2*hw*t;
            float ox0,oy0,oz0,ox1,oy1,oz1;
            transformVert(lx2,fy,-hd,o.rotY,rc,ox0,oy0,oz0);
            transformVert(lx2,fy,+hd,o.rotY,rc,ox1,oy1,oz1);
            wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
        };
        faceX(-hh);faceX(hh);
        auto faceZ=[&](float fy){
            float lz2=-hd+2*hd*t;
            float ox0,oy0,oz0,ox1,oy1,oz1;
            transformVert(-hw,fy,lz2,o.rotY,rc,ox0,oy0,oz0);
            transformVert(+hw,fy,lz2,o.rotY,rc,ox1,oy1,oz1);
            wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
        };
        faceZ(-hh);faceZ(hh);
        // Side faces
        auto sideH=[&](float lx2){ // vertical lines
            float lz2=-hd+2*hd*t;
            float ox0,oy0,oz0,ox1,oy1,oz1;
            transformVert(lx2,-hh,lz2,o.rotY,rc,ox0,oy0,oz0);
            transformVert(lx2,+hh,lz2,o.rotY,rc,ox1,oy1,oz1);
            wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
        };
        sideH(-hw);sideH(hw);
        auto sideV=[&](float lz2){ // horizontal lines on sides
            float ly2=-hh+2*hh*t;
            float ox0,oy0,oz0,ox1,oy1,oz1;
            transformVert(-hw,ly2,lz2,o.rotY,rc,ox0,oy0,oz0);
            transformVert(+hw,ly2,lz2,o.rotY,rc,ox1,oy1,oz1);
            wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
            transformVert(lz2,ly2,-hw,o.rotY,rc,ox0,oy0,oz0);
            transformVert(lz2,ly2,+hw,o.rotY,rc,ox1,oy1,oz1);
            wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
        };
        sideV(-hd);sideV(hd);
    }
}

// ── CYLINDER ──────────────────────────────────────────────────────
static void renderCylinder(const RenderCtx& rc,const SceneObject& o,
    uint8_t wr,uint8_t wg,uint8_t wb,bool cap=true)
{
    float rad=o.radius,hh=o.height*0.5f;
    int nSeg=o.segsV,hSeg=o.segsU;
    int steps=std::max(64,nSeg*8);
    // Side rings
    for(int h=0;h<=hSeg;h++){
        float fy=-hh+2*hh*(float)h/hSeg;
        int px0=-1,py0=-1;
        for(int j=0;j<=steps;j++){
            float lon=2*(float)M_PI*j/steps;
            float ox,oy,oz;transformVert(rad*cosf(lon),fy,rad*sinf(lon),o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;
        }
    }
    // Vertical lines
    for(int i=0;i<nSeg;i++){
        float lon=2*(float)M_PI*i/nSeg;
        float ox0,oy0,oz0,ox1,oy1,oz1;
        transformVert(rad*cosf(lon),-hh,rad*sinf(lon),o.rotY,rc,ox0,oy0,oz0);
        transformVert(rad*cosf(lon),+hh,rad*sinf(lon),o.rotY,rc,ox1,oy1,oz1);
        wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
    }
    // Cap circles already drawn by rings at h=0 and h=hSeg
}

// ── CONE ──────────────────────────────────────────────────────────
static void renderCone(const RenderCtx& rc,const SceneObject& o,uint8_t wr,uint8_t wg,uint8_t wb){
    float rad=o.radius,hh=o.height;
    int nSeg=o.segsV,hSeg=o.segsU;
    int steps=std::max(64,nSeg*8);
    // Rings (base to tip)
    for(int h=0;h<hSeg;h++){
        float t=(float)h/hSeg;
        float fy=-hh*0.5f+hh*t;
        float r2=rad*(1.f-t);
        int px0=-1,py0=-1;
        for(int j=0;j<=steps;j++){
            float lon=2*(float)M_PI*j/steps;
            float ox,oy,oz;transformVert(r2*cosf(lon),fy,r2*sinf(lon),o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;
        }
    }
    // Lines to tip
    float tox,toy,toz;transformVert(0,hh*0.5f,0,o.rotY,rc,tox,toy,toz);
    for(int i=0;i<nSeg;i++){
        float lon=2*(float)M_PI*i/nSeg;
        float ox0,oy0,oz0;
        transformVert(rad*cosf(lon),-hh*0.5f,rad*sinf(lon),o.rotY,rc,ox0,oy0,oz0);
        wireLine(rc,ox0,oy0,oz0,tox,toy,toz,wr,wg,wb);
    }
}

// ── TORUS ─────────────────────────────────────────────────────────
static void renderTorus(const RenderCtx& rc,const SceneObject& o,uint8_t wr,uint8_t wg,uint8_t wb){
    float R=o.radius,r=o.radius2;
    int nMaj=o.segsV,nMin=o.segsU;
    int steps=std::max(48,nMaj*4);
    int stepsM=std::max(24,nMin*4);
    // Tube circles
    for(int i=0;i<nMaj;i++){
        float phi=2*(float)M_PI*i/nMaj;
        int px0=-1,py0=-1;
        for(int j=0;j<=stepsM;j++){
            float theta=2*(float)M_PI*j/stepsM;
            float lx2=(R+r*cosf(theta))*cosf(phi);
            float ly2=r*sinf(theta);
            float lz2=(R+r*cosf(theta))*sinf(phi);
            float ox,oy,oz;transformVert(lx2,ly2,lz2,o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;
        }
    }
    // Longitude rings at fixed theta
    for(int j=0;j<nMin;j++){
        float theta=2*(float)M_PI*j/nMin;
        int px0=-1,py0=-1;
        for(int i=0;i<=steps;i++){
            float phi=2*(float)M_PI*i/steps;
            float lx2=(R+r*cosf(theta))*cosf(phi);
            float ly2=r*sinf(theta);
            float lz2=(R+r*cosf(theta))*sinf(phi);
            float ox,oy,oz;transformVert(lx2,ly2,lz2,o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;
        }
    }
}

// ── PLANE ─────────────────────────────────────────────────────────
static void renderPlane(const RenderCtx& rc,const SceneObject& o,uint8_t wr,uint8_t wg,uint8_t wb){
    float hw=o.radius,hd=o.height*0.5f;
    int nx=o.segsU,nz=o.segsV;
    // Grid lines along X
    for(int i=0;i<=nx;i++){
        float lx2=-hw+2*hw*(float)i/nx;
        float ox0,oy0,oz0,ox1,oy1,oz1;
        transformVert(lx2,0,-hd,o.rotY,rc,ox0,oy0,oz0);
        transformVert(lx2,0,+hd,o.rotY,rc,ox1,oy1,oz1);
        wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
    }
    // Grid lines along Z
    for(int i=0;i<=nz;i++){
        float lz2=-hd+2*hd*(float)i/nz;
        float ox0,oy0,oz0,ox1,oy1,oz1;
        transformVert(-hw,0,lz2,o.rotY,rc,ox0,oy0,oz0);
        transformVert(+hw,0,lz2,o.rotY,rc,ox1,oy1,oz1);
        wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
    }
}

// ── TUBE ──────────────────────────────────────────────────────────
static void renderTube(const RenderCtx& rc,const SceneObject& o,uint8_t wr,uint8_t wg,uint8_t wb){
    // Outer cylinder
    SceneObject outer=o; outer.radius=o.radius;
    renderCylinder(rc,outer,wr,wg,wb);
    // Inner cylinder
    SceneObject inner=o; inner.radius=o.radius2;
    renderCylinder(rc,inner,wr,wg,wb);
    // Caps: lines connecting inner to outer at top & bottom
    int nSeg=o.segsV;
    for(int i=0;i<nSeg;i++){
        float lon=2*(float)M_PI*i/nSeg;
        float cs=cosf(lon),ss=sinf(lon);
        for(int cap=-1;cap<=1;cap+=2){
            float fy=o.height*0.5f*cap;
            float ox0,oy0,oz0,ox1,oy1,oz1;
            transformVert(o.radius*cs,fy,o.radius*ss,o.rotY,rc,ox0,oy0,oz0);
            transformVert(o.radius2*cs,fy,o.radius2*ss,o.rotY,rc,ox1,oy1,oz1);
            wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
        }
    }
}

// ── PYRAMID ───────────────────────────────────────────────────────
static void renderPyramid(const RenderCtx& rc,const SceneObject& o,uint8_t wr,uint8_t wg,uint8_t wb){
    float hw=o.radius,hh=o.height;
    // Base
    float bx[4]={-hw,-hw,hw,hw},bz[4]={-hw,hw,-hw,hw};
    float by=-hh*0.5f;
    int sub=std::max(1,o.segsU);
    // Base edges
    int baseEdges[4][2]={{0,1},{0,2},{3,1},{3,2}};
    for(auto& e:baseEdges){
        int a=e[0],b2=e[1];
        int px0=-1,py0=-1;
        for(int k=0;k<=sub;k++){
            float t=(float)k/sub;
            float lx2=bx[a]+(bx[b2]-bx[a])*t,lz2=bz[a]+(bz[b2]-bz[a])*t;
            float ox,oy,oz;transformVert(lx2,by,lz2,o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;
        }
    }
    // Lines to apex
    float tax,tay,taz;transformVert(0,hh*0.5f,0,o.rotY,rc,tax,tay,taz);
    for(int i=0;i<4;i++){
        int px0=-1,py0=-1;
        for(int k=0;k<=sub;k++){
            float t=(float)k/sub;
            float lx2=bx[i]*t,lz2=bz[i]*t;
            float ly2=by+(hh)*t;
            float ox,oy,oz;transformVert(lx2,ly2,lz2,o.rotY,rc,ox,oy,oz);
            if(!rc.ortho&&oz>0){px0=-1;continue;}
            int sx,sy;projectVert(ox,oy,oz,rc,sx,sy);
            if(px0>=0)sbline(rc.s,px0,py0,sx,sy,wr,wg,wb);
            px0=sx;py0=sy;
        }
        (void)tax;(void)tay;(void)taz;
    }
    // Grid on base
    for(int i=1;i<sub;i++){
        float t=(float)i/sub;
        // X lines
        for(int zz=-1;zz<=1;zz+=2){
            float lz2=hw*(float)zz;
            float lx0=-hw+2*hw*t;
            float ox0,oy0,oz0,ox1,oy1,oz1;
            transformVert(lx0,by,-hw,o.rotY,rc,ox0,oy0,oz0);
            transformVert(lx0,by,+hw,o.rotY,rc,ox1,oy1,oz1);
            wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
            (void)lz2;
        }
        float lz2=-hw+2*hw*t;
        float ox0,oy0,oz0,ox1,oy1,oz1;
        transformVert(-hw,by,lz2,o.rotY,rc,ox0,oy0,oz0);
        transformVert(+hw,by,lz2,o.rotY,rc,ox1,oy1,oz1);
        wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
    }
}

// ── GEOSPHERE ─────────────────────────────────────────────────────
// Icosphere subdivided once
static void renderGeoSphere(const RenderCtx& rc,const SceneObject& o,uint8_t wr,uint8_t wg,uint8_t wb){
    float rad=o.radius;
    // Icosahedron base vertices (normalized)
    float t=(1.f+(float)sqrt(5.f))/2.f;
    float vb[12][3]={
        {-1,t,0},{1,t,0},{-1,-t,0},{1,-t,0},
        {0,-1,t},{0,1,t},{0,-1,-t},{0,1,-t},
        {t,0,-1},{t,0,1},{-t,0,-1},{-t,0,1}
    };
    // Normalize and scale
    for(auto& v:vb){ float l=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); v[0]*=rad/l;v[1]*=rad/l;v[2]*=rad/l; }
    // Faces
    int faces[20][3]={
        {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
        {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
        {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
        {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
    };
    int sub=std::max(1,o.segsU/4);
    for(auto& f:faces){
        float* a=vb[f[0]],*b2=vb[f[1]],*c=vb[f[2]];
        for(int i=0;i<=sub;i++){
            float s=(float)i/sub;
            for(int j=0;j<=sub-i;j++){
                float t2=(float)j/sub;
                float u=1.f-s-t2;
                if(u<0)continue;
                // interpolate and project onto sphere
                auto interp=[&](float s2,float t3,float u2)->std::array<float,3>{
                    float vx=a[0]*u2+b2[0]*s2+c[0]*t3;
                    float vy=a[1]*u2+b2[1]*s2+c[1]*t3;
                    float vz=a[2]*u2+b2[2]*s2+c[2]*t3;
                    float l=sqrtf(vx*vx+vy*vy+vz*vz); l=l<0.001f?1.f:l;
                    return{vx*rad/l,vy*rad/l,vz*rad/l};
                };
                auto p00=interp(s,t2,u);
                float s1=(float)(i+1)/sub,t21=(float)j/sub,u1=1.f-s1-t21;
                float s2_=(float)i/sub,t22=(float)(j+1)/sub,u2=1.f-s2_-t22;
                if(u1>=0){
                    auto p10=interp(s1,t21,u1);
                    float ox0,oy0,oz0,ox1,oy1,oz1;
                    transformVert(p00[0],p00[1],p00[2],o.rotY,rc,ox0,oy0,oz0);
                    transformVert(p10[0],p10[1],p10[2],o.rotY,rc,ox1,oy1,oz1);
                    wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
                }
                if(u2>=0){
                    auto p01=interp(s2_,t22,u2);
                    float ox0,oy0,oz0,ox1,oy1,oz1;
                    transformVert(p00[0],p00[1],p00[2],o.rotY,rc,ox0,oy0,oz0);
                    transformVert(p01[0],p01[1],p01[2],o.rotY,rc,ox1,oy1,oz1);
                    wireLine(rc,ox0,oy0,oz0,ox1,oy1,oz1,wr,wg,wb);
                }
            }
        }
    }
}

// ── DISPATCH ──────────────────────────────────────────────────────
static void renderObject(SDL_Surface* surf,const SceneObject& obj,
    const VPCamera& cam,bool wire,bool selected)
{
    int W=surf->w,H=surf->h;
    ViewType vt=cam.viewType;
    bool ortho=isOrtho(vt);
    float zoom=cam.zoom;

    // World scale: how many pixels per world-unit
    float scl=std::min(W,H)*0.38f*zoom;
    float worldScale=scl/100.f;

    // Object screen center
    float baseCX=W*0.5f+(ortho?cam.panX:0);
    float baseCY=H*0.5f+(ortho?cam.panY:0);

    float camDist=scl*(obj.radius*obj.scale/100.f)*3.5f;
    float sx,sy;
    worldToScreen(obj.x,obj.y,obj.z,vt,cam.az,cam.el,worldScale,camDist,baseCX,baseCY,sx,sy);

    float rs=scl*(obj.radius*obj.scale/100.f);
    rs=std::max(4.f,std::min(rs,(float)std::min(W,H)*3.f));

    RenderCtx rc;
    rc.s=surf; rc.vt=vt; rc.ortho=ortho; rc.wire=wire; rc.selected=selected;
    rc.az=cam.az; rc.el=cam.el; rc.worldScale=worldScale; rc.camDist=camDist;
    rc.cx=sx; rc.cy=sy; rc.rs=rs;

    // Light
    float lx,ly,lz;
    if(!ortho){lx=cosf(cam.az+0.5f)*0.7f;ly=-0.5f;lz=sinf(cam.az+0.5f)*0.7f+0.4f;}
    else{lx=0.4f;ly=-0.6f;lz=0.5f;}
    float ll=sqrtf(lx*lx+ly*ly+lz*lz);lx/=ll;ly/=ll;lz/=ll;

    // Shaded fill (only in shaded mode, i.e., not pure wire)
    if(!wire){
        switch(obj.type){
            case PrimType::Sphere:
            case PrimType::GeoSphere:
            case PrimType::Cylinder:
            case PrimType::Cone:
            case PrimType::Tube:
                drawShadeCircle(rc,lx,ly,lz,obj.smooth);
                break;
            case PrimType::Box:
            case PrimType::Pyramid:
            case PrimType::Torus:
            case PrimType::Plane:
                // Simple fill rectangle hint
                {
                    float hw=rs*0.9f;
                    for(int py2=std::max(0,(int)(sy-hw));py2<std::min(H,(int)(sy+hw));py2++)
                        for(int px2=std::max(0,(int)(sx-hw));px2<std::min(W,(int)(sx+hw));px2++){
                            float ddx=(px2-sx)/hw,ddy=(py2-sy)/hw;
                            float d2=ddx*ddx+ddy*ddy;
                            if(d2<1.f){
                                float lf=0.3f+0.5f*(1.f-sqrtf(d2));
                                uint8_t v=(uint8_t)(160*lf);
                                spx(surf,px2,py2,v,(uint8_t)(v/4),(uint8_t)(v/4));
                            }
                        }
                }
                break;
        }
    }

    // Wireframe color: yellow if selected, else grey/light depending on mode
    uint8_t wr,wg,wb;
    if(selected){ wr=255;wg=200;wb=0; }      // yellow selection
    else if(wire){ wr=180;wg=220;wb=180; }    // light green for ortho wire
    else{ wr=70;wg=90;wb=70; }               // dark for shaded overlay

    // Draw primitive wireframe
    switch(obj.type){
        case PrimType::Sphere:    renderSphere(rc,obj,wr,wg,wb); break;
        case PrimType::GeoSphere: renderGeoSphere(rc,obj,wr,wg,wb); break;
        case PrimType::Box:       renderBox(rc,obj,wr,wg,wb); break;
        case PrimType::Cylinder:  renderCylinder(rc,obj,wr,wg,wb); break;
        case PrimType::Cone:      renderCone(rc,obj,wr,wg,wb); break;
        case PrimType::Torus:     renderTorus(rc,obj,wr,wg,wb); break;
        case PrimType::Plane:     renderPlane(rc,obj,wr,wg,wb); break;
        case PrimType::Tube:      renderTube(rc,obj,wr,wg,wb); break;
        case PrimType::Pyramid:   renderPyramid(rc,obj,wr,wg,wb); break;
    }

    // Selection highlight ring
    if(selected){
        int iRs=(int)rs;
        scirc(surf,(int)sx,(int)sy,iRs+3,255,200,0);
        scirc(surf,(int)sx,(int)sy,iRs+4,255,200,0);
    }

    // Transform gizmo
    if(selected){
        int gl=std::max(18,(int)(rs*0.4f));
        int scx=(int)sx,scy=(int)sy;
        if(!ortho){
            sbline(surf,scx,scy,scx+gl,scy,          220,50,50);
            sbline(surf,scx,scy,scx,scy-gl,          50,220,50);
            sbline(surf,scx,scy,scx-gl/2,scy+gl/2,   50,100,220);
        } else {
            switch(vt){
                case ViewType::Top:
                    sbline(surf,scx,scy,scx+gl,scy,   220,50,50);
                    sbline(surf,scx,scy,scx,scy+gl,   50,100,220);
                    break;
                case ViewType::Front:case ViewType::Back:
                    sbline(surf,scx,scy,scx+gl,scy,   220,50,50);
                    sbline(surf,scx,scy,scx,scy-gl,   50,220,50);
                    break;
                case ViewType::Left:case ViewType::Right:
                    sbline(surf,scx,scy,scx+gl,scy,   50,100,220);
                    sbline(surf,scx,scy,scx,scy-gl,   50,220,50);
                    break;
                default:break;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  FULL VIEWPORT RENDER
// ═══════════════════════════════════════════════════════════════════
static void renderViewport(SDL_Surface* s,const AppState& app,int vpIdx)
{
    const VPCamera& cam=app.cam[vpIdx];
    bool ortho=isOrtho(cam.viewType);
    bool wire=app.wireframe[vpIdx];
    drawBG(s,ortho?cam.panX:0,ortho?cam.panY:0);

    // Render all objects (non-selected first, then selected on top)
    for(int pass=0;pass<2;pass++){
        for(auto& op:app.objects){
            bool sel=(op->id==app.selectedId);
            if(pass==0&&sel) continue;
            if(pass==1&&!sel) continue;
            renderObject(s,*op,cam,wire,sel);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  LAYOUT
// ═══════════════════════════════════════════════════════════════════
static void computeLayout(Rect out[4],int maximised,int WX,int WY,int WW,int WH){
    int hw=WW/2,hh=WH/2;
    if(maximised<0){
        out[0]=Rect(WX,WY,hw,hh);out[1]=Rect(WX+hw,WY,hw,hh);
        out[2]=Rect(WX,WY+hh,hw,hh);out[3]=Rect(WX+hw,WY+hh,hw,hh);
    } else for(int i=0;i<4;i++)out[i]=Rect(WX,WY,WW,WH);
}

// ── PICK OBJECT: find object closest to screen click ──────────────
// Returns selected id or -1
static int pickObject(int mx,int my,const AppState& app,int vpIdx){
    const VPCamera& cam=app.cam[vpIdx];
    ViewType vt=cam.viewType;
    bool ortho=isOrtho(vt);
    // We need approximate viewport size — use a dummy 1x1 to get world scale
    // Actually we need the surface size, but we don't have it here easily.
    // We'll use a reasonable default: 512x512 equivalent
    int W=512,H=512; // approximate, good enough for picking
    float scl=std::min(W,H)*0.38f*cam.zoom;
    float worldScale=scl/100.f;
    float baseCX=W*0.5f+(ortho?cam.panX:0);
    float baseCY=H*0.5f+(ortho?cam.panY:0);

    int best=-1; float bestD=40*40; // 40px threshold
    for(auto& op:app.objects){
        float camDist=scl*(op->radius*op->scale/100.f)*3.5f;
        float sx,sy;
        worldToScreen(op->x,op->y,op->z,vt,cam.az,cam.el,worldScale,camDist,baseCX,baseCY,sx,sy);
        // Scale mouse coords to our 512 space
        float ddx=mx-sx,ddy=my-sy;
        float d2=ddx*ddx+ddy*ddy;
        if(d2<bestD){bestD=d2;best=op->id;}
    }
    return best;
}

// ═══════════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════════
int main(int,char**){
    SDL_Init(SDL_INIT_VIDEO);
    int winW=1024,winH=720;
    SDL_Window* win=SDL_CreateWindow("3d Editor",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,winW,winH,SDL_WINDOW_RESIZABLE);
    SDL_Renderer* ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE);
    UIContext ctx; ctx.init(ren,winW,winH);

    AppState app;

    const int RPW=224;
    auto getVPArea=[&](int& WX,int& WY,int& WW,int& WH){WX=0;WY=46;WW=winW-RPW;WH=winH-46-28;};
    auto getRP=[&]()->int{return winW-RPW;};
    int WX,WY,WW,WH;getVPArea(WX,WY,WW,WH);

    SDL_Surface* vpSurf[4]={};
    bool vpDirty[4]={true,true,true,true};
    UIViewport3D* vps[4]={};

    struct DragState{bool active=false;int vpIdx=-1,btn=0,lastX=0,lastY=0;bool transforming=false;} drag;
    CtxMenu ctxMenu;

    auto markAll=[&](){for(int i=0;i<4;i++)vpDirty[i]=true;};

    // ── MenuBar ─────────────────────────────────────────────────────
    auto mb=std::unique_ptr<UIMenuBar>(new UIMenuBar("menubar",Rect(0,0,winW,20)));
    mb->addMenu("File",{MenuItem("New","file_new"),MenuItem("Open...","file_open"),
        MenuItem("Save","file_save"),MenuItem("","",true),MenuItem("Exit","file_exit")});
    mb->addMenu("Edit",{MenuItem("Undo","edit_undo"),MenuItem("Redo","edit_redo"),
        MenuItem("","",true),MenuItem("Delete Selected","edit_del")});
    mb->addMenu("Create",{MenuItem("Sphere","create_sphere"),MenuItem("Box","create_box"),
        MenuItem("Cylinder","create_cyl"),MenuItem("Cone","create_cone"),
        MenuItem("Torus","create_tor"),MenuItem("Plane","create_plane")});
    mb->addMenu("Rendering",{MenuItem("Render","render")});
    mb->addMenu("Help",{MenuItem("About","about")});
    mb->on(EventType::MenuItemClicked,[&](UIComponent*,const UIEvent& e){
        if(e.svalue=="file_exit"){SDL_Event q;q.type=SDL_QUIT;SDL_PushEvent(&q);}
        else if(e.svalue=="edit_del"){
            if(app.selectedId>=0){
                auto& v=app.objects;
                v.erase(std::remove_if(v.begin(),v.end(),
                    [&](const std::unique_ptr<SceneObject>& o){return o->id==app.selectedId;}),v.end());
                app.selectedId=-1;markAll();ctx.needsRedraw=true;
                app.statusMsg="Object deleted";
            }
        }
    });
    ctx.add(std::move(mb));

    // ── Toolbar ─────────────────────────────────────────────────────
    auto tb_=std::unique_ptr<UIToolbar>(new UIToolbar("toolbar",Rect(0,20,winW,26)));
    UIButton* btnSel   =tb_->addButton("btn_sel",  2, 50,"Select",ctx);
    UIButton* btnMov   =tb_->addButton("btn_mov", 54, 50,"Move",  ctx);
    UIButton* btnRot   =tb_->addButton("btn_rot",106, 50,"Rotate",ctx);
    UIButton* btnScl   =tb_->addButton("btn_scl",158, 50,"Scale", ctx);
    UIButton* btnDel   =tb_->addButton("btn_del", 212, 50,"Del",  ctx);
    UIButton* btnRend  =tb_->addButton("btn_rnd", 266, 60,"Render",ctx);
    UIButton* btnGrid  =tb_->addButton("btn_rest",330, 70,"Grid[]",ctx);
    ctx.add(std::move(tb_));

    auto setTool=[&](ToolMode t,UIButton* active){
        app.tool=t;
        for(auto* b:{btnSel,btnMov,btnRot,btnScl})b->flat=true;
        active->flat=false;
        markAll();ctx.needsRedraw=true;
    };
    setTool(ToolMode::Select,btnSel);

    btnSel->on(EventType::Click,[&](UIComponent*,const UIEvent&){setTool(ToolMode::Select,btnSel);app.statusMsg="Select mode";});
    btnMov->on(EventType::Click,[&](UIComponent*,const UIEvent&){
        if(app.tool==ToolMode::Move){setTool(ToolMode::Select,btnSel);app.statusMsg="Select mode";}
        else{setTool(ToolMode::Move,btnMov);app.statusMsg="Move mode – drag object";}
    });
    btnRot->on(EventType::Click,[&](UIComponent*,const UIEvent&){
        if(app.tool==ToolMode::Rotate){setTool(ToolMode::Select,btnSel);app.statusMsg="Select mode";}
        else{setTool(ToolMode::Rotate,btnRot);app.statusMsg="Rotate mode – drag L/R";}
    });
    btnScl->on(EventType::Click,[&](UIComponent*,const UIEvent&){
        if(app.tool==ToolMode::Scale){setTool(ToolMode::Select,btnSel);app.statusMsg="Select mode";}
        else{setTool(ToolMode::Scale,btnScl);app.statusMsg="Scale mode – drag U/D";}
    });
    btnDel->on(EventType::Click,[&](UIComponent*,const UIEvent&){
        if(app.selectedId>=0){
            auto& v=app.objects;
            v.erase(std::remove_if(v.begin(),v.end(),
                [&](const std::unique_ptr<SceneObject>& o){return o->id==app.selectedId;}),v.end());
            app.selectedId=-1;markAll();ctx.needsRedraw=true;
            app.statusMsg="Object deleted";
        }
    });
    btnRend->on(EventType::Click,[&](UIComponent*,const UIEvent&){
        for(int i=0;i<4;i++){app.wireframe[i]=false;vpDirty[i]=true;}
        app.statusMsg="All shaded";ctx.needsRedraw=true;
    });
    btnGrid->on(EventType::Click,[&](UIComponent*,const UIEvent&){
        app.maximised=-1;getVPArea(WX,WY,WW,WH);
        Rect r[4];computeLayout(r,-1,WX,WY,WW,WH);
        for(int i=0;i<4;i++){vps[i]->setRect(r[i]);vps[i]->setVisible(true);vpDirty[i]=true;}
        ctx.needsRedraw=true;app.statusMsg="Grid view";
    });

    // ── Viewports ───────────────────────────────────────────────────
    {Rect r[4];computeLayout(r,-1,WX,WY,WW,WH);
    for(int i=0;i<4;i++){
        std::string vpid="vp"+std::to_string(i);
        auto vp=std::unique_ptr<UIViewport3D>(new UIViewport3D(vpid,r[i],viewTypeName(app.cam[i].viewType)));
        vps[i]=static_cast<UIViewport3D*>(ctx.add(std::move(vp)));
        vps[i]->on(EventType::DblClick,[&,i](UIComponent*,const UIEvent&){
            app.maximised=(app.maximised==i)?-1:i;
            getVPArea(WX,WY,WW,WH);
            Rect r2[4];computeLayout(r2,app.maximised,WX,WY,WW,WH);
            for(int j=0;j<4;j++){vps[j]->setRect(r2[j]);vps[j]->setVisible(app.maximised<0||j==app.maximised);vpDirty[j]=true;}
            ctx.needsRedraw=true;
            app.statusMsg=(app.maximised>=0)?std::string(viewTypeName(app.cam[i].viewType))+" maximised":"Grid view";
        });
    }}
    vps[0]->setActive(true);ctx.activeViewport=vps[0];

    // ── Right panel ─────────────────────────────────────────────────
    int RP=getRP();
    auto rp_=std::unique_ptr<UIPanel>(new UIPanel("rpanel",Rect(RP,20,RPW,winH),true));
    rp_->raised=false;ctx.add(std::move(rp_));

    auto mkLbl=[&](const std::string& id2,int x,int y,int w,int h,const std::string& t,int align=0)->UILabel*{
        auto l=std::unique_ptr<UILabel>(new UILabel(id2,Rect(x,y,w,h),t,true));
        l->align=align;return static_cast<UILabel*>(ctx.add(std::move(l)));
    };
    mkLbl("lbl_prim",RP+4,22,RPW-8,18,"Standard Primitives",1);

    // Object type buttons (9 types, 2 columns)
    struct OB{const char* id;const char* label;PrimType type;int col,row;};
    OB obs[]={
        {"ob_box",  "Box",      PrimType::Box,      0,0},{"ob_cone","Cone",   PrimType::Cone,     1,0},
        {"ob_sph",  "Sphere",   PrimType::Sphere,   0,1},{"ob_geo", "GeoSph", PrimType::GeoSphere,1,1},
        {"ob_cyl",  "Cylinder", PrimType::Cylinder, 0,2},{"ob_tube","Tube",   PrimType::Tube,     1,2},
        {"ob_tor",  "Torus",    PrimType::Torus,    0,3},{"ob_pyr", "Pyramid",PrimType::Pyramid,  1,3},
        {"ob_pln",  "Plane",    PrimType::Plane,    0,4},
    };
    int BW=(RPW-16)/2,BH=18;
    auto grpOT=std::unique_ptr<UIGroupBox>(new UIGroupBox("grp_ot",Rect(RP+4,40,RPW-8,130),"Object Type"));
    ctx.add(std::move(grpOT));
    // Active type indicator
    UIButton* activeTypeBtn=nullptr;
    for(auto& ob:obs){
        PrimType pt=ob.type;
        std::string lbl(ob.label);
        auto btn_=std::unique_ptr<UIButton>(new UIButton(ob.id,
            Rect(RP+4+ob.col*(BW+2),50+ob.row*(BH+2),BW,BH),lbl));
        auto* bptr=static_cast<UIButton*>(ctx.add(std::move(btn_)));
        bptr->flat=true;
        if(pt==PrimType::Sphere){activeTypeBtn=bptr;bptr->flat=false;}
        bptr->on(EventType::Click,[&,pt,bptr,lbl](UIComponent*,const UIEvent&){
            app.pendingType=pt;
            // Reset all type buttons to flat, highlight selected
            for(auto& ob2:obs) if(auto* c=static_cast<UIButton*>(ctx.findById(ob2.id))) c->flat=true;
            bptr->flat=false;
            ctx.needsRedraw=true;
            app.statusMsg="Type: "+lbl+" – set params, then Create";
        });
    }

    // Parameters group – dynamic labels change per type
    auto grpPar=std::unique_ptr<UIGroupBox>(new UIGroupBox("grp_par",Rect(RP+4,178,RPW-8,290),"Parameters"));
    ctx.add(std::move(grpPar));

    int py=198;
    // Param 1: Radius / Width
    mkLbl("lbl_p1",RP+6,py,58,18,"Radius:");
    auto spinP1=std::unique_ptr<UISpinner>(new UISpinner("spin_p1",Rect(RP+66,py,RPW-74,18),0.f,500.f,50.f,0.5f));
    auto* spP1=static_cast<UISpinner*>(ctx.add(std::move(spinP1)));
    {auto sl=std::unique_ptr<UISlider>(new UISlider("sld_p1",Rect(RP+66,py+20,RPW-74,14),0.f,500.f,50.f));
     ctx.add(std::move(sl));} py+=36;

    // Param 2: Height / Radius2
    mkLbl("lbl_p2",RP+6,py,58,18,"Height:");
    auto spinP2=std::unique_ptr<UISpinner>(new UISpinner("spin_p2",Rect(RP+66,py,RPW-74,18),0.f,1000.f,100.f,1.f));
    auto* spP2=static_cast<UISpinner*>(ctx.add(std::move(spinP2))); py+=24;

    // Param 3: Radius2 (inner/tube)
    mkLbl("lbl_p3",RP+6,py,58,18,"Radius2:");
    auto spinP3=std::unique_ptr<UISpinner>(new UISpinner("spin_p3",Rect(RP+66,py,RPW-74,18),0.f,500.f,30.f,0.5f));
    auto* spP3=static_cast<UISpinner*>(ctx.add(std::move(spinP3))); py+=24;

    // Segs U
    mkLbl("lbl_su",RP+6,py,58,18,"Segs U:");
    auto spinSU=std::unique_ptr<UISpinner>(new UISpinner("spin_su",Rect(RP+66,py,RPW-74,18),1.f,64.f,8.f,1.f));
    static_cast<UISpinner*>(spinSU.get())->decimals=0;
    auto* spSU=static_cast<UISpinner*>(ctx.add(std::move(spinSU))); py+=22;

    // Segs V
    mkLbl("lbl_sv",RP+6,py,58,18,"Segs V:");
    auto spinSV=std::unique_ptr<UISpinner>(new UISpinner("spin_sv",Rect(RP+66,py,RPW-74,18),1.f,64.f,8.f,1.f));
    static_cast<UISpinner*>(spinSV.get())->decimals=0;
    auto* spSV=static_cast<UISpinner*>(ctx.add(std::move(spinSV))); py+=26;

    // Smooth
    auto ckSm=std::unique_ptr<UICheckbox>(new UICheckbox("chk_sm",Rect(RP+6,py,RPW-12,18),"Smooth",true));
    ckSm->bgColor=Pal::DARK_PANEL;
    auto* pCkSm=static_cast<UICheckbox*>(ctx.add(std::move(ckSm))); py+=22;

    // Wireframe all
    auto ckWire=std::unique_ptr<UICheckbox>(new UICheckbox("chk_wire",Rect(RP+6,py,RPW-12,18),"Wireframe (all)",false));
    ckWire->bgColor=Pal::DARK_PANEL;
    auto* pCkWire=static_cast<UICheckbox*>(ctx.add(std::move(ckWire))); py+=26;

    // Name
    mkLbl("lbl_name",RP+6,py,52,18,"Name:");
    auto txtN=std::unique_ptr<UITextInput>(new UITextInput("txt_name",Rect(RP+62,py,RPW-68,18),""));
    auto* pTxtN=static_cast<UITextInput*>(ctx.add(std::move(txtN))); py+=24;

    // Create button
    auto btnCr=std::unique_ptr<UIButton>(new UIButton("btn_create",Rect(RP+4,py,RPW-8,22),"Create"));
    ctx.add(std::move(btnCr))->on(EventType::Click,[&](UIComponent*,const UIEvent&){
        static int counter=1;
        std::string nm=pTxtN->getText();
        if(nm.empty()){
            // default name
            const char* names[]{"Box","Cone","Sphere","GeoSph","Cylinder","Tube","Torus","Pyramid","Plane"};
            nm=std::string(names[(int)app.pendingType])+std::to_string(counter++);
        }
        auto obj=std::unique_ptr<SceneObject>(new SceneObject(app.pendingType,nm));
        obj->radius=spP1->getValue();
        obj->height=spP2->getValue();
        obj->radius2=spP3->getValue();
        obj->segsU=(int)spSU->getValue();
        obj->segsV=(int)spSV->getValue();
        obj->smooth=pCkSm->isChecked();
        // Spread objects slightly so they don't overlap
        float sp=(float)app.objects.size();
        obj->x=sp*120.f;

        app.selectedId=obj->id;
        char buf[128];snprintf(buf,sizeof(buf),"Created %s",nm.c_str());
        app.statusMsg=buf;
        app.objects.push_back(std::move(obj));
        markAll();ctx.needsRedraw=true;
    });
    py+=28;

    // Scene objects list label
    mkLbl("lbl_scene",RP+4,py,RPW-8,18,"Scene Objects:",0); py+=20;
    // We'll show a simple text list below
    auto lbl_list=std::unique_ptr<UILabel>(new UILabel("lbl_objlist",Rect(RP+4,py,RPW-8,100),"(none)",true));
    lbl_list->fgColor=Pal::DISABLED_TXT;
    auto* pLblList=static_cast<UILabel*>(ctx.add(std::move(lbl_list)));

    // Hint
    auto hint=std::unique_ptr<UILabel>(new UILabel("lbl_hint",Rect(RP+4,winH-100,RPW-8,90),
        "Wheel=zoom\nLMB=select/orbit\nMMB=pan\nRMB=view type\nDel=delete sel",true));
    static_cast<UILabel*>(hint.get())->fgColor=Color{120,120,120};
    ctx.add(std::move(hint));

    // Status bar
    auto sbar=std::unique_ptr<UIPanel>(new UIPanel("sbar",Rect(0,winH-28,winW,28)));
    sbar->raised=false;ctx.add(std::move(sbar));
    auto sLbl=std::unique_ptr<UILabel>(new UILabel("lbl_st",Rect(4,winH-26,winW-RPW-4,24),app.statusMsg));
    auto* stPtr=static_cast<UILabel*>(ctx.add(std::move(sLbl)));

    // Wire up param spinners to selected object (live edit)
    auto syncParamsToSelected=[&](){
        auto* o=app.selected();
        if(!o)return;
        o->radius=spP1->getValue();
        o->height=spP2->getValue();
        o->radius2=spP3->getValue();
        o->segsU=(int)spSU->getValue();
        o->segsV=(int)spSV->getValue();
        o->smooth=pCkSm->isChecked();
        markAll();ctx.needsRedraw=true;
    };
    spP1->on(EventType::ValueChanged,[&](UIComponent*,const UIEvent& e){
        if(auto* sl=static_cast<UISlider*>(ctx.findById("sld_p1"))){sl->setValue(e.fvalue);sl->markDirty();}
        syncParamsToSelected();
    });
    if(auto* sl=static_cast<UISlider*>(ctx.findById("sld_p1")))
        sl->on(EventType::ValueChanged,[&](UIComponent*,const UIEvent& e){spP1->setValue(e.fvalue);spP1->markDirty();syncParamsToSelected();});
    spP2->on(EventType::ValueChanged,[&](UIComponent*,const UIEvent&){syncParamsToSelected();});
    spP3->on(EventType::ValueChanged,[&](UIComponent*,const UIEvent&){syncParamsToSelected();});
    spSU->on(EventType::ValueChanged,[&](UIComponent*,const UIEvent&){syncParamsToSelected();});
    spSV->on(EventType::ValueChanged,[&](UIComponent*,const UIEvent&){syncParamsToSelected();});
    pCkSm->on(EventType::CheckChanged,[&](UIComponent*,const UIEvent&){syncParamsToSelected();});
    pCkWire->on(EventType::CheckChanged,[&](UIComponent*,const UIEvent& e){
        bool w=e.ivalue!=0;for(int i=0;i<4;i++)app.wireframe[i]=w;markAll();
    });
    pTxtN->on(EventType::ValueChanged,[&](UIComponent*,const UIEvent& e){app.pendingName=e.svalue;});

    // Helper: load selected object params into panel
    auto loadParamsFromSelected=[&](){
        auto* o=app.selected();
        if(!o)return;
        spP1->setValue(o->radius);spP1->markDirty();
        spP2->setValue(o->height);spP2->markDirty();
        spP3->setValue(o->radius2);spP3->markDirty();
        spSU->setValue((float)o->segsU);spSU->markDirty();
        spSV->setValue((float)o->segsV);spSV->markDirty();
        pCkSm->setChecked(o->smooth);
        if(auto* sl=static_cast<UISlider*>(ctx.findById("sld_p1"))){sl->setValue(o->radius);sl->markDirty();}
        pTxtN->setText(o->name);
        ctx.needsRedraw=true;
    };

    // Update scene object list label
    auto updateSceneList=[&](){
        std::string txt;
        for(auto& op:app.objects){
            bool sel=(op->id==app.selectedId);
            if(sel)txt+="► ";
            txt+=op->name+"\n";
        }
        if(txt.empty())txt="(no objects)";
        pLblList->setText(txt);
    };

    // ── VP find helper ───────────────────────────────────────────────
    auto vpUnder=[&](int mx,int my)->int{
        for(int i=3;i>=0;i--){if(!vps[i]->visible)continue;if(vps[i]->rect.contains(mx,my))return i;}
        return -1;
    };

    // ── Resize handler ───────────────────────────────────────────────
    auto doResize=[&](int nw,int nh){
        winW=nw;winH=nh;
        ctx.resize(winW,winH);
        getVPArea(WX,WY,WW,WH);
        RP=getRP();
        if(auto* c=ctx.findById("menubar"))c->setRect(Rect(0,0,winW,20));
        if(auto* c=ctx.findById("toolbar"))c->setRect(Rect(0,20,winW,26));
        if(auto* c=ctx.findById("sbar"))   c->setRect(Rect(0,winH-28,winW,28));
        if(auto* c=ctx.findById("lbl_st")) c->setRect(Rect(4,winH-26,winW-RPW-4,24));
        if(auto* c=ctx.findById("rpanel")) c->setRect(Rect(RP,20,RPW,winH));
        // Right panel widgets
        const int BW2=(RPW-16)/2,BH2=18;
        struct E{const char* id;int rx,ry,w,h;};
        E es[]={
            {"lbl_prim", 4, 22, RPW-8, 18},
            {"grp_ot",   4, 40, RPW-8,130},
            {"ob_box",   4+0*(BW2+2),50+0*(BH2+2),BW2,BH2},
            {"ob_cone",  4+1*(BW2+2),50+0*(BH2+2),BW2,BH2},
            {"ob_sph",   4+0*(BW2+2),50+1*(BH2+2),BW2,BH2},
            {"ob_geo",   4+1*(BW2+2),50+1*(BH2+2),BW2,BH2},
            {"ob_cyl",   4+0*(BW2+2),50+2*(BH2+2),BW2,BH2},
            {"ob_tube",  4+1*(BW2+2),50+2*(BH2+2),BW2,BH2},
            {"ob_tor",   4+0*(BW2+2),50+3*(BH2+2),BW2,BH2},
            {"ob_pyr",   4+1*(BW2+2),50+3*(BH2+2),BW2,BH2},
            {"ob_pln",   4+0*(BW2+2),50+4*(BH2+2),BW2,BH2},
            {"grp_par",  4,178,RPW-8,290},
            {"lbl_p1",   6,198, 58,18},{"spin_p1",66,198,RPW-74,18},
            {"sld_p1",  66,218,RPW-74,14},
            {"lbl_p2",   6,234, 58,18},{"spin_p2",66,234,RPW-74,18},
            {"lbl_p3",   6,258, 58,18},{"spin_p3",66,258,RPW-74,18},
            {"lbl_su",   6,282, 58,18},{"spin_su",66,282,RPW-74,18},
            {"lbl_sv",   6,304, 58,18},{"spin_sv",66,304,RPW-74,18},
            {"chk_sm",   6,330,RPW-12,18},
            {"chk_wire", 6,352,RPW-12,18},
            {"lbl_name", 6,378, 52,18},{"txt_name",62,378,RPW-68,18},
            {"btn_create",4,402,RPW-8,22},
            {"lbl_scene", 4,430,RPW-8,18},
            {"lbl_objlist",4,450,RPW-8,100},
            {"lbl_hint", 4,winH-120,RPW-8,110},
        };
        for(auto& e:es)if(auto* c=ctx.findById(e.id))c->setRect(Rect(RP+e.rx,e.ry,e.w,e.h));
        Rect r2[4];computeLayout(r2,app.maximised,WX,WY,WW,WH);
        for(int i=0;i<4;i++){
            vps[i]->setRect(r2[i]);vps[i]->setVisible(app.maximised<0||i==app.maximised);
            if(vpSurf[i]){SDL_FreeSurface(vpSurf[i]);vpSurf[i]=nullptr;}
            vpDirty[i]=true;
        }
        ctx.needsRedraw=true;
    };

    // ── MAIN LOOP ───────────────────────────────────────────────────
    bool running=true;
    while(running){
        SDL_Event ev;
        while(SDL_PollEvent(&ev)){
            // Context menu
            if(ctxMenu.open){
                if(ev.type==SDL_MOUSEMOTION){int ni=ctxMenu.itemAt(ev.motion.x,ev.motion.y);if(ni!=ctxMenu.hovered){ctxMenu.hovered=ni;ctx.needsRedraw=true;}continue;}
                if(ev.type==SDL_MOUSEBUTTONDOWN){
                    if(ev.button.button==SDL_BUTTON_LEFT){
                        int idx=ctxMenu.itemAt(ev.button.x,ev.button.y);
                        if(idx>=0){
                            int vi=ctxMenu.forVP;
                            app.cam[vi].viewType=ctxMenu.items[idx].view;
                            app.cam[vi].panX=app.cam[vi].panY=0;
                            vps[vi]->viewLabel=viewTypeName(app.cam[vi].viewType);
                            app.wireframe[vi]=isOrtho(app.cam[vi].viewType);
                            vpDirty[vi]=true;
                        }
                    }
                    ctxMenu.open=false;ctx.needsRedraw=true;continue;
                }
                if(ev.type==SDL_KEYDOWN&&ev.key.keysym.sym==SDLK_ESCAPE){ctxMenu.open=false;ctx.needsRedraw=true;continue;}
                if(ev.type==SDL_QUIT){running=false;break;}
                continue;
            }

            if(ev.type==SDL_QUIT){running=false;continue;}

            // Resize
            if(ev.type==SDL_WINDOWEVENT&&
               (ev.window.event==SDL_WINDOWEVENT_RESIZED||ev.window.event==SDL_WINDOWEVENT_SIZE_CHANGED)){
                doResize(ev.window.data1,ev.window.data2);continue;
            }

            // Delete key
            if(ev.type==SDL_KEYDOWN&&ev.key.keysym.sym==SDLK_DELETE&&app.selectedId>=0){
                auto& v=app.objects;
                v.erase(std::remove_if(v.begin(),v.end(),
                    [&](const std::unique_ptr<SceneObject>& o){return o->id==app.selectedId;}),v.end());
                app.selectedId=-1;markAll();ctx.needsRedraw=true;app.statusMsg="Object deleted";
                continue;
            }

            // Mouse wheel → zoom
            if(ev.type==SDL_MOUSEWHEEL){
                int mx,my;SDL_GetMouseState(&mx,&my);
                int vi=vpUnder(mx,my);
                if(vi>=0){
                    float f=ev.wheel.y>0?1.15f:(1.f/1.15f);
                    app.cam[vi].zoom=std::max(0.05f,std::min(20.f,app.cam[vi].zoom*f));
                    vpDirty[vi]=true;ctx.needsRedraw=true;
                    char b[64];snprintf(b,sizeof(b),"%s  zoom=%.2f",viewTypeName(app.cam[vi].viewType),app.cam[vi].zoom);
                    app.statusMsg=b;continue;
                }
                ctx.processEvent(ev);continue;
            }

            // Mouse down
            if(ev.type==SDL_MOUSEBUTTONDOWN){
                int mx=ev.button.x,my=ev.button.y;
                int vi=vpUnder(mx,my);
                if(vi>=0){
                    for(int j=0;j<4;j++)vps[j]->setActive(j==vi);
                    ctx.activeViewport=vps[vi];vpDirty[vi]=true;ctx.needsRedraw=true;

                    if(ev.button.button==SDL_BUTTON_RIGHT){
                        ctxMenu.open=true;ctxMenu.forVP=vi;ctxMenu.x=mx;ctxMenu.y=my;ctxMenu.hovered=-1;
                        ctx.needsRedraw=true;continue;
                    }
                    if(ev.button.button==SDL_BUTTON_LEFT||ev.button.button==SDL_BUTTON_MIDDLE){
                        drag.active=true;drag.vpIdx=vi;drag.btn=ev.button.button;
                        drag.lastX=mx;drag.lastY=my;
                        drag.transforming=(ev.button.button==SDL_BUTTON_LEFT&&
                                           app.tool!=ToolMode::Select&&app.selectedId>=0);

                        if(ev.button.button==SDL_BUTTON_LEFT&&app.tool==ToolMode::Select){
                            // Pick object
                            // Build a scaled mouse coord relative to VP center
                            const Rect& vr=vps[vi]->rect;
                            int rmx=mx-vr.x,rmy=my-vr.y;
                            // Use actual VP size for picking
                            int VW=vr.w,VH=vr.h;
                            const VPCamera& cam=app.cam[vi];
                            ViewType vt=cam.viewType;bool ortho=isOrtho(vt);
                            float scl=std::min(VW,VH)*0.38f*cam.zoom;
                            float worldScale=scl/100.f;
                            float baseCX=VW*0.5f+(ortho?cam.panX:0);
                            float baseCY=VH*0.5f+(ortho?cam.panY:0);
                            int oldSel=app.selectedId;
                            app.selectedId=-1;
                            float bestD=(float)(std::min(VW,VH)*0.15f);
                            bestD*=bestD;
                            for(auto& op:app.objects){
                                float camDist=scl*(op->radius*op->scale/100.f)*3.5f;
                                float sx,sy;
                                worldToScreen(op->x,op->y,op->z,vt,cam.az,cam.el,worldScale,camDist,baseCX,baseCY,sx,sy);
                                float ddx=rmx-sx,ddy=rmy-sy;
                                float d2=ddx*ddx+ddy*ddy;
                                // pick radius = screen radius of object
                                float pickR=scl*(op->radius*op->scale/100.f)+8;
                                if(d2<pickR*pickR&&d2<bestD){bestD=d2;app.selectedId=op->id;}
                            }
                            if(app.selectedId!=oldSel){
                                if(app.selectedId>=0){loadParamsFromSelected();app.statusMsg="Selected: "+app.selected()->name;}
                                else app.statusMsg="Deselected";
                                markAll();ctx.needsRedraw=true;
                            }
                        }
                    }
                    if(ev.button.clicks>=2){
                        UIEvent ue;ue.type=EventType::DblClick;ue.mx=mx;ue.my=my;ue.mbtn=ev.button.button;
                        vps[vi]->emit(ue);
                    }
                    continue;
                }
                ctx.processEvent(ev);continue;
            }

            if(ev.type==SDL_MOUSEBUTTONUP){
                if(drag.active&&ev.button.button==drag.btn)drag.active=false;
                ctx.processEvent(ev);continue;
            }

            if(ev.type==SDL_MOUSEMOTION){
                if(drag.active){
                    int mx2=ev.motion.x,my2=ev.motion.y;
                    int ddx=mx2-drag.lastX,ddy=my2-drag.lastY;
                    drag.lastX=mx2;drag.lastY=my2;
                    int vi=drag.vpIdx;
                    VPCamera& camv=app.cam[vi];
                    bool ortho=isOrtho(camv.viewType);

                    if(drag.btn==SDL_BUTTON_MIDDLE){camv.panX+=ddx;camv.panY+=ddy;vpDirty[vi]=true;ctx.needsRedraw=true;}
                    else if(drag.btn==SDL_BUTTON_LEFT){
                        if(drag.transforming){
                            auto* o=app.selected();
                            if(o){
                                float sens=0.8f/camv.zoom;
                                switch(app.tool){
                                    case ToolMode::Move:
                                        if(!ortho){
                                            float rx=cosf(camv.az),rz=sinf(camv.az);
                                            float ux=-sinf(camv.az)*sinf(camv.el),uy=cosf(camv.el),uz=cosf(camv.az)*sinf(camv.el);
                                            o->x+=(ddx*rx)*sens;o->z+=(ddx*rz)*sens;
                                            o->x+=(-ddy*ux)*sens;o->y+=(-ddy*uy)*sens;o->z+=(-ddy*uz)*sens;
                                        } else {
                                            switch(camv.viewType){
                                                case ViewType::Top:  o->x+=ddx*sens;o->z+=ddy*sens;break;
                                                case ViewType::Front:o->x+=ddx*sens;o->y+=-ddy*sens;break;
                                                case ViewType::Back: o->x-=ddx*sens;o->y+=-ddy*sens;break;
                                                case ViewType::Left: o->z+=ddx*sens;o->y+=-ddy*sens;break;
                                                case ViewType::Right:o->z-=ddx*sens;o->y+=-ddy*sens;break;
                                                default:break;
                                            }
                                        }
                                        {char b[80];snprintf(b,sizeof(b),"Pos: X=%.1f Y=%.1f Z=%.1f",o->x,o->y,o->z);app.statusMsg=b;}
                                        break;
                                    case ToolMode::Rotate:
                                        o->rotY+=ddx*0.012f;
                                        {char b[64];snprintf(b,sizeof(b),"RotY: %.1f deg",o->rotY*(180.f/(float)M_PI));app.statusMsg=b;}
                                        break;
                                    case ToolMode::Scale:
                                        o->scale=std::max(0.01f,o->scale*(1.f+(ddx-ddy)*0.005f));
                                        {char b[48];snprintf(b,sizeof(b),"Scale: %.3f",o->scale);app.statusMsg=b;}
                                        break;
                                    default:break;
                                }
                                markAll();ctx.needsRedraw=true;
                            }
                        } else if(!ortho){
                            camv.az+=ddx*0.008f;
                            camv.el+=ddy*0.008f;
                            camv.el=std::max(-(float)M_PI*0.48f,std::min((float)M_PI*0.48f,camv.el));
                            vpDirty[vi]=true;ctx.needsRedraw=true;
                            char b[64];snprintf(b,sizeof(b),"Orbit az=%.0f deg  el=%.0f deg",camv.az*(180.f/(float)M_PI),camv.el*(180.f/(float)M_PI));
                            app.statusMsg=b;
                        }
                    }
                    continue;
                }
                ctx.processEvent(ev);continue;
            }

            ctx.processEvent(ev);
        }

        // Update status and object list
        static std::string prevSt;
        if(app.statusMsg!=prevSt){prevSt=app.statusMsg;stPtr->setText(app.statusMsg);}
        static int prevObjCount=-1;
        static int prevSel=-2;
        if((int)app.objects.size()!=prevObjCount||app.selectedId!=prevSel){
            prevObjCount=(int)app.objects.size();prevSel=app.selectedId;
            updateSceneList();ctx.needsRedraw=true;
        }

        // Render viewports
        for(int i=0;i<4;i++){
            if(!vps[i]->visible||!vpDirty[i])continue;
            int VW=vps[i]->rect.w,VH=vps[i]->rect.h;
            if(VW<2||VH<2)continue;
            if(!vpSurf[i]||vpSurf[i]->w!=VW||vpSurf[i]->h!=VH){
                if(vpSurf[i])SDL_FreeSurface(vpSurf[i]);
                vpSurf[i]=SDL_CreateRGBSurface(0,VW,VH,32,0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
            }
            SDL_LockSurface(vpSurf[i]);
            renderViewport(vpSurf[i],app,i);
            SDL_UnlockSurface(vpSurf[i]);
            std::vector<uint32_t> tmp((size_t)VW*VH);
            memcpy(tmp.data(),vpSurf[i]->pixels,(size_t)VW*VH*4);
            vps[i]->updatePixels(tmp.data(),VW,VH);
            vpDirty[i]=false;
        }

        ctx.render();
        if(ctxMenu.open){
            SDL_LockSurface(ctx.root);ctxMenu.draw(ctx.root);SDL_UnlockSurface(ctx.root);
            SDL_UpdateTexture(ctx.tex,nullptr,ctx.root->pixels,ctx.root->pitch);
            SDL_RenderClear(ren);SDL_RenderCopy(ren,ctx.tex,nullptr,nullptr);SDL_RenderPresent(ren);
        }
        SDL_Delay(14);
    }
    for(int i=0;i<4;i++)if(vpSurf[i])SDL_FreeSurface(vpSurf[i]);
    ctx.destroy();SDL_DestroyRenderer(ren);SDL_DestroyWindow(win);SDL_Quit();
    return 0;
}