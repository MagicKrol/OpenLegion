#include "health_bars.h"
#include "Amos.h"
#include "legion.h"
#include <algorithm>

namespace {
bool paused=false;

void DrawArmyHealthBars(int army) {
    if(army<0 || army>40) return;
    for(int person=1;person<=10;++person) {
        const int health=ARMIA[army][person][TE];
        const int maximum=ARMIA[army][person][TEM];
        if(health<=0 || maximum<=0) continue;
        if(!BobIsVisible(person+(army==WRG && WRG!=ARM ? 10 : 0))) continue;
        const int x=ARMIA[army][person][TX], y=std::max(1,int(ARMIA[army][person][TY])-56);
        const int width=30;
        const int filled=std::max(1,int(width*std::min(health,maximum)/maximum));
        // Put the bar above the existing selection and quest markers.
        Gfx::Color(0.f,0.f,0.f,1.f); _Bar(x-16,y-1,x+16,y+5);
        Gfx::Color(0.32f,0.08f,0.08f,1.f); _Bar(x-15,y,x+15,y+4);
        if(health*4<=maximum) Gfx::Color(0.95f,0.18f,0.12f,1.f);
        else if(health*2<=maximum) Gfx::Color(1.f,0.75f,0.12f,1.f);
        else Gfx::Color(0.25f,0.85f,0.2f,1.f);
        _Bar(x-15,y,x-15+filled,y+4);
    }
}
}

void SetTacticalPauseHealthBars(bool value) { paused=value; }
bool TacticalPauseHealthBars() { return paused; }
void DrawTacticalPauseHealthBars() {
    if(!paused) return;
    const int saved_screen=Screen();
    const float red=Gfx::red,green=Gfx::green,blue=Gfx::blue,alpha=Gfx::alpha;
    Screen(0);
    DrawArmyHealthBars(ARM);
    if(WRG!=ARM) DrawArmyHealthBars(WRG);
    Gfx::Color(red,green,blue,alpha);
    Screen(saved_screen);
}
