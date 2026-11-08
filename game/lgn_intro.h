#ifndef D_LGN_INTRO_H_D
#define D_LGN_INTRO_H_D

#include <atomic>

void _INTRO(void);
void EXTRO(void);
void pc_intro(void);

extern std::atomic<bool> ib_intro_playing;

extern bool ib_czysc_miecz,
						ib_skip_intro;

#endif
