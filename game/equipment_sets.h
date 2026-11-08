#ifndef LEGION_EQUIPMENT_SETS_H
#define LEGION_EQUIPMENT_SETS_H
#include <cstddef>

int EquipmentMode(int army,int person);
int SpareHandItem(int army,int person,int hand);
void StoreSpareHandItem(int army,int person,int hand,int item);
void StoreEquipmentMode(int army,int person,int mode);
void ResetEquipmentSet(int army,int person);
void ResetAllEquipmentSets();
std::size_t EquipmentSetsSaveSize();
void WriteEquipmentSets(unsigned char *out);
void ReadEquipmentSets(const unsigned char *data,std::size_t length);
bool SwitchEquipmentSet(int army,int person,int mode);
#endif
