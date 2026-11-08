#include "android_bridge.h"
#include "zoom_math.h"
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <dlfcn.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <Engine2d/Core.h>
#include "../../game/lgn_intro.h"
#include "../../game/legion.h"
#include "../../game/Amos.h"

namespace {
std::mutex transform_mutex;
AndroidZoomTransform current_transform;
std::atomic<int> viewport_width(1), viewport_height(1), drawable_width(1), drawable_height(1);
std::atomic<bool> world_map_active(false);
std::atomic<bool> refresh_requested(false);
std::atomic<bool> tactical_scene_active(false);
std::atomic<bool> target_selection_active(false), target_preview(false);
std::atomic<int> move_decision(0);
std::mutex tactical_touch_mutex;
bool tactical_touch_pending=false;
int tactical_touch_x=0,tactical_touch_y=0;
std::mutex map_drag_mutex;
float map_drag_x=0.f,map_drag_y=0.f;
void actualDrawableSize(int &width,int &height) {
    typedef void *(*GetWindow)();
    typedef void (*GetSize)(void *,int *,int *);
    static void *sdl=dlopen("libSDL2.so",RTLD_NOW|RTLD_LOCAL);
    static GetWindow getWindow=sdl ? reinterpret_cast<GetWindow>(dlsym(sdl,"SDL_GL_GetCurrentWindow")) : 0;
    static GetSize getSize=sdl ? reinterpret_cast<GetSize>(dlsym(sdl,"SDL_GL_GetDrawableSize")) : 0;
    if(getWindow && getSize) {
        void *window=getWindow(); int w=0,h=0;
        if(window) getSize(window,&w,&h);
        if(w>0 && h>0) { width=w; height=h; }
    }
}
JavaVM *java_vm=0;
jclass activity_class=0;
std::mutex input_mutex;
std::string input_result;
bool input_done=false, input_accepted=false;
std::mutex choice_mutex;
bool choice_done=false;
int choice_result=-1;
AndroidZoomTransform transform() {
    std::lock_guard<std::mutex> lock(transform_mutex);
    return current_transform;
}
std::atomic<int> last_pointer_x(0), last_pointer_y(0);
void pointer(int phase, float nx, float ny, int button) {
    const bool inside=transform().toGame(nx,ny,drawable_width.load(),drawable_height.load(),
                                         viewport_width.load(),viewport_height.load());
    int x, y;
    if(!inside && !ib_intro_playing.load()) {
        // Always release a pressed button, even when the gesture ends in a
        // letterbox margin or is cancelled outside the game viewport.
        if(phase!=1) return;
        x=last_pointer_x.load(); y=last_pointer_y.load();
    } else {
        if(!inside) nx=ny=.5f;
        x=std::max(0,std::min(int(Core::il_screen_width)-1,int(nx*Core::il_screen_width)));
        y=std::max(0,std::min(int(Core::il_screen_height)-1,int(ny*Core::il_screen_height)));
        last_pointer_x.store(x); last_pointer_y.store(y);
    }
    SDL_Event event; std::memset(&event,0,sizeof(event));
    event.type=SDL_MOUSEMOTION; event.motion.x=x; event.motion.y=y;
    SDL_PushEvent(&event);
    if (phase==2 && target_selection_active.load()) target_preview.store(true);
    if(phase==0 || phase==1) {
        std::memset(&event,0,sizeof(event));
        event.type=phase==0 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        event.button.button=button==2 ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
        event.button.state=phase==0 ? SDL_PRESSED : SDL_RELEASED;
        event.button.x=x; event.button.y=y;
        SDL_PushEvent(&event);
    }
}
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeSetWorkingDirectory(JNIEnv *env, jclass, jstring path) {
    // A later ordinary launch in the same process must never reuse the
    // debug-only scene requested by an earlier activity.
    unsetenv("LEGION_ANDROID_TEST_WORLD");
    unsetenv("LEGION_ANDROID_TEST_CITY");
    unsetenv("LEGION_SKIP_INTRO");
    env->GetJavaVM(&java_vm);
    if(!activity_class) {
        jclass local=env->FindClass("org/legion/game/LegionActivity");
        if(local) { activity_class=static_cast<jclass>(env->NewGlobalRef(local)); env->DeleteLocalRef(local); }
    }
    const char *value=env->GetStringUTFChars(path,0);
    if(value) { chdir(value); env->ReleaseStringUTFChars(path,value); }
    setenv("SDL12COMPAT_OPENGL_SCALING","0",1);
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeEnableTestWorld(JNIEnv *,jclass) {
    setenv("LEGION_ANDROID_TEST_WORLD","1",1);
    setenv("LEGION_SKIP_INTRO","1",1);
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeEnableTestCity(JNIEnv *,jclass,jboolean dialogue) {
    setenv("LEGION_ANDROID_TEST_WORLD","1",1);
    setenv("LEGION_ANDROID_TEST_CITY",dialogue ? "dialogue" : "city",1);
    setenv("LEGION_SKIP_INTRO","1",1);
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeEnableTestStats(JNIEnv *,jclass) {
    setenv("LEGION_ANDROID_TEST_WORLD","1",1);
    setenv("LEGION_ANDROID_TEST_CITY","stats",1);
    setenv("LEGION_SKIP_INTRO","1",1);
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeEnableTestShop(JNIEnv *,jclass,jboolean granary) {
    setenv("LEGION_ANDROID_TEST_WORLD","1",1);
    setenv("LEGION_ANDROID_TEST_CITY",granary ? "granary" : "shop",1);
    setenv("LEGION_SKIP_INTRO","1",1);
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeTacticalTouchBegin(JNIEnv *,jclass,jfloat x,jfloat y) {
    float nx=x,ny=y;
    const bool inside=transform().toGame(nx,ny,drawable_width.load(),drawable_height.load(),viewport_width.load(),viewport_height.load());
    std::lock_guard<std::mutex> lock(tactical_touch_mutex);
    tactical_touch_x=inside ? std::max(128,std::min(447,int(nx*320)+130)) : 0;
    tactical_touch_y=inside ? std::max(40,std::min(298,int(ny*260)+40)) : 0;
    tactical_touch_pending=true;
}
bool AndroidTakeTacticalTouchStart(int &x,int &y) {
    std::lock_guard<std::mutex> lock(tactical_touch_mutex);
    if(!tactical_touch_pending) return false;
    tactical_touch_pending=false; x=tactical_touch_x; y=tactical_touch_y; return true;
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeSetZoom(JNIEnv *, jclass, jfloat scale, jfloat x, jfloat y) {
    std::lock_guard<std::mutex> lock(transform_mutex);
    current_transform=AndroidZoomTransform(scale,x,y);
}
extern "C" JNIEXPORT jboolean JNICALL
Java_org_legion_game_LegionActivity_nativeTacticalSceneActive(JNIEnv *,jclass) {
    return tactical_scene_active.load() ? JNI_TRUE : JNI_FALSE;
}
void AndroidSetTacticalScene(bool active) { tactical_scene_active.store(active); }
extern "C" JNIEXPORT jboolean JNICALL
Java_org_legion_game_LegionActivity_nativeWorldMapActive(JNIEnv *,jclass) {
    return world_map_active.load() ? JNI_TRUE : JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeRequestRefresh(JNIEnv *,jclass) {
    refresh_requested.store(true);
}
bool AndroidTakeRefreshRequest() { return refresh_requested.exchange(false); }
void AndroidResetZoomForScene() {
    {
        std::lock_guard<std::mutex> lock(transform_mutex);
        current_transform=AndroidZoomTransform();
    }
    AndroidApplyViewport(viewport_width.load(),viewport_height.load());
    if (!java_vm || !activity_class) return;
    JNIEnv *env=0;
    if (java_vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK) return;
    jmethodID method=env->GetStaticMethodID(activity_class,"resetZoomForScene","()V");
    if (!method) { env->ExceptionClear(); return; }
    env->CallStaticVoidMethod(activity_class,method);
    if (env->ExceptionCheck()) env->ExceptionClear();
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeWorldMapDrag(JNIEnv *,jclass,jfloat dx,jfloat dy) {
    if(!world_map_active.load()) return;
    std::lock_guard<std::mutex> lock(map_drag_mutex);
    map_drag_x+=dx; map_drag_y+=dy;
}
extern "C" JNIEXPORT jboolean JNICALL
Java_org_legion_game_LegionActivity_nativeTargetSelectionActive(JNIEnv *,jclass) {
    return target_selection_active.load() ? JNI_TRUE : JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeSetMoveDecision(JNIEnv *,jclass,jint decision) { move_decision.store(decision); }
bool AndroidTakeTargetPreview() { return target_preview.exchange(false); }
int AndroidTakeMoveDecision() { return move_decision.exchange(0); }
void AndroidSetTargetSelection(bool active,bool polish) {
    target_preview.store(false); move_decision.store(0);
    target_selection_active.store(active);
    if (!java_vm || !activity_class) return;
    JNIEnv *env=0;
    if (java_vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK) return;
    jmethodID method=env->GetStaticMethodID(activity_class,"showMoveControls","(ZZ)V");
    if (!method) { env->ExceptionClear(); return; }
    env->CallStaticVoidMethod(activity_class,method,active ? JNI_TRUE : JNI_FALSE,polish ? JNI_TRUE : JNI_FALSE);
    if (env->ExceptionCheck()) env->ExceptionClear();
}
void AndroidSetWorldMapActive(bool active) {
    world_map_active.store(active);
    if(!active) { std::lock_guard<std::mutex> lock(map_drag_mutex); map_drag_x=map_drag_y=0.f; }
}
void AndroidApplyWorldMapDrag() {
    if(!world_map_active.load()) return;
    int screenWidth=drawable_width.load(),screenHeight=drawable_height.load();
    int vx,vy,vw,vh;
    transform().viewport(screenWidth,screenHeight,viewport_width.load(),viewport_height.load(),vx,vy,vw,vh);
    if(vw<=0 || vh<=0) return;
    std::lock_guard<std::mutex> lock(map_drag_mutex);
    float worldX=map_drag_x*screenWidth*320.f/vw;
    float worldY=map_drag_y*screenHeight*260.f/vh;
    int moveX=int(worldX),moveY=int(worldY);
    if(!moveX && !moveY) return;
    map_drag_x-=moveX*float(vw)/(screenWidth*320.f);
    map_drag_y-=moveY*float(vh)/(screenHeight*260.f);
    SX=std::max(0,std::min(int(MSX),int(SX)-moveX));
    SY=std::max(0,std::min(int(MSY),int(SY)-moveY));
    ScreenOffset(0,SX,SY);
}
extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativePointer(JNIEnv *, jclass, jint phase, jfloat x, jfloat y, jint button) {
    pointer(phase,x,y,button);
}
void AndroidGetViewport(int &x,int &y,int &w,int &h,int &screenWidth,int &screenHeight) {
    screenWidth=drawable_width.load(); screenHeight=drawable_height.load();
    transform().viewport(screenWidth,screenHeight,viewport_width.load(),viewport_height.load(),x,y,w,h);
}
float AndroidMapTouchRadius() { return std::max(8.f,18.f/transform().scale); }
void AndroidApplyViewport(int width,int height) {
    viewport_width.store(width); viewport_height.store(height);
    int screenWidth=width,screenHeight=height;
    actualDrawableSize(screenWidth,screenHeight);
    drawable_width.store(screenWidth); drawable_height.store(screenHeight);
    int x,y,w,h; transform().viewport(screenWidth,screenHeight,width,height,x,y,w,h);
    glViewport(x,y,w,h);
}
void AndroidApplyScissor(int x,int y,int width,int height) {
    transform().scissor(drawable_width.load(),drawable_height.load(),
                        viewport_width.load(),viewport_height.load(),x,y,width,height);
    glScissor(x,y,width,height);
}

extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeSetInputResult(JNIEnv *env, jclass, jstring value, jboolean accepted) {
    const char *text=env->GetStringUTFChars(value,0);
    std::lock_guard<std::mutex> lock(input_mutex);
    input_result=text ? text : ""; input_accepted=accepted; input_done=true;
    if(text) env->ReleaseStringUTFChars(value,text);
}
bool AndroidBeginTextInput(const std::string &text,int max_length) {
    if(!java_vm || !activity_class) return false;
    JNIEnv *env=0;
    if(java_vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK) return false;
    jmethodID method=env->GetStaticMethodID(activity_class,"requestText","(Ljava/lang/String;I)V");
    if(!method) { env->ExceptionClear(); return false; }
    { std::lock_guard<std::mutex> lock(input_mutex); input_done=false; }
    jstring initial=env->NewStringUTF(text.c_str());
    env->CallStaticVoidMethod(activity_class,method,initial,max_length);
    env->DeleteLocalRef(initial);
    if(env->ExceptionCheck()) { env->ExceptionClear(); return false; }
    return true;
}
bool AndroidTakeTextInput(std::string &text,bool &accepted) {
    std::lock_guard<std::mutex> lock(input_mutex);
    if(!input_done) return false;
    text=input_result; accepted=input_accepted; input_done=false; return true;
}

extern "C" JNIEXPORT void JNICALL
Java_org_legion_game_LegionActivity_nativeSetMapChoice(JNIEnv *,jclass,jint choice) {
    std::lock_guard<std::mutex> lock(choice_mutex);
    choice_result=choice; choice_done=true;
}
static bool BeginChoice(const std::string &title,const std::vector<std::string> &labels,const std::string &cancel,bool compact) {
    if(!java_vm || !activity_class || labels.empty()) return false;
    JNIEnv *env=0;
    if(java_vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK) return false;
    jmethodID method=env->GetStaticMethodID(activity_class,"requestChoice","(Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;Z)V");
    jclass stringClass=env->FindClass("java/lang/String");
    if(!method || !stringClass) { env->ExceptionClear(); return false; }
    jobjectArray array=env->NewObjectArray(labels.size(),stringClass,0);
    env->DeleteLocalRef(stringClass);
    if(!array) return false;
    for(size_t i=0;i<labels.size();++i) {
        jstring text=env->NewStringUTF(labels[i].c_str());
        env->SetObjectArrayElement(array,i,text); env->DeleteLocalRef(text);
    }
    { std::lock_guard<std::mutex> lock(choice_mutex); choice_done=false; choice_result=-1; }
    jstring dialogTitle=env->NewStringUTF(title.c_str());
    jstring cancelLabel=env->NewStringUTF(cancel.c_str());
    env->CallStaticVoidMethod(activity_class,method,dialogTitle,array,cancelLabel,compact ? JNI_TRUE : JNI_FALSE);
    env->DeleteLocalRef(dialogTitle);
    env->DeleteLocalRef(cancelLabel);
    env->DeleteLocalRef(array);
    if(env->ExceptionCheck()) { env->ExceptionClear(); return false; }
    return true;
}
bool AndroidBeginChoice(const std::string &title,const std::vector<std::string> &labels,const std::string &cancel) {
    return BeginChoice(title,labels,cancel,true);
}
bool AndroidBeginMapChoice(const std::vector<std::string> &labels) {
    return BeginChoice("Wybierz obiekt",labels,"Anuluj",false);
}
bool AndroidTakeMapChoice(int &selection) {
    std::lock_guard<std::mutex> lock(choice_mutex);
    if(!choice_done) return false;
    selection=choice_result; choice_done=false; return true;
}
