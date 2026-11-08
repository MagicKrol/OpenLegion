#ifndef LEGION_STRENGTH_OVERFLOW_H
#define LEGION_STRENGTH_OVERFLOW_H

#include <cstddef>

// Strength beyond the visible 100-point limit is retained for later unequips.
long StrengthBeforeLimit(int army,int person);
void ApplyStrengthChange(int army,int person,long delta);
void ResetStrengthOverflow(int army,int person);
void ResetAllStrengthOverflow();
size_t StrengthOverflowSaveSize();
void WriteStrengthOverflow(unsigned char *out);
void ReadStrengthOverflow(const unsigned char *data,size_t length);

#endif
