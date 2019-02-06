/**
 * @file settings.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "settings.h"

#include "nvs_flash.h"
#include "esp_heap_caps.h"

#include "string.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const char* NvsNamespace = "esplay";
static const char* NvsKey_Backlight = "backlight";
static const char* NvsKey_Partition = "rom-partition";
static const char* NvsKey_RomName = "rom_name";
static const char* NvsKey_Volume = "volume";
static const char* NvsKey_MenuFlag = "menu_flag";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int32_t get_backlight_settings()
{
    // TODO: Move to header
    int result = 30;

    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    // Read
    err = nvs_get_i32(my_handle, NvsKey_Backlight, &result);
    if (err == ESP_OK)
    {
        printf("%s: value=%d\n", __func__, result);
    }

    // Close
    nvs_close(my_handle);

    return result;
}

void set_backlight_settings(int32_t value)
{
    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    printf("Try saving ... \n");

    // Read
    err = nvs_set_i32(my_handle, NvsKey_Backlight, value);
    if (err != ESP_OK) abort();

    printf("Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Close
    nvs_close(my_handle);
}

int8_t get_rom_partition_settings()
{
    // TODO: Move to header
    int result = -1;

    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    // Read
    err = nvs_get_i8(my_handle, NvsKey_Partition, &result);
    if (err == ESP_OK)
    {
        printf("%s: value=%d\n", __func__, result);
    }

    // Close
    nvs_close(my_handle);

    return result;
}

void set_rom_partition_settings(int8_t value)
{
    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    printf("Try saving ... \n");

    // Read
    err = nvs_set_i8(my_handle, NvsKey_Partition, value);
    if (err != ESP_OK) abort();

    printf("Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Close
    nvs_close(my_handle);
}

char* system_util_GetFileName(const char* path)
{
    int length = strlen(path);
    int fileNameStart = length;

    if (fileNameStart < 1) abort();

    while (fileNameStart > 0)
    {
        if (path[fileNameStart] == '/')
        {
            ++fileNameStart;
            break;
        }

        --fileNameStart;
    }

    int size = length - fileNameStart + 1;

    char* result = malloc(size);
    if (!result) abort();

    result[size - 1] = 0;
    for (int i = 0; i < size - 1; ++i)
    {
        result[i] = path[fileNameStart + i];
    }

    //printf("GetFileName: result='%s'\n", result);

    return result;
}

char* system_util_GetFileExtenstion(const char* path)
{
    // Note: includes '.'
    int length = strlen(path);
    int extensionStart = length;

    if (extensionStart < 1) abort();

    while (extensionStart > 0)
    {
        if (path[extensionStart] == '.')
        {
            break;
        }

        --extensionStart;
    }

    int size = length - extensionStart + 1;

    char* result = malloc(size);
    if (!result) abort();

    result[size - 1] = 0;
    for (int i = 0; i < size - 1; ++i)
    {
        result[i] = path[extensionStart + i];
    }

    //printf("GetFileExtenstion: result='%s'\n", result);

    return result;
}

char* system_util_GetFileNameWithoutExtension(const char* path)
{
    char* fileName = system_util_GetFileName(path);

    int length = strlen(fileName);
    int extensionStart = length;

    if (extensionStart < 1) abort();

    while (extensionStart > 0)
    {
        if (fileName[extensionStart] == '.')
        {
            break;
        }

        --extensionStart;
    }

    int size = extensionStart + 1;

    char* result = malloc(size);
    if (!result) abort();

    result[size - 1] = 0;
    for (int i = 0; i < size - 1; ++i)
    {
        result[i] = fileName[i];
    }

    free(fileName);

    //printf("GetFileNameWithoutExtension: result='%s'\n", result);

    return result;
}

char* get_rom_name_settings()
{
    char* result = NULL;

    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();


    // Read
    size_t required_size;
    err = nvs_get_str(my_handle, NvsKey_RomName, NULL, &required_size);
    if (err == ESP_OK)
    {
        char* value = malloc(required_size);
        if (!value) abort();

        err = nvs_get_str(my_handle, NvsKey_RomName, value, &required_size);
        if (err != ESP_OK) abort();

        result = value;

        printf("%s: value='%s'\n", __func__, value);
    }


    // Close
    nvs_close(my_handle);

    return result;
}

void set_rom_name_settings(char* value)
{
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    // Write key
    err = nvs_set_str(my_handle, NvsKey_RomName, value);
    if (err != ESP_OK) abort();

    // Close
    nvs_close(my_handle);
}

int8_t get_volume_settings()
{
    // TODO: Move to header
    int result = 1;

    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    // Read
    err = nvs_get_i8(my_handle, NvsKey_Volume, &result);
    if (err == ESP_OK)
    {
        printf("%s: value=%d\n", __func__, result);
    }

    // Close
    nvs_close(my_handle);

    return result;
}

void set_volume_settings(int8_t value)
{
    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    printf("Try saving ... \n");

    // Read
    err = nvs_set_i8(my_handle, NvsKey_Volume, value);
    if (err != ESP_OK) abort();

    printf("Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Close
    nvs_close(my_handle);
}

int8_t get_menu_flag_settings()
{
    // Default force to menu
    int result = 1;

    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    // Read
    err = nvs_get_i8(my_handle, NvsKey_MenuFlag, &result);
    if (err == ESP_OK)
    {
        printf("%s: value=%d\n", __func__, result);
    }

    // Close
    nvs_close(my_handle);

    return result;
}

void set_menu_flag_settings(int8_t value)
{
    // Open
    nvs_handle my_handle;
    esp_err_t err = nvs_open(NvsNamespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) abort();

    printf("Try saving ... \n");

    // Read
    err = nvs_set_i8(my_handle, NvsKey_MenuFlag, value);
    if (err != ESP_OK) abort();

    printf("Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    // Close
    nvs_close(my_handle);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
