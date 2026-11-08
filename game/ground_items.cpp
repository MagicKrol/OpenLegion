#include "ground_items.h"
#include "Amos.h"
#include "legion.h"
#include <algorithm>
#include <vector>
#include <SDL/SDL.h>

namespace {
struct Position {int item,x,y;};
Position positions[111][4];
int clamp_to(int value,int lo,int hi) {return std::max(lo,std::min(value,hi));}
bool valid(int sector,int slot) {return sector>=0 && sector<110 && slot>=0 && slot<4;}
void slot_position(int sector,int slot,int &x,int &y) {
    int left=(sector%10)*64, top=(sector/10)*50;
    if(top>=500) { x=left+8+slot*14; y=501; }
    else {x=left+12+(slot%2)*24; y=top+10+(slot/2)*20;}
}
}
void ResetGroundItemPositions() {
    for(int sector=0;sector<111;++sector) for(int slot=0;slot<4;++slot)
        positions[sector][slot]=Position{0,0,0};
}
void SetGroundDropPosition(int sector,int slot,int item,int x,int y) {
    if(!valid(sector,slot) || item<=0 || item>120) return;
    int left=(sector%10)*64, top=(sector/10)*50;
    if(top>=500) {slot_position(sector,slot,x,y);}
    else {
        // Keep the four gently scattered icons inside the pickup sector even
        // when the character dies near its edge.
        x=clamp_to(x,left+19,left+45)+(slot%2 ? 9 : -9);
        y=clamp_to(y,top+16,top+34)+(slot/2 ? 8 : -8);
    }
    positions[sector][slot]=Position{item,x,y};
}
bool GroundItemPosition(int sector,int slot,int &x,int &y) {
    if(!valid(sector,slot)) return false;
    int item=GLEBA[sector][slot];
    Position &p=positions[sector][slot];
    if(item<=0 || item>120) {p=Position{0,0,0};return false;}
    if(p.item==item) {x=p.x;y=p.y;}
    else {p=Position{0,0,0};slot_position(sector,slot,x,y);}
    return true;
}
void NearbyGroundSlots(int x,int y,GroundSlot slots[4]) {
    struct Candidate { int distance,sector,slot; };
    std::vector<Candidate> candidates;
    const int own=clamp_to(x,0,639)/64+(clamp_to(y,0,511)/50)*10;
    for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) {
        int ix,iy;
        if(!GroundItemPosition(sector,slot,ix,iy)) continue;
        int dx=x-ix,dy=y-iy,dist=dx*dx+dy*dy;
        // Preserve the original sector reach and additionally include items
        // close to the character across a sector boundary.
        if(sector==own || dist<=32*32) candidates.push_back(Candidate{dist,sector,slot});
    }
    std::sort(candidates.begin(),candidates.end(),[](const Candidate &a,const Candidate &b) {
        if(a.distance!=b.distance) return a.distance<b.distance;
        if(a.sector!=b.sector) return a.sector<b.sector;
        return a.slot<b.slot;
    });
    int count=0;
    for(size_t i=0;i<candidates.size() && count<4;++i)
        slots[count++]=GroundSlot{candidates[i].sector,candidates[i].slot};
    // Empty UI cells drop items into empty slots in the character's sector.
    for(int slot=0;slot<4 && count<4;++slot)
        if(GLEBA[own][slot]==0) slots[count++]=GroundSlot{own,slot};
    // Keep references valid even if a corrupt/unsupported item ID occupies a
    // sector slot and therefore has no drawable position.
    for(int slot=0;slot<4 && count<4;++slot) {
        bool used=false;
        for(int i=0;i<count;++i) if(slots[i].sector==own && slots[i].slot==slot) used=true;
        if(!used) slots[count++]=GroundSlot{own,slot};
    }
}
void ReplaceGroundItem(GroundSlot slot,int item,int x,int y) {
    if(!valid(slot.sector,slot.slot)) return;
    int oldx,oldy;
    bool had_item=GroundItemPosition(slot.sector,slot.slot,oldx,oldy);
    GLEBA[slot.sector][slot.slot]=item;
    if(item<=0) positions[slot.sector][slot.slot]=Position{0,0,0};
    else if(had_item) positions[slot.sector][slot.slot]=Position{item,oldx,oldy};
    else SetGroundDropPosition(slot.sector,slot.slot,item,x,y);
}
void DrawGroundItems() {
    Screen(0);
    for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) {
        int x,y;
        if(GroundItemPosition(sector,slot,x,y)) {
            int item=GLEBA[sector][slot];
            // Natural resources stay quiet; dropped equipment and loot blink.
            const int type=BRON[item][B_TYP];
            if (type!=13 && type!=16 && item!=81 && (SDL_GetTicks()/500)%2==0) {
                Gfx::Color(1.0f,0.85f,0.25f,1.0f);
                _Box(x-2,y-2,x+12,y+12);
            }
            // The same icon/bank as the inventory, visible above the scene.
            PasteBob(x,y,BRON[item][B_BOB]+BROBY+GOBY,0.65f);
        }
    }
}
