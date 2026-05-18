/*****************************************************************************
* | File      	:   EPD_4in26g.h
* | Author      :   Waveshare team
* | Function    :   4in26 e-paper (G)
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-12-22
* | Info        :
* -----------------------------------------------------------------------------
******************************************************************************/
#ifndef __EPD_4IN26G_H_
#define __EPD_4IN26G_H_

#include "DEV_Config.h"

// Display resolution
#define EPD_4IN26G_WIDTH        800
// Initialization must use this resolution
#define EPD_4IN26G_HEIGHT_INIT  680
#define EPD_4IN26G_HEIGHT       480

// Color
#define  EPD_4IN26G_BLACK   0x0
#define  EPD_4IN26G_WHITE   0x1
#define  EPD_4IN26G_YELLOW  0x2
#define  EPD_4IN26G_RED     0x3

void EPD_4IN26G_Init(void);
void EPD_4IN26G_Init_Fast(void);
void EPD_4IN26G_Clear(const UBYTE color);
void EPD_4IN26G_Display(const UBYTE *Image);
void EPD_4IN26G_Sleep(void);

#endif
