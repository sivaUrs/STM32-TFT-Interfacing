#ifndef __BSP_LCD__H___
#define __BSP_LCD__H___

#include "stm32f407xx.h"

#define GPIO_PIN_0 				0U
#define GPIO_PIN_1 				1U
#define GPIO_PIN_2 				2U
#define GPIO_PIN_3 				3U
#define GPIO_PIN_4 				4U
#define GPIO_PIN_5 				5U
#define GPIO_PIN_6 				6U
#define GPIO_PIN_7 				7U
#define GPIO_PIN_8 				8U
#define GPIO_PIN_9 				9U
#define GPIO_PIN_10 			10U
#define GPIO_PIN_11 			11U
#define GPIO_PIN_12 			12U
#define GPIO_PIN_13 			13U
#define GPIO_PIN_14 			14U
#define GPIO_PIN_15 			15U

#define SPI SPI2

#define LCD_SCL_PIN  GPIO_PIN_13
#define LCD_SCL_PORT GPIOB

#define LCD_SDI_PIN  GPIO_PIN_15 //MOSI
#define LCD_SDI_PORT GPIOB

#define LCD_SDO_PIN  GPIO_PIN_2 //MISO
#define LCD_SDO_PORT GPIOC

#define LCD_RESX_PIN  GPIO_PIN_10
#define LCD_RESX_PORT GPIOD

#define LCD_CSX_PIN  GPIO_PIN_11
#define LCD_CSX_PORT GPIOD

#define LCD_DCX_PIN  GPIO_PIN_9
#define LCD_DCX_PORT GPIOD

typedef enum
{
	BLACK=0,
	WHITE,
	RED,
	GREEN,
	BLUE,
	VIOLET,
	INDIGO,
	YELLOW,
	ORANGE,
	COLOR_END
}COLOR;

typedef enum
{
	PORTRAIT = 0,
	LANDSCAPE
}ORIENTATION;

#define LANDSCAPE_ACTIVE_WIDTH 320
#define LANDSCAPE_ACTIVE_HEIGHT 240
#define PORTRAIT_ACTIVE_WIDTH 240
#define PORTRAIT_ACTIVE_HEIGHT 320
#define TOTAL_ACTIVE_AREA (240 * 320)
#define BUFF_SIZE  (1 * 1024)

typedef struct
{
	ORIENTATION orientation;
	union
	{
		uint8_t whole;
		struct {
			uint8_t unused:2;
			uint8_t mac_mh:1;
			uint8_t mac_bgr:1;
			uint8_t mac_ml:1;
			uint8_t mac_mv:1;
			uint8_t mac_mx:1;
			uint8_t mac_my:1;
		}bits;

	}mac_register;
}orientation_t;

typedef union word
{
	uint16_t whole;
	struct
	{
		uint8_t lsb;
		uint8_t msb;
	}bytes;

}word;

typedef struct
{
	word x1;
	word y1;
	word x2;
	word y2;
}lcd_area_t;

typedef union
{
	word bytes;
	struct
	{
		uint16_t b:5;
		uint16_t g:6;
		uint16_t r:5;
	}colors;

}rgb565_t;

typedef union
{
	uint16_t whole;
	struct
	{
		uint16_t b:8;
		uint16_t g:8;
		uint16_t r:8;
	}colors;
}rgb888_t;


typedef struct
{
	orientation_t ori;
	lcd_area_t area;

}lcd_context_t;

typedef struct
{
	COLOR color;
	rgb565_t pixel;
}bg_color_map_t;



void LCD_Init();
void LCD_Pin_Init();
void LCD_SPI_Init();
void LCD_SPI_Enable();
void LCD_Reset();
void LCD_Config();
void LCD_Write_Cmd(uint8_t cmd);
void LCD_send_write_gram_cmd();
void LCD_Write_Data(uint8_t *buffer, uint32_t len);
void LCD_display_area_set(const lcd_area_t *area);
void LCD_set_orientation(lcd_context_t *context);
void LCD_write_data_to_display(lcd_area_t *area, uint16_t *buffer, uint32_t len);
void LCD_fill_background(lcd_area_t *area, COLOR color);
void LCD_set_background(lcd_context_t *context,COLOR color);
void LCD_write_graphic_data(rgb565_t *buffer, uint32_t len);
void LCD_write_graphic_data_2x(const uint8_t *buffer8, uint32_t len);
void LCD_fill_color_with_area(lcd_context_t *context, COLOR *colors);
void LCD_display_image(const uint8_t *buffer, uint32_t len_in_pixels, const lcd_context_t *context);

#endif /* __BSP_LCD__H___ */
