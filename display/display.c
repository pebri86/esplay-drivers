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

static int bilinier_scaling(uint16_t * bufs, int pitch, int x, int y)
{
    int a,b,c,d,i,j;
    float x_diff, y_diff, red , green, blue, col;
    float x_ratio = ((float) (GB_FRAME_WIDTH-1))/(GB_FRAME_WIDTH + (LCD_HEIGHT - GB_FRAME_HEIGHT));
    float y_ratio = ((float) (GB_FRAME_HEIGHT-1))/LCD_HEIGHT;

    i = (int) (x_ratio * x);
    j = (int) (y_ratio * y);

    x_diff = (x_ratio * x) - i;
    y_diff = (y_ratio * y) - j;
    
    a=bufs[i+(j*(pitch))];
    b=bufs[(i+1)+(j*(pitch))];
    c=bufs[i+((j+1)*(pitch))];
    d=bufs[(i+1)+((j+1)*(pitch))];

    red = ((a >> 11) & 0x1f) * (1-x_diff) * (1-y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1-y_diff) +
        ((c >> 11) & 0x1f) * (y_diff) * (1-x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff);

    green = ((a >> 5) & 0x3f) * (1-x_diff) * (1-y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1-y_diff) +
        ((c >> 5) & 0x3f) * (y_diff) * (1-x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff);

    blue = ((a) & 0x1f) * (1-x_diff) * (1-y_diff) + ((b) & 0x1f) * (x_diff) * (1-y_diff) +
        ((c) & 0x1f) * (y_diff) * (1-x_diff) + ((d) & 0x1f) * (x_diff * y_diff);

    col = ((int)red << 11) | ((int)green << 5) | ((int)blue);

    return col;
}

//Averages four pixels into one
static int getAvgPix(uint16_t* bufs, int pitch, int x, int y) 
{
    int col, p1, p2, p3, p4, d1, d2;
    if (x<0 || x>=LCD_WIDTH) return 0;
    //16-bit: E79C
    //15-bit: 739C
    /*
    col=(bufs[x+(y*(pitch>>1))]&0xE79C)>>2;
    col+=(bufs[(x+1)+(y*(pitch>>1))]&0xE79C)>>2;
    col+=(bufs[x+((y+1)*(pitch>>1))]&0xE79C)>>2;
    col+=(bufs[(x+1)+((y+1)*(pitch>>1))]&0xE79C)>>2;
    */    
    p1=bufs[x+(y*(pitch))];
    p2=bufs[(x+1)+(y*(pitch))];
    p3=bufs[x+((y+1)*(pitch))];
    p4=bufs[(x+1)+((y+1)*(pitch))];

    d1 = AVERAGE(p1,p2);
    d2 = AVERAGE(p3,p4);
    col = AVERAGE(d1,d2);

    return col&0xffff;
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
                        uint16_t sample = bilinier_scaling(data, GB_FRAME_WIDTH, x, (y+i));
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
