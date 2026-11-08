#ifndef LEGION_PATHFINDING_H
#define LEGION_PATHFINDING_H

// Called on the tactical screen. Does not modify the unit's final destination.
void ResetPaths();
bool PathYield(int army,int unit,int zone,int x,int y,int goal_x,int goal_y,bool moving_goal=false);
bool PathWaypoint(int army, int unit, int zone, int x, int y, int goal_x, int goal_y,
                  bool replan, int &waypoint_x, int &waypoint_y,
                  int range_x=0,int range_y=0);

#endif
