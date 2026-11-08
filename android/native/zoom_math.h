#ifndef LEGION_ANDROID_ZOOM_MATH_H
#define LEGION_ANDROID_ZOOM_MATH_H
#include <algorithm>
struct AndroidZoomTransform {
    float scale, x, y;
    AndroidZoomTransform(float s=1.f,float dx=0.f,float dy=0.f) : scale(std::max(1.f,std::min(4.f,s))), x(dx), y(dy) {}
    static void baseViewport(int screenWidth,int screenHeight,int gameWidth,int gameHeight,
                             float &left,float &top,float &width,float &height) {
        float factor=std::min(float(screenWidth)/gameWidth,float(screenHeight)/gameHeight);
        width=gameWidth*factor; height=gameHeight*factor;
        left=(screenWidth-width)*.5f; top=(screenHeight-height)*.5f;
    }
    bool toGame(float &nx,float &ny,int screenWidth,int screenHeight,int gameWidth,int gameHeight) const {
        float left,top,width,height; baseViewport(screenWidth,screenHeight,gameWidth,gameHeight,left,top,width,height);
        float ux=((nx-.5f-x)/scale+.5f)*screenWidth;
        float uy=((ny-.5f-y)/scale+.5f)*screenHeight;
        nx=(ux-left)/width; ny=(uy-top)/height;
        return nx>=0.f && nx<=1.f && ny>=0.f && ny<=1.f;
    }
    void viewport(int screenWidth,int screenHeight,int gameWidth,int gameHeight,
                  int &vx,int &vy,int &vw,int &vh) const {
        float left,top,width,height; baseViewport(screenWidth,screenHeight,gameWidth,gameHeight,left,top,width,height);
        vx=int(left*scale+(1.f-scale)*screenWidth*.5f+x*screenWidth);
        vy=int(top*scale+(1.f-scale)*screenHeight*.5f-y*screenHeight);
        vw=int(width*scale); vh=int(height*scale);
    }
    void scissor(int screenWidth,int screenHeight,int gameWidth,int gameHeight,
                 int &sx,int &sy,int &sw,int &sh) const {
        int vx,vy,vw,vh; viewport(screenWidth,screenHeight,gameWidth,gameHeight,vx,vy,vw,vh);
        float dx=float(vw)/gameWidth,dy=float(vh)/gameHeight;
        sx=vx+int(sx*dx); sy=vy+int(sy*dy); sw=int(sw*dx); sh=int(sh*dy);
    }
};
#endif
