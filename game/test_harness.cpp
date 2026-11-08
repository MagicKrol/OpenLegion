// Opt-in deterministic test runner. Normal play never enters this code.
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <Engine2d/Core.h>
#include <Engine2d/Gfx.h>
#include <Engine2d/Pictures.h>
#include <Engine2d/Textures.h>
#include <Engine2d/Rand.h>
#include "Amos.h"
#include "legion.h"
#include "lgn_akcja.h"
#include "lgn_ekran1.h"
#include "lgn_armia.h"
#include "lgn_sceneria.h"
#include "lgn_sklep.h"
#include "lgn_mapa.h"
#include "lgn_miasto.h"
#include "lgn_util.h"
#include "lgn_diskacs.h"
#include "test_harness.h"
#include "ground_items.h"
#include "quest_indicators.h"
#include "health_bars.h"
#include "action_markers.h"
#include "strength_overflow.h"
#include "equipment_sets.h"

extern void _main_code_rysuj_your_name(void);
extern void main_code(void);
extern void _ARMIA_RYSUJ_INFO(aint A);
extern void _ZAPIS(astr name,aint slot);
int legion_test_status = 0;
namespace {
struct Rect { int id, x1, y1, x2, y2; };
struct Point { int x, y; };
struct TimedClick { int x,y; };
Uint32 push_timed_click(Uint32,void *data);
TimedClick scaled_click(int x,int y) {
    return TimedClick{int((x*Core::il_screen_width+639)/640),int((y*Core::il_screen_height+519)/520)};
}
struct TimedKeys { int step; };
struct TimedDialogue { int step; };
struct TimedQuestTalk { int step,slot; };
struct TimedInventory { int step,first_x,first_y,second_x,second_y; };
struct TimedSwap {int step,x,y;};
struct TimedShopEquipment { int step,mode; };
struct TimedQuestAction {int step;};
struct TimedGrowth {int step;};
struct TimedHover {int step; int mode;};
struct TimedNavigation {int step;};
struct TimedTitle {int step; bool manual;};
struct TimedLootPage {int step; bool full;};
struct TimedEndAction {int step;};
struct TimedRightThenSpace {int step; TimedClick click;};
bool growth_test_enabled=false; int growth_frame_mask=0;
bool inspection_test_enabled=false,inspection_seen=false;
bool tactical_frame_test=false,tactical_frame_seen=false;
bool tactical_mode_test=false,tactical_mode_seen=false; Uint32 tactical_mode_start=0;
bool hostile_end_test=false,hostile_end_seen=false;
bool quest_scene_test_enabled=false,quest_scene_seen=false;
bool item_info_capture_enabled=false,shop_info_capture_enabled=false;
TimedHover *hover_test=0; int hover_mask=0;
TimedNavigation *navigation_test=0; int navigation_mask=0;
bool shop_duplicate_check=false,shop_duplicate_ok=false;
bool shop_worn_capture=false,shop_worn_seen=false;
bool shop_restored_capture=false,shop_restored_seen=false;
bool shop_transition_test=false,shop_transition_ok=true; int shop_transition_count[2]={0,0};
bool army_frame_seen[2]={false,false};
bool dialog_capture_enabled=false, dialog_capture_ok=false,dialog_label_ok=false;
bool title_test_active=false,title_dialog_captured=false;
TimedTitle *title_pending=0; SDL_TimerID title_timer=0; int title_menu_frames=0;
int title_dialog_frames=0;
bool inventory_capture_enabled=false;
bool map_loot_capture_enabled=false,map_loot_capture_seen=false;
bool map_loot_held_test=false,map_loot_held_seen=false; int map_loot_expected_bag=0;
bool mode_capture_enabled=false,mode_capture_seen=false;
bool quest_answer_enabled=false,quest_answer_captured=false;
bool action_frame_enabled=false,action_frame_unpaused=false,action_frame_markers=false;
std::string quest_answer_text;
Uint32 push_quest_talk(Uint32 interval,void *data) {
    TimedQuestTalk *p=static_cast<TimedQuestTalk *>(data);
    TimedClick click=p->step==0 ? scaled_click(60,366+36*p->slot) : scaled_click(340,454);
    push_timed_click(0,&click);
    return ++p->step==2 ? 0 : interval;
}
Uint32 push_inventory_input(Uint32 interval,void *data) {
    TimedInventory *p=static_cast<TimedInventory *>(data);
    if(p->step<2) {
        TimedClick click=p->step==0 ? scaled_click(p->first_x,p->first_y) : scaled_click(p->second_x,p->second_y);
        push_timed_click(0,&click);
    } else {
        SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONDOWN;
        e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED;
        TimedClick click=scaled_click(260,274);
        e.button.x=click.x; e.button.y=click.y; SDL_PushEvent(&e);
    }
    return ++p->step==3 ? 0 : interval;
}
Uint32 push_hover_test(Uint32 interval,void *data) {
    TimedHover *p=static_cast<TimedHover *>(data);
    if(p->step<5) {
        const int shopX[]={526,526,526,446,606},shopY[]={110,182,310,210,210};
        const int invX[]={110,110,110,30,190},invY[]={274,346,474,382,382};
        TimedClick c=scaled_click(p->mode==1 ? shopX[p->step] : invX[p->step],p->mode==1 ? shopY[p->step] : invY[p->step]);
        SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEMOTION; e.motion.which=255; e.motion.x=c.x; e.motion.y=c.y; SDL_PushEvent(&e);
    } else if(p->mode==2) {
        TimedClick c=scaled_click(540,156); push_timed_click(0,&c);
    } else {
        TimedClick c=scaled_click(260,274); SDL_Event e; SDL_memset(&e,0,sizeof(e));
        e.type=SDL_MOUSEBUTTONDOWN; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED; e.button.x=c.x; e.button.y=c.y; SDL_PushEvent(&e);
    }
    return ++p->step==6 ? 0 : interval;
}
Uint32 push_navigation_test(Uint32 interval,void *data) {
    TimedNavigation *p=static_cast<TimedNavigation *>(data);
    SDL_Event e; SDL_memset(&e,0,sizeof(e));
    if(p->step<4) {
        const SDLKey keys[]={SDLK_RIGHT,SDLK_LEFT,SDLK_LEFT,SDLK_RIGHT};
        e.type=SDL_KEYDOWN; e.key.which=255; e.key.keysym.sym=keys[p->step]; SDL_PushEvent(&e);
        e.type=SDL_KEYUP; SDL_PushEvent(&e);
    } else {
        TimedClick c=scaled_click(260,274); e.type=SDL_MOUSEBUTTONDOWN; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED; e.button.x=c.x; e.button.y=c.y; SDL_PushEvent(&e);
    }
    return ++p->step==5 ? 0 : interval;
}
Uint32 push_mode_click_and_key(Uint32 interval,void *data) {
    TimedGrowth *p=static_cast<TimedGrowth *>(data);
    if(p->step==0) { TimedClick c=scaled_click(206,220); push_timed_click(0,&c); }
    else {
        SDL_Event e; SDL_memset(&e,0,sizeof(e));
        if(p->step==1) { e.type=SDL_KEYDOWN; e.key.which=255; e.key.keysym.sym=SDLK_w; e.key.keysym.unicode='w'; SDL_PushEvent(&e); e.type=SDL_KEYUP; SDL_PushEvent(&e); }
        else { e.type=SDL_MOUSEBUTTONDOWN; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED; TimedClick c=scaled_click(260,274); e.button.x=c.x; e.button.y=c.y; SDL_PushEvent(&e); }
    }
    return ++p->step==3 ? 0 : interval;
}
Uint32 push_switch_key(Uint32,void *) {
    SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_KEYDOWN; e.key.which=255; e.key.keysym.sym=SDLK_w; e.key.keysym.unicode='w'; SDL_PushEvent(&e);
    e.type=SDL_KEYUP; SDL_PushEvent(&e); return 0;
}
Uint32 push_title_continue(Uint32 interval,void *data) {
    TimedTitle *p=static_cast<TimedTitle *>(data);
    const int manualY[]={260,355,260,220};
    const int autoY[]={260,196};
    TimedClick click=scaled_click(400,p->manual ? manualY[p->step] : autoY[p->step]);
    push_timed_click(0,&click);
    return ++p->step==(p->manual ? 4 : 2) ? 0 : interval;
}
Uint32 push_map_loot_page(Uint32 interval,void *data) {
    TimedLootPage *p=static_cast<TimedLootPage *>(data);
    const int rightDown=p->full ? 6 : 5;
    if(p->step==rightDown || p->step==rightDown+1) {
        TimedClick c=scaled_click(300,160); SDL_Event e; SDL_memset(&e,0,sizeof(e));
        e.type=p->step==rightDown ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT;
        e.button.state=p->step==rightDown ? SDL_PRESSED : SDL_RELEASED; e.button.x=c.x; e.button.y=c.y; SDL_PushEvent(&e);
    } else {
        const int xs[]={540,540,540,30,260,540,540,540,540},ys[]={104,56,104,152,274,156,156,156,156};
        TimedClick click=scaled_click(p->full && p->step==5 ? 206 : xs[p->step],p->full && p->step==5 ? 152 : ys[p->step]); push_timed_click(0,&click);
    }
    return ++p->step==(p->full ? 9 : 8) ? 0 : interval;
}
Uint32 push_map_loot_character(Uint32 interval,void *data) {
    TimedLootPage *p=static_cast<TimedLootPage *>(data);
    const int xs[]={580,30,260,540},ys[]={494,56,274,156};
    TimedClick click=scaled_click(xs[p->step],ys[p->step]); push_timed_click(0,&click);
    return ++p->step==4 ? 0 : interval;
}
Uint32 push_map_loot_supply(Uint32 interval,void *data) {
    TimedLootPage *p=static_cast<TimedLootPage *>(data);
    const int xs[]={30,260,540,540},ys[]={56,274,156,156};
    TimedClick click=scaled_click(xs[p->step],ys[p->step]); push_timed_click(0,&click);
    return ++p->step==4 ? 0 : interval;
}
Uint32 push_map_loot_swap(Uint32 interval,void *data) {
    TimedLootPage *p=static_cast<TimedLootPage *>(data);
    const int xs[]={30,30,540,540},ys[]={56,56,156,156};
    TimedClick click=scaled_click(xs[p->step],ys[p->step]); push_timed_click(0,&click);
    return ++p->step==4 ? 0 : interval;
}
Uint32 push_map_loot_cancel_owned(Uint32 interval,void *data) {
    TimedLootPage *p=static_cast<TimedLootPage *>(data);
    const int xs[]={p->full ? 110 : 260,540,540},ys[]={274,156,156};
    TimedClick click=scaled_click(xs[p->step],ys[p->step]); push_timed_click(0,&click);
    return ++p->step==3 ? 0 : interval;
}
Uint32 push_map_loot_owned_swap(Uint32 interval,void *data) {
    TimedLootPage *p=static_cast<TimedLootPage *>(data);
    const int xs[]={30,110,540,110,30,540},ys[]={56,274,156,274,56,156};
    TimedClick click=scaled_click(xs[p->step],ys[p->step]); push_timed_click(0,&click);
    return ++p->step==6 ? 0 : interval;
}
Uint32 push_map_loot_empty_row(Uint32 interval,void *data) {
    TimedLootPage *p=static_cast<TimedLootPage *>(data);
    const int xs[]={30,540,30,540},ys[]={56,104,152,156};
    TimedClick click=scaled_click(xs[p->step],ys[p->step]); push_timed_click(0,&click);
    return ++p->step==4 ? 0 : interval;
}
Uint32 push_end_action(Uint32 interval,void *data) {
    TimedEndAction *p=static_cast<TimedEndAction *>(data);
    if(p->step==0 || p->step==2 || p->step==3) {
        TimedClick c=scaled_click(p->step==0 ? 314 : 530,p->step==0 ? 490 : 490);
        SDL_Event e; SDL_memset(&e,0,sizeof(e));
        if(p->step!=3) { e.type=SDL_MOUSEMOTION; e.motion.which=255; e.motion.x=c.x; e.motion.y=c.y; SDL_PushEvent(&e); }
        if(p->step==2 || p->step==3) {
            SDL_memset(&e,0,sizeof(e)); e.type=p->step==2 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
            e.button.which=255; e.button.button=SDL_BUTTON_LEFT; e.button.state=p->step==2 ? SDL_PRESSED : SDL_RELEASED; e.button.x=c.x; e.button.y=c.y; SDL_PushEvent(&e);
        }
    } else {
        const int xs[]={0,314,0,0,30,260,540},ys[]={0,490,0,0,56,274,156};
        TimedClick c=scaled_click(xs[p->step],ys[p->step]); push_timed_click(0,&c);
    }
    return ++p->step==7 ? 0 : interval;
}
Uint32 push_hostile_end(Uint32 interval,void *data) {
    TimedEndAction *p=static_cast<TimedEndAction *>(data);
    const int x=p->step<2 ? 314 : (p->step<4 ? 530 : (p->step<6 ? 90 : 300));
    const int y=p->step<6 ? 490 : 200;
    TimedClick c=scaled_click(x,y); SDL_Event e; SDL_memset(&e,0,sizeof(e));
    if(p->step==1) push_timed_click(0,&c);
    else if(p->step==0 || p->step==6) { e.type=SDL_MOUSEMOTION; e.motion.which=255; e.motion.x=c.x; e.motion.y=c.y; SDL_PushEvent(&e); }
    else {
        if(p->step==2 || p->step==4) { e.type=SDL_MOUSEMOTION; e.motion.which=255; e.motion.x=c.x; e.motion.y=c.y; SDL_PushEvent(&e); }
        SDL_memset(&e,0,sizeof(e)); const bool down=(p->step==2 || p->step==4);
        e.type=down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_LEFT; e.button.state=down ? SDL_PRESSED : SDL_RELEASED; e.button.x=c.x; e.button.y=c.y; SDL_PushEvent(&e);
    }
    return ++p->step==7 ? 0 : interval;
}
Uint32 push_inventory_swap(Uint32 interval,void *data) {
    TimedSwap *p=static_cast<TimedSwap *>(data);
    if(p->step<3) {
        TimedClick click=p->step==1 ? scaled_click(p->x,p->y) : scaled_click(260,274);
        push_timed_click(0,&click);
    } else {
        SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONDOWN;
        e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED;
        TimedClick click=scaled_click(260,274); e.button.x=click.x; e.button.y=click.y; SDL_PushEvent(&e);
    }
    return ++p->step==4 ? 0 : interval;
}
Uint32 push_quest_action(Uint32 interval,void *data) {
    TimedQuestAction *p=static_cast<TimedQuestAction *>(data);
    const int xs[]={210,260,310}, ys[]={334,380,334};
    TimedClick click=scaled_click(xs[p->step],ys[p->step]);
    push_timed_click(0,&click);
    return ++p->step==3 ? 0 : interval;
}
Uint32 push_limit_growth(Uint32 interval,void *data) {
    TimedGrowth *p=static_cast<TimedGrowth *>(data);
    if(p->step<10) {
        const int x=p->step==0 ? 500 : (p->step==9 ? 30 : 380);
        const int y=p->step==0 ? 334 : (p->step==9 ? 200 : (p->step<=4 ? 94 : 128));
        TimedClick click=scaled_click(x,y); push_timed_click(0,&click);
    } else {
        SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONDOWN; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED;
        TimedClick click=scaled_click(260,274); e.button.x=click.x; e.button.y=click.y; SDL_PushEvent(&e);
    }
    return ++p->step==11 ? 0 : interval;
}
Uint32 push_shop_equipment(Uint32 interval,void *data) {
    TimedShopEquipment *p=static_cast<TimedShopEquipment *>(data);
    if(p->mode==8 || p->mode==9) {
        const int xs8[]={576,30,606,472},ys8[]={62,438,210,62};
        const int xs9[]={440,30,526},ys9[]={450,438,110};
        const int count=p->mode==8 ? 4 : 3;
        if(p->step<count) {
            TimedClick click=scaled_click(p->mode==8 ? xs8[p->step] : xs9[p->step],p->mode==8 ? ys8[p->step] : ys9[p->step]);
            push_timed_click(0,&click);
        } else {
            SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONDOWN;
            e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED;
            TimedClick click=scaled_click(300,200); e.button.x=click.x; e.button.y=click.y; SDL_PushEvent(&e);
        }
        return ++p->step==count+1 ? 0 : interval;
    }
    // Modes 0/1: sell or move the worn helmet; 2: cancel its removal;
    // 3: buy helmet; 4: rejected replacement; 5: put helmet back;
    // 6/7: move worn weapon to an empty/occupied other hand.
    const bool cancel=(p->mode==2 || p->mode==4 || p->mode==10);
    const int clicksBeforeRight=p->mode==2 ? 1 : 2;
    if(p->step<clicksBeforeRight) {
        int x=0,y=0;
        if(p->step==0) {
            if(p->mode<=2 || p->mode==5 || p->mode==10) { x=526; y=110; }
            else if(p->mode>=6) { x=606; y=210; }
            else { x=30; y=438; }
        }
        else if(p->mode==0 || p->mode==10) { x=30; y=438; }
        else if(p->mode==1) { x=490; y=450; }
        else if(p->mode==3 || p->mode==5) { x=526; y=110; }
        else if(p->mode>=6) { x=446; y=210; }
        else { x=606; y=210; }
        TimedClick click=scaled_click(x,y); push_timed_click(0,&click);
    } else {
        const int phase=p->step-clicksBeforeRight;
        if(p->mode==4 && phase==1) shop_restored_capture=true;
        SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=phase%2==0 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        e.button.which=255; e.button.button=SDL_BUTTON_RIGHT;
        e.button.state=phase%2==0 ? SDL_PRESSED : SDL_RELEASED;
        TimedClick click=scaled_click(300,200); e.button.x=click.x; e.button.y=click.y; SDL_PushEvent(&e);
    }
    ++p->step;
    return p->step==(clicksBeforeRight+(cancel ? 3 : 1)) ? 0 : interval;
}
Uint32 push_growth(Uint32 interval,void *data) {
    TimedGrowth *p=static_cast<TimedGrowth *>(data);
    if(p->step<7) {
        const int xs[]={500,380,380,380,380,380,30};
        const int ys[]={334,94,94,94,94,60,200};
        TimedClick click=scaled_click(xs[p->step],ys[p->step]); push_timed_click(0,&click);
    } else {
        SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONDOWN;
        e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED;
        TimedClick click=scaled_click(260,274); e.button.x=click.x; e.button.y=click.y; SDL_PushEvent(&e);
    }
    return ++p->step==8 ? 0 : interval;
}
Uint32 push_right_then_space(Uint32 interval,void *data) {
    TimedRightThenSpace *p=static_cast<TimedRightThenSpace *>(data);
    SDL_Event e; SDL_memset(&e,0,sizeof(e));
    if(p->step==0) {
        e.type=SDL_MOUSEMOTION; e.motion.which=255; e.motion.x=p->click.x; e.motion.y=p->click.y; SDL_PushEvent(&e);
        SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONDOWN; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_PRESSED; e.button.x=p->click.x; e.button.y=p->click.y; SDL_PushEvent(&e);
        e.type=SDL_MOUSEBUTTONUP; e.button.state=SDL_RELEASED; SDL_PushEvent(&e);
    } else {
        e.type=SDL_KEYDOWN; e.key.which=255; e.key.keysym.sym=SDLK_SPACE; SDL_PushEvent(&e);
        e.type=SDL_KEYUP; SDL_PushEvent(&e);
    }
    return ++p->step==2 ? 0 : interval;
}
Uint32 push_dialog_click(Uint32 interval,void *data) {
    TimedDialogue *p=static_cast<TimedDialogue *>(data);
    TimedClick click = p->step==0 ? scaled_click(60,260) : scaled_click(340,454);
    push_timed_click(0,&click);
    return ++p->step==2 ? 0 : interval;
}
Uint32 push_input_test_key(Uint32 interval,void *data) {
    TimedKeys *p=static_cast<TimedKeys *>(data);
    const SDLKey keys[]={SDLK_LEFT,SDLK_BACKSPACE,SDLK_a,SDLK_e,SDLK_RETURN};
    SDLMod mod=p->step==3 ? KMOD_LALT : KMOD_NONE;
    SDL_SetModState(mod);
    SDL_Event e; SDL_memset(&e,0,sizeof(e));
    e.type=SDL_KEYDOWN; e.key.which=255; e.key.keysym.sym=keys[p->step]; e.key.keysym.mod=mod; SDL_PushEvent(&e);
    e.type=SDL_KEYUP; SDL_PushEvent(&e);
    return ++p->step==5 ? 0 : interval;
}
Uint32 push_space(Uint32,void *) {
    SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_KEYDOWN;
    e.key.which=255; e.key.keysym.sym=SDLK_SPACE; SDL_PushEvent(&e);
    e.type=SDL_KEYUP; SDL_PushEvent(&e); return 0;
}
bool run_short_tactical_action() {
    ClearKey(); ProcessEvents();
    SDL_TimerID timer=SDL_AddTimer(250,push_space,0);
    if(!timer) return false;
    AKCJA(); SDL_RemoveTimer(timer);
    return TacticalPauseHealthBars();
}
Uint32 push_right_release(Uint32,void *data) {
    TimedClick *p=static_cast<TimedClick *>(data);
    SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP;
    e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=SDL_RELEASED; e.button.x=p->x; e.button.y=p->y; SDL_PushEvent(&e); return 0;
}
void push_right_event(TimedClick p,bool down) {
    SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEMOTION;
    e.motion.which=255; e.motion.x=p.x; e.motion.y=p.y; SDL_PushEvent(&e);
    SDL_memset(&e,0,sizeof(e)); e.type=down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; e.button.state=down ? SDL_PRESSED : SDL_RELEASED;
    e.button.x=p.x; e.button.y=p.y; SDL_PushEvent(&e);
}
Uint32 push_timed_click(Uint32,void *data) {
    TimedClick *p=static_cast<TimedClick *>(data);
    SDL_Event e; SDL_memset(&e,0,sizeof(e));
    e.type=SDL_MOUSEMOTION; e.motion.which=255; e.motion.x=p->x; e.motion.y=p->y; SDL_PushEvent(&e);
    SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONDOWN;
    e.button.which=255; e.button.button=SDL_BUTTON_LEFT; e.button.state=SDL_PRESSED; e.button.x=p->x; e.button.y=p->y; SDL_PushEvent(&e);
    e.type=SDL_MOUSEBUTTONUP; e.button.state=SDL_RELEASED; SDL_PushEvent(&e);
    return 0;
}
struct TestState {
    std::vector<Rect> obstacles;
    std::vector<Point> history[11];
    bool ready, font_sample, army_sample, name_sample, map_sample, defined[11];
    int army[11];
    int tick;
    TestState() : ready(false), font_sample(false), army_sample(false), name_sample(false), map_sample(false), tick(0) { std::fill(defined, defined+11, false); std::fill(army,army+11,0); }
};

void state_json(std::ostream &out, const TestState &t, int unit) {
    const int a=t.army[unit];
    const int x=ARMIA[a][unit][TX], y=ARMIA[a][unit][TY];
    const int dx=ARMIA[a][unit][TCELX]-x, dy=ARMIA[a][unit][TCELY]-y;
    Screen(0);
    out << "{\"tick\":" << t.tick << ",\"unit\":" << unit << ",\"army\":" << a
        << ",\"x\":" << x << ",\"y\":" << y
        << ",\"target_x\":" << ARMIA[a][unit][TCELX]
        << ",\"target_y\":" << ARMIA[a][unit][TCELY]
        << ",\"mode\":" << ARMIA[a][unit][TTRYB]
        << ",\"energy\":" << ARMIA[a][unit][TE]
        << ",\"speed\":" << ARMIA[a][unit][TSZ]
        << ",\"zone\":" << _Zone(x,y)
        << ",\"distance\":" << std::sqrt(double(dx*dx+dy*dy)) << "}";
}

void render(const TestState &t) {
    Screen(0);
    if(t.map_sample) { rysuj_mape(); glFlush(); return; }
    if(t.name_sample) {
        SetFont(FON2); OKX=0; OKY=0;
        _main_code_rysuj_your_name();
        glFlush(); return;
    }
    rysuj_scenerie();
    DrawTacticalPauseHealthBars();
    // Synthetic obstacles are visible in captures as red rectangles.
    Gfx::Color(1.f,0.f,0.f,0.35f);
    for (size_t i=0; i<t.obstacles.size(); ++i) {
        const Rect &r=t.obstacles[i];
        _Bar(r.x1,r.y1,r.x2,r.y2);
        Gfx::Color(1.f,0.2f,0.2f,1.f); _Box(r.x1,r.y1,r.x2,r.y2);
        Gfx::Color(1.f,0.f,0.f,0.35f);
    }
    if(t.font_sample) {
        Gfx::Color(0.f,0.f,0.f,1.f); _Bar(5,5,310,105);
        Gfx::Color(1.f,1.f,1.f,1.f);
        SetFont(FON2); Text(12,36,"Imię: Łódź");
        // Exercise retrieving an already-loaded font of a different size.
        USTAW_FONT("defender2",FONT_DEFENDER,8,1);
        Text(12,66,"Legion: żołnierze, szybkość 5");
    }
    if(t.army_sample) {
        Screen(0); SetFont(FON1); SX=0; SY=0;
        _ARMIA_RYSUJ_INFO(ARM);
    }
    Gfx::Color(1.f,1.f,1.f,1.f);
    glFlush();
}

bool capture_buffer(const std::string &path) {
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    if (vp[2]<=0 || vp[3]<=0) return false;
    Gfx::Pic image;
    image.Set(vp[2],vp[3],Gfx::PDM_RGBA);
    while (glGetError()!=GL_NO_ERROR) {} // Discard errors left by legacy rendering.
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadBuffer(GL_BACK);
    glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,image.GetBufferWR());
    Gfx::PicWriteFlags(Gfx::PICF_VREV);
    int result=image.SavePNG(path);
    Gfx::PicWriteFlags(Gfx::PICF_NONE);
    return result>=0 && glGetError()==GL_NO_ERROR;
}

bool snapshot(const TestState &t, const std::string &path) {
    render(t);
    return capture_buffer(path);
}

bool action_marker_pixel(const TestState &t,int n,int colour) {
    render(t);
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    Screen(0);
    const int px=XHard(ARMIA[ARM][n][TX])*vp[2]/640;
    const int py=YHard(ARMIA[ARM][n][TY]-48)*vp[3]/520;
    if(px<0 || px>=vp[2] || py<0 || py>=vp[3]) {
        std::cout<<"{\"event\":\"marker_outside_view\",\"unit\":"<<n<<",\"x\":"<<px<<",\"y\":"<<py<<"}"<<std::endl;
        return false;
    }
    unsigned char pixel[4]={}; glReadBuffer(GL_BACK);
    glReadPixels(vp[0]+px,vp[1]+vp[3]-1-py,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    bool result;
    if(colour==MarkerIdle) result=pixel[1]>180 && pixel[0]<100 && pixel[2]<100;
    else if(colour==MarkerMoving) result=pixel[0]>180 && pixel[1]>160 && pixel[2]<100;
    else if(colour==MarkerAttacking) result=pixel[0]>180 && pixel[1]<100 && pixel[2]<100;
    else result=pixel[0]<100 && pixel[1]>100 && pixel[2]>180;
    if(!result) std::cout<<"{\"event\":\"marker_pixel\",\"unit\":"<<n<<",\"expected\":"<<colour<<",\"r\":"<<int(pixel[0])<<",\"g\":"<<int(pixel[1])<<",\"b\":"<<int(pixel[2])<<"}"<<std::endl;
    return result;
}

bool fonts_visible(const TestState &t) {
    render(t);
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    if(vp[2]<=0 || vp[3]<=0) return false;
    std::vector<unsigned char> pixels(size_t(vp[2])*vp[3]*4);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadBuffer(GL_BACK);
    glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,&pixels[0]);
    int counts[2]={0,0}, min_y[2]={vp[3],vp[3]}, max_y[2]={-1,-1};
    for(int y=10*vp[3]/520;y<190*vp[3]/520;++y) {
        int band=y<90*vp[3]/520 ? 0 : 1;
        for(int x=20*vp[2]/640;x<600*vp[2]/640;++x) {
            size_t p=(size_t(vp[3]-1-y)*vp[2]+x)*4;
            if(pixels[p]>120 && pixels[p+1]>120 && pixels[p+2]>120) {
                ++counts[band]; min_y[band]=std::min(min_y[band],y); max_y[band]=std::max(max_y[band],y);
            }
        }
    }
    bool ok=counts[0]>30 && counts[1]>20 && max_y[0]-min_y[0]>max_y[1]-min_y[1];
    std::cout<<"{\"event\":\"fonts\",\"large_pixels\":"<<counts[0]<<",\"small_pixels\":"<<counts[1]
             <<",\"visible\":"<<(ok?"true":"false")<<"}"<<std::endl;
    return ok;
}

bool ground_icon_visible(const TestState &t,int sector,int slot) {
    int x,y;
    if(!GroundItemPosition(sector,slot,x,y)) return false;
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    if(vp[2]<=0 || vp[3]<=0) return false;
    std::vector<unsigned char> before(size_t(vp[2])*vp[3]*4),after(before.size());
    render(t); glPixelStorei(GL_PACK_ALIGNMENT,1); glReadBuffer(GL_BACK);
    glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,&before[0]);
    int item=GLEBA[sector][slot]; GLEBA[sector][slot]=0;
    render(t);
    glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,&after[0]);
    GLEBA[sector][slot]=item;
    Screen(0);
    int px=XHard(x)*vp[2]/640,py=YHard(y)*vp[3]/520,changed=0;
    int rx=std::max(1,30*vp[2]/640),ry=std::max(1,30*vp[3]/520);
    for(int yy=std::max(0,py-ry);yy<std::min(vp[3],py+ry);++yy)
        for(int xx=std::max(0,px-rx);xx<std::min(vp[2],px+rx);++xx) {
            size_t p=(size_t(vp[3]-1-yy)*vp[2]+xx)*4;
            int delta=std::abs(int(before[p])-int(after[p]))+
                      std::abs(int(before[p+1])-int(after[p+1]))+
                      std::abs(int(before[p+2])-int(after[p+2]));
            if(delta>60) ++changed;
        }
    std::cout<<"{\"event\":\"ground_render\",\"sector\":"<<sector<<",\"slot\":"<<slot<<",\"changed_pixels\":"<<changed<<"}"<<std::endl;
    return changed>8;
}

bool world_marker_visible(const TestState &t,int city) {
    if(!t.map_sample || !CityHasPendingQuestInfo(city)) return false;
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    if(vp[2]<=0 || vp[3]<=0) return false;
    std::vector<unsigned char> before(size_t(vp[2])*vp[3]*4),after(before.size());
    render(t); glPixelStorei(GL_PACK_ALIGNMENT,1); glReadBuffer(GL_BACK);
    glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,&before[0]);
    int types[4];
    for(int i=0;i<4;++i) {
        types[i]=PRZYGODY[i][P_TYP];
        if(PRZYGODY[i][P_LEVEL]>0 && PRZYGODY[i][P_X]==city) PRZYGODY[i][P_TYP]=0;
    }
    render(t); glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,&after[0]);
    for(int i=0;i<4;++i) PRZYGODY[i][P_TYP]=types[i];
    int changed=0;
    for(size_t p=0;p<before.size();p+=4)
        if(std::abs(int(before[p])-int(after[p]))+std::abs(int(before[p+1])-int(after[p+1]))+
           std::abs(int(before[p+2])-int(after[p+2]))>60) ++changed;
    std::cout<<"{\"event\":\"world_marker_render\",\"city\":"<<city<<",\"changed_pixels\":"<<changed<<"}"<<std::endl;
    return changed>8;
}

bool health_bar_visible(const TestState &t,int n) {
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    if(vp[2]<=0 || vp[3]<=0) return false;
    std::vector<unsigned char> before(size_t(vp[2])*vp[3]*4),after(before.size());
    bool paused=TacticalPauseHealthBars();
    render(t); glPixelStorei(GL_PACK_ALIGNMENT,1); glReadBuffer(GL_BACK);
    glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,&before[0]);
    SetTacticalPauseHealthBars(false); render(t);
    glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,&after[0]);
    SetTacticalPauseHealthBars(paused);
    Screen(0);
    int a=t.army[n],x=ARMIA[a][n][TX],y=std::max(1,int(ARMIA[a][n][TY])-56);
    int px=XHard(x)*vp[2]/640,py=YHard(y+2)*vp[3]/520,changed=0;
    int rx=std::max(1,40*vp[2]/640),ry=std::max(1,8*vp[3]/520);
    for(int yy=std::max(0,py-ry);yy<std::min(vp[3],py+ry);++yy)
        for(int xx=std::max(0,px-rx);xx<std::min(vp[2],px+rx);++xx) {
            size_t p=(size_t(vp[3]-1-yy)*vp[2]+xx)*4;
            int delta=std::abs(int(before[p])-int(after[p]))+std::abs(int(before[p+1])-int(after[p+1]))+std::abs(int(before[p+2])-int(after[p+2]));
            if(delta>50) ++changed;
        }
    std::cout<<"{\"event\":\"health_bar\",\"unit\":"<<n<<",\"changed_pixels\":"<<changed<<"}"<<std::endl;
    return changed>8;
}

int health_bar_fill(const TestState &t,int n) {
    render(t); GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    if(vp[2]<=0 || vp[3]<=0) return -1;
    std::vector<unsigned char> pixels(size_t(vp[2])*vp[3]*4);
    glPixelStorei(GL_PACK_ALIGNMENT,1); glReadBuffer(GL_BACK);
    glReadPixels(vp[0],vp[1],vp[2],vp[3],GL_RGBA,GL_UNSIGNED_BYTE,&pixels[0]);
    Screen(0); int a=t.army[n],x=ARMIA[a][n][TX],y=std::max(1,int(ARMIA[a][n][TY])-56)+2,count=0;
    for(int i=0;i<30;++i) {
        int px=XHard(x-15+i)*vp[2]/640,py=YHard(y)*vp[3]/520;
        if(px<0 || px>=vp[2] || py<0 || py>=vp[3]) continue;
        size_t p=(size_t(vp[3]-1-py)*vp[2]+px)*4;
        if(pixels[p]>140 || pixels[p+1]>140) ++count;
    }
    std::cout<<"{\"event\":\"health_fill\",\"unit\":"<<n<<",\"pixels\":"<<count<<"}"<<std::endl;
    return count;
}

bool valid_unit(const TestState &t,int n) {return n>=1 && n<=10 && t.defined[n];}
bool valid_xy(int x,int y) {return x>=16 && x<=623 && y>=22 && y<=508;}
bool inject_mouse(int x,int y) {
    SDL_Event e; SDL_memset(&e,0,sizeof(e));
    e.type=SDL_MOUSEMOTION; e.motion.which=255; e.motion.x=x; e.motion.y=y;
    SDL_PushEvent(&e); ProcessEvents();
    {
        SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONDOWN;
        e.button.which=255; e.button.button=SDL_BUTTON_LEFT; e.button.state=SDL_PRESSED;
        e.button.x=x; e.button.y=y; SDL_PushEvent(&e); ProcessEvents();
        bool observed=MouseClick();
        std::cout << "{\"event\":\"click\",\"screen_x\":" << x
                  << ",\"screen_y\":" << y << ",\"detected\":" << (observed?"true":"false")
                  << ",\"zone\":" << MouseZone() << "}" << std::endl;
        SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP;
        e.button.which=255; e.button.button=SDL_BUTTON_LEFT; e.button.state=SDL_RELEASED;
        e.button.x=x; e.button.y=y; SDL_PushEvent(&e); ProcessEvents();
        return observed;
    }
}
}

void LegionTestQuestDialogUpdated() {
    if(!dialog_capture_enabled) return;
    rysuj();
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    unsigned char pixel[4]={0,0,0,0};
    glReadBuffer(GL_BACK);
    glReadPixels(vp[0]+320*vp[2]/640,vp[1]+vp[3]-1-364*vp[3]/520,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    dialog_capture_ok=pixel[0]>25 && pixel[1]>15 && pixel[2]>10 && outdir && capture_buffer(std::string(outdir)+"/dialog_po_nowym_zadaniu.png");
    std::cout<<"{\"event\":\"quest_dialog_capture\",\"red\":"<<int(pixel[0])<<",\"green\":"<<int(pixel[1])<<",\"blue\":"<<int(pixel[2])<<",\"visible\":"<<(dialog_capture_ok?"true":"false")<<"}"<<std::endl;
}
void LegionTestQuestDialogOpened(const char *label) {
    if(!dialog_capture_enabled) return;
    dialog_label_ok=label && std::string(label)=="Nowe zadanie (!)";
    rysuj();
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/nowe_zadanie.png");
}

void LegionTestGrowthPanelFrame() {
    if(!growth_test_enabled) return;
    const int points=ARMIA[ARM][NUMER][TDOSW];
    const int bit=points==10 ? 1 : points==1 ? 2 : points==0 ? 4 : 0;
    if(!bit || (growth_frame_mask&bit)) return;
    growth_frame_mask|=bit;
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/rozwoj_"+std::to_string(points)+".png");
}
void LegionTestInspectionFrame() {
    if(!inspection_test_enabled || inspection_seen) return;
    inspection_seen=true; const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/statystyki_po_przytrzymaniu.png");
}
bool LegionTestTitleActive() { return title_test_active; }
void LegionTestTitleMenuFrame() {
    if(!title_test_active || !title_pending) return;
    ++title_menu_frames;
    if(title_menu_frames==10) {
        const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
        if(outdir) capture_buffer(std::string(outdir)+"/menu_windows.png");
    }
    if(!title_timer) title_timer=SDL_AddTimer(1400,push_title_continue,title_pending);
}
void LegionTestTitleDialogFrame() {
    if(!title_test_active) return;
    ++title_dialog_frames;
    if(title_dialog_captured) return;
    title_dialog_captured=true;
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/wybor_zapisu_menu.png");
}
void LegionTestEquipmentModeFrame() {
    if(!mode_capture_enabled || mode_capture_seen || EquipmentMode(ARM,NUMER)!=1) return;
    mode_capture_seen=true; const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/zestaw_2.png");
}
void LegionTestTacticalFrame() {
    if(tactical_frame_test && ARMIA[ARM][1][TTRYB]==2) { tactical_frame_seen=true; KONIEC_AKCJI=-1; }
    if(hostile_end_test && ARMIA[ARM][1][TTRYB]==0) { hostile_end_seen=true; KONIEC_AKCJI=-1; }
    if(tactical_mode_test && (EquipmentMode(ARM,1)==1 || SDL_GetTicks()-tactical_mode_start>4000)) {
        tactical_mode_seen=EquipmentMode(ARM,1)==1; KONIEC_AKCJI=-1;
    }
}
void LegionTestInventoryShown() {
    if(!inventory_capture_enabled) return;
    rysuj();
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/ekwipunek_przy_granicy.png");
}
void LegionTestMapLootFrame() {
    if(!map_loot_capture_enabled || map_loot_capture_seen) return;
    map_loot_capture_seen=true;
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/ekwipunek_mapy.png");
}
void LegionTestMapLootHeldFrame(int item) {
    if(!map_loot_held_test || map_loot_held_seen) return;
    map_loot_held_seen=item==24 && GLEBA[7][2]==0 && ARMIA[ARM][NUMER][TPLECAK]==map_loot_expected_bag;
    if(map_loot_held_seen) {
        _SpriteUpdate();
        const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
        if(outdir) capture_buffer(std::string(outdir)+"/ekwipunek_mapy_przedmiot.png");
    }
}
void LegionTestItemInfoShown() {
    if(!item_info_capture_enabled) return;
    item_info_capture_enabled=false;
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/parametry_przedmiotu.png");
}
void LegionTestHoverItemInfo(int item,bool shop,bool onCard) {
    if(!hover_test || (hover_test->mode==1)!=shop || hover_test->step<1 || hover_test->step>5) return;
    const int expected[]={24,27,25,3,4};
    const int slot=hover_test->step-1;
    if(item==expected[slot] && onCard && !(hover_mask & (1<<slot))) {
        hover_mask |= 1<<slot;
        if(slot==0) {
            const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
            if(outdir) capture_buffer(std::string(outdir)+(shop ? "/sklep_najechanie.png" : (hover_test->mode==2 ? "/lupy_najechanie.png" : "/ekwipunek_najechanie.png")));
        }
    }
}
void LegionTestInventoryNavigationFrame() {
    if(!navigation_test || navigation_test->step<1 || navigation_test->step>4) return;
    const int expected[]={3,1,3,1};
    const int step=navigation_test->step-1;
    if(NUMER==expected[step]) navigation_mask|=1<<step;
}
void LegionTestShopFrame() {
    if(!shop_restored_capture || shop_restored_seen || SKLEP[1][0]!=4) return;
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) shop_restored_seen=capture_buffer(std::string(outdir)+"/sklep_przywrocony_towar.png");
}
void LegionTestShopItemInfoShown() {
    if(shop_worn_capture && !shop_worn_seen) {
        shop_worn_seen=ARMIA[ARM][1][TGLOWA]==0 && SKLEP[1][0]==0 && GRACZE[1][1]==10000;
        if(shop_worn_seen) {
            _SpriteUpdate();
            const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
            if(outdir) capture_buffer(std::string(outdir)+"/sklep_zdjety_helm.png");
        }
    }
    if(shop_duplicate_check && !shop_duplicate_ok) {
        Gfx::LTex *tex=_GetImage(1,BRON[24][B_BOB]+BROBY);
        shop_duplicate_ok=tex && tex->GetHotSpotX()==0.f && tex->GetHotSpotY()==0.f && ARMIA[ARM][1][TGLOWA]==24 && ARMIA[ARM][1][TPLECAK]==0;
    }
    if(!shop_info_capture_enabled) return;
    shop_info_capture_enabled=false;
    if(shop_duplicate_check) _SpriteUpdate();
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/sklep_parametry.png");
}
void LegionTestQuestSceneStarted() {
    if(!quest_scene_test_enabled) return;
    quest_scene_seen=TRWA_PRZYGODA==0 && SCENERIA==8;
    KONIEC_AKCJI=-1;
}
void LegionTestArmyWindowFrame(int which) {
    if(!quest_scene_test_enabled || which<1 || which>2 || army_frame_seen[which-1]) return;
    army_frame_seen[which-1]=true;
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+(which==1 ? "/ramka_legionu.png" : "/ramka_rozkazow.png"));
}
void LegionTestShopTransitionFrame(bool entering) {
    if(!shop_transition_test) return;
    const int phase=entering ? 0 : 1;
    const int count=shop_transition_count[phase]++;
    if(count%12!=0) return;
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT,vp);
    unsigned char pixels[16*16*4]={};
    glFlush(); glReadBuffer(GL_BACK);
    glReadPixels(vp[0]+vp[2]/2-8,vp[1]+vp[3]/2-8,16,16,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    int lit=0; for(int i=0;i<16*16;i++) if(pixels[i*4]+pixels[i*4+1]+pixels[i*4+2]>24) ++lit;
    if(lit<8) shop_transition_ok=false;
    if(count==12) {
        const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
        if(outdir) capture_buffer(std::string(outdir)+(entering ? "/sklep_wejscie.png" : "/sklep_wyjscie.png"));
    }
}
void LegionTestActiveActionFrame() {
    if(!action_frame_enabled) return;
    action_frame_enabled=false; action_frame_unpaused=!TacticalPauseHealthBars();
    action_frame_markers=ActionMarkersVisible();
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/akcja_bez_paskow.png");
}
void LegionTestQuestAnswer(int,const char *text) {
    if(!quest_answer_enabled) return;
    quest_answer_text=text; quest_answer_captured=true;
    rysuj();
    const char *outdir=std::getenv("LEGION_TEST_OUTPUT");
    if(outdir) capture_buffer(std::string(outdir)+"/odpowiedz_na_zadanie.png");
}

int RunLegionTest(const char *script_path) {
    std::ifstream in(script_path);
    if (!in) { std::cerr << "TEST: cannot open scenario: " << script_path << std::endl; return 2; }
    const char *outputEnv=std::getenv("LEGION_TEST_OUTPUT");
    if (!outputEnv || !*outputEnv) { std::cerr << "TEST: LEGION_TEST_OUTPUT is required" << std::endl; return 2; }
    // Modifying environment variables on Windows can invalidate getenv pointers.
    const std::string outputStorage(outputEnv);
    const char *outdir=outputStorage.c_str();
    std::ofstream trace((std::string(outdir)+"/trace.jsonl").c_str());
    if (!trace) { std::cerr << "TEST: cannot open trace" << std::endl; return 2; }
    TestState t;
    ARM=0; WRG=40; KTO_ATAKUJE=ARM; TESTING=0; POWER=7; PREFS[3]=0;
    MSX=320; MSY=278;
    AN[0]=0; AN[1]=1; AN[2]=0; AN[3]=2;
    // SETUP needs fonts even when the ordinary intro/menu is skipped.
    ScreenOpen(0,640,512,32,LOWRES); Screen(0);
    USTAW_FONT("defender2",FONT_DEFENDER,8,1); FON1=FONR;
    USTAW_FONT("garnet",FONT_GARNET,24,1); FON2=FONR;
    SETUP("TEST","AUTOMATION","");
    Screen(0);
    Core::Randomize(1);
    std::string line; int lineno=0;
    while (std::getline(in,line)) {
        ++lineno; std::istringstream args(line); std::string op;
        if (!(args>>op) || op[0]=='#') continue;
        bool ok=true;
        if (op=="seed") {
            unsigned long n; ok=bool(args>>n); if(ok) Core::Randomize(n);
        } else if (op=="expect_display") {
            int width,height; ok=bool(args>>width>>height) && width>0 && height>0;
            if(ok) {
                GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
                SDL_Surface *display=SDL_GetVideoSurface();
                ok=display && display->w==width && display->h==height && viewport[2]==width && viewport[3]==height;
                std::cout<<"{\"event\":\"display\",\"width\":"<<viewport[2]<<",\"height\":"<<viewport[3]<<",\"expected\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="expect_scaled_display") {
            GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
            SDL_Surface *display=SDL_GetVideoSurface();
            int width=viewport[2],height=viewport[3];
            ok=display && display->w==width && display->h==height && width>640 && height>520 &&
               width<=1280 && height<=1040 && std::abs(width*520-height*640)<=640;
            std::cout<<"{\"event\":\"display\",\"width\":"<<width<<",\"height\":"<<height<<",\"expected\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="load_save") {
            int slot; ok=!t.ready && bool(args>>slot) && slot>=1 && slot<=5;
            if(ok) {
                std::string path=KAT_S+"archiwum/zapis "+std::to_string(slot);
                std::ifstream save(path.c_str(),std::ios::binary);
                std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(save)),std::istreambuf_iterator<char>());
                ok=bytes.size()>20 && bytes.size()<=60000;
                if(ok) { size_t length=bytes.size()-20; bytes.resize(60000,0); ODCZYT(&bytes[20],length); ARM=0; WRG=40; }
            }
        } else if (op=="scene" || op=="reload_scene") {
            int n; std::string mode; ok=(!t.ready || op=="reload_scene") && bool(args>>n>>mode) && n>=1 && n<=10 && (mode=="original" || mode=="clear" || mode=="village" || mode=="village_original");
            if(ok) {
                if(op=="reload_scene") {
                    Screen(0); for(int i=21;i<200;++i) ResetZone(i);
                    for(int sector=0;sector<=110;++sector) for(int slot=0;slot<=4;++slot) GLEBA[sector][slot]=0;
                    // SETUP clears the sprite bank before loading a new scene;
                    // loading a second terrain into it would append duplicate sprites.
                    SETUP("TEST","AUTOMATION",""); Screen(0);
                    t=TestState();
                }
                if(mode=="village" || mode=="village_original") {
                    for(int i=2;i<=20;++i) MIASTA[3][i][M_LUDZIE]=0;
                    MIASTA[3][2][M_LUDZIE]=1;
                    MIASTA[3][2][M_X]=190; MIASTA[3][2][M_Y]=145;
                }
                RYSUJ_SCENERIE(n,(mode=="village" || mode=="village_original") ? 3 : -1); Screen(0);
                if(mode=="clear" || mode=="village") {
                    for(int i=21;i<200;++i) ResetZone(i);
                    for(int s=0;s<=110;++s) for(int i=0;i<=4;++i) GLEBA[s][i]=0;
                }
                t.obstacles.clear(); t.ready=true;
                std::cout << "{\"event\":\"scene\",\"terrain\":"<<n<<",\"mode\":\""<<mode<<"\"}" << std::endl;
            }
        } else if (op=="expect_generated_arrows") {
            int minimum,maximum; ok=t.ready && bool(args>>minimum>>maximum);
            if(ok) {
                int count=0; for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot)
                    if(GLEBA[sector][slot]==39) ++count;
                ok=count>=minimum && count<=maximum;
                std::cout<<"{\"event\":\"generated_arrows\",\"count\":"<<count<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="obstacle") {
            Rect r; ok=t.ready && bool(args>>r.id>>r.x1>>r.y1>>r.x2>>r.y2) && r.id>=60 && r.id<=119 && r.x1>=0 && r.y1>=0 && r.x2<=639 && r.y2<=511 && r.x1<r.x2 && r.y1<r.y2;
            if(ok) {Screen(0); SetZone(r.id,r.x1,r.y1,r.x2,r.y2); t.obstacles.push_back(r);}
        } else if (op=="ground") {
            int sector,slot,item;ok=t.ready && bool(args>>sector>>slot>>item) && sector>=0 && sector<110 && slot>=0 && slot<4 && item>=0 && item<=120;
            if(ok) GLEBA[sector][slot]=item;
        } else if (op=="world_map") {
            ok=t.ready;
            if(ok) { SETUP0(); Screen(0); t.map_sample=true; }
        } else if (op=="map_city") {
            int city,x,y; ok=bool(args>>city>>x>>y) && city>=0 && city<50 && valid_xy(x,y);
            if(ok) { MIASTA[city][0][M_X]=x; MIASTA[city][0][M_Y]=y; }
        } else if (op=="expect_world_quest") {
            int city,expected; ok=bool(args>>city>>expected) && city>=0 && city<50 && (expected==0 || expected==1);
            if(ok) { bool actual=CityHasPendingQuestInfo(city); ok=actual==(expected!=0);
                std::cout<<"{\"event\":\"world_quest\",\"city\":"<<city<<",\"marker\":"<<(actual?"true":"false")<<"}"<<std::endl; }
        } else if (op=="expect_world_marker_rendered") {
            int city; ok=bool(args>>city) && city>=0 && city<50;
            if(ok) ok=world_marker_visible(t,city);
        } else if (op=="city") {
            int city;ok=t.ready && bool(args>>city) && city>=0 && city<50;
            if(ok) {ARMIA[ARM][0][TNOGI]=city+70;BeginCityQuestOffers(city);}
        } else if (op=="city_name") {
            int city; std::string name; ok=bool(args>>city>>name) && city>=0 && city<50;
            if(ok) MIASTA_S[city]=name;
        } else if (op=="city_travel") {
            int city; ok=t.ready && bool(args>>city) && city>=0 && city<50;
            if(ok) {
                ARMIA[ARM][0][TX]=MIASTA[city][0][M_X]; ARMIA[ARM][0][TY]=MIASTA[city][0][M_Y];
                ARMIA[ARM][0][TNOGI]=city+70; BeginCityQuestOffers(city);
            }
        } else if (op=="quest_offer") {
            int n,available;ok=bool(args>>n>>available) && valid_unit(t,n) && t.army[n]==WRG && (available==0 || available==1);
            if(ok) SetCityQuestOffer(n,available!=0);
        } else if (op=="quest_state") {
            int slot,type,level,city,person,last;
            ok=bool(args>>slot>>type>>level>>city>>person>>last) && slot>=0 && slot<4 && type>=0 && type<=20 && level>=0 && level<=4 && city>=0 && city<50 && person>=1 && person<=10 && last>=-1 && last<50;
            if(ok) {PRZYGODY[slot][P_TYP]=type;PRZYGODY[slot][P_LEVEL]=level;PRZYGODY[slot][P_X]=city;PRZYGODY[slot][P_Y]=person;PRZYGODY[slot][P_STAREX]=last;}
        } else if (op=="quest_price") {
            int slot,price; ok=bool(args>>slot>>price) && slot>=0 && slot<4 && price>=0;
            if(ok) PRZYGODY[slot][P_CENA]=price;
        } else if (op=="expect_quest_markers") {
            int n,offer,info;ok=bool(args>>n>>offer>>info) && valid_unit(t,n) && t.army[n]==WRG && (offer==0 || offer==1) && (info==0 || info==1);
            if(ok) {bool a=CityHasNewQuest(n),b=CityHasQuestInfo(n);
                std::cout<<"{\"event\":\"quest_markers\",\"unit\":"<<n<<",\"offer\":"<<(a?"true":"false")<<",\"info\":"<<(b?"true":"false")<<"}"<<std::endl;
                ok=a==(offer!=0) && b==(info!=0);}
        } else if (op=="expect_quest_marker") {
            int n; char expected; ok=bool(args>>n>>expected) && valid_unit(t,n) && (expected=='!' || expected=='?' || expected=='-');
            if(ok) {char actual=CityQuestMarker(n); if(!actual) actual='-'; ok=actual==expected;
                std::cout<<"{\"event\":\"quest_marker\",\"unit\":"<<n<<",\"marker\":\""<<actual<<"\"}"<<std::endl;}
        } else if (op=="dialog_quest") {
            int player,npc,slot,advance; ok=bool(args>>player>>npc>>slot>>advance) && valid_unit(t,player) && valid_unit(t,npc) && t.army[player]==ARM && t.army[npc]==WRG && slot>=0 && slot<4 && PRZYGODY[slot][P_LEVEL]>0 && (advance==0 || advance==1);
            if(ok) {
                int oldlevel=PRZYGODY[slot][P_LEVEL];
                int x=(ARMIA[ARM][player][TX]+ARMIA[WRG][npc][TX])/2, y=(ARMIA[ARM][player][TY]+ARMIA[WRG][npc][TY])/2;
                Screen(0); CENTER_V=60; CENTER(x,y,0); CENTER_V=100;
                TimedQuestTalk clicks={0,slot}; quest_answer_enabled=true; quest_answer_captured=false; quest_answer_text.clear();
                SDL_TimerID timer=SDL_AddTimer(800,push_quest_talk,&clicks); GADKA(player,npc);
                if(timer) SDL_RemoveTimer(timer); quest_answer_enabled=false;
                ok=clicks.step==2 && quest_answer_captured && PRZYGODY[slot][P_LEVEL]==oldlevel-advance;
                std::cout<<"{\"event\":\"quest_talk\",\"slot\":"<<slot<<",\"level\":"<<PRZYGODY[slot][P_LEVEL]<<",\"city\":"<<PRZYGODY[slot][P_X]<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="expect_answer_contains") {
            std::string expected; std::getline(args,expected); if(!expected.empty() && expected[0]==' ') expected.erase(0,1);
            ok=quest_answer_captured && !expected.empty() && quest_answer_text.find(expected)!=std::string::npos;
        } else if (op=="dialog_offer") {
            int player,npc; ok=bool(args>>player>>npc) && valid_unit(t,player) && valid_unit(t,npc) && t.army[player]==ARM && t.army[npc]==WRG && CityHasNewQuest(npc);
            if(ok) {
                int x=(ARMIA[ARM][player][TX]+ARMIA[WRG][npc][TX])/2;
                int y=(ARMIA[ARM][player][TY]+ARMIA[WRG][npc][TY])/2;
                Screen(0); CENTER_V=60; CENTER(x,y,0); CENTER_V=100;
                TimedDialogue clicks={0}; dialog_capture_enabled=true; dialog_capture_ok=false; dialog_label_ok=false;
                SDL_TimerID timer=SDL_AddTimer(800,push_dialog_click,&clicks);
                GADKA(player,npc);
                if(timer) SDL_RemoveTimer(timer);
                dialog_capture_enabled=false;
                ok=clicks.step==2 && dialog_capture_ok && dialog_label_ok && PRZYGODY[0][P_TYP]>0 && !CityHasNewQuest(npc);
                std::cout<<"{\"event\":\"dialog_offer\",\"clicks\":"<<clicks.step<<",\"quest_type\":"<<PRZYGODY[0][P_TYP]<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="equip") {
            int n,slot,item;ok=bool(args>>n>>slot>>item) && valid_unit(t,n) && slot>=0 && slot<4 && item>=0 && item<=120;
            if(ok) ARMIA[t.army[n]][n][TKORP+slot]=item;
        } else if (op=="stat_setup") {
            int n,strength,speed,hp; ok=bool(args>>n>>strength>>speed>>hp) && valid_unit(t,n) && t.army[n]==ARM && strength>=0 && speed>=0 && hp>0;
            if(ok) {
                ResetStrengthOverflow(ARM,n); ResetEquipmentSet(ARM,n);
                ARMIA[ARM][n][TSI]=strength; ARMIA[ARM][n][TAMO]=speed; ARMIA[ARM][n][TSZ]=speed;
                ARMIA[ARM][n][TEM]=hp; ARMIA[ARM][n][TE]=hp;
                for(int i=0;i<13;++i) ARMIA[ARM][n][TGLOWA+i]=0;
            }
        } else if (op=="wear") {
            int n,slot,item; ok=bool(args>>n>>slot>>item) && valid_unit(t,n) && t.army[n]==ARM && slot>=0 && slot<5 && item>0 && item<=MX_WEAPON && ARMIA[ARM][n][TGLOWA+slot]==0;
            if(ok) {NUMER=n; ARMIA[ARM][n][TGLOWA+slot]=item; PRZELICZ(slot,1); WAGA(ARM,n);}
        } else if (op=="unwear") {
            int n,slot; ok=bool(args>>n>>slot) && valid_unit(t,n) && t.army[n]==ARM && slot>=0 && slot<5 && ARMIA[ARM][n][TGLOWA+slot]>0;
            if(ok) {NUMER=n; PRZELICZ(slot,-1); ARMIA[ARM][n][TGLOWA+slot]=0; WAGA(ARM,n);}
        } else if (op=="backpack") {
            int n,slot,item; ok=bool(args>>n>>slot>>item) && valid_unit(t,n) && t.army[n]==ARM && slot>=0 && slot<8 && item>=0 && item<=MX_WEAPON;
            if(ok) {ARMIA[ARM][n][TPLECAK+slot]=item; WAGA(ARM,n);}
        } else if (op=="consume") {
            int n,item; ok=bool(args>>n>>item) && valid_unit(t,n) && t.army[n]==ARM && item>0 && item<=MX_WEAPON && ARMIA[ARM][n][TGLOWA]==0 && (BRON[item][B_TYP]==13 || BRON[item][B_TYP]==18);
            if(ok) {NUMER=n; ARMIA[ARM][n][TGLOWA]=item; PRZELICZ(0,1); ARMIA[ARM][n][TGLOWA]=0; WAGA(ARM,n);}
        } else if (op=="expect_stats") {
            int n,strength,raw,speed,base_speed; ok=bool(args>>n>>strength>>raw>>speed>>base_speed) && valid_unit(t,n);
            if(ok) {int a=t.army[n]; long actual_raw=StrengthBeforeLimit(a,n);
                std::cout<<"{\"event\":\"stats\",\"unit\":"<<n<<",\"strength\":"<<ARMIA[a][n][TSI]<<",\"raw\":"<<actual_raw<<",\"speed\":"<<ARMIA[a][n][TSZ]<<",\"base_speed\":"<<ARMIA[a][n][TAMO]<<"}"<<std::endl;
                ok=ARMIA[a][n][TSI]==strength && actual_raw==raw && ARMIA[a][n][TSZ]==speed && ARMIA[a][n][TAMO]==base_speed;}
        } else if (op=="inventory_consume") {
            int n,item; ok=bool(args>>n>>item) && valid_unit(t,n) && t.army[n]==ARM && item>0 && item<=MX_WEAPON && ARMIA[ARM][n][TPLECAK]==0 && (BRON[item][B_TYP]==13 || BRON[item][B_TYP]==18);
            if(ok) {
                int helmet=ARMIA[ARM][n][TGLOWA];
                ARMIA[ARM][n][TPLECAK]=item; NUMER=n; TimedInventory clicks={0,260,274,110,274};
                SDL_TimerID timer=SDL_AddTimer(800,push_inventory_input,&clicks);
                if(timer) {WYBOR(0); SDL_RemoveTimer(timer);} else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==3 && ARMIA[ARM][n][TPLECAK]==0 && ARMIA[ARM][n][TGLOWA]==helmet;
                std::cout<<"{\"event\":\"inventory_consume\",\"unit\":"<<n<<",\"item\":"<<item<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="inventory_growth") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; const int oldStrength=ARMIA[ARM][n][TSI], oldMax=ARMIA[ARM][n][TEM];
                ARMIA[ARM][n][TDOSW]=10; TimedGrowth clicks={0}; growth_frame_mask=0; growth_test_enabled=true;
                SDL_TimerID timer=SDL_AddTimer(700,push_growth,&clicks);
                if(timer) {WYBOR(0); SDL_RemoveTimer(timer);} else ok=false;
                growth_test_enabled=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==8 && growth_frame_mask==7 && ARMIA[ARM][n][TDOSW]==0 && ARMIA[ARM][n][TSI]==oldStrength+3 && ARMIA[ARM][n][TEM]==oldMax+1;
                // Open again with no points and close via the explicit button.
                TimedInventory closeClicks={0,500,334,380,224};
                timer=SDL_AddTimer(700,push_inventory_input,&closeClicks);
                if(timer) {WYBOR(0); SDL_RemoveTimer(timer);} else ok=false;
                SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && closeClicks.step==3 && ARMIA[ARM][n][TDOSW]==0 && ARMIA[ARM][n][TSI]==oldStrength+3;
                std::cout<<"{\"event\":\"inventory_growth\",\"frames\":"<<growth_frame_mask<<",\"points\":"<<ARMIA[ARM][n][TDOSW]<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="arrow_counter") {
            int n,enemy; ok=bool(args>>n>>enemy) && valid_unit(t,n) && valid_unit(t,enemy) && t.army[n]==ARM && t.army[enemy]==WRG;
            if(ok) {
                NUMER=n; ARMIA[ARM][n][TLEWA]=20; ARMIA[ARM][n][TPRAWA]=39; STRZALY[n]=10;
                ARMIA[WRG][enemy][TKORP]=180;
                ScreenDisplay(0,130,40,320,234); ScreenDisplay(1,130,275,320,25);
                Screen(1); EKRAN1(); WYKRESY(ARM,n); rysuj();
                ok=capture_buffer(std::string(outdir)+"/strzaly_10.png");
                const int ex=ARMIA[WRG][enemy][TX],ey=ARMIA[WRG][enemy][TY]-8;
                ok=ok && TacticalContextCommand(ex,ey,true) && STRZALY[n]==10;
                A_STRZAL(ARM,n);
                ok=ok && STRZALY[n]==9;
                rysuj(); ok=ok && capture_buffer(std::string(outdir)+"/strzaly_9.png");
                ARMIA[ARM][n][TLEWA]=39; ARMIA[ARM][n][TPRAWA]=20; STRZALY[n]=0;
                Screen(1); WYKRESY(ARM,n); rysuj();
                ok=ok && capture_buffer(std::string(outdir)+"/strzaly_lewa_0.png");
                std::cout<<"{\"event\":\"arrow_counter\",\"shot_remaining\":9,\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="arrow_shot_lifecycle") {
            int n,other,enemy; ok=bool(args>>n>>other>>enemy) && valid_unit(t,n) && valid_unit(t,other) && valid_unit(t,enemy) && t.army[n]==ARM && t.army[other]==ARM && t.army[enemy]==WRG;
            if(ok) {
                NUMER=n; ARMIA[ARM][0][TE]=2; KTO_ATAKUJE=ARM;
                ARMIA[WRG][enemy][TKORP]=180; ARMIA[WRG][enemy][TTRYB]=0;
                ResetEquipmentSet(ARM,n); ResetEquipmentSet(ARM,other);
                ARMIA[ARM][n][TLEWA]=20; ARMIA[ARM][n][TPRAWA]=39; STRZALY[n]=10;
                ARMIA[ARM][other][TLEWA]=20; ARMIA[ARM][other][TPRAWA]=39; STRZALY[other]=10;
                const int ex=ARMIA[WRG][enemy][TX],ey=ARMIA[WRG][enemy][TY]-8;
                SetTacticalPauseHealthBars(true);
                // Re-aim using both the explicit shoot order and contextual clicks.
                SetImmediateTarget(ex,ey); STRZAL();
                ok=STRZALY[n]==10 && ARMIA[ARM][n][TTRYB]==3;
                ok=ok && TacticalContextCommand(ex,ey,true) && TacticalContextCommand(ex+5,ey,true) && STRZALY[n]==10 && ARMIA[ARM][n][TCELX]==ex+5;
                ok=ok && TacticalContextCommand(400,300,true) && ARMIA[ARM][n][TTRYB]==1 && STRZALY[n]==10;
                ok=ok && run_short_tactical_action() && STRZALY[n]==10;
                // Each of two queued shots fires once; another pause does not consume again.
                NUMER=n; ok=ok && TacticalContextCommand(ex,ey,true) && STRZALY[n]==10;
                NUMER=other; ok=ok && TacticalContextCommand(ex,ey,true) && STRZALY[other]==10;
                ok=ok && run_short_tactical_action() && STRZALY[n]==9 && STRZALY[other]==9;
                // Once launched, switching equipment must not cancel the arrow in flight.
                NUMER=n; StoreSpareHandItem(ARM,n,0,3); StoreSpareHandItem(ARM,n,1,0);
                const float beforeFlight=VEKTOR_F[n][3];
                ok=ok && ARMIA[ARM][n][TTRYB]==3 && SwitchEquipmentSet(ARM,n,1);
                ok=ok && run_short_tactical_action() && STRZALY[n]==9 && STRZALY[other]==9 && VEKTOR_F[n][3]>beforeFlight;
                ok=ok && SwitchEquipmentSet(ARM,n,0);
                // Changing the active set before launch cancels the prepared arrow.
                NUMER=n; ARMIA[ARM][n][TTRYB]=0; ResetEquipmentSet(ARM,n);
                ARMIA[ARM][n][TLEWA]=20; ARMIA[ARM][n][TPRAWA]=39;
                StoreSpareHandItem(ARM,n,0,3); StoreSpareHandItem(ARM,n,1,0);
                ok=ok && TacticalContextCommand(ex,ey,true) && STRZALY[n]==9;
                ok=ok && SwitchEquipmentSet(ARM,n,1) && run_short_tactical_action();
                ok=ok && STRZALY[n]==9 && ARMIA[ARM][n][TTRYB]==0;
                // A dead unit cannot spend the arrow it had queued.
                NUMER=other; ARMIA[ARM][other][TTRYB]=0;
                ok=ok && TacticalContextCommand(ex,ey,true) && STRZALY[other]==9;
                const int health=ARMIA[ARM][other][TE]; ARMIA[ARM][other][TE]=0; NUMER=n;
                ok=ok && run_short_tactical_action() && STRZALY[other]==9;
                ARMIA[ARM][other][TE]=health; ARMIA[ARM][other][TTRYB]=0;
                // No arrow is launched or subtracted at zero ammunition.
                SwitchEquipmentSet(ARM,n,0); ARMIA[ARM][n][TTRYB]=0; STRZALY[n]=0;
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==0 && STRZALY[n]==0;
                // Replacing a queued arrow with a javelin consumes no arrow.
                int javelin=0; for(int item=1;item<=MX_WEAPON;++item) if(BRON[item][B_TYP]==9) { javelin=item; break; }
                ok=ok && javelin>0;
                STRZALY[n]=9; ARMIA[ARM][n][TLEWA]=20; ARMIA[ARM][n][TPRAWA]=39;
                VEKTOR_F[n][5]=javelin;
                ok=ok && TacticalContextCommand(ex,ey,true) && int(VEKTOR_F[n][5])==0 && STRZALY[n]==9;
                ARMIA[ARM][n][TLEWA]=javelin; ARMIA[ARM][n][TPRAWA]=0;
                SetImmediateTarget(ex,ey); STRZAL();
                ok=ok && ARMIA[ARM][n][TTRYB]==3 && int(VEKTOR_F[n][5])==javelin && STRZALY[n]==9;
                ok=ok && run_short_tactical_action() && STRZALY[n]==9;
                std::cout<<"{\"event\":\"arrow_shot_lifecycle\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="context_commands") {
            int n,friendUnit,enemy; ok=bool(args>>n>>friendUnit>>enemy) && valid_unit(t,n) && valid_unit(t,friendUnit) && valid_unit(t,enemy) && t.army[n]==ARM && t.army[friendUnit]==ARM && t.army[enemy]==WRG;
            if(ok) {
                NUMER=n; Screen(0); ARMIA[ARM][0][TE]=2; ResetEquipmentSet(ARM,n);
                ARMIA[ARM][n][TLEWA]=0; ARMIA[ARM][n][TPRAWA]=0;
                const int ex=ARMIA[WRG][enemy][TX], ey=ARMIA[WRG][enemy][TY]-8;
                ARMIA[WRG][enemy][TRASA]=1; ARMIA[WRG][enemy][TKORP]=20;
                ARMIA[WRG][enemy][TTRYB]=2; ARMIA[WRG][enemy][TCELY]=ARM;
                ok=TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==2 && ARMIA[WRG][enemy][TKORP]>90;
                ARMIA[ARM][n][TTRYB]=0; ARMIA[WRG][enemy][TTRYB]=0; ARMIA[WRG][enemy][TKORP]=20;
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==6 && ARMIA[ARM][n][TCELX]==enemy;
                ARMIA[WRG][enemy][TKORP]=180;
                // A visible part of the enemy may overlap a friendly unit at
                // its centre. The chosen enemy must remain the target.
                SetZone(friendUnit,ex-15,ey-15,ex+5,ey+15);
                ARMIA[ARM][n][TLEWA]=3;
                ok=ok && TacticalContextCommand(ex+10,ey,true) && ARMIA[ARM][n][TTRYB]==2 && ARMIA[ARM][n][TCELY]==WRG;
                const int friendX=ARMIA[ARM][friendUnit][TX],friendY=ARMIA[ARM][friendUnit][TY];
                SetZone(friendUnit,friendX-15,friendY-15,friendX+15,friendY);
                ARMIA[ARM][n][TLEWA]=3;
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==2 && ARMIA[ARM][n][TCELX]==enemy;
                StoreEquipmentMode(ARM,n,0);
                ARMIA[ARM][n][TLEWA]=20; ARMIA[ARM][n][TPRAWA]=39; STRZALY[n]=5;
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==3 && STRZALY[n]==5;
                A_STRZAL(ARM,n); ok=ok && STRZALY[n]==4;
                StoreEquipmentMode(ARM,n,1);
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==3 && STRZALY[n]==4;
                A_STRZAL(ARM,n); ok=ok && STRZALY[n]==3;
                ARMIA[ARM][n][TLEWA]=20; ARMIA[ARM][n][TPRAWA]=39; STRZALY[n]=0; ARMIA[ARM][n][TTRYB]=0;
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==0;
                ARMIA[ARM][n][TLEWA]=60; ARMIA[ARM][n][TPRAWA]=64;
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==3 && ARMIA[ARM][n][TPRAWA]==0;
                ARMIA[ARM][n][TLEWA]=48; ARMIA[ARM][n][TPRAWA]=42; ARMIA[ARM][n][TMAG]=20;
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==4 && int(VEKTOR_F[n][0])==42 && ARMIA[ARM][n][TMAG]==15;
                ARMIA[ARM][n][TLEWA]=48; ARMIA[ARM][n][TPRAWA]=0; ARMIA[ARM][n][TTRYB]=0;
                ok=ok && TacticalContextCommand(ex,ey,true) && ARMIA[ARM][n][TTRYB]==2 && ARMIA[ARM][n][TMAG]==15;
                const int fx=ARMIA[ARM][friendUnit][TX],fy=ARMIA[ARM][friendUnit][TY]-8;
                ok=ok && TacticalContextCommand(fx,fy,true) && NUMER==friendUnit;
                NUMER=n; const int health=ARMIA[ARM][n][TE]; ARMIA[ARM][n][TE]=0;
                ok=ok && TacticalContextCommand(fx,fy,true) && NUMER==friendUnit;
                ARMIA[ARM][n][TE]=health; NUMER=n; ARMIA[ARM][n][TLEWA]=0; ARMIA[ARM][n][TPRAWA]=0;
                ok=ok && TacticalContextCommand(350,200,true) && ARMIA[ARM][n][TTRYB]==1 && ARMIA[ARM][n][TCELX]==350 && ARMIA[ARM][n][TCELY]==208;
                // Real SDL right-button events: short click, drag, modal exit,
                // and a held enemy must remain distinct and never double-act.
                Screen(0); CENTER(ex,ey,0); NUMER=n; StoreEquipmentMode(ARM,n,0);
                TimedClick right=scaled_click(XHard(ex),YHard(ey)); ++right.x; ++right.y;
                MouseEnableTacticalContext(true); ARMIA[ARM][n][TTRYB]=0;
                push_right_event(right,true); push_right_event(right,false); ProcessEvents(); TacticalHandleRightInput();
                ok=ok && ARMIA[ARM][n][TTRYB]==2;
                ARMIA[ARM][n][TTRYB]=0; MouseEnableTacticalContext(false);
                push_right_event(right,true); push_right_event(right,false); ProcessEvents();
                MouseEnableTacticalContext(true); TacticalHandleRightInput();
                ok=ok && ARMIA[ARM][n][TTRYB]==0;
                TimedClick moved=right; moved.x+=30;
                push_right_event(right,true); push_right_event(moved,false); ProcessEvents(); TacticalHandleRightInput();
                ok=ok && ARMIA[ARM][n][TTRYB]==0;
                ARMIA[WRG][enemy][TGLOWA]=0; inspection_seen=false; inspection_test_enabled=true;
                push_right_event(right,true); ProcessEvents();
                SDL_TimerID held=SDL_AddTimer(900,push_right_release,&right);
                if(held) {
                    bool movedEnemy=false; const Uint32 begin=SDL_GetTicks();
                    while(MouseKey()==PRAWY) {
                        TacticalHandleRightInput();
                        if(!movedEnemy && SDL_GetTicks()-begin>150) {
                            movedEnemy=true; ARMIA[WRG][enemy][TX]+=35;
                            SetZone(enemy+10,ex+20,ey-7,ex+50,ey+8);
                        }
                        rysuj(); TacticalDrawContextOverlay(); WaitVbl();
                    }
                    TacticalHandleRightInput(); SDL_RemoveTimer(held);
                    ARMIA[WRG][enemy][TX]=ex; SetZone(enemy+10,ex-15,ey-7,ex+15,ey+8);
                } else ok=false;
                inspection_test_enabled=false; MouseEnableTacticalContext(false);
                ok=ok && inspection_seen && ARMIA[ARM][n][TTRYB]==0 && ARMIA[WRG][enemy][TGLOWA]==0;
                std::cout<<"{\"event\":\"context_commands\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="active_right_click") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ARMIA[ARM][0][TE]=2; ARMIA[ARM][n][TTRYB]=0; KTO_ATAKUJE=ARM;
                Screen(0); CENTER(ARMIA[ARM][n][TX],ARMIA[ARM][n][TY],0);
                TimedRightThenSpace clicks={0,scaled_click(XHard(300),YHard(190))}; ++clicks.click.x; ++clicks.click.y;
                SDL_TimerID timer=SDL_AddTimer(600,push_right_then_space,&clicks);
                if(timer) { AKCJA(); SDL_RemoveTimer(timer); } else ok=false;
                ok=ok && clicks.step==2 && ARMIA[ARM][n][TTRYB]==1 && ARMIA[ARM][n][TCELX]==300 && ARMIA[ARM][n][TCELY]==198;
                std::cout<<"{\"event\":\"active_right_click\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="paused_right_click") {
            int n,enemy; ok=bool(args>>n>>enemy) && n==1 && valid_unit(t,n) && valid_unit(t,enemy) && t.army[n]==ARM && t.army[enemy]==WRG;
            if(ok) {
                NUMER=n; ARMIA[ARM][0][TE]=2; ARMIA[ARM][n][TTRYB]=0;
                ARMIA[WRG][enemy][TKORP]=180; ARMIA[WRG][enemy][TRASA]=1;
                Screen(0); CENTER(ARMIA[ARM][n][TX],ARMIA[ARM][n][TY],0);
                TimedClick click=scaled_click(XHard(ARMIA[WRG][enemy][TX]),YHard(ARMIA[WRG][enemy][TY]-8)); ++click.x; ++click.y;
                TimedRightThenSpace clicks={0,click}; tactical_frame_test=true; tactical_frame_seen=false;
                SDL_TimerID timer=SDL_AddTimer(600,push_right_then_space,&clicks);
                if(timer) { MAIN_ACTION(); SDL_RemoveTimer(timer); } else ok=false;
                tactical_frame_test=false; ok=ok && tactical_frame_seen;
                std::cout<<"{\"event\":\"paused_right_click\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="hover_worn") {
            int n,mode; ok=bool(args>>n>>mode) && valid_unit(t,n) && t.army[n]==ARM && mode>=0 && mode<=2;
            if(ok) {
                NUMER=n; const int items[]={24,27,25,3,4};
                for(int slot=0;slot<5;++slot) ARMIA[ARM][n][TGLOWA+slot]=items[slot];
                MIASTA[3][1][M_LUDZIE]=4; GRACZE[1][1]=10000;
                TimedHover steps={0,mode}; hover_mask=0; hover_test=&steps;
                SDL_TimerID timer=SDL_AddTimer(650,push_hover_test,&steps);
                if(timer) { if(mode==1) _SKLEP(3,1,ARM,n); else WYBOR(0,mode==2); SDL_RemoveTimer(timer); } else ok=false;
                hover_test=0;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && steps.step==6 && hover_mask==31;
                for(int slot=0;slot<5;++slot) ok=ok && ARMIA[ARM][n][TGLOWA+slot]==items[slot];
                std::cout<<"{\"event\":\"hover_worn\",\"mode\":"<<mode<<",\"mask\":"<<hover_mask<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="inventory_arrow_navigation") {
            int n,other; ok=bool(args>>n>>other) && n==1 && other==3 && valid_unit(t,n) && valid_unit(t,other) && t.army[n]==ARM && t.army[other]==ARM;
            if(ok) {
                NUMER=n; TimedNavigation steps={0}; navigation_mask=0; navigation_test=&steps;
                ClearKey(); SDL_TimerID timer=SDL_AddTimer(700,push_navigation_test,&steps);
                if(timer) { WYBOR(0); SDL_RemoveTimer(timer); } else ok=false;
                navigation_test=0;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && steps.step==5 && navigation_mask==15 && NUMER==n;
                std::cout<<"{\"event\":\"inventory_arrow_navigation\",\"mask\":"<<navigation_mask<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="inventory_swap") {
            int n,held,slot=4; ok=bool(args>>n>>held); if(args>>slot) {} else args.clear();
            ok=ok && valid_unit(t,n) && t.army[n]==ARM && slot>=0 && slot<5 && ARMIA[ARM][n][TPLECAK]==0 && ARMIA[ARM][n][TGLOWA+slot]>0 && BRON[held][B_PLACE]==(slot<3 ? slot+1 : 4);
            if(ok) {
                NUMER=n; const int old=ARMIA[ARM][n][TGLOWA+slot]; ARMIA[ARM][n][TPLECAK]=held;
                const int xs[]={110,110,110,30,190}, ys[]={274,346,474,382,382};
                TimedSwap clicks={0,xs[slot],ys[slot]}; item_info_capture_enabled=true;
                SDL_TimerID timer=SDL_AddTimer(800,push_inventory_swap,&clicks);
                if(timer) { WYBOR(0); SDL_RemoveTimer(timer); } else ok=false;
                item_info_capture_enabled=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==4 && ARMIA[ARM][n][TGLOWA+slot]==held && ARMIA[ARM][n][TPLECAK]==old;
                std::cout<<"{\"event\":\"inventory_swap\",\"slot\":"<<slot<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_item_info") {
            int n,item; ok=bool(args>>n>>item) && valid_unit(t,n) && t.army[n]==ARM && item>0 && item<=MX_WEAPON;
            if(ok) {
                NUMER=n; MIASTA[3][1][M_LUDZIE]=4; SKLEP[1][0]=item; GRACZE[1][1]=10000;
                TimedInventory clicks={0,30,438,30,438}; shop_info_capture_enabled=true;
                shop_transition_test=true; shop_transition_ok=true; shop_transition_count[0]=shop_transition_count[1]=0;
                SDL_TimerID timer=SDL_AddTimer(1200,push_inventory_input,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                const bool captured=!shop_info_capture_enabled; shop_info_capture_enabled=false; shop_transition_test=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==3 && captured && shop_transition_ok && shop_transition_count[0]>=37 && shop_transition_count[1]>=38 && SKLEP[1][0]==item && GRACZE[1][1]==10000;
                std::cout<<"{\"event\":\"shop_item_info\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="quest_action_entry") {
            IMIONA_S[1]="Test"; IMIONA_S[2]="A"; IMIONA_S[3]="B"; IMIONA_S[4]="C";
            SETUP0(); ZROB_MIASTA(); ZROB_ARMIE(); PREFS[5]=0;
            for(int i=0;i<4;++i) PRZYGODY[i][P_TYP]=0;
            const int x=ARMIA[0][0][TX],y=ARMIA[0][0][TY];
            PRZYGODY[0][P_TYP]=12; PRZYGODY[0][P_LEVEL]=0; PRZYGODY[0][P_X]=x; PRZYGODY[0][P_Y]=y; PRZYGODY[0][P_TEREN]=8;
            MIASTA[3][0][M_X]=x; MIASTA[3][0][M_Y]=y;
            ARMIA[0][0][TNOGI]=73; ARMIA[0][0][TWAGA]=0; VISUAL_OBJECTS();
            TimedQuestAction clicks={0}; quest_scene_test_enabled=true; quest_scene_seen=false; army_frame_seen[0]=army_frame_seen[1]=false;
            SDL_TimerID timer=SDL_AddTimer(2000,push_quest_action,&clicks);
            if(timer) { _ARMIA(0); SDL_RemoveTimer(timer); } else ok=false;
            quest_scene_test_enabled=false;
            ok=ok && clicks.step==3 && quest_scene_seen && army_frame_seen[0] && army_frame_seen[1] && PRZYGODY[0][P_TYP]==0 && ARMIA[0][0][TNOGI]==73;
            std::cout<<"{\"event\":\"quest_action_entry\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="ground_blink_capture") {
            while ((SDL_GetTicks()/500)%2!=0) SDL_Delay(5);
            render(t); ok=capture_buffer(std::string(outdir)+"/lup_obwodka.png");
            while ((SDL_GetTicks()/500)%2==0) SDL_Delay(5);
            render(t); ok=ok && capture_buffer(std::string(outdir)+"/lup_bez_obwodki.png");
            std::cout<<"{\"event\":\"ground_blink_capture\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="quest_marker_stable") {
            SETUP0(); SX=0; SY=0;
            for(int i=0;i<4;++i) PRZYGODY[i][P_TYP]=0;
            PRZYGODY[0][P_TYP]=1; PRZYGODY[0][P_LEVEL]=0; PRZYGODY[0][P_X]=200; PRZYGODY[0][P_Y]=200; PRZYGODY[0][P_TEREN]=0;
            MIASTA[3][0][M_X]=200; MIASTA[3][0][M_Y]=200;
            ARMIA[0][0][TE]=1; ARMIA[0][0][TX]=203; ARMIA[0][0][TY]=203;
            VISUAL_OBJECTS(); VISUAL_OBJECTS();
            ok=PRZYGODY[0][P_X]==200 && PRZYGODY[0][P_Y]==200 && NearbyWorldQuest(203,203)==0 && NearbyWorldQuest(230,230)==-1 && PRZYGODY[0][P_TEREN]>=1 && PRZYGODY[0][P_TEREN]<=7;
            std::cout<<"{\"event\":\"quest_marker_stable\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="growth_equipment_limits") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ResetStrengthOverflow(ARM,n);
                for(int i=0;i<13;++i) ARMIA[ARM][n][TGLOWA+i]=0;
                const int race=ARMIA[ARM][n][TRASA], strLimit=RASY[race][1]/2+40, speedLimit=RASY[race][2]+30;
                ARMIA[ARM][n][TSI]=strLimit-1; ARMIA[ARM][n][TAMO]=speedLimit-1; ARMIA[ARM][n][TSZ]=speedLimit-1;
                ARMIA[ARM][n][TEM]=10; ARMIA[ARM][n][TE]=10;
                ARMIA[ARM][n][TPRAWA]=3; PRZELICZ(4,1); ARMIA[ARM][n][TNOGI]=25; PRZELICZ(2,1); WAGA(ARM,n);
                const long strengthBefore=StrengthBeforeLimit(ARM,n),speedBefore=ARMIA[ARM][n][TAMO];
                ARMIA[ARM][n][TDOSW]=30; TimedGrowth clicks={0};
                SDL_TimerID timer=SDL_AddTimer(500,push_limit_growth,&clicks);
                if(timer) { WYBOR(0); SDL_RemoveTimer(timer); } else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==11 && StrengthBeforeLimit(ARM,n)==strengthBefore+1 && ARMIA[ARM][n][TAMO]==speedBefore+1 && ARMIA[ARM][n][TDOSW]==24;
                std::cout<<"{\"event\":\"growth_equipment_limits\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="equipment_inventory_switch") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ARMIA[ARM][n][TLEWA]=3; ARMIA[ARM][n][TPRAWA]=0;
                StoreSpareHandItem(ARM,n,0,20); StoreSpareHandItem(ARM,n,1,39);
                TimedGrowth clicks={0}; mode_capture_enabled=true; mode_capture_seen=false;
                SDL_TimerID timer=SDL_AddTimer(800,push_mode_click_and_key,&clicks);
                if(timer) { WYBOR(0); SDL_RemoveTimer(timer); } else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                mode_capture_enabled=false;
                ok=ok && clicks.step==3 && mode_capture_seen && EquipmentMode(ARM,n)==0 && ARMIA[ARM][n][TLEWA]==3 && ARMIA[ARM][n][TPRAWA]==0 && SpareHandItem(ARM,n,0)==20 && SpareHandItem(ARM,n,1)==39;
                std::cout<<"{\"event\":\"equipment_inventory_switch\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_duplicate_helmet") {
            int n; ok=bool(args>>n) && n==1 && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ARMIA[ARM][n][TGLOWA]=24; ARMIA[ARM][n][TPLECAK]=24;
                MIASTA[3][1][M_LUDZIE]=4; GRACZE[1][1]=10000;
                TimedInventory clicks={0,490,450,490,450}; shop_info_capture_enabled=true;
                shop_duplicate_check=true; shop_duplicate_ok=false;
                SDL_TimerID timer=SDL_AddTimer(1200,push_inventory_input,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                shop_info_capture_enabled=false; shop_duplicate_check=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==3 && shop_duplicate_ok && ARMIA[ARM][n][TGLOWA]==24 && ARMIA[ARM][n][TPLECAK]==24 && GRACZE[1][1]==10000;
                std::cout<<"{\"event\":\"shop_duplicate_helmet\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_character_sets") {
            int n,other; ok=bool(args>>n>>other) && n==1 && other==3 && valid_unit(t,n) && valid_unit(t,other) && t.army[n]==ARM && t.army[other]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ResetEquipmentSet(ARM,other);
                ARMIA[ARM][other][TLEWA]=3; ARMIA[ARM][other][TPRAWA]=0;
                StoreSpareHandItem(ARM,other,0,20); StoreSpareHandItem(ARM,other,1,39);
                MIASTA[3][1][M_LUDZIE]=4; TimedInventory clicks={0,440,450,576,62};
                SDL_TimerID timer=SDL_AddTimer(1200,push_inventory_input,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==3 && NUMER==n && EquipmentMode(ARM,n)==0 && EquipmentMode(ARM,other)==1 && ARMIA[ARM][other][TLEWA]==20 && ARMIA[ARM][other][TPRAWA]==39 && SpareHandItem(ARM,other,0)==3;
                std::cout<<"{\"event\":\"shop_character_sets\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="map_loot_pages") {
            int n,full; ok=bool(args>>n>>full) && valid_unit(t,n) && t.army[n]==ARM && (full==0 || full==1);
            if(ok) {
                NUMER=n; for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                for(int i=0;i<35;++i) GLEBA[i/4][i%4]=i==30 ? 24 : 81;
                for(int i=0;i<8;++i) ARMIA[ARM][n][TPLECAK+i]=full ? 3 : 0;
                TimedLootPage clicks={0,bool(full)}; map_loot_capture_enabled=true; map_loot_capture_seen=false;
                map_loot_held_test=true; map_loot_held_seen=false; map_loot_expected_bag=full ? 3 : 0;
                SDL_TimerID timer=SDL_AddTimer(800,push_map_loot_page,&clicks);
                if(timer) { WYBOR(0,true); SDL_RemoveTimer(timer); } else ok=false;
                map_loot_capture_enabled=false; map_loot_held_test=false;
                ok=ok && clicks.step==(full ? 9 : 8) && map_loot_capture_seen && map_loot_held_seen && GLEBA[7][2]==(full ? 3 : 0) && ARMIA[ARM][n][TPLECAK]==24;
                std::cout<<"{\"event\":\"map_loot_pages\",\"full\":"<<full<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="map_loot_character") {
            int n,other; ok=bool(args>>n>>other) && n==1 && other==3 && valid_unit(t,n) && valid_unit(t,other) && t.army[n]==ARM && t.army[other]==ARM;
            if(ok) {
                NUMER=n; ARMIA[ARM][2][TE]=0; ARMIA[ARM][other][TE]=100; ARMIA[ARM][other][TEM]=100;
                for(int i=0;i<8;++i) { ARMIA[ARM][n][TPLECAK+i]=3; ARMIA[ARM][other][TPLECAK+i]=0; }
                for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                GLEBA[0][0]=24; TimedLootPage clicks={0};
                SDL_TimerID timer=SDL_AddTimer(800,push_map_loot_character,&clicks);
                if(timer) { WYBOR(0,true); SDL_RemoveTimer(timer); } else ok=false;
                ok=ok && clicks.step==4 && NUMER==other && GLEBA[0][0]==0 && ARMIA[ARM][other][TPLECAK]==24 && ARMIA[ARM][n][TPLECAK]==3;
                std::cout<<"{\"event\":\"map_loot_character\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="map_loot_supply_limit") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                int item=0; for(int i=1;i<=MX_WEAPON;++i) if(BRON[i][B_TYP]==17 && BRON[i][B_DOSW]>1) { item=i; break; }
                if(!item) ok=false;
                for(int pass=0;pass<2 && ok;++pass) {
                    NUMER=n; for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                    GLEBA[0][0]=item; ARMIA[ARM][0][TAMO]=pass ? 320-(BRON[item][B_DOSW]-1) : 320;
                    const int amount=ARMIA[ARM][0][TAMO]; ARMIA[ARM][n][TPLECAK]=3; TimedLootPage clicks={0};
                    SDL_TimerID timer=SDL_AddTimer(800,push_map_loot_supply,&clicks);
                    if(timer) { WYBOR(0,true); SDL_RemoveTimer(timer); } else ok=false;
                    ok=ok && clicks.step==4 && GLEBA[0][0]==item && ARMIA[ARM][0][TAMO]==amount && ARMIA[ARM][n][TPLECAK]==3;
                }
                std::cout<<"{\"event\":\"map_loot_supply_limit\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="map_loot_swap") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                GLEBA[0][0]=24; GLEBA[0][1]=3; ARMIA[ARM][n][TPLECAK]=0;
                TimedLootPage clicks={0}; SDL_TimerID timer=SDL_AddTimer(800,push_map_loot_swap,&clicks);
                if(timer) { WYBOR(0,true); SDL_RemoveTimer(timer); } else ok=false;
                ok=ok && clicks.step==4 && GLEBA[0][0]==3 && GLEBA[0][1]==24 && ARMIA[ARM][n][TPLECAK]==0;
                std::cout<<"{\"event\":\"map_loot_swap\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="map_loot_cancel_owned") {
            int n,equipment; ok=bool(args>>n>>equipment) && valid_unit(t,n) && t.army[n]==ARM && (equipment==0 || equipment==1);
            if(ok) {
                NUMER=n; for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=81;
                for(int i=0;i<8;++i) ARMIA[ARM][n][TPLECAK+i]=3;
                ARMIA[ARM][n][TGLOWA]=equipment ? 24 : 0;
                if(!equipment) ARMIA[ARM][n][TPLECAK]=24;
                TimedLootPage clicks={0,bool(equipment)}; SDL_TimerID timer=SDL_AddTimer(800,push_map_loot_cancel_owned,&clicks);
                if(timer) { WYBOR(0,true); SDL_RemoveTimer(timer); } else ok=false;
                ok=ok && clicks.step==3 && (equipment ? ARMIA[ARM][n][TGLOWA]==24 : ARMIA[ARM][n][TPLECAK]==24);
                for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) ok=ok && GLEBA[sector][slot]==81;
                std::cout<<"{\"event\":\"map_loot_cancel_owned\",\"equipment\":"<<equipment<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="map_loot_owned_swap") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                int helmet=0; for(int i=1;i<=MX_WEAPON;++i) if(i!=24 && BRON[i][B_PLACE]==1 && BRON[i][B_TYP]!=13 && BRON[i][B_TYP]!=18) { helmet=i; break; }
                ok=helmet>0; NUMER=n;
                for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                GLEBA[0][0]=24; ARMIA[ARM][n][TGLOWA]=helmet;
                for(int i=0;i<8;++i) ARMIA[ARM][n][TPLECAK+i]=3;
                TimedLootPage clicks={0}; SDL_TimerID timer=SDL_AddTimer(800,push_map_loot_owned_swap,&clicks);
                if(timer) { WYBOR(0,true); SDL_RemoveTimer(timer); } else ok=false;
                ok=ok && clicks.step==6 && GLEBA[0][0]==24 && ARMIA[ARM][n][TGLOWA]==helmet;
                for(int i=0;i<8;++i) ok=ok && ARMIA[ARM][n][TPLECAK+i]==3;
                std::cout<<"{\"event\":\"map_loot_owned_swap\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="map_loot_empty_row") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                for(int i=0;i<31;++i) GLEBA[i/4][i%4]=i==0 ? 24 : 81;
                TimedLootPage clicks={0}; SDL_TimerID timer=SDL_AddTimer(800,push_map_loot_empty_row,&clicks);
                if(timer) { WYBOR(0,true); SDL_RemoveTimer(timer); } else ok=false;
                ok=ok && clicks.step==4 && GLEBA[0][0]==24;
                std::cout<<"{\"event\":\"map_loot_empty_row\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="end_action_hostile") {
            int n; ok=bool(args>>n) && n==1 && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                for(int i=1;i<=10;++i) { if(i!=n) ARMIA[ARM][i][TE]=0; ARMIA[WRG][i][TE]=0; }
                ARMIA[WRG][2][TE]=50; ARMIA[WRG][2][TKORP]=150; ARMIA[WRG][2][TRASA]=1;
                ARMIA[ARM][n][TX]=160; ARMIA[ARM][n][TY]=200; ARMIA[ARM][n][TE]=100; ARMIA[ARM][n][TEM]=100; ARMIA[ARM][n][TTRYB]=1;
                ARMIA[ARM][0][TE]=1; KTO_ATAKUJE=ARM; NUMER=n; GLEBA[0][0]=24;
                TimedEndAction clicks={0}; map_loot_capture_enabled=true; map_loot_capture_seen=false; hostile_end_test=true; hostile_end_seen=false;
                ClearKey(); ProcessEvents(); SDL_TimerID timer=SDL_AddTimer(800,push_hostile_end,&clicks);
                if(timer) { MAIN_ACTION(); SDL_RemoveTimer(timer); } else ok=false;
                hostile_end_test=false; map_loot_capture_enabled=false;
                ok=ok && hostile_end_seen && !map_loot_capture_seen && WYNIK_AKCJI==0;
                std::cout<<"{\"event\":\"end_action_hostile\",\"steps\":"<<clicks.step<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="end_action_loot") {
            int n,retreat,neutral; ok=bool(args>>n>>retreat>>neutral) && n==1 && valid_unit(t,n) && t.army[n]==ARM && (retreat==0 || retreat==1) && (neutral==0 || neutral==1);
            if(ok) {
                for(int i=1;i<=10;++i) { if(i!=n) ARMIA[ARM][i][TE]=0; ARMIA[WRG][i][TE]=0; }
                if(neutral) { ARMIA[WRG][2][TE]=50; ARMIA[WRG][2][TKORP]=20; ARMIA[WRG][2][TRASA]=1; }
                NUMER=n; ARMIA[ARM][n][TX]=retreat ? 10 : 160; ARMIA[ARM][n][TY]=retreat ? 10 : 200; ARMIA[ARM][n][TE]=100; ARMIA[ARM][n][TEM]=100;
                ARMIA[ARM][0][TE]=1; ARMIA[ARM][n][TPLECAK]=0; KTO_ATAKUJE=ARM;
                for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                GLEBA[0][0]=24; TimedEndAction clicks={0}; map_loot_capture_enabled=true; map_loot_capture_seen=false;
                ClearKey(); ProcessEvents(); SDL_TimerID timer=SDL_AddTimer(800,push_end_action,&clicks);
                if(timer) { MAIN_ACTION(); SDL_RemoveTimer(timer); } else ok=false;
                map_loot_capture_enabled=false;
                ok=ok && (retreat ? (!map_loot_capture_seen && WYNIK_AKCJI==0 && ARMIA[ARM][n][TPLECAK]==0) : (map_loot_capture_seen && WYNIK_AKCJI==1 && ARMIA[ARM][n][TPLECAK]==24));
                std::cout<<"{\"event\":\"end_action_loot\",\"retreat\":"<<retreat<<",\"neutral\":"<<neutral<<",\"steps\":"<<clicks.step<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_worn_transfer") {
            int n,mode; ok=bool(args>>n>>mode) && valid_unit(t,n) && t.army[n]==ARM && mode>=0 && mode<=3;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ResetStrengthOverflow(ARM,n);
                for(int i=0;i<13;++i) ARMIA[ARM][n][TGLOWA+i]=0;
                for(int i=0;i<20;++i) SKLEP[1][i]=0;
                ARMIA[ARM][n][TSI]=20; ARMIA[ARM][n][TP]=0;
                ARMIA[ARM][n][TAMO]=25; ARMIA[ARM][n][TSZ]=25;
                ARMIA[ARM][n][TEM]=100; ARMIA[ARM][n][TE]=50;
                const int oldEnergy=BRON[24][B_EN]; BRON[24][B_EN]=10;
                ARMIA[ARM][n][TGLOWA]=24; PRZELICZ(0,1); WAGA(ARM,n);
                if(mode>=2) ARMIA[ARM][n][TE]=1;
                const int health=ARMIA[ARM][n][TE],strength=ARMIA[ARM][n][TSI],weight=ARMIA[ARM][n][TWAGA],armor=ARMIA[ARM][n][TP];
                MIASTA[3][1][M_LUDZIE]=4; GRACZE[1][1]=10000;
                const int price0=BRON[24][B_CENA]+BRON[24][B_CENA]*MIASTA[3][BRON[24][B_TYP]][M_MUR]/100;
                const int price=price0-price0*10/100;
                TimedShopEquipment clicks={0,mode==3 ? 5 : mode}; shop_worn_capture=(mode==0); shop_worn_seen=false;
                SDL_TimerID timer=SDL_AddTimer(800,push_shop_equipment,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                shop_worn_capture=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                if(mode==0) ok=ok && shop_worn_seen && ARMIA[ARM][n][TGLOWA]==0 && SKLEP[1][0]==24 && GRACZE[1][1]==10000+price && ARMIA[ARM][n][TWAGA]==weight-BRON[24][B_WAGA];
                else if(mode==1) ok=ok && ARMIA[ARM][n][TGLOWA]==0 && ARMIA[ARM][n][TPLECAK]==24 && GRACZE[1][1]==10000 && ARMIA[ARM][n][TWAGA]==weight;
                else ok=ok && ARMIA[ARM][n][TGLOWA]==24 && ARMIA[ARM][n][TE]==health && ARMIA[ARM][n][TSI]==strength && ARMIA[ARM][n][TP]==armor && ARMIA[ARM][n][TWAGA]==weight && GRACZE[1][1]==10000;
                BRON[24][B_EN]=oldEnergy;
                std::cout<<"{\"event\":\"shop_worn_transfer\",\"mode\":"<<mode<<",\"steps\":"<<clicks.step<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_worn_move_hands") {
            int n,occupied; ok=bool(args>>n>>occupied) && valid_unit(t,n) && t.army[n]==ARM && (occupied==0 || occupied==1);
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ResetStrengthOverflow(ARM,n);
                for(int i=0;i<13;++i) ARMIA[ARM][n][TGLOWA+i]=0;
                for(int i=0;i<20;++i) SKLEP[1][i]=0;
                ARMIA[ARM][n][TSI]=20; ARMIA[ARM][n][TP]=0; ARMIA[ARM][n][TAMO]=25; ARMIA[ARM][n][TSZ]=25;
                ARMIA[ARM][n][TEM]=100; ARMIA[ARM][n][TE]=50;
                const int energy3=BRON[3][B_EN],energy4=BRON[4][B_EN]; BRON[3][B_EN]=10; BRON[4][B_EN]=3;
                ARMIA[ARM][n][TPRAWA]=3; PRZELICZ(4,1);
                if(occupied) { ARMIA[ARM][n][TLEWA]=4; PRZELICZ(3,1); }
                ARMIA[ARM][n][TE]=2; WAGA(ARM,n); const int weight=ARMIA[ARM][n][TWAGA];
                MIASTA[3][1][M_LUDZIE]=4; GRACZE[1][1]=10000;
                TimedShopEquipment clicks={0,occupied ? 7 : 6}; SDL_TimerID timer=SDL_AddTimer(800,push_shop_equipment,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && ARMIA[ARM][n][TPRAWA]==0 && ARMIA[ARM][n][TLEWA]==3 && ARMIA[ARM][n][TPLECAK]==(occupied ? 4 : 0) && ARMIA[ARM][n][TE]==(occupied ? 1 : 2) && ARMIA[ARM][n][TWAGA]==weight && GRACZE[1][1]==10000;
                BRON[3][B_EN]=energy3; BRON[4][B_EN]=energy4;
                std::cout<<"{\"event\":\"shop_worn_move_hands\",\"occupied\":"<<occupied<<",\"steps\":"<<clicks.step<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_full_stock_cancel") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n);
                for(int i=0;i<13;++i) ARMIA[ARM][n][TGLOWA+i]=0;
                for(int i=0;i<20;++i) SKLEP[1][i]=3;
                ARMIA[ARM][n][TGLOWA]=24; ARMIA[ARM][n][TE]=30; ARMIA[ARM][n][TEM]=100;
                MIASTA[3][1][M_LUDZIE]=4; GRACZE[1][1]=10000;
                TimedShopEquipment clicks={0,10}; SDL_TimerID timer=SDL_AddTimer(800,push_shop_equipment,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && ARMIA[ARM][n][TGLOWA]==24 && ARMIA[ARM][n][TE]==30 && GRACZE[1][1]==10000;
                for(int i=0;i<20;++i) ok=ok && SKLEP[1][i]==3;
                std::cout<<"{\"event\":\"shop_full_stock_cancel\",\"steps\":"<<clicks.step<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_direct_purchase") {
            int n,full; ok=bool(args>>n>>full) && valid_unit(t,n) && t.army[n]==ARM && (full==0 || full==1);
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n);
                for(int i=0;i<13;++i) ARMIA[ARM][n][TGLOWA+i]=0;
                for(int i=0;i<20;++i) SKLEP[1][i]=0;
                for(int i=0;i<8;++i) ARMIA[ARM][n][TPLECAK+i]=full ? 3 : 0;
                if(full) ARMIA[ARM][n][TPRAWA]=3;
                MIASTA[3][1][M_LUDZIE]=4; SKLEP[1][0]=full ? 4 : 24; GRACZE[1][1]=10000;
                const int item=SKLEP[1][0]; const int price=BRON[item][B_CENA]+BRON[item][B_CENA]*MIASTA[3][BRON[item][B_TYP]][M_MUR]/100;
                shop_restored_capture=false; shop_restored_seen=false;
                TimedShopEquipment clicks={0,full ? 4 : 3}; SDL_TimerID timer=SDL_AddTimer(800,push_shop_equipment,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                if(full) ok=ok && shop_restored_seen && ARMIA[ARM][n][TPRAWA]==3 && SKLEP[1][0]==4 && GRACZE[1][1]==10000;
                else ok=ok && ARMIA[ARM][n][TGLOWA]==24 && SKLEP[1][0]==0 && GRACZE[1][1]==10000-price;
                shop_restored_capture=false;
                std::cout<<"{\"event\":\"shop_direct_purchase\",\"full\":"<<full<<",\"steps\":"<<clicks.step<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_sets_direct_buy") {
            int n; ok=bool(args>>n) && n==1 && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n);
                for(int i=0;i<13;++i) ARMIA[ARM][n][TGLOWA+i]=0;
                for(int i=0;i<20;++i) SKLEP[1][i]=0;
                ARMIA[ARM][n][TPRAWA]=3; StoreSpareHandItem(ARM,n,0,0); StoreSpareHandItem(ARM,n,1,0);
                MIASTA[3][1][M_LUDZIE]=4; SKLEP[1][0]=4; GRACZE[1][1]=10000;
                const int price=BRON[4][B_CENA]+BRON[4][B_CENA]*MIASTA[3][BRON[4][B_TYP]][M_MUR]/100;
                TimedShopEquipment clicks={0,8}; SDL_TimerID timer=SDL_AddTimer(800,push_shop_equipment,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==5 && EquipmentMode(ARM,n)==0 && ARMIA[ARM][n][TPRAWA]==3 && SpareHandItem(ARM,n,1)==4 && SKLEP[1][0]==0 && GRACZE[1][1]==10000-price;
                std::cout<<"{\"event\":\"shop_sets_direct_buy\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_other_direct_buy") {
            int n,other; ok=bool(args>>n>>other) && n==1 && other==3 && valid_unit(t,n) && valid_unit(t,other) && t.army[n]==ARM && t.army[other]==ARM;
            if(ok) {
                NUMER=n; ARMIA[ARM][2][TE]=0; ARMIA[ARM][other][TE]=50; ARMIA[ARM][other][TEM]=100;
                ResetEquipmentSet(ARM,n); ResetEquipmentSet(ARM,other);
                ARMIA[ARM][n][TGLOWA]=0; ARMIA[ARM][other][TGLOWA]=0;
                for(int i=0;i<20;++i) SKLEP[1][i]=0;
                MIASTA[3][1][M_LUDZIE]=4; SKLEP[1][0]=24; GRACZE[1][1]=10000;
                const int price=BRON[24][B_CENA]+BRON[24][B_CENA]*MIASTA[3][BRON[24][B_TYP]][M_MUR]/100;
                TimedShopEquipment clicks={0,9}; SDL_TimerID timer=SDL_AddTimer(800,push_shop_equipment,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==4 && NUMER==n && ARMIA[ARM][other][TGLOWA]==24 && ARMIA[ARM][n][TGLOWA]==0 && SKLEP[1][0]==0 && GRACZE[1][1]==10000-price;
                std::cout<<"{\"event\":\"shop_other_direct_buy\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="shop_equipment_buy") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ARMIA[ARM][n][TLEWA]=0; ARMIA[ARM][n][TPRAWA]=3;
                for(int i=0;i<8;++i) ARMIA[ARM][n][TPLECAK+i]=0;
                MIASTA[3][1][M_LUDZIE]=4; SKLEP[1][0]=4; GRACZE[1][1]=10000;
                const int price=BRON[4][B_CENA]+BRON[4][B_CENA]*MIASTA[3][BRON[4][B_TYP]][M_MUR]/100;
                TimedInventory clicks={0,30,438,606,210}; shop_info_capture_enabled=true;
                SDL_TimerID timer=SDL_AddTimer(1200,push_inventory_input,&clicks);
                if(timer) { _SKLEP(3,1,ARM,n); SDL_RemoveTimer(timer); } else ok=false;
                shop_info_capture_enabled=false;
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=ok && clicks.step==3 && ARMIA[ARM][n][TPRAWA]==4 && ARMIA[ARM][n][TPLECAK]==3 && SKLEP[1][0]==0 && GRACZE[1][1]==10000-price;
                std::cout<<"{\"event\":\"shop_equipment_buy\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="paused_mode_key") {
            int n; ok=bool(args>>n) && n==1 && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ARMIA[ARM][n][TLEWA]=3; ARMIA[ARM][n][TPRAWA]=0;
                StoreSpareHandItem(ARM,n,0,20); StoreSpareHandItem(ARM,n,1,39);
                Screen(0); CENTER(ARMIA[ARM][n][TX],ARMIA[ARM][n][TY],0);
                tactical_mode_test=true; tactical_mode_seen=false; tactical_mode_start=SDL_GetTicks(); KONIEC_AKCJI=0;
                SDL_TimerID timer=SDL_AddTimer(800,push_switch_key,0);
                if(timer) { MAIN_ACTION(); SDL_RemoveTimer(timer); } else ok=false;
                tactical_mode_test=false; KONIEC_AKCJI=0;
                ok=ok && tactical_mode_seen && ARMIA[ARM][n][TLEWA]==20 && ARMIA[ARM][n][TPRAWA]==39;
                std::cout<<"{\"event\":\"paused_mode_key\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="equipment_death_drop") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ARMIA[ARM][n][TX]=635; ARMIA[ARM][n][TY]=95; ARMIA[ARM][n][TE]=50;
                for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                for(int slot=0;slot<4;++slot) GLEBA[19][slot]=81;
                StoreSpareHandItem(ARM,n,0,20); StoreSpareHandItem(ARM,n,1,39);
                Screen(0); ZABIJ(ARM,n,1);
                int found=0; for(int sector=0;sector<110;++sector) for(int slot=0;slot<4;++slot)
                    if(GLEBA[sector][slot]==20 || GLEBA[sector][slot]==39) {
                        int x=0,y=0; if(GroundItemPosition(sector,slot,x,y) && (x-635)*(x-635)+(y-95)*(y-95)<10000) ++found;
                    }
                ok=found==2 && SpareHandItem(ARM,n,0)==0 && SpareHandItem(ARM,n,1)==0;
                NOWA_POSTAC(ARM,n,0);
                ok=ok && EquipmentMode(ARM,n)==0 && SpareHandItem(ARM,n,0)==0 && SpareHandItem(ARM,n,1)==0;
                std::cout<<"{\"event\":\"equipment_death_drop\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="equipment_sets_roundtrip") {
            int n,other; ok=bool(args>>n>>other) && valid_unit(t,n) && valid_unit(t,other) && t.army[n]==ARM && t.army[other]==ARM;
            if(ok) {
                NUMER=n; ResetEquipmentSet(ARM,n); ResetEquipmentSet(ARM,other);
                ARMIA[ARM][n][TLEWA]=3; ARMIA[ARM][n][TPRAWA]=0; PRZELICZ(3,1);
                const long strength=StrengthBeforeLimit(ARM,n), speed=ARMIA[ARM][n][TAMO];
                StoreSpareHandItem(ARM,n,0,20); StoreSpareHandItem(ARM,n,1,39); WAGA(ARM,n);
                const int weight=ARMIA[ARM][n][TWAGA]; ARMIA[ARM][n][TE]=1;
                const int originalEN=BRON[3][B_EN]; BRON[3][B_EN]=10;
                ok=SwitchEquipmentSet(ARM,n,1) && EquipmentMode(ARM,n)==1 && ARMIA[ARM][n][TLEWA]==20 && ARMIA[ARM][n][TPRAWA]==39 && SpareHandItem(ARM,n,0)==3 && ARMIA[ARM][n][TWAGA]==weight && ARMIA[ARM][n][TE]==1;
                ok=ok && SwitchEquipmentSet(ARM,n,0) && StrengthBeforeLimit(ARM,n)==strength && ARMIA[ARM][n][TAMO]==speed && ARMIA[ARM][n][TWAGA]==weight && ARMIA[ARM][n][TE]==1;
                BRON[3][B_EN]=originalEN;
                ok=ok && SwitchEquipmentSet(ARM,n,1);
                StoreSpareHandItem(ARM,other,0,4); StoreEquipmentMode(ARM,other,0);
                ok=ok && AutoSaveGame("equipment_sets_test");
                const astr path=SaveSlotPath(KAT_S+"archiwum/",0);
                std::ifstream f(path.c_str(),std::ios::binary);
                std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
                if(bytes.size()<=20+EquipmentSetsSaveSize()) ok=false;
                else {
                    const size_t length=bytes.size()-20; bytes.resize(60000,0);
                    ResetAllEquipmentSets(); ODCZYT(&bytes[20],length);
                    ok=ok && EquipmentMode(ARM,n)==1 && SpareHandItem(ARM,n,0)==3 && ARMIA[ARM][n][TLEWA]==20 && SpareHandItem(ARM,other,0)==4;
                    ODCZYT(&bytes[20],length-EquipmentSetsSaveSize());
                    ok=ok && EquipmentMode(ARM,n)==0 && SpareHandItem(ARM,n,0)==0 && SpareHandItem(ARM,other,0)==0;
                }
                std::cout<<"{\"event\":\"equipment_sets_roundtrip\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="title_continue_select") {
            // Enter the actual title menu: cancel, reopen, choose a manual
            // save, then repeat with autosave, all on isolated test data.
            DZIEN=77; _ZAPIS("DLUGI_ZAPIS_TEST_123",1);
            DZIEN=123; ok=AutoSaveGame("title_menu_test"); DZIEN=9;
            const char *oldSkip=std::getenv("LEGION_SKIP_INTRO");
            const bool hadSkip=oldSkip!=0; const std::string skip=oldSkip ? oldSkip : "";
            #ifdef _WIN32
            _putenv_s("LEGION_SKIP_INTRO","1");
#else
            setenv("LEGION_SKIP_INTRO","1",1);
#endif
            title_test_active=true; title_dialog_captured=false; title_dialog_frames=0;
            TimedTitle manual={0,true}; title_pending=&manual; title_timer=0; title_menu_frames=0;
            main_code(); if(title_timer) SDL_RemoveTimer(title_timer); else ok=false; title_pending=0; title_timer=0;
            ok=ok && manual.step==4 && title_dialog_captured && DZIEN==77;
            DZIEN=9; TimedTitle automatic={0,false};
            title_pending=&automatic; title_timer=0; title_menu_frames=0;
            main_code(); if(title_timer) SDL_RemoveTimer(title_timer); else ok=false; title_pending=0; title_timer=0;
            ok=ok && automatic.step==2 && DZIEN==123;
            title_test_active=false;
            #ifdef _WIN32
            _putenv_s("LEGION_SKIP_INTRO",hadSkip ? skip.c_str() : "");
#else
            if(hadSkip) setenv("LEGION_SKIP_INTRO",skip.c_str(),1); else unsetenv("LEGION_SKIP_INTRO");
#endif
            std::cout<<"{\"event\":\"title_continue_select\",\"manual\":"<<manual.step<<",\"auto\":"<<automatic.step<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
#ifdef _WIN32
        } else if (op=="windows_unicode_directory") {
            const wchar_t *value=_wgetenv(L"LEGION_UNICODE_TEST_DIR");
            const std::wstring directory=value ? value : L"";
            ok=!directory.empty() && _wputenv_s(L"LOCALAPPDATA",directory.c_str())==0 &&
               _wputenv_s(L"TEMP",directory.c_str())==0;
            std::cout<<"{\"event\":\"windows_unicode_directory\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="windows_save_directory") {
            const char *automation=std::getenv("LEGION_AUTOMATION");
            const std::string original=automation ? automation : "";
            _putenv_s("LEGION_AUTOMATION","");
            const astr directory=SaveDirectory();
            const astr path=SaveSlotPath(directory,0);
            DZIEN=37; ok=directory!=KAT_S+"archiwum/" && AutoSaveGame("windows_test_first");
            DZIEN=38; ok=ok && AutoSaveGame("windows_test_replace");
            std::ifstream file(path.c_str(),std::ios::binary);
            std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
            if(bytes.size()<=20 || bytes.size()>60000) ok=false;
            else { const size_t size=bytes.size()-20; bytes.resize(60000,0); DZIEN=0; ODCZYT(&bytes[20],size); ok=ok && DZIEN==38; }
            _putenv_s("LEGION_AUTOMATION",original.c_str());
            std::cout<<"{\"event\":\"windows_save_directory\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
#endif
        } else if (op=="autosave_roundtrip") {
            const astr directory=KAT_S+"archiwum/";
            const astr path=SaveSlotPath(directory,0);
            DZIEN=123; GRACZE[1][2]=4567;
            _ZAPIS("MANUAL",1);
            std::ifstream before_manual(SaveSlotPath(directory,1).c_str(),std::ios::binary);
            std::vector<unsigned char> manual((std::istreambuf_iterator<char>(before_manual)),std::istreambuf_iterator<char>());
            ok=AutoSaveGame("regression") && !manual.empty();
            std::ifstream save(path.c_str(),std::ios::binary);
            std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(save)),std::istreambuf_iterator<char>());
            _ZAPIS("FORBIDDEN",0);
            std::ifstream after_auto(path.c_str(),std::ios::binary);
            std::vector<unsigned char> protected_bytes((std::istreambuf_iterator<char>(after_auto)),std::istreambuf_iterator<char>());
            std::ifstream after_manual(SaveSlotPath(directory,1).c_str(),std::ios::binary);
            std::vector<unsigned char> manual_after((std::istreambuf_iterator<char>(after_manual)),std::istreambuf_iterator<char>());
            ok=ok && bytes.size()>20 && bytes.size()<=60000 && bytes==protected_bytes && manual==manual_after;
            if(ok) {
                DZIEN=9; GRACZE[1][2]=1; ResetAllStrengthOverflow();
                size_t length=bytes.size()-20; bytes.resize(60000,0); ODCZYT(&bytes[20],length);
                ok=DZIEN==123 && GRACZE[1][2]==4567;
            }
            std::cout<<"{\"event\":\"autosave_roundtrip\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="autosave_menu") {
            SETUP0(); VISUAL_OBJECTS(); Screen(0); ReserveZone(130); SX=0; SY=0;
            rysuj(); SDIR("TEST",21,20);
            ok=_Zone(OKX+20,OKY+35)==7 && _Zone(OKX+20,OKY+55)==1 &&
                _Zone(OKX+20,OKY+135)==5 && _Zone(OKX+20,OKY+155)==6;
            if(ok) ok=capture_buffer(std::string(outdir)+"/autozapis_menu.png");
            ZOKNO();
            std::cout<<"{\"event\":\"autosave_menu\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="autosave_save_dialog") {
            SETUP0(); VISUAL_OBJECTS(); SX=0; SY=0;
            const astr path=SaveSlotPath(KAT_S+"archiwum/",0);
            std::ifstream before(path.c_str(),std::ios::binary);
            std::vector<unsigned char> original((std::istreambuf_iterator<char>(before)),std::istreambuf_iterator<char>());
            TimedInventory clicks={0,XHard(160),YHard(95),XHard(160),YHard(215)};
            SDL_TimerID timer=SDL_AddTimer(800,push_inventory_input,&clicks);
            if(timer) { _SAVE_GAME(); SDL_RemoveTimer(timer); } else ok=false;
            std::ifstream after(path.c_str(),std::ios::binary);
            std::vector<unsigned char> result((std::istreambuf_iterator<char>(after)),std::istreambuf_iterator<char>());
            ok=ok && clicks.step>=2 && !original.empty() && result==original;
            std::cout<<"{\"event\":\"autosave_save_dialog\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="autosave_load_dialog") {
            SETUP0(); VISUAL_OBJECTS(); SX=0; SY=0;
            DZIEN=9; GRACZE[1][2]=1; ProcessEvents();
            TimedClick click=scaled_click(XHard(160),YHard(95));
            SDL_TimerID timer=SDL_AddTimer(800,push_timed_click,&click);
            if(timer) { _LOAD_GAME(); SDL_RemoveTimer(timer); } else ok=false;
            ok=ok && Param!=0 && DZIEN==123 && GRACZE[1][2]==4567;
            std::cout<<"{\"event\":\"autosave_load_dialog\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
        } else if (op=="save_reload") {
            _ZAPIS("TEST_STAT",5);
            std::ifstream save((KAT_S+"archiwum/zapis 5").c_str(),std::ios::binary);
            std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(save)),std::istreambuf_iterator<char>());
            ok=bytes.size()>20 && bytes.size()<=60000;
            if(ok) {size_t length=bytes.size()-20; ResetAllStrengthOverflow(); bytes.resize(60000,0); ODCZYT(&bytes[20],length);}
        } else if (op=="expect_energy") {
            int n,energy; ok=bool(args>>n>>energy) && valid_unit(t,n) && ARMIA[t.army[n]][n][TE]==energy;
        } else if (op=="health") {
            int n,current,maximum; ok=bool(args>>n>>current>>maximum) && valid_unit(t,n) && current>=0 && maximum>=0 && current<=10000 && maximum<=10000;
            if(ok) {ARMIA[t.army[n]][n][TE]=current; ARMIA[t.army[n]][n][TEM]=maximum;}
        } else if (op=="action_markers_test") {
            int a,b,c; ok=bool(args>>a>>b>>c) && valid_unit(t,a) && valid_unit(t,b) && valid_unit(t,c) && t.army[a]==ARM && t.army[b]==ARM && t.army[c]==ARM;
            if(ok) {
                ARMIA[ARM][a][TTRYB]=0; ARMIA[ARM][b][TTRYB]=1; ARMIA[ARM][c][TTRYB]=2;
                MARKERS(); SetTacticalPauseHealthBars(true);
                snapshot(t,std::string(outdir)+"/kolory_rozkazow.png");
                ok=action_marker_pixel(t,a,MarkerIdle) && action_marker_pixel(t,b,MarkerMoving) && action_marker_pixel(t,c,MarkerAttacking);
                const int expected[]={MarkerIdle,MarkerMoving,MarkerAttacking,MarkerAttacking,MarkerOther,MarkerOther,MarkerMoving,MarkerMoving,MarkerMoving};
                VEKTOR_F[a][0]=0;
                for(int mode=0;mode<=8 && ok;++mode) {
                    ARMIA[ARM][a][TTRYB]=mode;
                    ok=TacticalActionMarkerColour(mode)==expected[mode] && action_marker_pixel(t,a,expected[mode]);
                }
                // Both branches of magic: offensive fireball versus healing.
                for(int mode=4;mode<=5 && ok;++mode) {
                    ARMIA[ARM][a][TTRYB]=mode; VEKTOR_F[a][0]=42;
                    ok=TacticalActionMarkerColour(mode,42)==MarkerAttacking && action_marker_pixel(t,a,MarkerAttacking);
                    for(int spell=47;spell<=49 && ok;++spell) {
                        VEKTOR_F[a][0]=spell;
                        ok=TacticalActionMarkerColour(mode,spell)==MarkerOther && action_marker_pixel(t,a,MarkerOther);
                    }
                    if(ok && mode==5) ok=snapshot(t,std::string(outdir)+"/kolory_czarow.png");
                }
                SetTacticalPauseHealthBars(false);
                ARMIA[ARM][a][TTRYB]=1;
                ok=ok && ActionMarkersVisible() && action_marker_pixel(t,a,MarkerMoving);
                // Exercise the movement implementation until it completes,
                // rather than only assigning its state in the test.
                ARMIA[ARM][a][TCELX]=ARMIA[ARM][a][TX]+24;
                ARMIA[ARM][a][TCELY]=ARMIA[ARM][a][TY];
                Screen(0); A_RUCH(ARM,a);
                ok=ok && ARMIA[ARM][a][TTRYB]==1 && action_marker_pixel(t,a,MarkerMoving);
                for(int tick=0;tick<100 && ARMIA[ARM][a][TTRYB]==1;++tick) A_RUCH(ARM,a);
                ok=ok && ARMIA[ARM][a][TTRYB]==0 && action_marker_pixel(t,a,MarkerIdle);
                ARMIA[ARM][a][TTRYB]=1;
                BobOnOff(a,false);
                ok=ok && !action_marker_pixel(t,a,MarkerMoving);
                BobOnOff(a,true); MARKERS_OFF();
                ok=ok && !ActionMarkersVisible() && !action_marker_pixel(t,a,MarkerMoving);
                ARMIA[ARM][a][TTRYB]=ARMIA[ARM][b][TTRYB]=ARMIA[ARM][c][TTRYB]=0;
                std::cout<<"{\"event\":\"action_markers\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="pause_bars") {
            int value; ok=bool(args>>value) && (value==0 || value==1);
            if(ok) SetTacticalPauseHealthBars(value!=0);
        } else if (op=="bob_visibility") {
            int n,on; ok=bool(args>>n>>on) && valid_unit(t,n) && (on==0 || on==1);
            if(ok) BobOnOff(n+(t.army[n]==WRG ? 10 : 0),on!=0);
        } else if (op=="visible_corpse_drop") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM;
            if(ok) {
                ARMIA[ARM][n][TY]+=20;
                const int x=ARMIA[ARM][n][TX],y=ARMIA[ARM][n][TY],sector=x/64+(y/50)*10;
                for(int slot=0;slot<4;++slot) GLEBA[sector][slot]=0;
                ARMIA[ARM][n][TKORP]=3; ARMIA[ARM][n][TE]=50; PREFS[4]=1;
                Screen(0); ZABIJ(ARM,n,0); render(t);
                ok=GLEBA[sector][0]==3 && capture_buffer(std::string(outdir)+"/zwloki_i_lup.png");
                std::cout<<"{\"event\":\"visible_corpse_drop\",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="expect_health_bar") {
            int n,expected; ok=bool(args>>n>>expected) && valid_unit(t,n) && (expected==0 || expected==1);
            if(ok) ok=health_bar_visible(t,n)==(expected!=0);
        } else if (op=="expect_health_fill") {
            int n,expected; ok=bool(args>>n>>expected) && valid_unit(t,n) && expected>=0 && expected<=30;
            if(ok) ok=std::abs(health_bar_fill(t,n)-expected)<=2;
        } else if (op=="action_pause_cycle") {
            int n; ok=bool(args>>n) && valid_unit(t,n) && t.army[n]==ARM && ARMIA[ARM][n][TE]>0;
            if(ok) {
                NUMER=n; ARMIA[ARM][0][TE]=1; KTO_ATAKUJE=ARM;
                SetTacticalPauseHealthBars(true); action_frame_enabled=true; action_frame_unpaused=false; action_frame_markers=false;
                MARKERS();
                ClearKey(); ProcessEvents();
                SDL_TimerID timer=SDL_AddTimer(600,push_space,0);
                if(timer) {AKCJA(); SDL_RemoveTimer(timer);} else ok=false;
                action_frame_enabled=false;
                ok=ok && action_frame_unpaused && action_frame_markers && TacticalPauseHealthBars() && ActionMarkersVisible();
                MARKERS_OFF();
                if(ok) {rysuj(); if(!capture_buffer(std::string(outdir)+"/pauza_po_akcji.png")) ok=false;}
                std::cout<<"{\"event\":\"action_pause_cycle\",\"active_without_bars\":"<<(action_frame_unpaused?"true":"false")<<",\"paused\":"<<(TacticalPauseHealthBars()?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="kill") {
            int n;ok=bool(args>>n) && valid_unit(t,n) && ARMIA[t.army[n]][n][TE]>0;
            if(ok) {Screen(0); ZABIJ(t.army[n],n,1);}
        } else if (op=="expect_ground") {
            int sector,slot,item;ok=bool(args>>sector>>slot>>item) && sector>=0 && sector<110 && slot>=0 && slot<4 && item>=0 && item<=120;
            if(ok) {
                int x=0,y=0;bool shown=GroundItemPosition(sector,slot,x,y);
                ok=GLEBA[sector][slot]==item && (shown==(item>0));
                std::cout<<"{\"event\":\"ground\",\"sector\":"<<sector<<",\"slot\":"<<slot<<",\"item\":"<<GLEBA[sector][slot]<<",\"x\":"<<x<<",\"y\":"<<y<<"}"<<std::endl;
            }
        } else if (op=="expect_nearby") {
            int x,y,sector,slot,expected; ok=bool(args>>x>>y>>sector>>slot>>expected) && valid_xy(x,y) && sector>=0 && sector<110 && slot>=0 && slot<4 && (expected==0 || expected==1);
            if(ok) {
                GroundSlot found[4]; NearbyGroundSlots(x,y,found); bool present=false;
                for(int i=0;i<4;++i) if(found[i].sector==sector && found[i].slot==slot) present=true;
                ok=present==(expected!=0);
                std::cout<<"{\"event\":\"nearby\",\"x\":"<<x<<",\"y\":"<<y<<",\"sector\":"<<sector<<",\"slot\":"<<slot<<",\"present\":"<<(present?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="inventory_pick") {
            int n,sector,slot,item; ok=bool(args>>n>>sector>>slot>>item) && valid_unit(t,n) && t.army[n]==ARM && sector>=0 && sector<110 && slot>=0 && slot<4 && GLEBA[sector][slot]==item;
            if(ok) {
                NUMER=n; TimedInventory clicks={0,260,464,260,274};
                inventory_capture_enabled=true;
                SDL_TimerID timer=SDL_AddTimer(800,push_inventory_input,&clicks);
                WYBOR(0);
                inventory_capture_enabled=false;
                if(timer) SDL_RemoveTimer(timer);
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                ok=clicks.step==3 && GLEBA[sector][slot]==0 && ARMIA[ARM][n][TPLECAK]==item;
                std::cout<<"{\"event\":\"inventory_pick\",\"clicks\":"<<clicks.step<<",\"ground\":"<<GLEBA[sector][slot]<<",\"backpack\":"<<ARMIA[ARM][n][TPLECAK]<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="inventory_drop") {
            int n,sector,slot,item; ok=bool(args>>n>>sector>>slot>>item) && valid_unit(t,n) && t.army[n]==ARM && sector>=0 && sector<110 && slot>=0 && slot<4 && GLEBA[sector][slot]==0 && ARMIA[ARM][n][TPLECAK]==item;
            if(ok) {
                NUMER=n; TimedInventory clicks={0,260,274,310,464};
                SDL_TimerID timer=SDL_AddTimer(800,push_inventory_input,&clicks);
                WYBOR(0);
                if(timer) SDL_RemoveTimer(timer);
                SDL_Event e; SDL_memset(&e,0,sizeof(e)); e.type=SDL_MOUSEBUTTONUP; e.button.which=255; e.button.button=SDL_BUTTON_RIGHT; SDL_PushEvent(&e); ProcessEvents();
                int x,y; ok=clicks.step==3 && GLEBA[sector][slot]==item && ARMIA[ARM][n][TPLECAK]==0 && GroundItemPosition(sector,slot,x,y) && x/64+(y/50)*10==sector;
                std::cout<<"{\"event\":\"inventory_drop\",\"clicks\":"<<clicks.step<<",\"ground\":"<<GLEBA[sector][slot]<<",\"backpack\":"<<ARMIA[ARM][n][TPLECAK]<<",\"success\":"<<(ok?"true":"false")<<"}"<<std::endl;
            }
        } else if (op=="expect_ground_spread") {
            int sector;ok=bool(args>>sector) && sector>=0 && sector<110;
            if(ok) for(int i=0;i<4;++i) {
                int x1,y1;
                if(!GroundItemPosition(sector,i,x1,y1)) continue;
                if(x1<0 || x1>639 || y1<0 || y1>511 || x1/64+(y1/50)*10!=sector) ok=false;
                for(int j=i+1;j<4;++j) {
                    int x2,y2;
                    if(GroundItemPosition(sector,j,x2,y2) && std::abs(x1-x2)<12 && std::abs(y1-y2)<12) ok=false;
                }
            }
        } else if (op=="expect_ground_rendered" || op=="expect_ground_hidden") {
            int sector,slot;ok=t.ready && bool(args>>sector>>slot) && sector>=0 && sector<110 && slot>=0 && slot<4;
            if(ok) {
                int x,y; ok=GroundItemPosition(sector,slot,x,y);
                if(ok) { bool visible=ground_icon_visible(t,sector,slot); ok=visible==(op=="expect_ground_rendered"); }
            }
        } else if (op=="expect_ground_min") {
            int sector,minimum;ok=bool(args>>sector>>minimum) && sector>=0 && sector<110 && minimum>=0 && minimum<=4;
            if(ok) {int count=0;for(int i=0;i<4;++i) if(GLEBA[sector][i]>0) ++count;
                std::cout<<"{\"event\":\"ground_count\",\"sector\":"<<sector<<",\"count\":"<<count<<"}"<<std::endl;
                ok=count>=minimum;}
        } else if (op=="unit" || op=="enemy_unit") {
            int n,x,y,race,speed; ok=t.ready && bool(args>>n>>x>>y>>race>>speed) && n>=1 && n<=10 && !t.defined[n] && valid_xy(x,y) && race>=0 && race<=9 && speed>=1 && speed<=100;
            if(ok) {
                int a=op=="enemy_unit" ? WRG : ARM, zone=op=="enemy_unit" ? n+10 : n;
                Screen(0); NOWA_POSTAC(a,n,race); t.defined[n]=true; t.army[n]=a;
                ARMIA[a][n][TX]=x; ARMIA[a][n][TY]=y; ARMIA[a][n][TSZ]=speed;
                ARMIA[a][n][TBOB]=RASY[race][7]; ARMIA[a][n][TTRYB]=0;
                ARMIA[a][n][TCELX]=x; ARMIA[a][n][TCELY]=y;
                SetZone(zone,x-15,y-15,x+15,y); _Bob(zone,x,y,ARMIA[a][n][TBOB]+7);
                Point p={x,y};t.history[n].push_back(p);
            }
        } else if (op=="move") {
            int n,x,y; ok=bool(args>>n>>x>>y) && valid_unit(t,n) && valid_xy(x,y);
            if(ok) {int a=t.army[n];ARMIA[a][n][TCELX]=x; ARMIA[a][n][TCELY]=y; ARMIA[a][n][TTRYB]=1;}
        } else if (op=="ui_move") {
            int n,x,y; ok=bool(args>>n>>x>>y) && valid_unit(t,n) && t.army[n]==ARM && valid_xy(x,y);
            if(ok) {
                NUMER=n; Screen(0); CENTER(x,y,0);
                // RUCH adds 8 to the clicked world Y. Convert the intended
                // target to actual window coordinates at the current scale.
                TimedClick p=scaled_click(XHard(x),YHard(y-8));
                // Aim inside the logical pixel, not exactly at its rounded edge.
                ++p.x; ++p.y;
                ok=p.x>=0 && p.x<Core::il_screen_width && p.y>=0 && p.y<Core::il_screen_height;
                if(ok) {
                    MouseClick(); ProcessEvents();
                    SDL_TimerID timer=SDL_AddTimer(100,push_timed_click,&p);
                    if(!timer) ok=false;
                    else {
                        RUCH(); SDL_RemoveTimer(timer);
                        ok=ARMIA[ARM][n][TTRYB]==1 && ARMIA[ARM][n][TCELX]==x && ARMIA[ARM][n][TCELY]==y;
                        std::cout<<"{\"event\":\"ui_move\",\"unit\":"<<n<<",\"target_x\":"<<ARMIA[ARM][n][TCELX]
                                 <<",\"target_y\":"<<ARMIA[ARM][n][TCELY]<<",\"accepted\":"<<(ok?"true":"false")<<"}"<<std::endl;
                    }
                }
            }
        } else if (op=="tick") {
            int count; ok=bool(args>>count) && t.ready && count>=0 && count<=10000;
            if(ok) for(int k=0;k<count;++k) {
                ++t.tick;
                for(int n=1;n<=10;++n) if(t.defined[n]) {
                    int a=t.army[n];
                    if(ARMIA[a][n][TE]>0 && ARMIA[a][n][TTRYB]==1) A_RUCH(a,n);
                    Point p={ARMIA[a][n][TX],ARMIA[a][n][TY]};t.history[n].push_back(p);
                    state_json(trace,t,n); trace << '\n';
                }
                ProcessEvents();
            }
        } else if (op=="report") {
            int n; ok=bool(args>>n) && valid_unit(t,n);
            if(ok) {std::cout<<"TEST_STATE "; state_json(std::cout,t,n); std::cout<<std::endl;}
        } else if (op=="snapshot") {
            std::string name; ok=bool(args>>name) && t.ready && name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-. ")==std::string::npos && name.find('/')==std::string::npos && name!="." && name!="..";
            if(ok) ok=snapshot(t,std::string(outdir)+"/"+name);
        } else if (op=="font_sample") {
            ok=t.ready; if(ok) t.font_sample=true;
        } else if (op=="army_sample") {
            ok=valid_unit(t,1) && t.army[1]==ARM;
            if(ok) {
                t.font_sample=false; t.name_sample=false; t.army_sample=true;
                ARMIA_S[ARM][0]="Legion Testowy";
                ARMIA[ARM][0][TMAG]=1; ARMIA[ARM][0][TAMO]=100;
            }
        } else if (op=="name_sample") {
            ok=t.ready; if(ok) {t.font_sample=false; t.army_sample=false; t.name_sample=true;}
        } else if (op=="expect_fonts") {
            ok=t.ready && t.font_sample && fonts_visible(t);
        } else if (op=="expect_input_edit") {
            ok=t.ready;
            if(ok) {
                Screen(0); SetFont(FON1);
                TimedKeys keys={0};
                SDL_TimerID timer=SDL_AddTimer(120,push_input_test_key,&keys);
                if(!timer) ok=false;
                else {
                    astr name="Zażółć";
                    WPISZ_PC(20,120,200,20,12,name,0);
                    SDL_RemoveTimer(timer); SDL_SetModState(KMOD_NONE);
                    ok=name=="Zażóaęć";
                    std::cout<<"{\"event\":\"input_edit\",\"text\":\""<<name<<"\",\"ok\":"<<(ok?"true":"false")<<"}"<<std::endl;
                }
            }
        } else if (op=="camera") {
            int x,y; ok=t.ready && bool(args>>x>>y) && x>=0 && x<=639 && y>=0 && y<=511;
            if(ok) { Screen(0); CENTER(x,y,0); }
        } else if (op=="zone") {
            int x,y; ok=bool(args>>x>>y) && t.ready && x>=0 && x<=639 && y>=0 && y<=511;
            if(ok) {Screen(0); std::cout<<"{\"event\":\"zone\",\"x\":"<<x<<",\"y\":"<<y<<",\"zone\":"<<_Zone(x,y)<<"}"<<std::endl;}
        } else if (op=="expect_free") {
            int x,y;ok=bool(args>>x>>y) && t.ready && valid_xy(x,y);
            if(ok) {Screen(0);ok=!ZoneBlockedExcept(x,y,0,true);}
        } else if (op=="click") {
            int x,y; ok=bool(args>>x>>y) && t.ready && x>=0 && x<Core::il_screen_width && y>=0 && y<Core::il_screen_height;
            if(ok) ok=inject_mouse(x,y);
        } else if (op=="key") {
            int key; ok=bool(args>>key) && key>=0 && key<SDLK_LAST;
            if(ok) {
                ClearKey();
                SDL_Event e;SDL_memset(&e,0,sizeof(e));e.type=SDL_KEYDOWN;e.key.which=255;e.key.keysym.sym=SDLKey(key);SDL_PushEvent(&e);ProcessEvents();
                std::string character=Inkey_S(); int scancode=ScanCode();
                bool detected=key>=32 && key<=126 ? character.size()==1 && character[0]==key : scancode!=0;
                std::cout<<"{\"event\":\"key\",\"sdl\":"<<key<<",\"scancode\":"<<scancode<<",\"detected\":"<<(detected?"true":"false")<<"}"<<std::endl;
                SDL_memset(&e,0,sizeof(e));e.type=SDL_KEYUP;e.key.which=255;e.key.keysym.sym=SDLKey(key);SDL_PushEvent(&e);ProcessEvents();
                ok=detected;
            }
        } else if (op=="expect_reached") {
            int n; ok=bool(args>>n) && valid_unit(t,n);
            if(ok) {int a=t.army[n];ok=ARMIA[a][n][TTRYB]==0 && std::abs(ARMIA[a][n][TX]-ARMIA[a][n][TCELX])<=4 && std::abs(ARMIA[a][n][TY]-ARMIA[a][n][TCELY])<=4;}
        } else if (op=="expect_idle_near") {
            int n,r;ok=bool(args>>n>>r) && valid_unit(t,n) && r>=0;
            if(ok) {int a=t.army[n],dx=ARMIA[a][n][TX]-ARMIA[a][n][TCELX],dy=ARMIA[a][n][TY]-ARMIA[a][n][TCELY];
                ok=ARMIA[a][n][TTRYB]==0 && dx*dx+dy*dy<=r*r;}
        } else if (op=="expect_stuck") {
            int n,steps;ok=bool(args>>n>>steps) && valid_unit(t,n) && steps>0 && size_t(steps)<t.history[n].size();
            if(ok) {const Point &p=t.history[n][t.history[n].size()-1], &q=t.history[n][t.history[n].size()-1-steps];ok=p.x==q.x && p.y==q.y && ARMIA[t.army[n]][n][TTRYB]==1;}
        } else ok=false;
        std::string extra;
        if(ok && (args>>extra) && extra[0]!='#') ok=false;
        if(!ok) { std::ofstream failure((std::string(outdir)+"/test_error.txt").c_str()); failure<<"TEST ERROR at line "<<lineno<<": "<<line<<std::endl; std::cerr<<"TEST ERROR at line "<<lineno<<": "<<line<<std::endl; return 2; }
    }
    trace.flush(); if(!trace) {std::cerr<<"TEST: trace write failed"<<std::endl;return 2;}
    std::cout << "TEST_OK ticks=" << t.tick << std::endl;
    return 0;
}
