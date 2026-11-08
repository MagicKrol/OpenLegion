
#include <cstdio>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#include <GL/gl.h>
//#include <GL/glut.h>

#include "Font.h"
#include "Utils.h"
#include "Textures.h"
#include "Bob.h"
#include "Basic.h"
#include "Settings.h"

using namespace std;
using namespace Core;

namespace Gfx {

namespace {
// Keep the original Latin-1 slots and add Polish characters outside it.
const Uint16 extra_glyphs[] = {0x0104,0x0105,0x0106,0x0107,0x0118,0x0119,
    0x0141,0x0142,0x0143,0x0144,0x015a,0x015b,0x0179,0x017a,0x017b,0x017c};
const int GLYPH_COUNT = 256 + sizeof(extra_glyphs)/sizeof(extra_glyphs[0]);
Uint16 glyph_code(int slot) { return slot<256 ? Uint16(slot) : extra_glyphs[slot-256]; }
bool lowercase(Uint16 code) {
    if(code>=0xe0 && code<=0xff) return true;
    switch(code) {
        case 0x0105: case 0x0107: case 0x0119: case 0x0142:
        case 0x0144: case 0x015b: case 0x017a: case 0x017c: return true;
        default: return false;
    }
}
float fallback_scale(TTF_Font *font,TTF_Font *fallback,Uint16 reference) {
    if(!fallback) return 1.f;
    int x1,x2,y1,y2,advance, fx1,fx2,fy1,fy2,fadvance;
    if(TTF_GlyphMetrics(font,reference,&x1,&x2,&y1,&y2,&advance)<0 ||
       TTF_GlyphMetrics(fallback,reference,&fx1,&fx2,&fy1,&fy2,&fadvance)<0 || fy2<=fy1)
        return 1.f;
    float scale=float(y2-y1)/float(fy2-fy1);
    return scale<0.4f ? 0.4f : scale>2.f ? 2.f : scale;
}
int next_glyph(const std::string &text,size_t &pos) {
    unsigned char c=static_cast<unsigned char>(text[pos++]);
    unsigned int code=c;
    if(c>=0xc2 && c<=0xdf && pos<text.size()) {
        unsigned char next=static_cast<unsigned char>(text[pos]);
        if((next&0xc0)==0x80) { code=((c&0x1f)<<6)|(next&0x3f);++pos; }
    } else if(c>=0xe0 && c<=0xef && pos+1<text.size()) {
        unsigned char b=static_cast<unsigned char>(text[pos]), d=static_cast<unsigned char>(text[pos+1]);
        if((b&0xc0)==0x80 && (d&0xc0)==0x80) { code=((c&15)<<12)|((b&63)<<6)|(d&63);pos+=2; }
    }
    if(code<256) return int(code);
    for(int i=256;i<GLYPH_COUNT;++i) if(glyph_code(i)==code) return i;
    return '?';
}
}

void Font :: _reset(void) {
	ptex = 0;
	ltex = 0;
	glyphs_width = 0;
	glyphs_height = 0;
	glyphs_xshift = 0;
	glyphs_yshift = 0;
	glyphs_advance = 0;
	ib_antialiasing = true;
}
void Font :: _destroy(void) {
	delete ptex;
	delete[] ltex;
	delete[] glyphs_width;
	delete[] glyphs_height;
	delete[] glyphs_xshift;
	delete[] glyphs_yshift;
	delete[] glyphs_advance;
}
Font :: Font(void) {
	_reset();
}
Font :: ~Font() {
	_destroy();
}
void Font :: SetAntialiasing(bool pb_aa) {
	ib_antialiasing = pb_aa;
}
int Font :: Load(const string& name,int ptx,const string& fallback) {

	//inicjujemy SDL_ttf
	if( TTF_Init() < 0 ) {
    ERROR("TTF_Init: " + TTF_GetError());
    return -1;
	}

	//ladujemy font do pamieci
	TTF_Font *font;
	font = TTF_OpenFontIndex(name.c_str(), ptx, 0);
	if( !font ) {
    ERROR("TTF_OpenFontIndex: " + TTF_GetError());
    TTF_Quit();
    return -2;
	}
	TTF_Font *fallback_font = !fallback.empty() && fallback!=name ? TTF_OpenFontIndex(fallback.c_str(),ptx,0) : 0;
	TTF_Font *glyph_font[GLYPH_COUNT];
	float lower_scale=fallback_scale(font,fallback_font,'e');
	float upper_scale=fallback_scale(font,fallback_font,'A');

	//inicjujemy atrybuty
	ptex = new PTex();
	ltex = new LTex[GLYPH_COUNT];
	glyphs_width = new float[GLYPH_COUNT];
	glyphs_height = new float[GLYPH_COUNT];
	glyphs_xshift = new float[GLYPH_COUNT];
	glyphs_yshift = new float[GLYPH_COUNT];
	glyphs_advance = new float[GLYPH_COUNT];

	//zbieramy statystyki o znaczkach
	int minx, maxx, miny, maxy, advance;
	int w,h,maxh,maxw,avgw;
	int li_ret;
	int gw[GLYPH_COUNT], gh[GLYPH_COUNT];
	float font_height = TTF_FontHeight(font);
//	float font_ascent = TTF_FontAscent(font);
//	float font_descent = TTF_FontDescent(font);
	maxh = 0; maxw = 0; avgw = 0;
	for( int i=0; i < GLYPH_COUNT; ++i ) {
		Uint16 code=glyph_code(i);
		glyph_font[i]=(fallback_font && code>=32 && !TTF_GlyphIsProvided(font,code)) ? fallback_font : font;
		li_ret = TTF_GlyphMetrics( glyph_font[i], code, &minx, &maxx, &miny, &maxy, &advance);
		if( li_ret < 0 ) {
	    ERROR("TTF_GlyphMetrics: " + TTF_GetError());
			if(fallback_font) TTF_CloseFont(fallback_font);
			TTF_CloseFont(font);
			TTF_Quit();
	    return -3;
		}

		w = maxx - minx;
		h = maxy - miny;
		gw[i] = w;
		gh[i] = h;

//INFO(toString(i,"$%2x")+": "+char(i)+" w="+toString(w)+" h="+toString(h)+" adv="+toString(advance)+" minx="+toString(minx)+" maxy="+toString(maxy));

		float scale=glyph_font[i]==fallback_font ? (lowercase(code) ? lower_scale : upper_scale) : 1.f;
		glyphs_width[i]  = scale * w / font_height;
		glyphs_height[i] = scale * h / font_height;
		glyphs_xshift[i] = scale * minx / font_height;
		glyphs_yshift[i] = scale * -maxy / font_height;
		glyphs_advance[i] = scale * advance / font_height;

		avgw += w;
		if( h > maxh ) {
			maxh = h;
		}
		if( w > maxw ) {
			maxw = w;
		}
	}
	INFO("MAX FONT HEIGHT=" + toString((float)maxh));
	INFO("AVG FONT WIDTH=" + toString(((float)avgw+GLYPH_COUNT)/GLYPH_COUNT));

	//obliczenie rozmiaru tekstury
	int tx_w, tx_h;
	// gluBuild2DMipmaps rescales non-power-of-two images; pad instead so
	// small UI glyphs retain their original pixels and remain legible.
	tx_w=1;
	while(tx_w < 16*(maxh+1)) tx_w*=2;
	int rows=1, row_x=0;
	for(int i=0;i<GLYPH_COUNT;++i) if(gw[i]>0) {
		if(row_x>0 && row_x+gw[i]>=tx_w) { ++rows;row_x=0; }
		row_x+=gw[i]+1;
	}
	tx_h=1;
	while(tx_h < rows*(maxh+1)) tx_h*=2;

	// A single byte per pixel becomes the alpha channel of the font texture.
	SDL_Surface *rgba_img = SDL_CreateRGBSurface(SDL_SWSURFACE, tx_w, tx_h, 8, 0, 0, 0, 0);
	if( !rgba_img ) {
		ERROR("SDL_CreateRGBSurface:");
		if(fallback_font) TTF_CloseFont(fallback_font);
		TTF_CloseFont(font);
		TTF_Quit();
		return -4;
	}
	SDL_FillRect(rgba_img, NULL, 0);

	int xo = 0, yo = 0;
	SDL_Color fg = {255,255,255,0};
	for( int i=0; i<GLYPH_COUNT; ++i ) {
		if( gw[i] <= 0 ) continue;
		// The bundled SDL_ttf's Shaded glyph routine copies rows from a
		// different (empty) cache. Blended produces the actual glyph coverage.
		SDL_Surface *glyph = TTF_RenderGlyph_Blended(glyph_font[i], glyph_code(i), fg);
		if( !glyph ) {
			ERROR("TTF_RenderGlyph_Blended: " + TTF_GetError());
		} else {
			if( xo > 0 && xo + gw[i] >= tx_w ) { yo += maxh + 1; xo = 0; }
			if( SDL_LockSurface(glyph) == 0 ) {
				for( int y=0; y<glyph->h && y<maxh && yo+y<tx_h; ++y ) {
					const Uint32 *src = reinterpret_cast<const Uint32 *>(
						static_cast<const Uint8 *>(glyph->pixels) + y*glyph->pitch);
					Uint8 *dst = static_cast<Uint8 *>(rgba_img->pixels) + (yo+y)*rgba_img->pitch + xo;
					for( int x=0; x<glyph->w && x<gw[i] && xo+x<tx_w; ++x ) {
						Uint8 r,g,b,a;
						SDL_GetRGBA(src[x],glyph->format,&r,&g,&b,&a);
						dst[x]=a;
					}
				}
				SDL_UnlockSurface(glyph);
			} else {
				ERROR("Cannot lock rendered glyph");
			}
			SDL_FreeSurface(glyph);
			xo += gw[i] + 1;
		}
	}

	if( ib_antialiasing )
		ptex->SetFiltering(ptex->FM_LINEAR);
	else
		ptex->SetFiltering(ptex->FM_NEAREST);
	ptex->Set(rgba_img,GL_ALPHA);
	INFO("FONT_IMG_SIZE: " +  toString(ptex->GetImgWidth()) + "," + toString(ptex->GetImgHeight()) );
	INFO("FONT_TEXT_SIZE: " + toString(ptex->GetWidth()) + "," + toString(ptex->GetHeight()) );

	SDL_FreeSurface(rgba_img);

	//zwalniamy pamiec
	if(fallback_font) TTF_CloseFont(fallback_font);
	TTF_CloseFont(font);

	//zamykamy SDL_ttf
	TTF_Quit();

	//wyliczmy jeszcze tekstury logiczne
	xo = 0;	yo = 0;
	for( int i = 0; i < GLYPH_COUNT; ++i ) {
		if( gw[i] > 0 ) {
			//kopiujemy znaczek na teksture
			if( xo > 0 && xo + gw[i] >= tx_w ) {
				//znaczek nie zmiesci sie w tym wierszu
				yo += maxh + 1;
				xo = 0;
			}

//			ltex[i].Set( ptex, xo, yo, xo+gw[i], yo + maxh );
			ltex[i].Set( ptex, xo, yo, xo+gw[i], yo + gh[i] );

			xo += gw[i] + 1;
		} else {
			ltex[i].Set( ptex, xo,yo, xo,yo );
		}
	}

	return 1;
}


void FontManager :: _reset(void) {
	fonts.clear();
}
void FontManager :: _destroy(void) {
	Font* f;
	map<string,Font*>::iterator fit;
	for( fit=fonts.begin(); fit != fonts.end(); ++fit ) {
		f = fit->second;
		delete f;
	}
	fonts.clear();
}
string FontManager :: _key(const string& id,int siz) {
	return id + "|" + toString((int)siz);
}
FontManager :: FontManager(void) {
	_reset();
}
FontManager :: ~FontManager() {
	_destroy();
}
bool FontManager :: Exists(const string& id,int siz) {
	return fonts.find(_key(id,siz)) != fonts.end();
}
void FontManager :: Add(const string& id,int siz,Font* t,bool overwrite) {
	if( Exists(id,siz) ) {
		if( overwrite ) {
			Del(id,siz);	//usuwamy poprzednia teksture
		} else {
			return;				//jest juz i nienadpisujemy, wiec koniec
		}
	}
	fonts[_key(id,siz)] = t;
	return;
}
Font* FontManager :: Get(const string& id,int siz) {
	map<string,Font*>::const_iterator it;
	it = fonts.find(_key(id,siz));
	if( it != fonts.end() )
		return it->second;
	return 0;
}
Font* FontManager :: Rem(const string& id,int siz) {
	Font* t = 0;
	map<string,Font*>::iterator it;
	it = fonts.find(_key(id,siz));
	if( it != fonts.end() ) {
		t = it->second;
		fonts.erase(it);
	}
	return t;
}
void FontManager :: Del(const string& id,int siz) {
	Font* t;
	map<string,Font*>::iterator it;
	it = fonts.find(_key(id,siz));
	if( it != fonts.end() ) {
		t = it->second;
		delete t;
		fonts.erase(it);
	}
}
int FontManager :: Load(const string& filename,const string& root) {
	Settings sets;
	int ret = sets.ReadFile(filename);
	if( ret < 0 ) {
		ERROR("Blad wczytywania pliku z opisem fontĂłw: " + sets.ErrorMsg());
		return -1;
	}
	return Load(sets,root);
}

int FontManager :: Load(const Settings& sets,const string& root) {
	if( ! sets.isValid() ) {
		ERROR("Obiekt opisujÂący tekstury jest niewaÂżny");
		return -2;
	}

	//!@# TODO
	return -1000;

//
//
//	Settings s, s1;
//	int n, li_ret;
//	string id, pid, file_name;
//	PTex *pt;
//	LTex *lt;
//	int ix1, ix2, iy1, iy2, ihsx, ihsy;
//
//	//usuwamy wszystko
//	_destroy();
//	_reset();
//
//	//ladujemy tekstury fizyczne
//	if( root != "" )
//		s = sets.Extract(root + ".Phisical");
//	else
//		s = sets.Extract(root + "Phisical");
//	if( ! s.isValid() ) {
//		ERROR("Blad pobierania danych na temat tekstur fizycznych: " + s.ErrorMsg());
//		return -3;
//	}
//	cout << s.Dump() << endl;
//
//	//TODO - dorobic pobieranie ilosci wpisow o tekturach
//	n = 1;
//	s1 = s.Extract("Phisical["+toString(n)+"]");
//	while( s1.isValid() ) {
//		cout << "tekstura<" + toString(n) + ">" << endl;
//		cout << s1.Dump() << endl;
//
//		id = s1.GetValS("Phisical.ID");
//		file_name = s1.GetValS("Phisical.FileName");
//		cout << "plik: " << id << " : " << file_name << endl;
//
//		pt = new PTex();
//		li_ret = pt->Load(file_name);
//		if( li_ret < 0 ) {
//			delete pt;
//			ERROR("Error loading the texture file (filename=" + file_name + ") !");
//			return -4;
//		}
//
//		AddPTex(id,pt);
//		cout << "Dodano teksture o id=" << id << endl;
//
//		n++;
//		s1 = s.Extract("Phisical["+toString(n)+"]");
//	}
//	cout << "Blad: " << s1.ErrorMsg() << endl;
//
//	//ladujemy tekstury logiczne
//	if( root != "" )
//		s = sets.Extract(root + ".Logical");
//	else
//		s = sets.Extract(root + "Logical");
//	if( ! s.isValid() ) {
//		ERROR("Blad pobierania danych na temat tekstur logicznych: " + s.ErrorMsg());
//		return -4;
//	}
//	cout << s.Dump() << endl;
//
//	//TODO - dorobic pobieranie ilosci wpisow o tekturach
//	n = 1;
//	s1 = s.Extract("Logical["+toString(n)+"]");
//	while( s1.isValid() ) {
//		cout << "tekstura<" + toString(n) + ">" << endl;
//		cout << s1.Dump() << endl;
//
//		id = s1.GetValS("Logical.ID");
//		pid = s1.GetValS("Logical.PID");
//		ix1 = s1.GetValI("Logical.Pos",0);
//		iy1 = s1.GetValI("Logical.Pos",1);
//		ix2 = s1.GetValI("Logical.Pos",2);
//		iy2 = s1.GetValI("Logical.Pos",3);
//		ihsx = s1.GetValI("Logical.HotSpot",0);
//		ihsy = s1.GetValI("Logical.HotSpot",1);
//
//		pt = GetPTex(pid);
//		if( 0 == pt ) {
//			ERROR("Brak tekstury fizycznej o ID=\"" + pid + "\". Tekstura logiczna o ID=\"" + id + "\" nie moÂże byĂ¦ utworzona !!!");
//		} else {
//			lt = new LTex();
//			lt->Set(pt,ix1,iy1,ix2,iy2);
//			lt->SetHotSpot( ((GLfloat)(ihsx - ix1))/lt->GetWidth(), ((GLfloat)(ihsy - iy1))/lt->GetHeight() );
//			AddLTex(id,lt);
//			cout << "Dodano teksture o id=" << id << endl;
//		}
//
//		n++;
//		s1 = s.Extract("Logical["+toString(n)+"]");
//	}
//	cout << "Blad: " << s1.ErrorMsg() << endl;
//
//
//	cout << "Koniec textur" << endl;
//
//	return 1;
}



void Text(Font *font,const string& text, float x, float y, float sizh, float sizv) {

	int len = text.length();
	if( len <= 0 ) {
		return;
	}

	BeginBobBlits();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

		glTranslatef(x, y, 0.0f);
		glScalef(sizh,sizv,1.0f);

		glBindTexture(GL_TEXTURE_2D, font->ptex->GetTexNum() );

			glColor4f(red,green,blue,alpha);

			GLfloat x1, x2, y1, y2;
			int c;
			for( size_t ci = 0; ci < text.size(); ) {
				c = next_glyph(text,ci);
				x1 = font->glyphs_xshift[c];
				x2 = x1 + font->glyphs_width[c];
				y1 = font->glyphs_yshift[c];
				y2 = y1 + font->glyphs_height[c];

				LTex &ltex = *(font->ltex + c);
				glBegin(GL_QUADS);
					glTexCoord2f( ltex.Getfx1(), ltex.Getfy1() );		glVertex2f( x1, y1 );
					glTexCoord2f( ltex.Getfx2(), ltex.Getfy1() );		glVertex2f( x2,	y1 );
					glTexCoord2f( ltex.Getfx2(), ltex.Getfy2() );		glVertex2f( x2, y2 );
					glTexCoord2f( ltex.Getfx1(), ltex.Getfy2() );		glVertex2f( x1, y2 );
				glEnd();

				glTranslatef(font->glyphs_advance[c],0.0f,0.0f);

			}


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	EndBobBlits();
}

float TextLength(Font *font,const std::string& text,float sizh) {
	int len = text.length();
	if( len <= 0 ) {
		return 0.0f;
	}

	float ret=0.0f;
	int c;
	for( size_t ci = 0; ci < text.size(); ) {
		c = next_glyph(text,ci);
		ret += font->glyphs_advance[c] * sizh;
	}

	return ret;
}

void _render_font(Font *font) {
	BeginBobBlits();

	LTex ltex(font->ptex);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

		glTranslatef(0.0f, 0.0f, 0.0f);

		glBindTexture(GL_TEXTURE_2D, ltex.GetTextureID() );
		glColor4f(red,green,blue,alpha);
		glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glTexCoord2f( ltex.Getfx1(), ltex.Getfy1() );		glVertex2f( 0.0f, 0.0f );
			glTexCoord2f( ltex.Getfx2(), ltex.Getfy1() );		glVertex2f(  1024/2,	0.0f );
			glTexCoord2f( ltex.Getfx2(), ltex.Getfy2() );		glVertex2f(  1024/2,  1024/2 );
			glTexCoord2f( ltex.Getfx1(), ltex.Getfy2() );		glVertex2f( 0.0f,  1024/2 );
		glEnd();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();



	EndBobBlits();
}

};	//namespace Gfx

