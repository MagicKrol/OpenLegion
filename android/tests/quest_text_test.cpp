#include "../../game/quest_text.h"
#include <cassert>
int main() {
    assert(ExpandQuestName("Szukam $'a", "Grom") == "Szukam Grom'a");
    assert(ExpandQuestName("$ jest bohaterem", "Żelazny") == "Żelazny jest bohaterem");
    assert(ExpandQuestName("$ i $", "Anna") == "Anna i Anna");
    assert(ExpandQuestName("Bez imienia", "Anna") == "Bez imienia");
    assert(ExpandQuestName("$", "") == "$");
}
