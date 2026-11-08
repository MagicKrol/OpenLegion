#include "pathfinding.h"
#include "Amos.h"
#include "legion.h"
#include <algorithm>
#include <cstdlib>
#include <queue>
#include <vector>

namespace {
// The coordinates are world pixels. Eight-pixel cells leave enough resolution
// for the original collision checks while keeping searches bounded.
const int STEP=8, WIDTH=80, HEIGHT=64, COUNT=WIDTH*HEIGHT;
struct Point { int x,y; };
struct Route {
    int gx,gy,cooldown;
    bool yielding;
    Point yield_point;
    std::vector<Point> points;
    size_t next;
    Route():gx(-1),gy(-1),cooldown(0),yielding(false),yield_point{0,0},next(0) {}
};
Route routes[50][11];
struct QueueEntry {
    int f,g,node;
    bool operator<(const QueueEntry &other) const {
        if(f!=other.f) return f>other.f;
        if(g!=other.g) return g<other.g;
        return node>other.node;
    }
};
int clamp(int value,int low,int high) { return std::max(low,std::min(value,high)); }
int heuristic(int a,int b) {
    return 10*(std::abs(a%WIDTH-b%WIDTH)+std::abs(a/WIDTH-b/WIDTH));
}
bool clear_cell(int x,int y,int own,bool player) {
    if(x<16 || x>623 || y<22 || y>508) return false;
    // The game's movement uses these asymmetric probes. Include the centre as
    // well, and ignore the moving unit's own collision zone.
    const int dx[]={0,-17,17,0,0};
    const int dy[]={0,0,0,-21,2};
    for(int k=0;k<5;++k)
        if(ZoneBlockedExcept(x+dx[k],y+dy[k],own,player)) return false;
    return true;
}
bool can_move(int x,int y,int dx,int dy,int own,bool player) {
    // Match A_RUCH's directional probes, rather than requiring clearance in
    // all four directions at once. Check the entire edge to avoid tunnelling.
    bool inside=ZoneBlockedExcept(x,y,own,player);
    for(int i=0;i<STEP;++i) {
        int px=x+dx*i,py=y+dy*i;
        int check_x=px+(dx ? dx*17 : 0);
        int check_y=py+(dy<0 ? -21 : dy>0 ? 2 : 0);
        if(ZoneBlockedExcept(check_x,check_y,own,player)) return false;
        bool next_inside=ZoneBlockedExcept(px+dx,py+dy,own,player);
        if(!inside && next_inside) return false;
        if(!next_inside) inside=false;
    }
    return true;
}
bool find_path(Route &r,int x,int y,int gx,int gy,int own,bool player) {
    // Anchor the grid at the actual position. Rounding the start into a tree
    // can otherwise make the first waypoint impossible to reach.
    if(x<0 || x>=WIDTH*STEP || y<0 || y>=HEIGHT*STEP) return false;
    const int ox=x%STEP,oy=y%STEP;
    const int sx=x/STEP,sy=y/STEP;
    const int tx=clamp((gx-ox+STEP/2)/STEP,0,WIDTH-1);
    const int ty=clamp((gy-oy+STEP/2)/STEP,0,HEIGHT-1);
    const int start=sy*WIDTH+sx, goal_hint=ty*WIDTH+tx;
    std::vector<int> cost(COUNT,0x3fffffff), parent(COUNT,-1);
    std::priority_queue<QueueEntry> open;
    cost[start]=0; open.push(QueueEntry{heuristic(start,goal_hint),0,start});
    const int dx[]={1,0,-1,0};
    const int dy[]={0,1,0,-1};
    int goal=-1;
    while(!open.empty()) {
        QueueEntry q=open.top(); open.pop();
        if(q.g!=cost[q.node]) continue;
        int cx=q.node%WIDTH,cy=q.node/WIDTH;
        int wx=cx*STEP+ox,wy=cy*STEP+oy;
        if(std::abs(wx-gx)<=4 && std::abs(wy-gy)<=4) {goal=q.node;break;}
        for(int d=0;d<4;++d) {
            int nx=cx+dx[d],ny=cy+dy[d];
            int nwx=nx*STEP+ox,nwy=ny*STEP+oy;
            if(nx<0 || nx>=WIDTH || ny<0 || ny>=HEIGHT ||
               nwx<16 || nwx>623 || nwy<22 || nwy>508) continue;
            int n=ny*WIDTH+nx,g=q.g+10;
            if(g>=cost[n] || !can_move(wx,wy,dx[d],dy[d],own,player)) continue;
            cost[n]=g;parent[n]=q.node;
            open.push(QueueEntry{g+heuristic(n,goal_hint),g,n});
        }
    }
    if(goal<0) return false;
    std::vector<Point> rev;
    for(int n=goal;n!=start;n=parent[n]) rev.push_back(Point{(n%WIDTH)*STEP+ox,(n/WIDTH)*STEP+oy});
    r.points.assign(rev.rbegin(),rev.rend());
    r.points.push_back(Point{gx,gy});
    r.next=0;
    return true;
}
}
void ResetPaths() {
    for(int a=0;a<50;++a) for(int i=0;i<11;++i) routes[a][i]=Route();
}
bool PathYield(int army,int unit,int zone,int x,int y,int gx,int gy) {
    if(army<0 || army>=50 || unit<1 || unit>10) return false;
    Route &r=routes[army][unit];
    if(r.gx!=gx || r.gy!=gy) {r=Route();r.gx=gx;r.gy=gy;}
    if(r.yielding) return false; // A blocked sidestep needs a fresh route.
    const bool horizontal=std::abs(gx-x)>=std::abs(gy-y);
    const int distances[]={horizontal ? 32 : 40,horizontal ? 24 : 36,horizontal ? 40 : 48};
    for(int d=0;d<3;++d) {
        for(int side=0;side<2;++side) {
            int delta=(side==0 ? 1 : -1)*distances[d];
            int nx=x+(horizontal ? 0 : delta), ny=y+(horizontal ? delta : 0);
            if(clear_cell(nx,ny,zone,army==ARM)) {
                r.points.clear();r.next=0;r.yielding=true;r.yield_point=Point{nx,ny};
                return true;
            }
        }
    }
    return false;
}
bool PathWaypoint(int army,int unit,int zone,int x,int y,int gx,int gy,bool replan,int &wx,int &wy) {
    if(army<0 || army>=50 || unit<1 || unit>10) return false;
    Route &r=routes[army][unit];
    if(r.gx!=gx || r.gy!=gy) { r=Route();r.gx=gx;r.gy=gy; }
    if(replan) r.yielding=false;
    if(r.yielding) {
        if(x==r.yield_point.x && y==r.yield_point.y) r.yielding=false;
        else {wx=r.yield_point.x;wy=r.yield_point.y;return true;}
    }
    if(!replan && r.next<r.points.size() &&
       (std::abs(r.points[r.next].x-x)>24 || std::abs(r.points[r.next].y-y)>24)) {
        r.points.clear();r.next=0;
    }
    if(replan) {
        r.points.clear();r.next=0;
        if(r.cooldown>0) { --r.cooldown;return false; }
        if(!find_path(r,x,y,gx,gy,zone,army==ARM)) { r.cooldown=15;return false; }
        r.cooldown=0;
    }
    while(r.next<r.points.size() && r.points[r.next].x==x && r.points[r.next].y==y) ++r.next;
    if(r.next>=r.points.size()) { r.points.clear();return false; }
    wx=r.points[r.next].x;wy=r.points[r.next].y;
    return true;
}
