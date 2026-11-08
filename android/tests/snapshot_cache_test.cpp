#include "../native/snapshot_cache.h"
#include <cassert>
#include <cmath>

int main() {
    std::vector<unsigned char> snapshot(4*4*4,0);
    for(size_t i=0;i<snapshot.size();i+=4) { snapshot[i]=200; snapshot[i+3]=255; }
    std::vector<unsigned char> visible(2*4*4,0);
    for(size_t i=0;i<visible.size();i+=4) { visible[i+2]=180; visible[i+3]=255; }
    AndroidUpdateSnapshot(snapshot,4,4,-2,0,4,4,0,0,2,4,visible);
    for(int y=0;y<4;++y) for(int x=0;x<4;++x) {
        size_t p=size_t(y*4+x)*4;
        assert(snapshot[p] == (x<2 ? 200 : 0));
        assert(snapshot[p+2] == (x<2 ? 0 : 180));
        assert(snapshot[p+3]==255);
    }
    const std::vector<unsigned char> before=snapshot;
    AndroidUpdateSnapshot(snapshot,4,4,-4,0,4,4,0,0,0,4,visible);
    assert(snapshot==before);

    // A high-resolution panel retains four samples per logical pixel and
    // updates only the visible half while zoomed.
    std::vector<unsigned char> high(8*8*4,0);
    for(size_t i=0;i<high.size();i+=4) { high[i]=99; high[i+3]=255; }
    std::vector<unsigned char> update(4*8*4,0);
    for(size_t i=0;i<update.size();i+=4) { update[i+1]=127; update[i+3]=255; }
    AndroidUpdateSnapshot(high,8,8,-4,0,8,8,0,0,4,8,update);
    for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
        const size_t i=size_t(y*8+x)*4;
        assert(high[i] == (x<4 ? 99 : 0));
        assert(high[i+1] == (x<4 ? 0 : 127));
    }

    // A tactical panel sits at a fractional framebuffer position on the
    // phone. Repeated capture and redraw must not shift or smear pixels.
    const int tw=1280,th=96;
    const double rx=448.0,ry=2.0*996/520,rw=1225.0,rh=48.0*996/520;
    const int x0=int(std::floor(rx)),y0=int(std::floor(ry));
    const int vw=int(std::ceil(rx+rw))-x0,vh=int(std::ceil(ry+rh))-y0;
    std::vector<unsigned char> panel(size_t(vw)*vh*4,0),cache(size_t(tw)*th*4,0);
    for(int y=0;y<vh;++y) for(int x=0;x<vw;++x) {
        const size_t i=(size_t(y)*vw+x)*4;
        panel[i]=static_cast<unsigned char>((x*13+y*7)%251);
        panel[i+1]=static_cast<unsigned char>((x*5+y*19)%253);
        panel[i+3]=255;
    }
    const std::vector<unsigned char> original=panel;
    for(int cycle=0;cycle<20;++cycle) {
        AndroidUpdateSnapshot(cache,tw,th,rx,ry,rw,rh,x0,y0,vw,vh,panel);
        for(int y=0;y<vh;++y) for(int x=0;x<vw;++x) {
            const double dx=x+x0+0.5,dy=y+y0+0.5;
            if(dx<rx || dx>=rx+rw || dy<ry || dy>=ry+rh) continue;
            const int tx=int((dx-rx)*tw/rw),ty=int((dy-ry)*th/rh);
            const size_t to=(size_t(y)*vw+x)*4, from=(size_t(ty)*tw+tx)*4;
            for(int c=0;c<4;++c) panel[to+c]=cache[from+c];
            assert(panel[to]==original[to] && panel[to+1]==original[to+1]);
        }
    }
}
