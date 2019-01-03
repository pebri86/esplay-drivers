/**
 * @file st7735r.h
 *
 */

#ifndef __ST7735R_H__
#define __ST7735R_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if CONFIG_USE_LVGL
#include "../lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define ST7735R_HOR_RES	160
#define ST7735R_VER_RES	128

#define ST7735R_RST  CONFIG_HW_LCD_RESET_GPIO
#define ST7735R_BCKL CONFIG_HW_LCD_BL_GPIO

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void st7735r_init(void);
void st7735r_prepare();
void st7735r_poweroff();
int is_st_backlight_initialized();
void st_backlight_percentage_set(int value);

#if CONFIG_USE_LVGL
void st7735r_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color);
void st7735r_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_map);
#endif

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ST7735R_H*/
