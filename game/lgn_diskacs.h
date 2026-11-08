#ifndef D_LGN_DISKACS_H_D
#define D_LGN_DISKACS_H_D

#include <cstddef>

void ODCZYT(unsigned char *MEM,std::size_t length);
void CLEAR_TABLES(void);
void SDIR(astr A_S,aint K1,aint K2);
void REQUEST(astr A_S,astr NAPI_S);
void _LOAD( astr A_S, astr B_S, astr NAPI_S, aint TRYB);
astr SaveDirectory();
astr SaveSlotPath(const astr &directory, aint slot);
astr _sdir_get_file_name(aint index);
astr AutoSaveLabel(const astr &directory);
bool AutoSaveGame(const char *reason);
void _SAVE_GAME(void);
void _LOAD_GAME(void);

#endif // D_LGN_DISKACS_H_D

