#include "equipment_sets.h"
#include <cstring>
namespace {
struct EquipmentSet { unsigned char mode,left,right; };
EquipmentSet sets[41][11]={};
const unsigned char magic[8]={'L','G','N','S','E','T',1,0};
bool valid(int a,int n) { return a>=0 && a<=40 && n>=1 && n<=10; }
}
int EquipmentMode(int a,int n) { return valid(a,n) ? sets[a][n].mode : 0; }
int SpareHandItem(int a,int n,int h) { return valid(a,n) ? (h==0 ? sets[a][n].left : sets[a][n].right) : 0; }
void StoreSpareHandItem(int a,int n,int h,int item) { if(valid(a,n) && item>=0 && item<=120) (h==0 ? sets[a][n].left : sets[a][n].right)=item; }
void StoreEquipmentMode(int a,int n,int mode) { if(valid(a,n)) sets[a][n].mode=mode==1; }
void ResetEquipmentSet(int a,int n) { if(valid(a,n)) sets[a][n]=EquipmentSet{}; }
void ResetAllEquipmentSets() { std::memset(sets,0,sizeof(sets)); }
std::size_t EquipmentSetsSaveSize() { return sizeof(magic)+41*10*3; }
void WriteEquipmentSets(unsigned char *out) {
    std::memcpy(out,magic,sizeof(magic)); out+=sizeof(magic);
    for(int a=0;a<=40;++a) for(int n=1;n<=10;++n) { *out++=sets[a][n].mode; *out++=sets[a][n].left; *out++=sets[a][n].right; }
}
void ReadEquipmentSets(const unsigned char *data,std::size_t length) {
    ResetAllEquipmentSets();
    if(length<EquipmentSetsSaveSize() || std::memcmp(data,magic,sizeof(magic))!=0) return;
    data+=sizeof(magic);
    for(int a=0;a<=40;++a) for(int n=1;n<=10;++n) {
        const int mode=*data++,left=*data++,right=*data++;
        if(mode<=1 && left<=120 && right<=120) sets[a][n]=EquipmentSet{static_cast<unsigned char>(mode),static_cast<unsigned char>(left),static_cast<unsigned char>(right)};
    }
}
