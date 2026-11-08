#ifndef LEGION_QUEST_TEXT_H
#define LEGION_QUEST_TEXT_H
#include <string>
// The localized adventure text uses $ as the quest character's name.
inline std::string ExpandQuestName(std::string text,const std::string &name) {
    if (name.empty()) return text;
    std::string::size_type pos=0;
    while ((pos=text.find('$',pos))!=std::string::npos) {
        text.replace(pos,1,name);
        pos+=name.size();
    }
    return text;
}
#endif
