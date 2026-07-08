#include "bsp_lcd.h"
#include "reg_util.h"
#include "ili9341_reg.h"

lcd_context_t g_lcd_context = {0};
#define BUFF_SIZE  (1 * 1024)
rgb565_t img_buff[BUFF_SIZE] = {0};

#define LCD_RESX_HIGH() REG_SET_BIT(LCD_RESX_PORT->ODR, LCD_RESX_PIN)
#define LCD_RESX_LOW() REG_CLR_BIT(LCD_RESX_PORT->ODR, LCD_RESX_PIN)

#define LCD_CSX_HIGH() REG_SET_BIT(LCD_CSX_PORT->ODR, LCD_CSX_PIN)
#define LCD_CSX_LOW() REG_CLR_BIT(LCD_CSX_PORT->ODR, LCD_CSX_PIN)

#define LCD_DCX_HIGH() REG_SET_BIT(LCD_DCX_PORT->ODR, LCD_DCX_PIN)
#define LCD_DCX_LOW() REG_CLR_BIT(LCD_DCX_PORT->ODR, LCD_DCX_PIN)

#define __enable_spi() REG_SET_BIT(SPI->CR1,SPI_CR1_SPE_Pos)
#define __disable_spi() do												\
						{												\
							while(REG_READ_BIT(SPI->SR, SPI_SR_BSY_Pos));	\
							REG_CLR_BIT(SPI->CR1, SPI_CR1_SPE_Pos);		\
						}while(0)
#define __spi_set_dff_8bit()  			REG_CLR_BIT(SPI->CR1,SPI_CR1_DFF_Pos)
#define __spi_set_dff_16bit()			REG_SET_BIT(SPI->CR1,SPI_CR1_DFF_Pos)

bg_color_map_t bg_color_map[COLOR_END]  = {
		{.color=BLACK,  .pixel={.colors={.r=0x00, 	.g=0x00, 	.b=0x00}}},
		{.color=WHITE,  .pixel={.colors={.r=0x1F, 	.g=0x3F, 	.b=0x1F}}},
		{.color=RED,    .pixel={.colors={.r=0x1F, 	.g=0x00, 	.b=0x00}}},
		{.color=GREEN,  .pixel={.colors={.r=0x00, 	.g=0x3F, 	.b=0x00}}},
		{.color=BLUE,   .pixel={.colors={.r=0x00, 	.g=0x00, 	.b=0x1F}}},
		{.color=VIOLET, .pixel={.colors={.r=0x1D, 	.g=0x0A, 	.b=0x1D}}},
		{.color=INDIGO, .pixel={.colors={.r=0x09, 	.g=0x00, 	.b=0x0A}}},
		{.color=YELLOW, .pixel={.colors={.r=0x1F, 	.g=0x3F, 	.b=0x00}}},
		{.color=ORANGE, .pixel={.colors={.r=0x1F, 	.g=0x20, 	.b=0x00}}}
};

rgb565_t get_color_pixel(COLOR color)
{
	for(uint8_t i = 0; i < (uint8_t)COLOR_END; i++ )
	{
		if(bg_color_map[i].color == color)
		{
			return bg_color_map[i].pixel;
		}
	}
	return bg_color_map[0].pixel;
}

void LCD_Pin_Init()
{
	RCC_TypeDef *pRCC = RCC;

	REG_SET_BIT(pRCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN_Pos);/*Enabling the clock to port B*/
	REG_SET_BIT(pRCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN_Pos);/*Enabling the clock to port C*/
	REG_SET_BIT(pRCC->AHB1ENR, RCC_AHB1ENR_GPIODEN_Pos);/*Enabling the clock to port D*/

	REG_SET_VAL(LCD_RESX_PORT->MODER, 0x1U, 0x3U, (LCD_RESX_PIN * 2)); // setting reset pin as output.
	REG_CLR_BIT(LCD_RESX_PORT->OTYPER, LCD_RESX_PIN); //Setting output type as push pull.
	REG_SET_VAL(LCD_RESX_PORT->OSPEEDR, 0x2U, 0x3U, LCD_RESX_PIN); //output speed as high speed.

	REG_SET_VAL(LCD_CSX_PORT->MODER, 0x1U, 0x3U, (LCD_CSX_PIN * 2));  // setting cs pin as output.
	REG_CLR_BIT(LCD_CSX_PORT->OTYPER, LCD_CSX_PIN); //Setting output type as push pull.
	REG_SET_VAL(LCD_CSX_PORT->OSPEEDR, 0x2U, 0x3U, LCD_CSX_PIN); //output speed as high speed.

	REG_SET_VAL(LCD_DCX_PORT->MODER, 0x1U, 0x3U, (LCD_DCX_PIN * 2));  // setting dc pin as output.
	REG_CLR_BIT(LCD_DCX_PORT->OTYPER, LCD_DCX_PIN); //Setting output type as push pull.
	REG_SET_VAL(LCD_DCX_PORT->OSPEEDR, 0x2U, 0x3U, LCD_DCX_PIN); //output speed as high speed.

	/*SPI Clock pin configuration*/
	REG_SET_VAL(LCD_SCL_PORT->MODER, 0x2U, 0x3U, (LCD_SCL_PIN * 2));  // setting SCL pin as alternate function
	REG_CLR_BIT(LCD_SCL_PORT->OTYPER, LCD_SCL_PIN); //Setting output type as push pull.
	REG_SET_VAL(LCD_SCL_PORT->OSPEEDR, 0x2U, 0x3U, (LCD_SCL_PIN * 2)); //output speed as high speed.
	REG_SET_VAL(LCD_SCL_PORT->AFR[1],0x5U, 0xFU, ((LCD_SCL_PIN % 8) * 4)); //Alternate function selection for SCK

	/*SDI pin configuration*/
	REG_SET_VAL(LCD_SDI_PORT->MODER, 0x2U, 0x3U, (LCD_SDI_PIN * 2));  // setting SDI pin as alternate function
	REG_CLR_BIT(LCD_SDI_PORT->OTYPER, LCD_SDI_PIN); //Setting output type as push pull.
	REG_SET_VAL(LCD_SDI_PORT->OSPEEDR, 0x2U, 0x3U, (LCD_SDI_PIN * 2)); //output speed as high speed.
	REG_SET_VAL(LCD_SDI_PORT->AFR[1],0x5U, 0xFU, ((LCD_SDI_PIN % 8) * 4)); //Alternate function selection for SDI

	/*SDO pin configuration*/
	REG_SET_VAL(LCD_SDO_PORT->MODER, 0x2U, 0x3U, (LCD_SDO_PIN * 2));  // setting SCL pin as alternate function
	REG_CLR_BIT(LCD_SDO_PORT->OTYPER, LCD_SDO_PIN); //Setting output type as push pull.
	REG_SET_VAL(LCD_SDO_PORT->OSPEEDR, 0x2U, 0x3U, LCD_SDO_PIN); //output speed as high speed.
	REG_SET_VAL(LCD_SDO_PORT->AFR[0],0x5U, 0xFU, (LCD_SDO_PIN * 4)); //Alternate function selection for SDI

	/*Keep initial state as high*/
	LCD_CSX_HIGH();
	LCD_RESX_HIGH();
	LCD_DCX_HIGH();
}
void LCD_SPI_Init()
{
	RCC_TypeDef *pRCC = RCC;

	REG_SET_BIT(pRCC->APB1ENR, RCC_APB1ENR_SPI2EN_Pos);

	REG_SET_BIT(SPI->CR1, SPI_CR1_MSTR_Pos); // Master mode selected as SPI master.
	REG_CLR_BIT(SPI->CR1, SPI_CR1_BIDIMODE_Pos);
	REG_CLR_BIT(SPI->CR1, SPI_CR1_DFF_Pos); // Data frame format 8 bit mode is selected of out of 8 and 16
	REG_SET_BIT(SPI->CR1, SPI_CR1_SSM_Pos); // software slave management is enabled
	REG_SET_BIT(SPI->CR1, SPI_CR1_SSI_Pos); // set to 1 to set NSS
	REG_CLR_BIT(SPI->CR1, SPI_CR1_LSBFIRST_Pos); //msb first mode selected
	REG_SET_VAL(SPI->CR1, 0x0U,0x7U,SPI_CR1_BR_Pos); //Baudrate = 42MHZ/2 = 21MHZ
	REG_CLR_BIT(SPI->CR1, SPI_CR1_CPOL_Pos); // clock polarity 0 when clock is idle
	REG_CLR_BIT(SPI->CR1, SPI_CR1_CPHA_Pos); // clock phase first clock transition edge is first data capture edge.
	REG_CLR_BIT(SPI->CR2, SPI_CR2_FRF_Pos); // Selected SPI Motorola mode.
}

void LCD_SPI_Enable()
{
	__enable_spi();
}

void delay_50ms()
{
	for(int i = 0; i < (0xFFFFU * 10U); i++);
}

void LCD_Reset()
{
	LCD_RESX_LOW();
	delay_50ms();
	LCD_RESX_HIGH();
	delay_50ms();
}
void LCD_Config()
{
	uint8_t params[15];
	LCD_Write_Cmd(ILI9341_SWRESET);
	LCD_Write_Cmd(ILI9341_POWERB);
	params[0] = 0x00;
	params[1] = 0xD9;
	params[2] = 0x30;
	LCD_Write_Data(params, 3);

	LCD_Write_Cmd(ILI9341_POWER_SEQ);
	params[0]= 0x64;
	params[1]= 0x03;
	params[2]= 0X12;
	params[3]= 0X81;
	LCD_Write_Data(params, 4);

	LCD_Write_Cmd(ILI9341_DTCA);
	params[0]= 0x85;
	params[1]= 0x10;
	params[2]= 0x7A;
	LCD_Write_Data(params, 3);

	LCD_Write_Cmd(ILI9341_POWERA);
	params[0]= 0x39;
	params[1]= 0x2C;
	params[2]= 0x00;
	params[3]= 0x34;
	params[4]= 0x02;
	LCD_Write_Data(params, 5);

	LCD_Write_Cmd(ILI9341_PRC);
	params[0]= 0x20;
	LCD_Write_Data(params, 1);

	LCD_Write_Cmd(ILI9341_DTCB);
	params[0]= 0x00;
	params[1]= 0x00;
	LCD_Write_Data(params, 2);

	LCD_Write_Cmd(ILI9341_POWER1);
	params[0]= 0x1B;
	LCD_Write_Data(params, 1);

	LCD_Write_Cmd(ILI9341_POWER2);
	params[0]= 0x12;
	LCD_Write_Data(params, 1);

	LCD_Write_Cmd(ILI9341_VCOM1);
	params[0]= 0x08;
	params[1]= 0x26;
	LCD_Write_Data(params, 2);

	LCD_Write_Cmd(ILI9341_VCOM2);
	params[0]= 0XB7;
	LCD_Write_Data(params, 1);

	LCD_Write_Cmd(ILI9341_PIXEL_FORMAT);
	params[0]= 0x55; //select RGB565
	LCD_Write_Data(params, 1);

	LCD_Write_Cmd(ILI9341_FRMCTR1);
	params[0]= 0x00;
	params[1]= 0x1B;//frame rate = 70
	LCD_Write_Data(params, 2);

	LCD_Write_Cmd(ILI9341_DFC);    // Display Function Control
	params[0]= 0x0A;
	params[1]= 0xA2;
	LCD_Write_Data(params, 2);

//	LCD_Write_Cmd(ILI9341_3GAMMA_EN);    // 3Gamma Function Disable
//	params[0]= 0x02; //LCD_WR_DATA(0x00);
//	LCD_Write_Data(params, 1);

//	LCD_Write_Cmd(ILI9341_GAMMA);
//	params[0]= 0x01;
//	LCD_Write_Data(params, 1);

//	LCD_Write_Cmd(ILI9341_PGAMMA);    //Set Gamma
//	params[0]= 0x0F;
//	params[1]= 0x1D;
//	params[2]= 0x1A;
//	params[3]= 0x0A;
//	params[4]= 0x0D;
//	params[5]= 0x07;
//	params[6]= 0x49;
//	params[7]= 0X66;
//	params[8]= 0x3B;
//	params[9]= 0x07;
//	params[10]= 0x11;
//	params[11]= 0x01;
//	params[12]= 0x09;
//	params[13]= 0x05;
//	params[14]= 0x04;
//	LCD_Write_Data(params, 15);
//
//	LCD_Write_Cmd(ILI9341_NGAMMA);
//	params[0]= 0x00;
//	params[1]= 0x18;
//	params[2]= 0x1D;
//	params[3]= 0x02;
//	params[4]= 0x0F;
//	params[5]= 0x04;
//	params[6]= 0x36;
//	params[7]= 0x13;
//	params[8]= 0x4C;
//	params[9]= 0x07;
//	params[10]= 0x13;
//	params[11]= 0x0F;
//	params[12]= 0x2E;
//	params[13]= 0x2F;
//	params[14]= 0x05;
//	LCD_Write_Data(params, 15);

	LCD_Write_Cmd(ILI9341_SLEEP_OUT); //Exit Sleep
	delay_50ms();
	delay_50ms();
	LCD_Write_Cmd(ILI9341_DISPLAY_ON); //display on
}

void LCD_Write_Cmd(uint8_t cmd)
{
	LCD_CSX_LOW();
	LCD_DCX_LOW();
	while(!REG_READ_BIT(SPI->SR, SPI_SR_TXE_Pos));
	REG_WRITE(SPI->DR, cmd);
	while(!REG_READ_BIT(SPI->SR, SPI_SR_TXE_Pos));
	while(REG_READ_BIT(SPI->SR, SPI_SR_BSY_Pos));
	LCD_DCX_HIGH();
	LCD_CSX_HIGH();
}

void LCD_Write_Data(uint8_t *buffer, uint32_t len)
{
	if(!buffer)
	{
		return;
	}

	LCD_CSX_LOW();

	for(int i = 0; i < len; i++)
	{
		while(!REG_READ_BIT(SPI->SR, SPI_SR_TXE_Pos));
		REG_WRITE(SPI->DR, buffer[i]);
		while(!REG_READ_BIT(SPI->SR, SPI_SR_TXE_Pos));
		while(REG_READ_BIT(SPI->SR, SPI_SR_BSY_Pos));
	}
	LCD_CSX_HIGH();
}

void LCD_display_area_set(const lcd_area_t *area)
{
	if(area == NULL)
	{
		return;
	}
	uint8_t params[4] = {0};

	params[0] = area->x1.bytes.msb;
	params[1] = area->x1.bytes.lsb;
	params[2] = area->x2.bytes.msb;
	params[3] = area->x2.bytes.lsb;
	LCD_Write_Cmd(ILI9341_CASET);
	LCD_Write_Data(params, 4);


	params[0] = area->y1.bytes.msb;
	params[1] = area->y1.bytes.lsb;
	params[2] = area->y2.bytes.msb;
	params[3] = area->y2.bytes.lsb;
	LCD_Write_Cmd(ILI9341_RASET);
	LCD_Write_Data(params, 4);

}

void LCD_set_orientation(lcd_context_t *context)
{
	if(context == NULL)
	{
		return;
	}

	LCD_Write_Cmd(ILI9341_MAC);
	if(context->ori.orientation == PORTRAIT)
	{
		context->ori.mac_register.whole = 0;
		context->ori.mac_register.bits.mac_mx = 1;
		context->ori.mac_register.bits.mac_my = 1;
		context->ori.mac_register.bits.mac_bgr = 0;
	}
	else if(context->ori.orientation == LANDSCAPE)
	{
		context->ori.mac_register.whole = 0;
		context->ori.mac_register.bits.mac_mv = 1;
		context->ori.mac_register.bits.mac_my = 1;
		context->ori.mac_register.bits.mac_bgr = 0;
	}
	LCD_Write_Data((uint8_t *)&(context->ori.mac_register.whole), 1);

}

void LCD_set_background(lcd_context_t *context, COLOR color)
{
	lcd_area_t area = {
			.x1={.whole = 0},
			.y1={.whole = 0},
			.x2={.whole = (context->ori.orientation == PORTRAIT ? PORTRAIT_ACTIVE_WIDTH : LANDSCAPE_ACTIVE_WIDTH) - 1},
			.y2={.whole = (context->ori.orientation == PORTRAIT ? PORTRAIT_ACTIVE_HEIGHT : LANDSCAPE_ACTIVE_HEIGHT) - 1}
	};
	LCD_display_area_set(&area);
	LCD_fill_background(&area, color);
}

void LCD_send_write_gram_cmd()
{
	LCD_Write_Cmd(ILI9341_GRAM);
}

void LCD_fill_background(lcd_area_t *area, COLOR color)
{

	LCD_Write_Cmd(ILI9341_GRAM);
	rgb565_t pixel = get_color_pixel(color);

	__disable_spi();
	__spi_set_dff_16bit();
	__enable_spi();

	LCD_CSX_LOW();

	for(uint32_t i = 0; i < TOTAL_ACTIVE_AREA ; i++)
	{
		while(!REG_READ_BIT(SPI->SR, SPI_SR_TXE_Pos));
		REG_WRITE(SPI->DR, pixel.bytes.whole);
	}

	__disable_spi();

	LCD_CSX_HIGH();
	__spi_set_dff_8bit();
	__enable_spi();

}

void LCD_display_image(const uint8_t *buffer, uint32_t len_in_pixels, const lcd_context_t *context)
{

	if(buffer == NULL || context == NULL)
	{
		return;
	}

	if(len_in_pixels > TOTAL_ACTIVE_AREA)
	{
		len_in_pixels = TOTAL_ACTIVE_AREA;
	}

#if 0
	uint32_t outer_loop = len_in_pixels/BUFF_SIZE; //outer loop runs with .pixels
	uint32_t remaining = len_in_pixels % BUFF_SIZE;
	uint32_t inner_loop = BUFF_SIZE * 2; //inner loop runs with bytes/
	uint32_t offset = 0;
	rgb565_t *out;
#endif

    LCD_display_area_set(&context->area);
    LCD_send_write_gram_cmd();

    LCD_write_graphic_data_2x(buffer, len_in_pixels);


#if 0

	for (uint32_t i = 0; i < outer_loop; i++)
	{
		out = img_buff;
		for(uint32_t j = 0; j < inner_loop; j+= 2)
		{
			out->bytes.bytes.lsb = buffer[offset + j];
			out->bytes.bytes.msb = buffer[offset + j + 1];
			out++;
		}
		LCD_write_graphic_data(img_buff, BUFF_SIZE);
		offset += inner_loop;
	}

	if( remaining > 0)
	{
		out = img_buff;
		for(uint32_t j = 0; j < (remaining * 2); j+=2)
		{
			out->bytes.bytes.lsb = buffer[offset + j];
			out->bytes.bytes.msb = buffer[offset + j + 1];
			out++;
		}
		LCD_write_graphic_data(img_buff, remaining);
	}
#endif
}

void LCD_write_graphic_data_2x(const uint8_t *buffer8, uint32_t len)
{
	__disable_spi();
	__spi_set_dff_16bit();
	__enable_spi();

	if(buffer8 == NULL)
	{
		return;
	}

	uint16_t *buffer16 = (uint16_t *)buffer8;


	LCD_CSX_LOW();

	for(int i = 0; i < len; i++)
	{
		while(!REG_READ_BIT(SPI->SR, SPI_SR_TXE_Pos));
		REG_WRITE(SPI->DR, *buffer16);
		buffer16++;

	}

	__disable_spi();

	LCD_CSX_HIGH();
	__spi_set_dff_8bit();
	__enable_spi();
}


void LCD_write_graphic_data(rgb565_t *buffer, uint32_t len)
{
	__disable_spi();
	__spi_set_dff_16bit();
	__enable_spi();


	LCD_CSX_LOW();

	for(int i = 0; i < len; i++)
	{
		while(!REG_READ_BIT(SPI->SR, SPI_SR_TXE_Pos));
		REG_WRITE(SPI->DR, buffer[i].bytes.whole);

	}

	__disable_spi();

	LCD_CSX_HIGH();
	__spi_set_dff_8bit();
	__enable_spi();
}




void LCD_Init()
{
	LCD_Pin_Init();
	LCD_SPI_Init();
	LCD_SPI_Enable();
	LCD_Reset();
	LCD_Config();

}





