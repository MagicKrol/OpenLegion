#ifndef LEGION_TEST_HARNESS_H
#define LEGION_TEST_HARNESS_H

extern int legion_test_status;
int RunLegionTest(const char *script_path);
bool LegionTestTitleActive();
void LegionTestTitleMenuFrame();
void LegionTestTitleDialogFrame();
void LegionTestQuestDialogUpdated();
void LegionTestInventoryShown();
void LegionTestMapLootFrame();
void LegionTestMapLootSaleBlocked();
void LegionTestGrowthPanelFrame();
void LegionTestInspectionFrame();
void LegionTestTacticalFrame();
void LegionTestEquipmentModeFrame();
void LegionTestItemInfoShown();
void LegionTestConsumableNoticeFrame();
void LegionTestHoverItemInfo(int item,bool shop,bool onCard);
void LegionTestInventoryNavigationFrame();
void LegionTestMapLootHeldFrame(int item);
void LegionTestShopItemInfoShown();
void LegionTestShopFrame();
void LegionTestShopEntered();
void LegionTestQuestSceneStarted();
void LegionTestArmyWindowFrame(int which);
void LegionTestShopTransitionFrame(bool entering);
void LegionTestQuestAnswer(int quest,const char *text);
void LegionTestQuestDialogOpened(const char *label);
void LegionTestActiveActionFrame();

#endif
