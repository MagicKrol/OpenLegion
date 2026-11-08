#ifndef LEGION_ANDROID_SNAPSHOT_CACHE_H
#define LEGION_ANDROID_SNAPSHOT_CACHE_H
#include <cstddef>
#include <cmath>
#include <vector>

// Pixels use OpenGL's bottom-to-top row order. Update only the logical
// pixels represented by the visible part of the current framebuffer.
inline void AndroidUpdateSnapshot(std::vector<unsigned char> &snapshot,
        int w,int h,double rx,double ry,double rw,double rh,
        int x0,int y0,int visibleW,int visibleH,
        const std::vector<unsigned char> &visible) {
    if(w<=0 || h<=0 || rw<=0 || rh<=0 || visibleW<=0 || visibleH<=0) return;
    for(int py=0;py<h;++py) {
        const int sy=static_cast<int>(std::floor(ry+(py+0.5)*rh/h));
        if(sy<y0 || sy>=y0+visibleH) continue;
        for(int px=0;px<w;++px) {
            const int sx=static_cast<int>(std::floor(rx+(px+0.5)*rw/w));
            if(sx<x0 || sx>=x0+visibleW) continue;
            const size_t source=(size_t(sy-y0)*visibleW+sx-x0)*4;
            const size_t dest=(size_t(py)*w+px)*4;
            snapshot[dest]=visible[source];
            snapshot[dest+1]=visible[source+1];
            snapshot[dest+2]=visible[source+2];
            snapshot[dest+3]=255;
        }
    }
}
#endif
