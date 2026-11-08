#ifndef LEGION_ACTION_MARKERS_H
#define LEGION_ACTION_MARKERS_H

enum ActionMarkerColour { MarkerIdle, MarkerMoving, MarkerAttacking, MarkerOther };
ActionMarkerColour TacticalActionMarkerColour(int mode,int spell=0);
void SetActionMarkersVisible(bool visible);
bool ActionMarkersVisible();
void DrawActionMarkers();

#endif
