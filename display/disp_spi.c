/**
 * @file disp_spi.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "disp_spi.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/
/*Clock out at 40 MHz*/
#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
#define SPI_CLOCK_SPEED 40000000
#endif

/*ST7735 can't handle correctly at 40 Mhz speed*/
#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
#define SPI_CLOCK_SPEED 26000000
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static spi_device_handle_t spi;
static void disp_spi_pre_transfer_callback(spi_transaction_t *t);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void disp_spi_init(void)
{
    gpio_set_direction(DISP_SPI_DC, GPIO_MODE_OUTPUT);
    esp_err_t ret;

    spi_bus_config_t buscfg={
        .miso_io_num=-1,
        .mosi_io_num=DISP_SPI_MOSI,
        .sclk_io_num=DISP_SPI_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    spi_device_interface_config_t devcfg={
        .clock_speed_hz=SPI_CLOCK_SPEED,
        .mode=0,                                //SPI mode 0
        .spics_io_num=DISP_SPI_CS,              //CS pin
        .queue_size=7,
        .pre_cb=disp_spi_pre_transfer_callback,
        .post_cb=NULL,
        .flags=SPI_DEVICE_NO_DUMMY
    };

    //Initialize the SPI bus
    ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);

    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
    assert(ret==ESP_OK);
}

void send_lines(int ypos, int width, uint16_t *linedata)
{
    esp_err_t ret;
    int x;
    static spi_transaction_t trans[6];

    for (x=0; x<6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x&1)==0) {
            //Even transfers are commands
            trans[x].length=8;
            trans[x].user=(void*)0;
        } else {
            //Odd transfers are data
            trans[x].length=8*4;
            trans[x].user=(void*)1;
        }
        trans[x].flags=SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0]=0x2A;           //Column Address Set
    trans[1].tx_data[0]=0;              //Start Col High
    trans[1].tx_data[1]=0;              //Start Col Low
    trans[1].tx_data[2]=(width)>>8;     //End Col High
    trans[1].tx_data[3]=(width)&0xff;   //End Col Low
    trans[2].tx_data[0]=0x2B;           //Page address set
    trans[3].tx_data[0]=ypos>>8;        //Start page high
    trans[3].tx_data[1]=ypos&0xff;      //start page low
    trans[3].tx_data[2]=ypos>>8;        //end page high
    trans[3].tx_data[3]=ypos&0xff;      //end page low
    trans[4].tx_data[0]=0x2C;           //memory write
    trans[5].tx_buffer=linedata;        //finally send the line data
    trans[5].length=width*2*8;          //Data length, in bits
    trans[5].flags=0; //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x=0; x<6; x++) {
        ret=spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
        assert(ret==ESP_OK);
    }
}

void send_line_finish(void)
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x=0; x<6; x++) {
        ret=spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret==ESP_OK);
    }
}

void disp_spi_send(uint8_t * data, uint16_t length, int dc)
{
    if (length == 0) return;           //no need to send anything

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));           //Zero out the transaction
    t.length = length * 8;              //Length is in bytes, transaction length is in bits.
    t.tx_buffer = data;                 //Data
    t.user=(void*)dc;

    spi_device_queue_trans(spi, &t, portMAX_DELAY);

    spi_transaction_t * rt;
    spi_device_get_trans_result(spi, &rt, portMAX_DELAY);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void disp_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(DISP_SPI_DC, dc);
}
