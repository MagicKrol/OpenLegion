#pragma once
#include <algorithm>
inline bool AndroidMapWithinTouchRadius(int dx,int dy,float radius) {
    return float(dx*dx+dy*dy)<=radius*radius;
}
inline bool AndroidMapAmbiguous(float nearestDistance,int candidateSquared,float radius) {
    const float limit=nearestDistance+std::max(2.5f,radius*0.28f);
    return float(candidateSquared)<=limit*limit;
}
