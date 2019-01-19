#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "display.h"

#define NES_FRAME_WIDTH 256
#define NES_FRAME_HEIGHT 240

#define GB_FRAME_WIDTH 160
#define GB_FRAME_HEIGHT 144

#define U16x2toU32(m,l) ((((uint32_t)(l>>8|(l&0xFF)<<8))<<16)|(m>>8|(m&0xFF)<<8))
#define AVERAGE(a, b) ( ((((a) ^ (b)) & 0xf7deU) >> 1) + ((a) & (b)) )
#define ABS(a)          ( (a) >= 0 ? (a) : -(a) )

#define LINE_BUFFERS (2)
#define LINE_COUNT   (4)

uint16_t* line[LINE_BUFFERS];
extern uint16_t myPalette[];


void set_display_brightness(int percent)
{
#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
    backlight_percentage_set(percent);
#endif

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
    st_backlight_percentage_set(percent);
#endif
}

void display_prepare(int percent)
{
#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
    ili9341_prepare();
#endif

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
    st7735r_prepare();
#endif
}

void display_poweroff(int percent)
{
#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
    ili9341_poweroff();
#endif

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
    st7735r_poweroff();
#endif
}

static uint16_t averageSamples(const uint8_t * data[], int dx, int dy)
{
    uint16_t a,b;
    int y = dy*NES_FRAME_HEIGHT/LCD_HEIGHT;
    int x = dx*NES_FRAME_WIDTH/LCD_WIDTH;
    a = AVERAGE(myPalette[(unsigned char) (data[y][x])],myPalette[(unsigned char) (data[y][x + 1])]);
    b = AVERAGE(myPalette[(unsigned char) (data[y + 1][x])],myPalette[(unsigned char) (data[y + 1][x + 1])]);
    return AVERAGE(a,b);
}

void write_nes_frame(const uint8_t * data[])
{
    short x,y;
    uint16_t a,b;
    int sending_line=-1;
    int calc_line=0;
    for (y=0; y<LCD_HEIGHT; y++) {
        for (x=0; x<LCD_WIDTH; x++) {
            if (data == NULL)
            {
                line[calc_line][x] = 0;
            }
            else
            {
                a = averageSamples(data, x, y);
                b = averageSamples(data, x, y);
                line[calc_line][x]=U16x2toU32(a,b);
            }
        }
        if (sending_line!=-1) send_line_finish();
        sending_line=calc_line;
        calc_line=(calc_line==1)?0:1;
        send_lines(y, LCD_WIDTH, line[sending_line], 1);
    }
    send_line_finish();
}

/* Resize using bilinear interpolation */
static uint16_t getPixel(const uint16_t * bufs, int x, int y, int w1, int h1, int w2, int h2)
{
    int x_diff, y_diff, xv, yv, red , green, blue, col, a, b, c, d, index;
    int x_ratio = (int) (((w1-1)<<16)/w2) + 1;
    int y_ratio = (int) (((h1-1)<<16)/h2) + 1;

    xv = (int) ((x_ratio * x)>>16);
    yv = (int) ((y_ratio * y)>>16);

    x_diff = ((x_ratio * x)>>16) - (xv);
    y_diff = ((y_ratio * y)>>16) - (yv);

    index = yv*w1+xv ;

    a = bufs[index];
    b = bufs[index+1];
    c = bufs[index+w1];
    d = bufs[index+w1+1];

    red = (((a >> 11) & 0x1f) * (1-x_diff) * (1-y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1-y_diff) +
        ((c >> 11) & 0x1f) * (y_diff) * (1-x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff));

    green = (((a >> 5) & 0x3f) * (1-x_diff) * (1-y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1-y_diff) +
        ((c >> 5) & 0x3f) * (y_diff) * (1-x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff));

    blue = (((a) & 0x1f) * (1-x_diff) * (1-y_diff) + ((b) & 0x1f) * (x_diff) * (1-y_diff) +
        ((c) & 0x1f) * (y_diff) * (1-x_diff) + ((d) & 0x1f) * (x_diff * y_diff));

    col = ((int)red << 11) | ((int)green << 5) | ((int)blue);

    return col;
}

void write_gb_frame(const uint16_t * data, bool scale)
{
    short x,y;
    int sending_line=-1;
    int calc_line=0;

    if (data == NULL)
    {
        for (y=0; y<LCD_HEIGHT; ++y) {
            for (x=0; x<LCD_WIDTH; x++) {
                line[calc_line][x] = 0;
            }
            if (sending_line!=-1) send_line_finish();
            sending_line=calc_line;
            calc_line=(calc_line==1)?0:1;
            send_lines_ext(y, 0, LCD_WIDTH, line[sending_line], 1);
            }
        send_line_finish(); 
    }
    else
    {
        if (scale)
        {            
            int outputHeight = LCD_HEIGHT;
            int outputWidth = GB_FRAME_WIDTH + (LCD_HEIGHT - GB_FRAME_HEIGHT);
            int xpos = (LCD_WIDTH - outputWidth) / 2;

            for (y=0; y<outputHeight; y+=LINE_COUNT) 
            {
                for (int i = 0; i < LINE_COUNT; ++i)
                {
                    if((y + i) >= outputHeight) break;

                    int index = (i) * outputWidth;
                
                    for (x=0; x<outputWidth; ++x) 
                    {
                        /* use float on small lcd doesn't impact performance plus good quality image */
                        #if CONFIG_HW_LCD_TYPE == LCD_TYPE_ST
                        float x_ratio = ((float) ((GB_FRAME_WIDTH-1))/outputWidth);
                        float y_ratio = ((float) ((GB_FRAME_HEIGHT-1))/outputHeight);

                        int xv = (int) (x_ratio * x);
                        int yv = (int) (y_ratio * (y+i));

                        float x_diff = (x_ratio * x) - xv;
                        float y_diff = (y_ratio * (y+i)) - yv;

                        int k = yv*GB_FRAME_WIDTH+xv ;

                        int a = data[k];
                        int b = data[k+1];
                        int c = data[k+GB_FRAME_WIDTH];
                        int d = data[k+GB_FRAME_WIDTH+1];

                        int red = (((a >> 11) & 0x1f) * (1-x_diff) * (1-y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1-y_diff) +
                            ((c >> 11) & 0x1f) * (y_diff) * (1-x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff));

                        int green = (((a >> 5) & 0x3f) * (1-x_diff) * (1-y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1-y_diff) +
                            ((c >> 5) & 0x3f) * (y_diff) * (1-x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff));

                        int blue = (((a) & 0x1f) * (1-x_diff) * (1-y_diff) + ((b) & 0x1f) * (x_diff) * (1-y_diff) +
                            ((c) & 0x1f) * (y_diff) * (1-x_diff) + ((d) & 0x1f) * (x_diff * y_diff));

                        uint16_t sample = ((int)red << 11) | ((int)green << 5) | ((int)blue);
                        #else
                        /* on bigger lcd use float impact to performance so use bilinear using integer instead */
                        uint16_t sample = getPixel(data, x, (y+i), GB_FRAME_WIDTH, GB_FRAME_HEIGHT, outputWidth, outputHeight);
                        #endif
                        line[calc_line][index++]=((sample >> 8) | ((sample) << 8));
                    }
                }                
                if (sending_line!=-1) send_line_finish();
                sending_line=calc_line;
                calc_line=(calc_line==1)?0:1;
                send_lines_ext(y, xpos, outputWidth, line[sending_line], LINE_COUNT);
            }
            send_line_finish();
        }
        else
        {
            int ypos = (LCD_HEIGHT - GB_FRAME_HEIGHT)/2;
            int xpos = (LCD_WIDTH - GB_FRAME_WIDTH)/2;

            for (y=0; y<GB_FRAME_HEIGHT; y+=LINE_COUNT)
            {
                for (int i = 0; i < LINE_COUNT; ++i)
                {
                    if((y + i) >= GB_FRAME_HEIGHT) break;

                    int index = (i) * GB_FRAME_WIDTH;
                    int bufferIndex = ((y + i) * GB_FRAME_WIDTH);

                    for (x = 0; x < GB_FRAME_WIDTH; ++x)
                    {
                        uint16_t sample = data[bufferIndex++];
                        line[calc_line][index++] = ((sample >> 8) | ((sample & 0xff) << 8));
                    }
                }
                if (sending_line!=-1) send_line_finish();
                sending_line=calc_line;
                calc_line=(calc_line==1)?0:1;
                send_lines_ext(y+ypos, xpos, GB_FRAME_WIDTH, line[sending_line], LINE_COUNT);
            }
            send_line_finish();
        }
    }
}

void display_init()
{
    // Line buffers
    const size_t lineSize = LCD_WIDTH * LINE_COUNT * sizeof(uint16_t);
    for (int x = 0; x < LINE_BUFFERS; x++)
    {
        line[x] = heap_caps_malloc(lineSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!line[x]) abort();
    }
    // Initialize the LCD
    disp_spi_init();
#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
    st7735r_init();
#endif

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
    ili9341_init();
#endif
}
