#include "strength_overflow.h"
#include "Amos.h"
#include "legion.h"
#include <algorithm>
#include <cstring>

namespace {
unsigned short overflow[41][11]={};
const unsigned char magic[8]={'L','G','N','S','T','R',1,0};
bool valid(int army,int person) {return army>=0 && army<=40 && person>=1 && person<=10;}
}
long StrengthBeforeLimit(int army,int person) {
    if(!valid(army,person)) return 0;
    return ARMIA[army][person][TSI]+overflow[army][person];
}
void ApplyStrengthChange(int army,int person,long delta) {
    if(!valid(army,person)) return;
    const long total=StrengthBeforeLimit(army,person)+delta;
    ARMIA[army][person][TSI]=std::min(100L,total);
    overflow[army][person]=static_cast<unsigned short>(std::min(65535L,std::max(0L,total-100)));
}
void ResetStrengthOverflow(int army,int person) {
    if(valid(army,person)) overflow[army][person]=0;
}
void ResetAllStrengthOverflow() { std::memset(overflow,0,sizeof(overflow)); }
size_t StrengthOverflowSaveSize() { return sizeof(magic)+41*10*2; }
void WriteStrengthOverflow(unsigned char *out) {
    std::memcpy(out,magic,sizeof(magic)); out+=sizeof(magic);
    for(int a=0;a<=40;++a) for(int n=1;n<=10;++n) {
        const unsigned short value=overflow[a][n]; *out++=value>>8; *out++=value&255;
    }
}
void ReadStrengthOverflow(const unsigned char *data,size_t length) {
    ResetAllStrengthOverflow();
    if(length<StrengthOverflowSaveSize() || std::memcmp(data,magic,sizeof(magic))!=0) return;
    data+=sizeof(magic);
    for(int a=0;a<=40;++a) for(int n=1;n<=10;++n) {
        overflow[a][n]=static_cast<unsigned short>((unsigned(data[0])<<8)|data[1]); data+=2;
    }
}
