#include "tftjp.h"

int xOffset = 0;
int yOffset = 0;

void TFT_print_w(char *st) {
	
	RomFontx fx(ILGH24XB,ILGZ24XB);
	
	const uint8_t *p;
	uint8_t w,h;
	
	Utf8Decoder d;
	uint32_t ucode;

	for(char *s = st; *s; s++){
		
		if(*s == '\r'){
		}
		else if(*s == '\n'){
			TFT_X = 0;
			TFT_Y += FONTSIZE;
		}
		else if(d.decode(*s, &ucode)){
			if(!fx.getGlyph(ucode, &p, &w, &h)){
				printf("getGlyph failed. code:%x\n",ucode);
			} else {
				
				if(TFT_X > dispWin.x2){
					TFT_X = 0;
					TFT_Y += h;
				}

				uint8_t ch = 0;
				int i, j;
				int cx, cy;
				int len, bufPos;
				
				
				uint8_t mask = 0x80;
				disp_select();
				for (j=0; j < h; j++) {
					for (i=0; i < w; i++) {
						ch = p[i/8];
						if(ch & (0x80 >> (i % 8))){
							cx = (uint16_t)(TFT_X+i);
							cy = (uint16_t)(TFT_Y+j);
							TFT_drawPixel(cx, cy, _fg, 0);
						}else{
							//
						}
					}
					p += (w + 7)/8;
				}
				disp_deselect();
				
			}
			TFT_X += w;
		}
	}

}

void TFT_reset(){
	
	TFT_fillWindow(TFT_BLACK);
	TFT_X = 0;
	TFT_Y = 0;
}
	
	