#include "group_attack.h"
#include "Amos.h"
#include "legion.h"
#include "lgn_akcja.h"
#include "lgn_util.h"
#include "pathfinding.h"
#include <algorithm>
#include <cmath>

namespace {
struct AutoOrder { bool enabled; int target, patrol, lastx, lasty, stalled; int retry, retryTarget, retryX, retryY; };
AutoOrder orders[11]={};
bool issuingAutomaticOrder=false;
bool battleOpponents=false;
struct TacticalScreen { int previous; TacticalScreen():previous(Screen()) {Screen(0);} ~TacticalScreen(){Screen(previous);} };
int clamp(int n,int lo,int hi) { return std::max(lo,std::min(hi,n)); }
bool hostile(int enemy) {
    if(enemy<1 || enemy>10 || ARMIA[WRG][enemy][TE]<=0 || !BobIsVisible(enemy+10)) return false;
    if(battleOpponents) return true;
    if(ARMIA[WRG][enemy][TRASA]>=10 || ARMIA[WRG][enemy][TKORP]>90) return true;
    if((ARMIA[WRG][enemy][TTRYB]==2 || ARMIA[WRG][enemy][TTRYB]==5) && ARMIA[WRG][enemy][TCELY]==ARM) return true;
    for(int i=1;i<=10;++i) if(ARMIA[ARM][i][TE]>0 &&
        (ARMIA[ARM][i][TTRYB]==2 || ARMIA[ARM][i][TTRYB]==5) &&
        ARMIA[ARM][i][TCELY]==WRG && ARMIA[ARM][i][TCELX]==enemy) return true;
    return false;
}
int nearest(int n) {
    int target=0,best=0x7fffffff;
    const int x=ARMIA[ARM][n][TX],y=ARMIA[ARM][n][TY];
    for(int i=1;i<=10;++i) if(hostile(i)) {
        const int dx=ARMIA[WRG][i][TX]-x,dy=ARMIA[WRG][i][TY]-y,d=dx*dx+dy*dy;
        const int sight=std::max(1,int(WIDOCZNOSC));
        if(d<=sight*sight && d<best) {best=d;target=i;}
    }
    return target;
}
bool blocks_line(int x1,int y1,int x2,int y2,int bx,int by) {
    // Same unit collision bounds as tactical zones, with a two-pixel margin.
    const double dx=x2-x1,dy=y2-y1;
    double low=0,high=1;
    const int origin[]={x1,y1}, delta[]={x2-x1,y2-y1};
    const int minv[]={bx-17,by-17},maxv[]={bx+17,by+2};
    for(int i=0;i<2;++i) {
        if(delta[i]==0) {if(origin[i]<minv[i] || origin[i]>maxv[i]) return false;continue;}
        double a=double(minv[i]-origin[i])/delta[i], b=double(maxv[i]-origin[i])/delta[i];
        if(a>b) std::swap(a,b);
        low=std::max(low,a); high=std::min(high,b);
        if(low>high) return false;
    }
    (void)dx; (void)dy;
    return low<=1 && high>=0;
}
bool clear_line_at(int n,int target,int x,int y) {
    const int tx=ARMIA[WRG][target][TX],ty=ARMIA[WRG][target][TY]-8;
    const int sy=y-20;
    for(int i=1;i<=10;++i) {
        if(i!=n && ARMIA[ARM][i][TE]>0 && blocks_line(x,sy,tx,ty,ARMIA[ARM][i][TX],ARMIA[ARM][i][TY])) return false;
        // A neutral person is not an acceptable accidental target either.
        if(i!=target && ARMIA[WRG][i][TE]>0 && !hostile(i) && BobIsVisible(i+10) &&
           blocks_line(x,sy,tx,ty,ARMIA[WRG][i][TX],ARMIA[WRG][i][TY])) return false;
    }
    const int dx=tx-x,dy=ty-sy,steps=std::max(std::abs(dx),std::abs(dy))/3;
    for(int s=1;s<steps;++s) {
        const int zone=_Zone(x+dx*s/steps,sy+dy*s/steps);
        if((zone>=21 && zone<=30) || zone>40) return false;
    }
    return true;
}
int spell(int n) {
    for(int hand=0;hand<2;++hand) {
        int item=ARMIA[ARM][n][hand==0 ? TLEWA : TPRAWA];
        if(item<=0 || item>MX_WEAPON || BRON[item][B_TYP]!=12 || BRON[item][B_MAG]>ARMIA[ARM][n][TMAG]) continue;
        const int type=BRON[item][B_DOSW];
        if((type==1 && item!=44) || (type==2 && (BRON[item][B_SI]>0 || BRON[item][B_SZ]<0))) return item;
    }
    return 0;
}
bool ranged(int n) {
    int l=ARMIA[ARM][n][TLEWA],r=ARMIA[ARM][n][TPRAWA];
    if(l<0 || l>MX_WEAPON || r<0 || r>MX_WEAPON) return false;
    int lt=BRON[l][B_TYP],rt=BRON[r][B_TYP];
    return (((lt==4 && rt==5) || (lt==5 && rt==4)) && STRZALY[n]>0) ||
           (lt==15 && rt==16) || (lt==16 && rt==15) ||
           (lt==9 && rt!=12) || (rt==9 && lt!=12);
}
bool has_bow_or_catapult(int n) {
    const int l=ARMIA[ARM][n][TLEWA],r=ARMIA[ARM][n][TPRAWA];
    if(l<0 || l>MX_WEAPON || r<0 || r>MX_WEAPON) return false;
    const int lt=BRON[l][B_TYP],rt=BRON[r][B_TYP];
    return lt==4 || rt==4 || lt==15 || rt==15;
}
bool can_melee(int n) {
    const int l=ARMIA[ARM][n][TLEWA],r=ARMIA[ARM][n][TPRAWA];
    if(l<0 || l>MX_WEAPON || r<0 || r>MX_WEAPON) return false;
    const int lt=BRON[l][B_TYP],rt=BRON[r][B_TYP];
    return lt!=4 && rt!=4 && lt!=15 && rt!=15 && lt!=12 && rt!=12;
}
void move_to(int n,int x,int y) {
    ARMIA[ARM][n][TCELX]=clamp(x,20,620); ARMIA[ARM][n][TCELY]=clamp(y,22,500); ARMIA[ARM][n][TTRYB]=1;
}
bool reposition(int n,int target) {
    const int x=ARMIA[ARM][n][TX],y=ARMIA[ARM][n][TY];
    const int dx=ARMIA[WRG][target][TX]-x,dy=ARMIA[WRG][target][TY]-y;
    const double len=std::sqrt(double(dx)*dx+double(dy)*dy);
    if(len<1) return false;
    const int offsets[]={40,-40,64,-64,88,-88};
    for(unsigned k=0;k<sizeof(offsets)/sizeof(offsets[0]);++k) {
        const int nx=clamp(x+int(-dy/len*offsets[k]),20,620),ny=clamp(y+int(dx/len*offsets[k]),22,500);
        const int z=_Zone(nx,ny);
        if((z!=0 && z!=n) || !clear_line_at(n,target,nx,ny)) continue;
        int wx,wy;
        if(PathWaypoint(ARM,n,n,x,y,nx,ny,true,wx,wy)) {move_to(n,nx,ny);return true;}
    }
    return false;
}
}
bool GroupAttackInternal() {return issuingAutomaticOrder;}
void GroupAttackSetBattle(bool battle) {battleOpponents=battle;}
bool GroupAttackEnabled(int n) {return n>=1 && n<=10 && orders[n].enabled;}
void GroupAttackCancel(int n) {if(n>=1 && n<=10) orders[n]=AutoOrder{};}
void GroupAttackCancelAll() {for(int n=1;n<=10;++n) GroupAttackCancel(n);}
bool GroupAttackLineClear(int n,int target) {TacticalScreen screen; return GroupAttackEnabled(n) && hostile(target) && clear_line_at(n,target,ARMIA[ARM][n][TX],ARMIA[ARM][n][TY]);}
bool GroupAttackSafeToLaunch(int n) {return GroupAttackEnabled(n) && GroupAttackLineClear(n,orders[n].target);}
void GroupAttackStart() {
    GroupAttackCancelAll();
    for(int n=1;n<=10;++n) if(ARMIA[ARM][n][TE]>0) {
        orders[n].enabled=true;
        const int target=nearest(n); orders[n].target=target;
        const int previous=ARMIA[ARM][n][TTRYB];
        if((previous==3 && TacticalProjectileStarted(n)) || previous==4 || previous==5 || previous==7 || previous==8) continue;
        // Prime melee orders during active pause; never cast or spend ammo here.
        if(target && !ranged(n) && !spell(n) && can_melee(n)) {
            ARMIA[ARM][n][TCELX]=target; ARMIA[ARM][n][TCELY]=WRG; ARMIA[ARM][n][TTRYB]=2;
        } else ARMIA[ARM][n][TTRYB]=0;
    }
}
void GroupAttackStep(int n) {
    TacticalScreen screen;
    if(!GroupAttackEnabled(n) || ARMIA[ARM][n][TE]<=0) return;
    int mode=ARMIA[ARM][n][TTRYB];
    // Never interrupt a projectile, spell already being cast, or knockback.
    if(mode>=3 && mode<=8) return;
    int target=nearest(n);
    // Enemy auto-defence can overwrite a ranged order with a melee order.
    // Never carry that order into the simulation while the ranged AI waits.
    if(mode==2 && (ranged(n) || spell(n) || has_bow_or_catapult(n))) {
        ARMIA[ARM][n][TTRYB]=0; mode=0;
    }
    // A bow or catapult without its ammunition cannot shoot. Hold position
    // when an enemy is visible instead of charging into melee unarmed.
    if(target && has_bow_or_catapult(n) && !ranged(n) && !spell(n)) {
        ARMIA[ARM][n][TTRYB]=0;
        orders[n].target=target;
        return;
    }
    if(mode==1) {
        const int x=ARMIA[ARM][n][TX],y=ARMIA[ARM][n][TY];
        if(x==orders[n].lastx && y==orders[n].lasty) ++orders[n].stalled;
        else {orders[n].lastx=x;orders[n].lasty=y;orders[n].stalled=0;}
        // Replan if an obstacle makes the chosen search/sidestep unreachable.
        if(orders[n].stalled>60) {orders[n].stalled=0; ARMIA[ARM][n][TTRYB]=0;}
        else if(!target || ((ranged(n) || spell(n)) && !clear_line_at(n,target,x,y)) || (!ranged(n) && !spell(n) && !can_melee(n))) return;
    }
    if(!target) {
        // Explore the terrain instead of freezing when no opponent is visible.
        int k=(orders[n].patrol++ + n)%8;
        static const int xs[]={80,320,560,560,560,320,80,80},ys[]={70,70,70,255,450,450,450,255};
        move_to(n,xs[k],ys[k]); orders[n].target=0; return;
    }
    orders[n].target=target;
    const int magic=spell(n);
    if(ranged(n) || magic) {
        if(!GroupAttackLineClear(n,target)) {
            AutoOrder &order=orders[n];
            const int tx=ARMIA[WRG][target][TX],ty=ARMIA[WRG][target][TY];
            if(order.retry>0 && order.retryTarget==target &&
               std::abs(tx-order.retryX)<16 && std::abs(ty-order.retryY)<16) {
                --order.retry;
                return;
            }
            if(!reposition(n,target)) {
                order.retry=60; order.retryTarget=target; order.retryX=tx; order.retryY=ty;
                if(can_melee(n)) {
                    ARMIA[ARM][n][TCELX]=target; ARMIA[ARM][n][TCELY]=WRG; ARMIA[ARM][n][TTRYB]=2;
                }
            }
            return;
        }
        orders[n].retry=0;
        const int saved=NUMER,tx=ARMIA[WRG][target][TX],ty=ARMIA[WRG][target][TY]-8;
        NUMER=n; issuingAutomaticOrder=true; SetImmediateTarget(tx,ty);
        if(ranged(n)) STRZAL();
        else {
            const int left=ARMIA[ARM][n][TLEWA],right=ARMIA[ARM][n][TPRAWA];
            CZARY(left==magic ? 12 : 0,right==magic ? 12 : 0);
        }
        ClearImmediateTarget(); issuingAutomaticOrder=false; NUMER=saved;
        return;
    }
    if(can_melee(n)) {
        ARMIA[ARM][n][TCELX]=target; ARMIA[ARM][n][TCELY]=WRG; ARMIA[ARM][n][TTRYB]=2;
    } else {
        // A unit with no ammunition or mana continues to advance and search.
        const int tx=ARMIA[WRG][target][TX],ty=ARMIA[WRG][target][TY];
        const int x=ARMIA[ARM][n][TX],y=ARMIA[ARM][n][TY];
        move_to(n,tx+(x<tx ? -32 : 32),ty+(y<ty ? -22 : 22));
    }
}
