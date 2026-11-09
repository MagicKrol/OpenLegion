#ifndef LEGION_GROUND_ITEMS_H
#define LEGION_GROUND_ITEMS_H

#include <cstddef>

// Ground positions are independent of the storage sector.
void ResetGroundItemPositions();
void ClearGroundItems();
void SetGroundDropPosition(int sector,int slot,int item,int x,int y);
bool GroundItemPosition(int sector,int slot,int &x,int &y);
bool GroundItemDiscovered(int sector,int slot);
bool GroundItemVisible(int sector,int slot);
int GroundSectorCount();
bool DropGroundItem(int item,int x,int y);
struct GroundSlot { int sector, slot; };
int GroundItem(GroundSlot slot);
void NearbyGroundSlots(int x,int y,GroundSlot slots[4],int page=0,int *total=0);
void ReplaceGroundItem(GroundSlot slot,int item,int x,int y);
void DrawGroundItems();

// A quest encounter retains its ground across retreats, independently per
// save and per phase of the final quest.
void BeginQuestGround(int quest);
void EndQuestGround();
void ClearQuestGround(int quest);
void ResetQuestGrounds();
void RestoreQuestGroundForAction(int scenery);
void CaptureQuestGroundAfterAction(int scenery,int outcome);
std::size_t QuestGroundSaveSize();
void WriteQuestGround(unsigned char *out);
void ReadQuestGround(const unsigned char *data,std::size_t length);

#endif
