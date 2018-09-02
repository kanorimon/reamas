#ifndef _TFTJP_H_
#define _TFTJP_H_

#include <stdio.h>
extern "C"{
#include "esp_system.h"
#include "tft.h"
#include "ILGZ24XB.h"
#include "ILGH24XB.h"
}

#include "Fontx.h"
#include "Utf8Decoder.h"

#define FONTSIZE 24

void TFT_print_w(char *st);
void TFT_reset();

#endif
