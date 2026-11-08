#ifndef LEGION_GROUND_ITEMS_H
#define LEGION_GROUND_ITEMS_H

// Visual positions are confined to the same sector used by the inventory UI.
void ResetGroundItemPositions();
void SetGroundDropPosition(int sector,int slot,int item,int x,int y);
bool GroundItemPosition(int sector,int slot,int &x,int &y);
struct GroundSlot { int sector, slot; };
void NearbyGroundSlots(int x,int y,GroundSlot slots[4]);
void ReplaceGroundItem(GroundSlot slot,int item,int x,int y);
void DrawGroundItems();

#endif
