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
#include "hourglass_empty_black_48dp.h"
#include "image_splash.h"

// NES
#define NES_FRAME_WIDTH 256
#define NES_FRAME_HEIGHT 224

// GB
#define GB_FRAME_WIDTH 160
#define GB_FRAME_HEIGHT 144

// SMS
#define SMS_FRAME_WIDTH 256
#define SMS_FRAME_HEIGHT 192

#define GAMEGEAR_FRAME_WIDTH 160
#define GAMEGEAR_FRAME_HEIGHT 144

#define PIXEL_MASK  (0x1F)

#define AVERAGE(a, b) ( ((((a) ^ (b)) & 0xf7deU) >> 1) + ((a) & (b)) )

#define LINE_BUFFERS (2)
#define LINE_COUNT   (4)

uint16_t* line[LINE_BUFFERS];
extern uint16_t myPalette[];
static uint16_t getPixel(const uint16_t * bufs, int x, int y, int w1, int h1, int w2, int h2);

void set_display_brightness(int percent)
{
    backlight_percentage_set(percent);
}

void backlight_deinit()
{
    ili9341_backlight_deinit();
}

void display_prepare(int percent)
{
    ili9341_prepare();
}

void display_poweroff(int percent)
{
    ili9341_poweroff();
}

/* Box Filter Scaling */
static uint16_t getPixelNes(const uint8_t * data[], int x, int y, int w1, int h1, int w2, int h2)
{
    uint16_t a,b;
    int dy = y*h1/h2;
    int dx = x*w1/w2;
    a = AVERAGE(myPalette[(unsigned char) (data[dy][dx])],myPalette[(unsigned char) (data[dy][dx + 1])]);
    b = AVERAGE(myPalette[(unsigned char) (data[dy + 1][dx])],myPalette[(unsigned char) (data[dy + 1][dx + 1])]);
    return AVERAGE(a,b);
}

void write_nes_frame(const uint8_t * data)
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
        /* place output on center of lcd */
        int outputHeight = NES_FRAME_HEIGHT;
        int outputWidth = NES_FRAME_WIDTH;
        int xpos = (LCD_WIDTH - outputWidth) / 2;
        int ypos = (LCD_HEIGHT - outputHeight) / 2;

        for (y=ypos; y<outputHeight; y+=LINE_COUNT) 
        {
            for (int i = 0; i < LINE_COUNT; ++i)
            {
                if((y + i) >= outputHeight) break;

                int index = (i) * outputWidth;
                int bufferIndex = ((y + i) * NES_FRAME_WIDTH);

                for (x=0; x<outputWidth; x++)
                {
                    line[calc_line][index++] = myPalette[(unsigned char) (data[bufferIndex++])];
                }
            }
            if (sending_line!=-1) send_line_finish();
            sending_line=calc_line;
            calc_line=(calc_line==1)?0:1;
            send_lines_ext(y, xpos, outputWidth, line[sending_line], LINE_COUNT);
        }
        send_line_finish();
    }
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
                       
                        uint16_t sample = getPixel(data, x, (y+i), GB_FRAME_WIDTH, GB_FRAME_HEIGHT, outputWidth, outputHeight);
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

static uint8_t getPixelSms(const uint8_t * bufs, int x, int y, int w1, int h1, int w2, int h2, bool isGameGear)
{
    int x_diff, y_diff, xv, yv, red , green, blue, col, a, b, c, d, index;
    int x_ratio = (int) (((w1-1)<<16)/w2) + 1;
    int y_ratio = (int) (((h1-1)<<16)/h2) + 1;

    xv = (int) ((x_ratio * x)>>16);
    yv = (int) ((y_ratio * y)>>16);

    x_diff = ((x_ratio * x)>>16) - (xv);
    y_diff = ((y_ratio * y)>>16) - (yv);

    if (isGameGear)
    {
        index = yv*256+xv+48;
    }
    else
    {
        index = yv*w1+xv ;
    }

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

void write_sms_frame(const uint8_t * data, uint16_t color[], bool isGameGear, bool scale)
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
        if(!isGameGear)
        {
            if (scale)
            {
                int outputHeight = LCD_HEIGHT;
                int outputWidth = SMS_FRAME_WIDTH + (LCD_HEIGHT - SMS_FRAME_HEIGHT);
                int xpos = (LCD_WIDTH - outputWidth) / 2;

                for (y=0; y<outputHeight; y+=LINE_COUNT) 
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if((y + i) >= outputHeight) break;

                        int index = (i) * outputWidth;
                        
                        for (x=0; x<outputWidth; ++x) 
                        {
                               
                            uint16_t sample = color[getPixelSms(data, x, (y+i), SMS_FRAME_WIDTH, SMS_FRAME_HEIGHT, outputWidth, outputHeight, isGameGear) & PIXEL_MASK];
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
                int outputHeight = SMS_FRAME_HEIGHT;
                int outputWidth = SMS_FRAME_WIDTH;
                int xpos = (LCD_WIDTH - outputWidth) / 2;
                int ypos = (LCD_HEIGHT - outputHeight) / 2;

                for (y=0; y<outputHeight; y+=LINE_COUNT)
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if((y + i) >= outputHeight) break;

                        int index = (i) * outputWidth;
                        int bufferIndex = ((y + i) * SMS_FRAME_WIDTH);
                        
                        for (x=0; x<outputWidth; ++x) 
                        {
                               
                            uint16_t sample = color[data[bufferIndex++] & PIXEL_MASK];
                            line[calc_line][index++]=((sample >> 8) | ((sample) << 8));
                        }
                    }                
                    if (sending_line!=-1) send_line_finish();
                    sending_line=calc_line;
                    calc_line=(calc_line==1)?0:1;
                    send_lines_ext(ypos+y, xpos, outputWidth, line[sending_line], LINE_COUNT);
                }
                send_line_finish();
            }
        }
        else
        {
            if (scale)
            {
                int outputHeight = LCD_HEIGHT;
                int outputWidth = GAMEGEAR_FRAME_WIDTH + (LCD_HEIGHT - GAMEGEAR_FRAME_HEIGHT);
                int xpos = (LCD_WIDTH - outputWidth) / 2;

                for (y=0; y<outputHeight; y+=LINE_COUNT) 
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if((y + i) >= outputHeight) break;

                        int index = (i) * outputWidth;
                        
                        for (x=0; x<outputWidth; ++x) 
                        {
                               
                            uint16_t sample = color[getPixelSms(data, x, (y+i), GAMEGEAR_FRAME_WIDTH, GAMEGEAR_FRAME_HEIGHT, outputWidth, outputHeight, isGameGear) & PIXEL_MASK];
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
                int outputHeight = GAMEGEAR_FRAME_HEIGHT;
                int outputWidth = GAMEGEAR_FRAME_WIDTH;
                int xpos = (LCD_WIDTH - outputWidth) / 2;
                int ypos = (LCD_HEIGHT - outputHeight) / 2;

                for (y=0; y<outputHeight; y+=LINE_COUNT)
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if((y + i) >= outputHeight) break;

                        int index = (i) * outputWidth;
                        int bufferIndex = ((y + i) * 256) + 48;;
                        
                        for (x=0; x<outputWidth; ++x) 
                        {
                               
                            uint16_t sample = color[data[bufferIndex++] & PIXEL_MASK];
                            line[calc_line][index++]=((sample >> 8) | ((sample) << 8));
                        }
                    }                
                    if (sending_line!=-1) send_line_finish();
                    sending_line=calc_line;
                    calc_line=(calc_line==1)?0:1;
                    send_lines_ext(ypos+y, xpos, outputWidth, line[sending_line], LINE_COUNT);
                }
                send_line_finish();
            }
        }
    }
}

void write_frame_rectangleLE(short left, short top, short width, short height, uint16_t* buffer)
{
    short x, y;
    int sending_line=-1;
    int calc_line=0;

    if (left < 0 || top < 0) abort();
    if (width < 1 || height < 1) abort();
    if (buffer == NULL)
    {
        for (y=0; y<LCD_HEIGHT; ++y) 
        {
            for (x=0; x<LCD_WIDTH; x++)
            {
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
        short xv;
        short yv = 0;
        for (y = top; y < top+height; y++)
        {
            xv = 0;
            for (int i = left; i < left+width; ++i)
            {
                uint16_t pixel = buffer[yv * width + xv];
                line[calc_line][xv] = ((pixel << 8) | (pixel >> 8));
                xv++;
            }

            if (sending_line!=-1) send_line_finish();
            sending_line=calc_line;
            calc_line=(calc_line==1)?0:1;
            send_lines_ext(y, left, width, line[sending_line], 1);
            yv++;
        }
    }
    
    send_line_finish();
}

void display_show_hourglass()
{
    write_frame_rectangleLE((LCD_WIDTH / 2) - (image_hourglass_empty_black_48dp.width / 2),
        (LCD_HEIGHT / 2) - (image_hourglass_empty_black_48dp.height / 2),
        image_hourglass_empty_black_48dp.width,
        image_hourglass_empty_black_48dp.height,
        image_hourglass_empty_black_48dp.pixel_data);
}

void display_show_splash()
{
    write_frame_rectangleLE(0, 0, image_splash.width, image_splash.height, image_splash.pixel_data);
}

void display_clear(uint16_t color)
{
    int sending_line=-1;
    int calc_line=0;
    // clear the buffer
    for (int i = 0; i < LINE_BUFFERS; ++i)
    {
        for (int j = 0; j < LCD_WIDTH * LINE_COUNT; ++j)
        {
            line[i][j] = color;
        }
    }

    for (int y = 0; y < LCD_HEIGHT; y += LINE_COUNT)
    {
        if (sending_line!=-1) send_line_finish();
        sending_line=calc_line;
        calc_line=(calc_line==1)?0:1;
        send_lines_ext(y, 0, LCD_WIDTH, line[sending_line], LINE_COUNT);
    }

    send_line_finish();
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
    ili9341_init();
}
