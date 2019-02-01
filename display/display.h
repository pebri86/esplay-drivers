#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <stdint.h>
#include "disp_spi.h"

//*****************************************************************************
//
// Make sure all of the definitions in this header have a C binding.
//
//*****************************************************************************

#ifdef __cplusplus
extern "C"
{
#endif

#include "ili9341.h"
#define LCD_WIDTH       ILI9341_HOR_RES
#define LCD_HEIGHT      ILI9341_VER_RES
#define DPI             100

void display_init();
void write_nes_frame(const uint8_t * data);
void write_gb_frame(const uint16_t * data, bool scale);
void write_frame_rectangleLE(short left, short top, short width, short height, uint16_t* buffer);
void display_show_hourglass();
void display_show_splash();
void set_display_brightness(int percent);
void display_prepare();
void display_poweroff();

#ifdef __cplusplus
}
#endif

#endif /*_DISPLAY_H_*/
