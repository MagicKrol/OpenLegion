#include "../native/map_touch_choice.h"
#include <cassert>
int main() {
    assert(AndroidMapWithinTouchRadius(10,10,18));
    assert(!AndroidMapWithinTouchRadius(15,15,18));
    assert(!AndroidMapAmbiguous(0,12*12,18));
    assert(AndroidMapAmbiguous(0,0,18));
    assert(AndroidMapAmbiguous(8,9*9,18));
    assert(!AndroidMapAmbiguous(0,5*5,8));
}
