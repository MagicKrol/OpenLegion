#include "action_markers.h"
#include "Amos.h"
#include "legion.h"

namespace {
bool visible=false;

void markerRow(int x,int y,int halfWidth) {
    const int x1=XHard(x-halfWidth),x2=XHard(x+halfWidth);
    const int y1=YHard(y),y2=YHard(y+1);
    glVertex2f(x1,y1); glVertex2f(x2,y1);
    glVertex2f(x2,y2); glVertex2f(x1,y2);
}
}

ActionMarkerColour TacticalActionMarkerColour(int mode,int spell) {
    if(mode==0) return MarkerIdle;
    if(mode==1 || mode==6 || mode==7 || mode==8) return MarkerMoving;
    if(mode==2 || mode==3) return MarkerAttacking;
    if(mode==4 || mode==5) {
        // Projectile, damage, slowing, conversion and other harmful spells.
        // Healing, light and information spells are not attacks.
        if(spell>0 && spell<=MX_WEAPON && BRON[spell][B_TYP]==12) {
            const int type=BRON[spell][B_DOSW];
            if(type==1 || type==7 || type==8 || type==9 || type==10 ||
               (type==2 && (BRON[spell][B_SI]>0 || BRON[spell][B_SZ]<0)))
                return MarkerAttacking;
        }
    }
    return MarkerOther;
}
void SetActionMarkersVisible(bool show) { visible=show; }
bool ActionMarkersVisible() { return visible; }

void DrawActionMarkers() {
    if(!visible || ARM<0 || ARM>=40) return;
    const int saved=Screen(); Screen(0);
    const float red=Gfx::red,green=Gfx::green,blue=Gfx::blue,alpha=Gfx::alpha;
    // Batch all rows in one call: this path runs every frame on mobile too.
    Gfx::BreakBobBlits();
    glBegin(GL_QUADS);
    for(int pass=0;pass<2;++pass) for(int person=1;person<=10;++person) {
        if(ARMIA[ARM][person][TE]<=0 || !BobIsVisible(person)) continue;
        const int x=ARMIA[ARM][person][TX],y=ARMIA[ARM][person][TY]-50;
        if(pass==0) glColor4f(0.f,0.f,0.f,1.f);
        else {
            const int mode=ARMIA[ARM][person][TTRYB];
            const int spell=(mode==4 || mode==5) ? int(VEKTOR_F[person][0]) : 0;
            switch(TacticalActionMarkerColour(mode,spell)) {
                case MarkerMoving: glColor4f(1.f,0.87f,0.05f,1.f); break;
                case MarkerAttacking: glColor4f(1.f,0.16f,0.10f,1.f); break;
                case MarkerOther: glColor4f(0.20f,0.65f,1.f,1.f); break;
                default: glColor4f(0.10f,1.f,0.10f,1.f); break;
            }
        }
        const int first=pass==0 ? 0 : 1, end=pass==0 ? 8 : 7;
        for(int row=first;row<end;++row)
            markerRow(x,y+row,(pass==0 ? 8 : 7)-row);
    }
    glEnd();
    Gfx::Color(red,green,blue,alpha);
    Screen(saved);
}
