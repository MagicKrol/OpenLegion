#ifndef LEGION_GROUP_ATTACK_H
#define LEGION_GROUP_ATTACK_H

void GroupAttackStart();
void GroupAttackSetBattle(bool battle);
void GroupAttackCancel(int unit);
void GroupAttackCancelAll();
void GroupAttackStep(int unit);
bool GroupAttackEnabled(int unit);
bool GroupAttackLineClear(int unit,int target);
bool GroupAttackInternal();
bool GroupAttackSafeToLaunch(int unit);

#endif
