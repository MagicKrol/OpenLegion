#include "quest_indicators.h"
#include "Amos.h"
#include "legion.h"
#include <algorithm>

namespace {
int visit_city=-1;
bool offers[11]={false};

bool eligible(int person) {
    return visit_city>=0 && visit_city<50 && WRG==40 && ARM>=0 && ARM<40 &&
        ARMIA[ARM][0][TNOGI]==visit_city+70 && person>=1 && person<=10 &&
        ARMIA[WRG][person][TE]>0;
}
void symbol(int x,int y,bool question,float r,float g,float b) {
    // Pixel glyphs remain legible against foliage and do not depend on the
    // current dialog font or its scaling. y is the baseline in world pixels.
    const unsigned char exclamation[]={4,4,4,4,4,0,4};
    const unsigned char interrogation[]={14,17,1,2,4,0,4};
    const unsigned char *rows=question ? interrogation : exclamation;
    for(int pass=0;pass<2;++pass) {
        if(pass==0) Gfx::Color(0.f,0.f,0.f,1.f);
        else Gfx::Color(r,g,b,1.f);
        for(int row=0;row<7;++row) for(int col=0;col<5;++col) {
            if(!(rows[row] & (1 << (4-col)))) continue;
            int px=x-5+col*2,py=y-14+row*2;
            int border=pass==0 ? 1 : 0;
            _Bar(px-border,py-border,px+2+border,py+2+border);
        }
    }
}
}
void ResetCityQuestOffers() {
    visit_city=-1;
    std::fill(offers,offers+11,false);
}
void BeginCityQuestOffers(int city) {
    ResetCityQuestOffers();
    if(city<0 || city>=50) return;
    visit_city=city;
    for(int person=1;person<=10;++person) offers[person]=(Rnd(7)==1);
}
void SetCityQuestOffer(int person,bool available) {
    if(person>=1 && person<=10) offers[person]=available;
}
bool CityHasNewQuest(int person) {
    if(!eligible(person) || !offers[person] || ARMIA[WRG][person][TKORP]>=60 ||
       ARMIA[WRG][person][TPRAWA]==-1) return false;
    for(int i=0;i<4;++i) if(PRZYGODY[i][P_TYP]==0) return true;
    return false;
}
bool CityHasPendingQuestInfo(int city) {
    if(city<0 || city>=50) return false;
    for(int i=0;i<4;++i) {
        int person=PRZYGODY[i][P_Y];
        if(PRZYGODY[i][P_TYP]>0 && PRZYGODY[i][P_LEVEL]>0 &&
           PRZYGODY[i][P_X]==city && person>=1 && person<=10) return true;
    }
    return false;
}
bool CityHasQuestInfo(int person) {
    if(!eligible(person) || ARMIA[WRG][person][TKORP]>=50) return false;
    for(int i=0;i<4;++i) {
        if(PRZYGODY[i][P_TYP]>0 && PRZYGODY[i][P_LEVEL]>0 &&
           PRZYGODY[i][P_X]==visit_city && PRZYGODY[i][P_Y]==person) return true;
    }
    return false;
}
char CityQuestMarker(int person) {
    if(CityHasNewQuest(person)) return '!';
    if(CityHasQuestInfo(person)) return '?';
    return 0;
}
void ConsumeCityQuestOffer(int person) {
    SetCityQuestOffer(person,false);
}
void DrawCityQuestIndicators() {
    if(visit_city<0) return;
    int saved_screen=Screen();
    Screen(0);
    const float red=Gfx::red,green=Gfx::green,blue=Gfx::blue,alpha=Gfx::alpha;
    for(int person=1;person<=10;++person) {
        char marker=CityQuestMarker(person);
        if(!marker) continue;
        int x=ARMIA[WRG][person][TX],y=std::max(15,ARMIA[WRG][person][TY]-35);
        if(marker=='!') symbol(x,y,false,1.f,0.83f,0.15f);
        else symbol(x,y,true,0.3f,0.85f,1.f);
    }
    Gfx::Color(red,green,blue,alpha);
    Screen(saved_screen);
}

void DrawWorldQuestIndicators() {
    const float red=Gfx::red,green=Gfx::green,blue=Gfx::blue,alpha=Gfx::alpha;
    for(int city=0;city<50;++city) {
        if(CityHasPendingQuestInfo(city))
            symbol(MIASTA[city][0][M_X],MIASTA[city][0][M_Y]-10,true,0.3f,0.85f,1.f);
    }
    Gfx::Color(red,green,blue,alpha);
}
