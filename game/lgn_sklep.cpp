
#include "Amos.h"

#include "legion.h"
#include "lgn_diskacs.h"
#include "lgn_ekran1.h"
#include "lgn_akcja.h"
#include "lgn_sklep.h"
#include "lgn_util.h"
#include "utl_locale.h"
#include "test_harness.h"
#include "equipment_sets.h"
#include <algorithm>

enum ShopItemSource { ShopStock, ShopBackpack, ShopWorn };

class SklepVars {
public:
	ShopItemSource source;
    int sourceHealth;
	aint NR, TYP, PUPIL, I, X, Y, B1, I2, BRO, TYPB, CENA, ZNAK, KONIEC,
	     WOJSKO, SPICH, SZMAL, MIASTO, MYSZ, STREFA, SNR, A, NR2, B2, BB,
	     XM, YM, J, J2, BR1, BR_INFO, CARD_INFO;
	bool infoOnCard;
	astr A_S, BRO1_S, BRO2_S, B_S;
	void *sb1, *sb2, *sb3;
	astr ls_klient, ls_szmal1, ls_szmal2, ls_napisz1, ls_napisz2;
#ifdef __ANDROID__
    bool closeRequested;
#endif

	SklepVars(void) {
        source=ShopStock; sourceHealth=0;
		NR=0; TYP=0; PUPIL=0; I=0; X=0; Y=0; B1=0; I2=0; BRO=0; TYPB=0; CENA=0;
		WOJSKO=0; SPICH=0; SZMAL=0; MIASTO=0; MYSZ=0; STREFA=0;
		ZNAK=0; KONIEC=0; SNR=0; A=0; NR2=0; B2=0; BB=0; XM=0; YM=0; J=0; J2=0;
		BR1=0; BR_INFO=0; CARD_INFO=0; infoOnCard=false;
		A_S=""; BRO1_S=""; BRO2_S=""; B_S="";
		sb1=0; sb2=0; sb3=0;
		ls_klient=""; ls_szmal1=""; ls_szmal2=""; ls_napisz1=""; ls_napisz2="";
#ifdef __ANDROID__
        closeRequested=false;
#endif
	}
	~SklepVars() {
	}
};
SklepVars *sklep_vars=0;

static void DrawShopCharacter(const SklepVars &v) {
    // The same mannequin and equipment-slot layout used by the inventory.
    const int x=211,y=42;
    GADGET(x,y,105,125,"",0,5,15,15,-1);
    PasteBob(x+14,y+5,GOBY+38);
    const int xs[]={42,42,42,2,82},ys[]={3,39,103,53,53};
    for(int slot=0;slot<5;++slot) {
        const int px=x+xs[slot],py=y+ys[slot];
        gad_light(1.f); _Box(px,py,px+20,py+20);
        const int item=ARMIA[v.A][v.NR][TGLOWA+slot];
        if(item>0 && item<=MX_WEAPON) PasteBob(px+2,py+2,BRON[item][B_BOB]+BROBY+GOBY);
        SetZone(60+slot,px,py,px+20,py+20);
    }
    for(int mode=0;mode<2;++mode) {
        const int bx=211+mode*53,by=22;
        GADGET(bx,by,50,17,"",5,0,8,1,65+mode);
        if(EquipmentMode(v.A,v.NR)==mode) { Gfx::Color(1.f,0.8f,0.2f,1.f); _Box(bx+1,by+1,bx+48,by+15); }
        gad_text(1.f); Text(bx+22,by+12,mode ? "2" : "1");
    }
}

#ifdef __ANDROID__
static void DrawShopCloseButton(int zone) {
    const int x=(sklep_vars && sklep_vars->TYP==9 ? 245 : 130), y=140, width=65;
    const astr label=GS("026");
    GADGET(x,y,width,20,"",5,0,8,1,zone);
    gad_text(1.0);
    Text(x+(width-TextLength(label))/2,y+14,label);
}
#endif

void _SKLEP_NAPISZ(SklepVars &v) {
	v.ls_napisz1=v.A_S;
	v.ls_napisz2=v.B_S;
}
void _SKLEP_SZMAL(SklepVars &v) {
	v.ls_szmal1 = Str_S(GRACZE[1][1]);
	v.ls_szmal2 = BRON2_S[v.PUPIL];
}
void _SKLEP_KLIENT(SklepVars &v) {
  v.ls_klient = ARMIA_S[v.A][v.NR]+" "+RASY_S[ARMIA[v.A][v.NR][TRASA]];
}
void _SKLEP_PLECAK(SklepVars &v) {
																																				//	   PLECAK:
	gad_back_sklep(1.0); _Bar(235,3,235+78,3+38);																																			//	   Ink 0 : Bar 235,3 To 235+78,3+38
	for( v.I=0; v.I<=3; ++v.I ) {																																			//	   For I=0 To 3
		if( ARMIA[v.A][v.NR][TPLECAK+v.I]>0 ) {																																			//	      If ARMIA(A,NR,TPLECAK+I)>0
			v.B1=BRON[ARMIA[v.A][v.NR][TPLECAK+v.I]][B_BOB];																																			//	         B1=BRON(ARMIA(A,NR,TPLECAK+I),B_BOB)
			PasteBob(236+v.I*20,4,BROBY+v.B1);																																			//	         Paste Bob 236+I*20,4,BROBY+B1
		}																																			//	      End If
		SetZone(40+v.I,236+v.I*20,4,256+v.I*20,24);																																			//	      Set Zone 40+I,236+I*20,4 To 256+I*20,24
		if( ARMIA[v.A][v.NR][TPLECAK+v.I+4]>0 ) {																																			//	      If ARMIA(A,NR,TPLECAK+I+4)>0
			v.B2=BRON[ARMIA[v.A][v.NR][TPLECAK+v.I+4]][B_BOB];																																			//	         B2=BRON(ARMIA(A,NR,TPLECAK+I+4),B_BOB)
			PasteBob(236+v.I*20,24,BROBY+v.B2);																																			//	         Paste Bob 236+I*20,24,BROBY+B2
		}																																			//	      End If
		SetZone(44+v.I,236+v.I*20,24,256+v.I*20,44);																																			//	      Set Zone 44+I,236+I*20,24 To 256+I*20,44
	}																																			//	   Next I
	_store_ekran1_sklep();
	return;																																			//	   Return
}
static bool TryShopEquip(SklepVars &v,int slot) {
    if(v.A!=ARM || slot<0 || slot>4 || v.BRO<=0 || v.BRO>MX_WEAPON) return false;
    const int type=BRON[v.BRO][B_TYP],place=BRON[v.BRO][B_PLACE];
    if(type==13 || type==18) return false;
    if(slot<3 ? place!=slot+1 : (place!=4 && place!=6)) return false;
    if(slot>=3) {
        const int other=ARMIA[v.A][v.NR][slot==3 ? TPRAWA : TLEWA];
        if(other>0 && (place==6 || BRON[other][B_PLACE]==6)) return false;
    }
    const int old=ARMIA[v.A][v.NR][TGLOWA+slot];
    int backpack=-1;
    if(old>0) {
        if(v.source==ShopBackpack) backpack=v.I2;
        else for(int i=0;i<8;++i) if(ARMIA[v.A][v.NR][TPLECAK+i]==0) { backpack=i; break; }
        if(backpack<0) {
            v.B_S=GetLang()=="PL" ? "Brak miejsca w plecaku" : "Backpack is full";
            _SKLEP_NAPISZ(v);
            return false;
        }
    }
    if(v.ZNAK==-1 && GRACZE[1][1]<v.CENA) return false;
    const int saved=NUMER; NUMER=v.NR;
    if(old>0) PRZELICZ(slot,-1);
    ARMIA[v.A][v.NR][TGLOWA+slot]=v.BRO; PRZELICZ(slot,1);
    if(old>0) ARMIA[v.A][v.NR][TPLECAK+backpack]=old;
    if(v.source==ShopWorn) {
        // Moving an already-worn item should not heal the character after
        // PRZELICZ temporarily clipped health while the item was on the cursor.
        int displacedEnergy=0;
        if(old>0) {
            const int oldType=BRON[old][B_TYP];
            if(oldType!=4 && oldType!=5 && oldType!=12 && oldType!=15 && oldType!=16)
                displacedEnergy=BRON[old][B_EN];
        }
        ARMIA[v.A][v.NR][TE]=std::max(1,std::min(int(ARMIA[v.A][v.NR][TEM]),v.sourceHealth-displacedEnergy));
    }
    WAGA(v.A,v.NR); NUMER=saved;
    if(v.ZNAK==-1) { GRACZE[1][1]-=v.CENA; _SKLEP_SZMAL(v); }
    _SKLEP_PLECAK(v);
    return true;
}
// Return an unfinished transaction to its actual source, including equipment bonuses.
static void RestoreShopItem(SklepVars &v) {
    if(v.source==ShopStock) {
        SKLEP[v.SNR][v.I2]=v.BRO;
        const int x=8+(v.I2%10)*20, y=4+(v.I2/10)*20;
        const int previous=Screen(); Screen(1);
        PasteBob(x,y,BRON[v.BRO][B_BOB]+BROBY);
        _store_ekran1_sklep(); Screen(previous);
    }
    else if(v.source==ShopBackpack) ARMIA[v.A][v.NR][TPLECAK+v.I2]=v.BRO;
    else {
        const int saved=NUMER; NUMER=v.NR;
        ARMIA[v.A][v.NR][TGLOWA+v.I2]=v.BRO;
        PRZELICZ(v.I2,1);
        ARMIA[v.A][v.NR][TE]=std::max(1,std::min(int(ARMIA[v.A][v.NR][TEM]),v.sourceHealth));
        NUMER=saved;
    }
    WAGA(v.A,v.NR);
    _SKLEP_PLECAK(v);
}
void _SKLEP_PICK(SklepVars &v) {
																																				//	   PICK:
																																				//	   Hide On
	v.BB=BRON[v.BRO][B_BOB]+BROBY;																																			//	   BB=BRON(BRO,B_BOB)+BROBY
	v.TYP=BRON[v.BRO][B_TYP];																																			//	   TYP=BRON(BRO,B_TYP)
    unsigned long lastClick=MouseClickSerial();
	do {																																			//	   Repeat
		v.XM=XScreen(XMouse());																																			//	      XM=X Screen(X Mouse)
		v.YM=YScreen(YMouse());																																			//	      YM=Y Screen(Y Mouse)
		/* Set the cursor hotspot only while creating its sprite. */																																			//	      Hot Spot BB,$11
		//!!!
		HotSpot(v.BB,0x11); Sprite(3,XMouse(),YMouse(),v.BB); HotSpot(v.BB,0x00);																																			//	      Sprite 53,X Mouse,Y Mouse,BB : Wait Vbl
		rysuj(); Screen(2); DrawItemStats(v.BRO,8,45); LegionTestShopItemInfoShown(); Screen(1); WaitVbl();
        if(MouseKey()==PRAWY) {
            SpriteOnOff(3,false); HotSpot(v.BB,0x00);
            RestoreShopItem(v); WAIT_NO_MOUSE();
            break;
        }
		if( MouseClick()==1 && MouseClickSerial()!=lastClick ) {
            lastClick=MouseClickSerial();																																			//	      If Mouse Click=1
            Screen(2); const int equipZone=MouseClickY()<255 ? MouseClickZone() : 0; Screen(1);
            if(equipZone>=60 && equipZone<=64) {
                if(TryShopEquip(v,equipZone-60)) {
                    SpriteOnOff(3,false); HotSpot(v.BB,0x00); v.KONIEC=-1;
                }
                continue;
            }
#ifdef __ANDROID__
            Screen(2);
            const bool closeShop=MouseClickY()<255 && MouseClickZone()==3;
            Screen(1);
            if (closeShop) {
                // Cancel the unfinished transaction and restore the item.
                SpriteOnOff(3,false);
                HotSpot(v.BB,0x00);
                RestoreShopItem(v);
                v.closeRequested=true;
                break;
            }
#endif
			//!!!
			SpriteOnOff(3,false);																																		//	         Sprite Off 53 : Wait Vbl
			HotSpot(v.BB,0x00);																																			//	         Hot Spot BB,$0
			v.J=MouseClickY()>=255 ? MouseClickZone() : 0;																																			//	         J=Zone(XM,YM)
			if( v.J>9 && v.J<38 ) {																																			//	         If J>9 and J<38
				v.J2=v.J-10;																																			//	            J2=J-10
				v.BR1=SKLEP[v.SNR][v.J2];																																			//	            BR1=SKLEP(SNR,J2)
                if(v.BR1!=0) {
                    v.B_S=GetLang()=="PL" ? "Wybierz puste pole sklepu" : "Choose an empty shop slot";
                    _SKLEP_NAPISZ(v); continue;
                }
				if( v.BR1==0 ) {																																			//	            If BR1=0
					if( v.J2<10 ) {																																			//	               If J2<10
						v.Y=4;																																			//	                  Y=4
						v.X=8+v.J2*20;																																			//	                  X=8+J2*20
					} else {																																			//	               Else
						v.Y=24;																																			//	                  Y=24
						v.X=8+(v.J2-10)*20;																																			//	                  X=8+(J2-10)*20
					}																																			//	               End If
					rysuj();
					PasteBob(v.X,v.Y,v.BB);																																			//	               Paste Bob X,Y,BB
					_store_ekran1_sklep();
					SKLEP[v.SNR][v.J2]=v.BRO;																																			//	               SKLEP(SNR,J2)=BRO
					v.KONIEC=-1;																																			//	               KONIEC=True
					if( v.ZNAK==1 ) {																																			//	               If ZNAK=1
						GRACZE[1][1]+=v.CENA*v.ZNAK;																										//	                  Add GRACZE(1,1),CENA*ZNAK
						_SKLEP_SZMAL(v);																																			//	                  Gosub SZMAL
					}																																		//	               End If
				}																																			//	            End If
			}																																			//	         End If
			if( v.J>39 && v.J<48 ) {																																			//	         If J>39 and J<48
				v.J2=v.J-40;																																			//	            J2=J-40
				v.BR1=ARMIA[v.A][v.NR][TPLECAK+v.J2];																																			//	            BR1=ARMIA(A,NR,TPLECAK+J2)
				if( v.BR1==0 ) {																																			//	            If BR1=0
				  if( v.J2<4 ) {																																			//	               If J2<4
						v.Y=4;																																			//	                  Y=4
						v.X=236+v.J2*20;																																			//	                  X=236+J2*20
					} else {																																			//	               Else
						v.Y=24;																																			//	                  Y=24
						v.X=236+(v.J2-4)*20;																																			//	                  X=236+(J2-4)*20
					}																																			//	               End If
					rysuj();
					PasteBob(v.X,v.Y,v.BB);																																			//	               Paste Bob X,Y,BB
					_store_ekran1_sklep();
					ARMIA[v.A][v.NR][TPLECAK+v.J2]=v.BRO;																																			//	               ARMIA(A,NR,TPLECAK+J2)=BRO
					v.KONIEC=-1;																																			//	               KONIEC=True
					if( v.ZNAK==-1 && GRACZE[1][1]+(v.CENA*v.ZNAK)>=0 ) {												//	               If ZNAK=-1 and GRACZE(1,1)+(CENA*ZNAK)>=0
						GRACZE[1][1]+=v.CENA*v.ZNAK;																									//	                  Add GRACZE(1,1),CENA*ZNAK
						_SKLEP_SZMAL(v);																																			//	                  Gosub SZMAL
					}																																			//	               End If
				}																																			//	            End If
			}																																			//	         End If
		}																																			//	      End If
	} while( v.KONIEC==0 );																																			//	   Until KONIEC
																																			//	   'WAGA
																																			//	   'Gosub WYPISZ
    WAGA(v.A,v.NR);
	v.KONIEC=0;																																			//	   KONIEC=False
																																				//	   Show On
	return;																																			//	   Return
}
void _SKLEP_ROZPIS(SklepVars &v) {
																																				//	   ROZPIS:
	if( v.SPICH==0 && v.ZNAK==1 ) v.ZNAK=0;																																			//	   If SPICH=0 and ZNAK=1 : ZNAK=0 : End If
	if( v.WOJSKO==0 && v.ZNAK==-1 ) v.ZNAK=0;																																			//	   If WOJSKO=0 and ZNAK=-1 : ZNAK=0 : End If
	if( v.WOJSKO>320 && v.ZNAK==1 ) v.ZNAK=0;																																			//	   If WOJSKO>320 and ZNAK=1 : ZNAK=0 : End If
	if( v.SZMAL-v.ZNAK*v.CENA<0 && v.ZNAK==1 ) v.ZNAK=0;																																			//	   If SZMAL-ZNAK*CENA<0 and ZNAK=1 : ZNAK=0 : End If
	if( v.ZNAK!=0 ) {																																			//	   If ZNAK<>0
		v.WOJSKO+=v.ZNAK;																																			//	      Add WOJSKO,ZNAK
		v.SPICH-=v.ZNAK;																																			//	      Add SPICH,-ZNAK
		v.SZMAL-=v.ZNAK*v.CENA;																																			//	      Add SZMAL,-ZNAK*CENA
																																							//	      Set Font FON2
		v.ls_szmal1=Str_S(v.SZMAL);																																			//	      Put Block 1 : OUTLINE[250,25,Str$(SZMAL),30,2] : Wait Vbl
																																		//	      Put Block 2 : Put Block 3
		v.ls_napisz1=Str_S(v.SPICH);																																			//	      Text 60,230,Str$(SPICH)
		v.ls_napisz2=Str_S(v.WOJSKO);																																			//	      Text 215,230,Str$(WOJSKO)
	}																																			//	   End If
	return;																																			//	   Return
}

void _rysuj_sklep_spichlerz(void) {
	aint s = Screen();

	Gfx::Color(0.f,0.f,0.f);
	_ClearRaster();
	Screen(2);
	_DrawIff(-1,0,0);

	if( sklep_vars ) {

		//szmal
		SetFont(FON2);
		OUTLINE(250,25,sklep_vars->ls_szmal1,30,2);
		OUTLINE(15,25,GS("032"),30,2);
		OUTLINE(60,40,sklep_vars->ls_szmal2,30,2);
		OUTLINE(15,210,GS("033"),30,2);
		OUTLINE(60,230,sklep_vars->ls_napisz1,30,2);
		OUTLINE(190,210,GS("034"),30,2);
		OUTLINE(215,230,sklep_vars->ls_napisz2,30,2);
		SetFont(FON1);

		if(sklep_vars->sb1) RestoreBuffer(sklep_vars->sb1);
	}

#ifdef __ANDROID__
    DrawShopCloseButton(2);
#endif
	Screen(s);
}
void _SKLEP_SPICHLERZ(SklepVars &v) {
																																				//	   SPICHLERZ:

	ScreenClose(2);																																			//	   Screen Close 2
																																			//	   Screen Hide 1
	for( v.I=50; v.I>=13; v.I-- ) { Mvolume(v.I); rysuj(); LegionTestShopTransitionFrame(true); WaitVbl(); }																																			//	   For I=50 To 13 Step -1 : Mvolume I : Wait Vbl : Next
	_LOAD(KAT_S+"dane/grafika/piekarz.hb","dane:grafika/piekarz.hb","Dane",5);																																			//	   _LOAD[KAT$+"dane/grafika/piekarz.hb","dane:grafika/piekarz.hb","Dane",5]
	ScreenDisplay(2,130,40,320,244);																																			//	   Screen Display 2,130,40,320,244 : View
	Screen(2);
	ReserveZone(3);																																			//	   Reserve Zone 3
	v.WOJSKO=ARMIA[ARM][0][TAMO];																															//	   WOJSKO=ARMIA(ARM,0,TAMO) : SPICH=MIASTA(MIASTO,1,M_LUDZIE) : SZMAL=GRACZE(1,1) : CENA=10+((10*MIASTA(MIASTO,17,M_MUR))/100)
	v.SPICH=MIASTA[v.MIASTO][1][M_LUDZIE];
	v.SZMAL=GRACZE[1][1];
	v.CENA=10+((10*MIASTA[v.MIASTO][17][M_MUR])/100);
	if( v.WOJSKO<0 ) v.WOJSKO=0;																																			//	   If WOJSKO<0 : WOJSKO=0 : End If
																																				//	   Gr Writing 0
																																				//	   Set Font FON2
	v.ls_szmal1=Str_S(v.SZMAL);																																			//	   Get Block 1,245,12,64,20 : OUTLINE[250,25,Str$(SZMAL),30,2]
																																				//	   OUTLINE[15,25,"Today's price:",30,2]
	v.ls_szmal2=Str_S(v.CENA);																																			//	   OUTLINE[60,40,Str$(CENA),30,2]
																																				//	   OUTLINE[15,210,"In Granary:",30,2]
	v.ls_napisz1=Str_S(v.SPICH);																																			//	   Get Block 2,60,220,64,25 : OUTLINE[60,230,Str$(SPICH),30,2]
																																				//	   OUTLINE[190,210,"Legion  :",30,2]
	v.ls_napisz2=Str_S(v.WOJSKO);																																			//	   Get Block 3,215,220,64,25 : OUTLINE[215,230,Str$(WOJSKO),30,2]
																																//	   Set Font FON1
	v.sb1=0;
	_rysuj_sklep_spichlerz();
	GADGET(140,215,12,15,"<",25,7,16,30,1);																																			//	   GADGET[140,215,12,15,"<",25,7,16,30,1]
	GADGET(160,215,12,15,">",25,7,16,30,3);																																		//	   GADGET[160,215,12,15,">",25,7,16,30,3]
	v.sb1 = StoreBuffer(140,215,32,15);
	Gfx::Color(1.f,1.f,1.f);																																			//	   Pen 30
	//_store_ekran1_sklep();
	rysuj_ekran_ptr = _rysuj_sklep_spichlerz;																				//
	do {																																			//	   Repeat
		rysuj(); WaitVbl();
#ifdef __ANDROID__
        if (MouseClick() && MouseClickZone()==2) break;
#endif
		v.MYSZ=MouseKey();																																			//	      MYSZ=Mouse Key
		if( v.MYSZ==LEWY ) {																																			//	      If MYSZ=LEWY
			v.STREFA=MouseZone();																																			//	         STREFA=Mouse Zone
			if( v.STREFA==1 || v.STREFA==3 ) {																																			//	         If STREFA=1 or STREFA=3
				v.ZNAK=v.STREFA-2;																																			//	            ZNAK=STREFA-2
				_SKLEP_ROZPIS(v);																																			//	            Gosub ROZPIS
				rysuj(); WaitVbl();
				Wait(10);																																			//	            Wait 10
				ProcessEvents();
				while( MouseKey()==LEWY ) {																																			//	            While Mouse Key=LEWY
					_SKLEP_ROZPIS(v);																																			//	               Gosub ROZPIS
					rysuj(); WaitVbl();
				}																																			//	            Wend
			}																																			//	         End If
		}																																			//	      End If
	} while ( v.MYSZ!=PRAWY );																																			//	   Until MYSZ=PRAWY
	ARMIA[ARM][0][TAMO]=v.WOJSKO; MIASTA[v.MIASTO][1][M_LUDZIE]=v.SPICH; GRACZE[1][1]=v.SZMAL;																																			//	   ARMIA(ARM,0,TAMO)=WOJSKO : MIASTA(MIASTO,1,M_LUDZIE)=SPICH : GRACZE(1,1)=SZMAL
																																				//	   Screen Show 1
	return;																																			//	   Return
}
void _rysuj_sklep(void) {
	aint s = Screen();

	//ekran mruga na niektorych kartach graficznych, wiec czyscimy to co nie bedzie rysowane
	Gfx::Color(0.f,0.f,0.f);
	Screen(1);
	_ClearRaster(0,ScreenY()-5,640,525);

	Screen(2);
	_DrawIff(-1,0,0);

	if( sklep_vars ) {
		DrawShopCharacter(*sklep_vars);

		//klient
		OUTLINE(240,190,sklep_vars->ls_klient,31,0);

		//szmal
		SetFont(FON2);
		OUTLINE(240,15,sklep_vars->ls_szmal1,31,0);
		OUTLINE(40,15,sklep_vars->ls_szmal2,31,0);
		SetFont(FON1);

		//napisz
		OUTLINE(20,192,sklep_vars->ls_napisz1,31,0);																							//	   OUTLINE[20,192,A$,31,0]
		OUTLINE(20,205,sklep_vars->ls_napisz2,31,0);																							//	   OUTLINE[20,205,B$,31,0]

	}


#ifdef __ANDROID__
    Screen(2);
    DrawShopCloseButton(3);
#endif
	_restore_ekran1();
	Screen(s);
}
void _SKLEP(aint pMIASTO,aint pSNR,aint pA,aint pNR2) {
	void (*rys)(void) = rysuj_ekran_ptr;
																																				//	Procedure SKLEP[MIASTO,SNR,A,NR2]
	SklepVars v;
	sklep_vars = &v;

	v.MIASTO = pMIASTO;
	v.SNR = pSNR;
	v.A = pA;
	v.NR2 = pNR2;

	v.NR=v.NR2;																																//	   NR=NR2
	v.TYP=MIASTA[v.MIASTO][v.SNR][M_LUDZIE];																		//	   TYP=MIASTA(MIASTO,SNR,M_LUDZIE)
	v.PUPIL=MIASTA[v.MIASTO][v.SNR][M_PODATEK];																	//	   PUPIL=MIASTA(MIASTO,SNR,M_PODATEK)
	if(v.TYP==4) { v.A_S="sklep1.hb"; }																				//	   If TYP=4 : A$="sklep1.hb" : End If
	if(v.TYP==5) { v.A_S="sklep2.hb"; }																				//	   If TYP=5 : A$="sklep2.hb" : End If
	if(v.TYP==6) { v.A_S="sala.hb"; }																					//	   If TYP=6 : A$="sala.hb" : End If
	if(v.TYP==7) { v.A_S="sklep1.hb"; }																				//	   If TYP=7 : A$="sklep1.hb" : End If
	if(v.TYP==8) { v.A_S="yodi.hb"; }																					//	   If TYP=8 : A$="yodi.hb" : End If
	if(v.TYP==9) { _SKLEP_SPICHLERZ(v); goto OVER; }													//	   If TYP=9 : Gosub SPICHLERZ : Goto OVER : End If
	ScreenClose(2);																												//	   Screen Close 2
	for( v.I=50; v.I>=13; v.I-=1 ) { Mvolume(v.I); rysuj(); LegionTestShopTransitionFrame(true); WaitVbl(); }									//	   For I=50 To 13 Step -1 : Mvolume I : Wait Vbl : Next
	_LOAD(KAT_S+"dane/grafika/"+v.A_S,"dane:grafika/"+v.A_S,"Dane",5);				//	   _LOAD[KAT$+"dane/grafika/"+A$,"dane:grafika/"+A$,"Dane",5]
	ScreenOpen(2,_IffWidth(2),_IffHeight(2),32,LOWRES);
	if( v.TYP==5 ) {																												//	   If TYP=5
//		Flash 1,"(eee,5)(fff,6)";																					//	      Flash 1,"(eee,5)(fff,6)"
//		Flash 29,"(ec8,5)(fd9,1)(fea,1)(ffb,2)(fea,1)(fd9,2)";						//	      Flash 29,"(ec8,5)(fd9,1)(fea,1)(ffb,2)(fea,1)(fd9,2)"
//		Flash 30,"(fd8,5)(fe9,2)(ffa,2)(fe9,1)";													//	      Flash 30,"(fd8,5)(fe9,2)(ffa,2)(fe9,1)"
//		Flash 28,"(ea8,5)(fb9,3)(fca,4)";																	//	      Flash 28,"(ea8,5)(fb9,3)(fca,4)"
//		Flash 24,"(e96,5)(fa7,3)(fb8,3)";																	//	      Flash 24,"(e96,5)(fa7,3)(fb8,3)"
	}																																			//	   End If
	if( v.TYP==4 || v.TYP==7 ) {																							//	   If TYP=4 or TYP=7
//	Flash 1,"(eee,9)(fff,6)";																						//	      Flash 1,"(eee,9)(fff,6)"
//	Flash 29,"(f72,6)(f83,6)(f94,6)";																		//	      Flash 29,"(f72,6)(f83,6)(f94,6)"
//	Flash 30,"(fc6,9)(fd6,9)";																					//	      Flash 30,"(fc6,9)(fd6,9)"
//	Flash 28,"(d85,8)(e96,6)(ea7,4)";																		//	      Flash 28,"(d85,8)(e96,6)(ea7,4)"
	}																																			//	   End If
	ScreenDisplay(2,130,40,320,244); 																			//	   Screen Display 2,130,40,320,244 : Screen To Front 2


	Screen(2); ReserveZone(66);																											//	   Reserve Zone 10
	SetFont(FON1);																												//	   Set Font FON1
																																				//	   Ink 21,0
																																				//	   Gr Writing 0
	v.sb1=StoreBuffer(20,180,112,30);																				//	   Get Block 1,20,180,112,30
	v.sb2=StoreBuffer(240,5,64,25);																					//	   Get Block 2,240,5,64,25
	v.sb3=StoreBuffer(240,180,80,20);																				//	   Get Block 3,240,180,80,20
	_Box(220,180,220+96,210);																							//	   '   Box 220,180 To 220+96,210
	ScreenDisplay(1,130,255,320,44);																			//	   Screen To Front 1 : Screen Display 1,130,255,320,44
	Screen(1);																														//	   Screen 1 : View
	ResetZone(-1);																												//	   Reset Zone
	SetFont(FON1);																												//	   Set Font FON1
	PasteBob(0,0,1);																											//	   Paste Bob 0,0,1
																																				//	   Colour 0,$310
	GADGET(6,2,200,40,"",19,6,1000,1,-1);																		//	   GADGET[6,2,200,40,"",19,6,0,1,-1]
	GADGET(234,2,80,40,"",19,6,1000,1,-1);																		//	   GADGET[234,2,80,40,"",19,6,0,1,-1]
	GADGET(210,2,20,16," -",5,0,8,1,1);																		//	   GADGET[210,2,20,16," «",5,0,8,1,1]
	GADGET(210,24,20,16," +",5,0,8,1,2);																	//	   GADGET[210,24,20,16," »",5,0,8,1,2]
	_SKLEP_KLIENT(v);																											//	   Gosub KLIENT
	_SKLEP_SZMAL(v);																												//	   Gosub SZMAL
																																				//	   Gr Writing 1
	v.I=0;																																	//	   I=0
	for( v.Y=0; v.Y<=1; ++v.Y ) {																								//	   For Y=0 To 1
		for( v.X=0; v.X<=9; ++v.X ) {																							//	      For X=0 To 9
			if( SKLEP[v.SNR][v.I]>0 ) {																						//	         If SKLEP(SNR,I)>0
				v.B1=BRON[SKLEP[v.SNR][v.I]][B_BOB];																	//	            B1=BRON(SKLEP(SNR,I),B_BOB)
				PasteBob(8+v.X*20,4+v.Y*20,BROBY+v.B1);																//	            Paste Bob 8+X*20,4+Y*20,BROBY+B1
			}																																	//	         End If
			SetZone(10+v.I,8+v.X*20,4+v.Y*20,28+v.X*20,24+v.Y*20);											//	         Set Zone 10+I,8+X*20,4+Y*20 To 28+X*20,24+Y*20
			v.I++;																															//	         Inc I
		}																																		//	      Next X
	}																																			//	   Next Y
																																				//
	_SKLEP_PLECAK(v);																											//	   Gosub PLECAK
	_store_ekran1_sklep();
	rysuj_ekran_ptr = _rysuj_sklep;																				//
	do {																																	//	   Repeat
		rysuj();
        LegionTestShopFrame();
		if( v.BR_INFO > 0 ) {
			if (v.infoOnCard) Screen(2);
			_WYBOR_SHOW_INFO(v.BR_INFO);
            Screen(2); DrawItemStats(v.BR_INFO,8,45);
            LegionTestHoverItemInfo(v.BR_INFO,true,v.infoOnCard);
            Screen(1);
		}
		WaitVbl();
		if( MouseClick()==1 ) {																							//	      If Mouse Click=1
            Screen(2); const int wornZone=MouseClickY()<255 ? MouseClickZone() : 0; Screen(1);
            if(wornZone==65 || wornZone==66) {
                const int saved=NUMER; NUMER=v.NR;
                if(v.A==ARM) SwitchEquipmentSet(v.A,v.NR,wornZone-65);
                NUMER=saved; v.CARD_INFO=0; v.BR_INFO=0;
                continue;
            }
            if(wornZone>=60 && wornZone<=64) {
                const int slot=wornZone-60;
                const int item=ARMIA[v.A][v.NR][TGLOWA+slot];
                if(v.A==ARM && item>0 && item<=MX_WEAPON) {
                    v.source=ShopWorn; v.I2=slot; v.BRO=item; v.ZNAK=1;
                    v.BRO1_S=BRON2_S[BRON[item][B_TYP]]; v.BRO2_S=BRON_S[item];
                    v.TYPB=BRON[item][B_TYP];
                    v.CENA=BRON[item][B_CENA]+BRON[item][B_CENA]*MIASTA[v.MIASTO][v.TYPB][M_MUR]/100;
                    v.CENA-=v.CENA*10/100;
                    v.A_S=v.BRO2_S+" "+v.BRO1_S; v.B_S=GS("037")+Str_S(v.CENA);
                    _SKLEP_NAPISZ(v); v.BR_INFO=v.CARD_INFO=0;
                    v.sourceHealth=ARMIA[v.A][v.NR][TE];
                    const int saved=NUMER; NUMER=v.NR;
                    PRZELICZ(slot,-1); ARMIA[v.A][v.NR][TGLOWA+slot]=0;
                    WAGA(v.A,v.NR); NUMER=saved;
                    _SKLEP_PICK(v);
#ifdef __ANDROID__
                    if(v.closeRequested) break;
#endif
                }
                continue;
            }
#ifdef __ANDROID__
            Screen(2);
            const bool closeShop=MouseClickY()<255 && MouseClickZone()==3;
            Screen(1);
            if (closeShop) break;
            v.I=MouseClickY()>=255 ? MouseClickZone() : 0;
#else
			v.I=MouseClickY()>=255 ? MouseClickZone() : 0;																										//	         I=Mouse Zone
#endif
			if( v.I==1 ) {																											//	         If I=1
				rysuj();
				GADGET(210,2,20,16," -",0,5,10,1,0);														//	            GADGET[210,2,20,16," «",0,5,10,1,0]
				GADGET(210,2,20,16," -",5,0,8,1,-1);														//	            GADGET[210,2,20,16," «",5,0,8,1,-1]
				AG2:																														//	            AG2:
				v.CARD_INFO=0; Add(v.NR,1,1,10);																									//	            Add NR,1,1 To 10
				if( ARMIA[ARM][v.NR][TE]<=0 ) goto AG2;														//	            If ARMIA(ARM,NR,TE)<=0 : Goto AG2 : End If
				_SKLEP_PLECAK(v);																								//	            Gosub PLECAK
				_SKLEP_KLIENT(v);																								//	            Gosub KLIENT
			}																																	//	         End If
			if( v.I==2 ) {																											//	         If I=2
				rysuj();
				GADGET(210,24,20,16," +",0,5,10,1,0);														//	            GADGET[210,24,20,16," »",0,5,10,1,0]
				GADGET(210,24,20,16," +",5,0,8,1,-1);														//	            GADGET[210,24,20,16," »",5,0,8,1,-1]
				AG1:																														//	            AG1:
				v.CARD_INFO=0; Add(v.NR,-1,1,10);																								//	            Add NR,-1,1 To 10
				if( ARMIA[ARM][v.NR][TE]<=0 ) goto AG1;														//	            If ARMIA(ARM,NR,TE)<=0 : Goto AG1 : End If
				_SKLEP_PLECAK(v);																								//	            Gosub PLECAK
				_SKLEP_KLIENT(v);																								//	            Gosub KLIENT
			}																																	//	         End If
																																				//
			if( v.I>9 && v.I<38 ) {																								//	         If I>9 and I<38
                v.source=ShopStock;
				v.I2=v.I-10;																												//	            I2=I-10
				v.BRO=SKLEP[v.SNR][v.I2];																							//	            BRO=SKLEP(SNR,I2)
				if( v.BRO>0 ) {																										//	            If BRO>0
					v.B1=BRON[v.BRO][B_BOB];																					//	               B1=BRON(BRO,B_BOB)
					v.BRO1_S=BRON2_S[BRON[v.BRO][B_TYP]];															//	               BRO1$=BRON2$(BRON(BRO,B_TYP))
					v.BRO2_S=BRON_S[v.BRO];																						//	               BRO2$=BRON$(BRO)
					v.TYPB=BRON[v.BRO][B_TYP];																				//	               TYPB=BRON(BRO,B_TYP)
					v.CENA=BRON[v.BRO][B_CENA]+((BRON[v.BRO][B_CENA]*MIASTA[v.MIASTO][v.TYPB][M_MUR])/100);	//	               CENA=BRON(BRO,B_CENA)+((BRON(BRO,B_CENA)*MIASTA(MIASTO,TYPB,M_MUR))/100)
					if( GRACZE[1][1]-v.CENA>=0 ) {																	//	               If GRACZE(1,1)-CENA>=0
						v.ZNAK=-1;																										//	                  ZNAK=-1
						v.A_S=v.BRO2_S+" "+v.BRO1_S;																			//	                  A$=BRO2$+" "+BRO1$
						v.B_S=GS("035")+Str_S(v.CENA);																		//	                  B$="costs:"+Str$(CENA)
						_SKLEP_NAPISZ(v);																	//	                  Gosub NAPISZ
						SKLEP[v.SNR][v.I2]=0;																						//	                  SKLEP(SNR,I2)=0
						if( v.I2<10 ) {																								//	                  If I2<10
							v.Y=4;																											//	                     Y=4
							v.X=8+v.I2*20;																								//	                     X=8+I2*20
						} else {																										//	                  Else
							v.Y=24;																											//	                     Y=24
							v.X=8+(v.I2-10)*20;																						//	                     X=8+(I2-10)*20
						}																														//	                  End If
						rysuj();
						gad_back_sklep(1.0); _Bar(v.X,v.Y,v.X+16,v.Y+16);											//	                  Ink 0 : Bar X,Y To X+16,Y+16
						_store_ekran1_sklep();
						_SKLEP_PICK(v);																							//	                  Gosub PICK
#ifdef __ANDROID__
                        if (v.closeRequested) break;
#endif
					} else {																											//	               Else
						v.A_S=v.BRO2_S+" "+v.BRO1_S;																			//	                  A$=BRO2$+" "+BRO1$
						v.B_S=GS("227")+Str_S(v.CENA);															//	                  B$="Not for you "+Str$(CENA)
						_SKLEP_NAPISZ(v);																	//	                  Gosub NAPISZ
					}																															//	               End If
				}																																//	            End If
			}																																	//	         End If
			if( v.I>39 && v.I<48 ) {																							//	         If I>39 and I<48
                v.source=ShopBackpack;
				v.I2=v.I-40;																												//	            I2=I-40
				v.BRO=ARMIA[v.A][v.NR][TPLECAK+v.I2];																		//	            BRO=ARMIA(A,NR,TPLECAK+I2)
				if( v.BRO>0 ) {																										//	            If BRO>0
					v.B1=BRON[v.BRO][B_BOB];																					//	               B1=BRON(BRO,B_BOB)
					v.BRO1_S=BRON2_S[BRON[v.BRO][B_TYP]];															//	               BRO1$=BRON2$(BRON(BRO,B_TYP))
					v.BRO2_S=BRON_S[v.BRO];																						//	               BRO2$=BRON$(BRO)
					v.A_S=v.BRO2_S+" "+v.BRO1_S;																				//	               A$=BRO2$+" "+BRO1$
					v.TYPB=BRON[v.BRO][B_TYP];																				//	               TYPB=BRON(BRO,B_TYP)
					v.CENA=BRON[v.BRO][B_CENA]+((BRON[v.BRO][B_CENA]*MIASTA[v.MIASTO][v.TYPB][M_MUR])/100);//	               CENA=BRON(BRO,B_CENA)+((BRON(BRO,B_CENA)*MIASTA(MIASTO,TYPB,M_MUR))/100)
					v.CENA=v.CENA-((v.CENA*10)/100);																		//	               CENA=CENA-((CENA*10)/100)
					v.ZNAK=1;																												//	               ZNAK=1
					v.B_S=GS("037")+Str_S(v.CENA);																	//	               B$="I'll pay "+Str$(CENA)
					_SKLEP_NAPISZ(v);																		//	               Gosub NAPISZ
					ARMIA[v.A][v.NR][TPLECAK+v.I2]=0;																		//	               ARMIA(A,NR,TPLECAK+I2)=0
					if( v.I2<4 ) {																									//	               If I2<4
						v.Y=4;																												//	                  Y=4
						v.X=236+v.I2*20;																								//	                  X=236+I2*20
					} else {																											//	               Else
						v.Y=24;																												//	                  Y=24
						v.X=236+(v.I2-4)*20;																						//	                  X=236+(I2-4)*20
					}																															//	               End If
					rysuj();
					gad_back_sklep(1.0); _Bar(v.X,v.Y,v.X+16,v.Y+16);												//	               Ink 0 : Bar X,Y To X+16,Y+16
					_store_ekran1_sklep();
					_SKLEP_PICK(v);																								//	               Gosub PICK
#ifdef __ANDROID__
                        if (v.closeRequested) break;
#endif
				}																																//	            End If
			}																																	//	         End If
		}	else {																														//	      End If
			//### ulepszenie - pokazuje info o przedmiocie po najechaniu go mycha
			v.BR_INFO = v.CARD_INFO;
            v.infoOnCard=false;
            Screen(2);
            const int wornZone=YMouse()<255 ? MouseZone() : 0;
            Screen(1);
            if(wornZone>=60 && wornZone<=64) {
                v.BR_INFO=ARMIA[v.A][v.NR][TGLOWA+wornZone-60];
                v.infoOnCard=true;
            }
			v.I=MouseZone();
			if( YMouse()>=255 && v.I>=10 && v.I<=37 ) {
				v.BR_INFO=SKLEP[v.SNR][v.I-10];
			}
			if( YMouse()>=255 && v.I>=40 && v.I<=47 ) {
				v.BR_INFO=ARMIA[v.A][v.NR][TPLECAK+v.I-40];
			}
		}
		if( Inkey_S()==GS("K_QUIT") || MouseKey()==PRAWY ) {								//	      If Inkey$="q" or Mouse Key=PRAWY
			v.KONIEC=-1;																											//	         KONIEC=True
		}																																		//	      End If
	} while(v.KONIEC==0);																									//	   Until KONIEC
																																				//	   Goto OVER
	OVER:																																	//	   OVER:
	rysuj_ekran_ptr = rys;
	sklep_vars = 0;
	ScreenClose(2);																												//	   Screen Close 2
																																				//	   Flash Off
	ScreenOpen(2,64,50,32,LOWRES);																				//	   Screen Open 2,64,50,32,Lowres : Curs Off : Flash Off
																																				//	   Screen Hide : Screen To Back
																																				//	   Del Block
	Screen(1);																														//	   Screen 1 : Colour 0,$0 : Screen Display 1,130,275,320,25 : Reset Zone : EKRAN1
	ScreenDisplay(1,130,275,320,25); ResetZone(-1); EKRAN1();
	ARMIA[v.A][v.NR2][TY] += 8;																						//	   Add ARMIA(A,NR2,TY),8
	ARMIA[v.A][v.NR2][TTRYB] = 0;																					//	   ARMIA(A,NR2,TTRYB)=0
																																				//	   '   SELECT[A,NUMER]
	Screen(0);																														//	   Screen 0
																																				//	   View
	for(v.I=13;v.I<=50;++v.I) { Mvolume(v.I); rysuj(); LegionTestShopTransitionFrame(false); WaitVbl(); } 								//	   For I=13 To 50 : Mvolume I : Wait Vbl : Next
																																				//	End Proc
}

