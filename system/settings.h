/**
 * @file settings.h
 *
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int32_t get_backlight_settings();
void set_backlight_settings(int32_t value);
int8_t get_rom_partition_settings();
void set_rom_partition_settings(int8_t value);
char* system_util_GetFileName(const char* path);
char* system_util_GetFileExtenstion(const char* path);
char* system_util_GetFileNameWithoutExtension(const char* path);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SETTINGS_H*/
