#include "ground_items.h"
#include "Amos.h"
#include "legion.h"
#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>
#include <SDL/SDL.h>

namespace {
// Discovery lasts for the encounter; discovered icons remain visible even
// after the party moves away. Positions are not inventory slots.
struct Position {int item,x,y; bool discovered;};
Position positions[GROUND_SECTOR_LIMIT][4];
int sectorCount=110;
const int reach=80;
struct QuestGroundEntry { unsigned short index,x,y; unsigned char item,discovered; };
struct QuestGroundSnapshot {
    bool present;
    unsigned short key[5];
    std::vector<QuestGroundEntry> items;
    QuestGroundSnapshot() : present(false), key{} {}
};
QuestGroundSnapshot questGround[4][2];
int activeQuest=-1;
const unsigned char questGroundMagic[8]={'L','G','N','Q','G','R',1,0};
int quest_phase(int quest,int scenery) {
    return PRZYGODY[quest][P_TYP]==13 && scenery==PRZYGODY[quest][P_TEREN] ? 1 : 0;
}
void quest_key(int quest,unsigned short key[5]) {
    const int fields[]={P_TYP,P_X,P_Y,P_TEREN,P_BRON};
    for(int i=0;i<5;++i) key[i]=static_cast<unsigned short>(PRZYGODY[quest][fields[i]]);
}
bool same_quest(const QuestGroundSnapshot &s,int quest) {
    unsigned short key[5]; quest_key(quest,key);
    for(int i=0;i<5;++i) if(s.key[i]!=key[i]) return false;
    return true;
}
void write16(unsigned char *&out,unsigned value) { *out++=value>>8; *out++=value&255; }
unsigned read16(const unsigned char *&in) {const unsigned v=(unsigned(in[0])<<8)|in[1];in+=2;return v;}
int clamp_to(int value,int lo,int hi) {return std::max(lo,std::min(value,hi));}
bool valid(int sector,int slot) {return sector>=0 && sector<sectorCount && slot>=0 && slot<4;}
unsigned hash_slot(int sector,int slot,int item) {
    unsigned h=unsigned(sector+1)*2654435761u ^ unsigned(slot+7)*2246822519u ^ unsigned(item+1)*3266489917u;
    h^=h>>16; h*=2246822519u; h^=h>>13; return h;
}
void slot_position(int sector,int slot,int item,int &x,int &y) {
    unsigned h=hash_slot(sector,slot,item);
    int left=(sector%10)*64, top=(sector/10)*50;
    x=clamp_to(left+4+int(h%56),0,627);
    y=clamp_to(top+3+int((h>>8)%44),0,499);
}
bool near_unit(int x,int y) {
    for(int unit=1;unit<=10;++unit) if(ARMIA[ARM][unit][TE]>0) {
        const int dx=ARMIA[ARM][unit][TX]-x,dy=ARMIA[ARM][unit][TY]-y;
        if(dx*dx+dy*dy<=reach*reach) return true;
    }
    return false;
}
}
void ResetGroundItemPositions() {
    for(int sector=0;sector<GROUND_SECTOR_LIMIT;++sector) for(int slot=0;slot<4;++slot) {
        positions[sector][slot]=Position{};
        if(sector>=110) GLEBA[sector][slot]=0;
    }
    sectorCount=110;
}
void ClearGroundItems() {
    for(int sector=0;sector<GROUND_SECTOR_LIMIT;++sector)
        for(int slot=0;slot<=4;++slot) GLEBA[sector][slot]=0;
    ResetGroundItemPositions();
}
int GroundSectorCount() { return sectorCount; }
int GroundItem(GroundSlot slot) { return valid(slot.sector,slot.slot) ? GLEBA[slot.sector][slot.slot] : 0; }
void SetGroundDropPosition(int sector,int slot,int item,int x,int y) {
    if(!valid(sector,slot) || item<=0 || item>120) return;
    const unsigned h=hash_slot(sector,slot,item);
    x=clamp_to(x+int(h%31)-15,0,627);
    y=clamp_to(y+int((h>>8)%25)-12,0,499);
    positions[sector][slot]=Position{item,x,y,false};
}
bool GroundItemPosition(int sector,int slot,int &x,int &y) {
    if(!valid(sector,slot)) return false;
    const int item=GLEBA[sector][slot];
    Position &p=positions[sector][slot];
    if(item<=0 || item>120) {p=Position{};return false;}
    if(p.item!=item) {
        slot_position(sector,slot,item,x,y);
        p=Position{item,x,y,false};
    }
    x=p.x; y=p.y; return true;
}
bool GroundItemVisible(int sector,int slot) {
    return GroundItemDiscovered(sector,slot);
}
bool GroundItemDiscovered(int sector,int slot) {
    int x,y;
    if(!GroundItemPosition(sector,slot,x,y)) return false;
    if(near_unit(x,y)) positions[sector][slot].discovered=true;
    return positions[sector][slot].discovered;
}
bool DropGroundItem(int item,int x,int y) {
    if(item<=0 || item>120) return false;
    int bestSector=-1,bestSlot=-1,bestDistance=2147483647;
    // Prefer a free slot in the nearest actual sector. The rendered item is
    // scattered around the corpse, even when its storage sector is farther.
    for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) if(GLEBA[sector][slot]==0) {
        const int dx=(sector%10)*64+32-x,dy=(sector/10)*50+25-y,dist=dx*dx+dy*dy;
        if(dist<bestDistance) {bestDistance=dist;bestSector=sector;bestSlot=slot;}
    }
    if(bestSector<0) {
        for(int sector=110;sector<sectorCount && bestSector<0;++sector)
            for(int slot=0;slot<4;++slot) if(GLEBA[sector][slot]==0) {bestSector=sector;bestSlot=slot;break;}
        if(bestSector<0 && sectorCount<GROUND_SECTOR_LIMIT) {bestSector=sectorCount++;bestSlot=0;}
    }
    if(bestSector<0) return false;
    GLEBA[bestSector][bestSlot]=item;
    SetGroundDropPosition(bestSector,bestSlot,item,x,y);
    return true;
}
void NearbyGroundSlots(int x,int y,GroundSlot slots[4],int page,int *total) {
    struct Candidate { int distance,sector,slot; };
    std::vector<Candidate> candidates;
    const int own=clamp_to(x,0,639)/64+(clamp_to(y,0,511)/50)*10;
    for(int sector=0;sector<sectorCount;++sector) for(int slot=0;slot<4;++slot) {
        int ix,iy;
        if(!GroundItemPosition(sector,slot,ix,iy)) continue;
        const int dx=x-ix,dy=y-iy,dist=dx*dx+dy*dy;
        if(dist<=reach*reach) {
            positions[sector][slot].discovered=true;
            candidates.push_back(Candidate{dist,sector,slot});
        }
    }
    std::sort(candidates.begin(),candidates.end(),[](const Candidate &a,const Candidate &b) {
        if(a.distance!=b.distance) return a.distance<b.distance;
        if(a.sector!=b.sector) return a.sector<b.sector;
        return a.slot<b.slot;
    });
    if(total) *total=int(candidates.size());
    const int pages=std::max(1,(int(candidates.size())+3)/4);
    page=clamp_to(page,0,pages-1);
    int count=0;
    for(size_t i=size_t(page*4);i<candidates.size() && count<4;++i) slots[count++]=GroundSlot{candidates[i].sector,candidates[i].slot};
    for(int slot=0;slot<4 && count<4;++slot) if(GLEBA[own][slot]==0) slots[count++]=GroundSlot{own,slot};
    // Fill unused cells only with empty storage. Storage sectors and visible
    // positions can differ, so a different occupied slot may be far away.
    for(int sector=0;sector<sectorCount && count<4;++sector) for(int slot=0;slot<4 && count<4;++slot) {
        if(GLEBA[sector][slot]!=0) continue;
        bool used=false; for(int i=0;i<count;++i) if(slots[i].sector==sector && slots[i].slot==slot) used=true;
        if(!used) slots[count++]=GroundSlot{sector,slot};
    }
    if(count<4 && sectorCount<GROUND_SECTOR_LIMIT) {
        const int sector=sectorCount++;
        for(int slot=0;slot<4 && count<4;++slot) slots[count++]=GroundSlot{sector,slot};
    }
    // At the absolute storage limit, use empty sentinel cells, never unrelated
    // occupied items. They are not writable and the UI handles them as empty.
    while(count<4) slots[count++]=GroundSlot{-1,-1};
}
void ReplaceGroundItem(GroundSlot slot,int item,int x,int y) {
    if(!valid(slot.sector,slot.slot)) return;
    int oldx,oldy;
    const bool had_item=GroundItemPosition(slot.sector,slot.slot,oldx,oldy);
    const bool discovered=had_item && positions[slot.sector][slot.slot].discovered;
    GLEBA[slot.sector][slot.slot]=item;
    if(item<=0) positions[slot.sector][slot.slot]=Position{};
    else if(had_item) positions[slot.sector][slot.slot]=Position{item,oldx,oldy,discovered};
    else SetGroundDropPosition(slot.sector,slot.slot,item,x,y);
}
void DrawGroundItems() {
    Screen(0);
    for(int sector=0;sector<sectorCount;++sector) for(int slot=0;slot<4;++slot) {
        int x,y;
        if(GroundItemVisible(sector,slot) && GroundItemPosition(sector,slot,x,y)) {
            const int item=GLEBA[sector][slot],type=BRON[item][B_TYP];
            if(type!=13 && type!=16 && item!=81 && (SDL_GetTicks()/500)%2==0) {
                Gfx::Color(1.0f,0.85f,0.25f,1.0f);
                _Box(x-2,y-2,x+12,y+12);
            }
            PasteBob(x,y,BRON[item][B_BOB]+BROBY+GOBY,0.65f);
        }
    }
}

void BeginQuestGround(int quest) { activeQuest=quest>=0 && quest<4 ? quest : -1; }
void ClearQuestGround(int quest) {
    if(quest<0 || quest>=4) return;
    for(int phase=0;phase<2;++phase) questGround[quest][phase]=QuestGroundSnapshot();
}
void ResetQuestGrounds() {
    for(int quest=0;quest<4;++quest) ClearQuestGround(quest);
    activeQuest=-1;
}
void EndQuestGround() {
    if(activeQuest>=0 && PRZYGODY[activeQuest][P_TYP]==0) ClearQuestGround(activeQuest);
    activeQuest=-1;
}
void RestoreQuestGroundForAction(int scenery) {
    if(activeQuest<0 || PRZYGODY[activeQuest][P_TYP]<=0) return;
    QuestGroundSnapshot &snapshot=questGround[activeQuest][quest_phase(activeQuest,scenery)];
    if(!snapshot.present) return;
    if(!same_quest(snapshot,activeQuest)) {snapshot=QuestGroundSnapshot();return;}
    // Discard only the newly generated encounter loot; the snapshot contains
    // what remained when this same phase last ended without a victory.
    ClearGroundItems();
    for(size_t i=0;i<snapshot.items.size();++i) {
        const QuestGroundEntry &entry=snapshot.items[i];
        const int sector=entry.index/4,slot=entry.index%4;
        sectorCount=std::max(sectorCount,sector+1);
        GLEBA[sector][slot]=entry.item;
        positions[sector][slot]=Position{entry.item,entry.x,entry.y,entry.discovered!=0};
    }
}
void CaptureQuestGroundAfterAction(int scenery,int outcome) {
    if(activeQuest<0 || PRZYGODY[activeQuest][P_TYP]<=0) return;
    const int phase=quest_phase(activeQuest,scenery);
    QuestGroundSnapshot &snapshot=questGround[activeQuest][phase];
    // Victory in the swamp is only phase one: a defeat against the boss
    // restarts that phase, so its already collected loot must stay gone.
    if(outcome==1 && !(PRZYGODY[activeQuest][P_TYP]==13 && phase==0)) {
        snapshot=QuestGroundSnapshot();return;
    }
    QuestGroundSnapshot replacement;
    replacement.present=true; quest_key(activeQuest,replacement.key);
    for(int sector=0;sector<sectorCount;++sector) for(int slot=0;slot<4;++slot) {
        int x,y; if(!GroundItemPosition(sector,slot,x,y)) continue;
        replacement.items.push_back(QuestGroundEntry{static_cast<unsigned short>(sector*4+slot),
            static_cast<unsigned short>(x),static_cast<unsigned short>(y),
            static_cast<unsigned char>(GLEBA[sector][slot]),static_cast<unsigned char>(positions[sector][slot].discovered)});
    }
    snapshot=std::move(replacement);
}
std::size_t QuestGroundSaveSize() {
    size_t size=sizeof(questGroundMagic); bool any=false;
    for(int quest=0;quest<4;++quest) for(int phase=0;phase<2;++phase) {
        const QuestGroundSnapshot &s=questGround[quest][phase];
        ++size;
        if(s.present) {any=true;size+=12+s.items.size()*8;}
    }
    return any ? size : 0;
}
void WriteQuestGround(unsigned char *out) {
    if(!QuestGroundSaveSize()) return;
    std::memcpy(out,questGroundMagic,sizeof(questGroundMagic)); out+=sizeof(questGroundMagic);
    for(int quest=0;quest<4;++quest) for(int phase=0;phase<2;++phase) {
        const QuestGroundSnapshot &s=questGround[quest][phase];
        *out++=s.present ? 1 : 0;
        if(!s.present) continue;
        for(int i=0;i<5;++i) write16(out,s.key[i]);
        write16(out,unsigned(s.items.size()));
        for(size_t i=0;i<s.items.size();++i) {
            const QuestGroundEntry &e=s.items[i];
            write16(out,e.index); write16(out,e.x); write16(out,e.y);
            *out++=e.item; *out++=e.discovered;
        }
    }
}
void ReadQuestGround(const unsigned char *data,std::size_t length) {
    ResetQuestGrounds();
    if(length<sizeof(questGroundMagic) || std::memcmp(data,questGroundMagic,sizeof(questGroundMagic))!=0) return;
    const unsigned char *in=data+sizeof(questGroundMagic),*end=data+length;
    QuestGroundSnapshot decoded[4][2];
    for(int quest=0;quest<4;++quest) for(int phase=0;phase<2;++phase) {
        if(in==end) return;
        const unsigned present=*in++;
        if(present>1) return;
        if(!present) continue;
        if(size_t(end-in)<12) return;
        QuestGroundSnapshot &s=decoded[quest][phase]; s.present=true;
        for(int i=0;i<5;++i) s.key[i]=static_cast<unsigned short>(read16(in));
        const unsigned count=read16(in);
        if(count>GROUND_SECTOR_LIMIT*4 || size_t(end-in)<size_t(count)*8) return;
        bool used[GROUND_SECTOR_LIMIT*4]={};
        for(unsigned i=0;i<count;++i) {
            const unsigned index=read16(in),x=read16(in),y=read16(in),item=*in++,discovered=*in++;
            if(index>=GROUND_SECTOR_LIMIT*4 || used[index] || x>639 || y>511 || item==0 || item>120 || discovered>1) return;
            used[index]=true;
            s.items.push_back(QuestGroundEntry{static_cast<unsigned short>(index),static_cast<unsigned short>(x),
                static_cast<unsigned short>(y),static_cast<unsigned char>(item),static_cast<unsigned char>(discovered)});
        }
    }
    for(int quest=0;quest<4;++quest) for(int phase=0;phase<2;++phase)
        questGround[quest][phase]=std::move(decoded[quest][phase]);
}
