#ifndef LEGION_ANDROID_BRIDGE_H
#define LEGION_ANDROID_BRIDGE_H
#include <string>
#include <vector>
bool AndroidBeginTextInput(const std::string &text,int max_length);
bool AndroidTakeTextInput(std::string &text,bool &accepted);
bool AndroidBeginMapChoice(const std::vector<std::string> &labels);
bool AndroidBeginChoice(const std::string &title,const std::vector<std::string> &labels,const std::string &cancel);
bool AndroidTakeMapChoice(int &selection);
void AndroidSetWorldMapActive(bool active);
void AndroidSetTacticalScene(bool active);
void AndroidSetTargetSelection(bool active,bool polish);
bool AndroidTakeTargetPreview();
bool AndroidTakeTacticalTouchStart(int &x,int &y);
int AndroidTakeMoveDecision();
void AndroidApplyWorldMapDrag();
bool AndroidTakeRefreshRequest();
void AndroidResetZoomForScene();
void AndroidGetViewport(int &x,int &y,int &w,int &h,int &screenWidth,int &screenHeight);
float AndroidMapTouchRadius();
void AndroidApplyViewport(int width,int height);
void AndroidApplyScissor(int x,int y,int width,int height);
#endif
