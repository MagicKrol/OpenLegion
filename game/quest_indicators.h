#ifndef LEGION_QUEST_INDICATORS_H
#define LEGION_QUEST_INDICATORS_H

// Offers are rolled once per newly generated city visit, then consumed when
// the player asks the corresponding person about news.
void ResetCityQuestOffers();
void BeginCityQuestOffers(int city);
void SetCityQuestOffer(int person,bool available);
bool CityHasNewQuest(int person);
bool CityHasQuestInfo(int person);
bool CityHasPendingQuestInfo(int city);
char CityQuestMarker(int person);
void ConsumeCityQuestOffer(int person);
void DrawCityQuestIndicators();
void DrawWorldQuestIndicators();

#endif
