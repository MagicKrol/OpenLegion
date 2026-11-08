#include "../native/zoom_math.h"
#include <cassert>
#include <cmath>
static bool near(float a,float b) { return std::fabs(a-b)<.0001f; }
int main() {
    const int sw=1920,sh=1080,gw=1280,gh=1040;
    AndroidZoomTransform identity; float x=.5f,y=.5f;
    assert(identity.toGame(x,y,sw,sh,gw,gh) && near(x,.5f) && near(y,.5f));
    x=.01f; y=.5f; assert(!identity.toGame(x,y,sw,sh,gw,gh));
    AndroidZoomTransform zoom(2.f,.25f,-.125f);
    x=.25f; y=.5f;
    assert(zoom.toGame(x,y,sw,sh,gw,gh) && near(x,.1389f) && near(y,.5625f));
    int vx,vy,vw,vh; zoom.viewport(sw,sh,gw,gh,vx,vy,vw,vh);
    assert(vx==110 && vy==-405 && vw==2658 && vh==2160);
    int sx=200,sy=150,swidth=100,sheight=80;
    zoom.scissor(sw,sh,gw,gh,sx,sy,swidth,sheight);
    assert(sx==525 && sy==-94 && swidth==207 && sheight==166);
    x=-1.f; y=.5f; assert(!zoom.toGame(x,y,sw,sh,gw,gh));
    assert(AndroidZoomTransform(7.f).scale==4.f && AndroidZoomTransform(.5f).scale==1.f);

    // At maximum pan the visible screen must reach the game image's corners,
    // even when the unzoomed image is letterboxed horizontally.
    const float scale=3.f;
    const float baseWidth=(float(gw)/gh)/(float(sw)/sh);
    const float panLimitX=(baseWidth*scale-1.f)/2.f;
    const float panLimitY=(scale-1.f)/2.f;
    AndroidZoomTransform topLeft(scale,panLimitX,panLimitY);
    x=.0001f; y=.0001f;
    assert(topLeft.toGame(x,y,sw,sh,gw,gh));
    assert(x<.001f && y<.001f);
    AndroidZoomTransform bottomRight(scale,-panLimitX,-panLimitY);
    x=.9999f; y=.9999f;
    assert(bottomRight.toGame(x,y,sw,sh,gw,gh));
    assert(x>.999f && y>.999f);
}
