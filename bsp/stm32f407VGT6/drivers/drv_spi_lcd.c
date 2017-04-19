#include "tft_lcd.h"
#include "board.h"
void              LCD_IO_Init(void);
void              LCD_IO_WriteData(uint8_t Data);
void              LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size);
void              LCD_IO_WriteReg(uint8_t LCDReg);
void              LCD_Delay(uint32_t delay);
static void 			SPIx_Init(void);
static void 			SPIx_Error (void);
extern void RT_HAL_Delay(__IO uint32_t Delay);


#ifdef HAL_SPI_MODULE_ENABLED
uint32_t SpixTimeout = NUCLEO_SPIx_TIMEOUT_MAX; /*<! Value of Timeout when SPI communication fails */
static SPI_HandleTypeDef hnucleo_Spi;
#endif
#ifdef HAL_SPI_MODULE_ENABLED

/**
  * @brief  Initializes SPI MSP.
  */
static void SPIx_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef  GPIO_InitStruct;  
  
  /*** Configure the GPIOs ***/ 
	NUCLEO_SPIx_CLK_ENABLE();	
  /* Enable GPIO clock */
  NUCLEO_SPIx_SCK_GPIO_CLK_ENABLE();
  NUCLEO_SPIx_MISO_MOSI_GPIO_CLK_ENABLE();
  
  /* Configure SPI SCK */
  GPIO_InitStruct.Pin = NUCLEO_SPIx_SCK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = NUCLEO_SPIx_SCK_AF;
  HAL_GPIO_Init(NUCLEO_SPIx_SCK_GPIO_PORT, &GPIO_InitStruct);

  /* Configure SPI MISO and MOSI */ 
  GPIO_InitStruct.Pin = NUCLEO_SPIx_MOSI_PIN;
  GPIO_InitStruct.Alternate = NUCLEO_SPIx_MISO_MOSI_AF;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  HAL_GPIO_Init(NUCLEO_SPIx_MISO_MOSI_GPIO_PORT, &GPIO_InitStruct);
  
 /* GPIO_InitStruct.Pin = NUCLEO_SPIx_MISO_PIN;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  HAL_GPIO_Init(NUCLEO_SPIx_MISO_MOSI_GPIO_PORT, &GPIO_InitStruct);*/

  /*** Configure the SPI peripheral ***/ 
  /* Enable SPI clock */
  NUCLEO_SPIx_CLK_ENABLE();
}

/**
  * @brief  Initializes SPI HAL.
  */
static void SPIx_Init(void)
{
  //if(HAL_SPI_GetState(&hnucleo_Spi) == HAL_SPI_STATE_RESET)
  //{
    /* SPI Config */
    //hnucleo_Spi.Instance = NUCLEO_SPIx;
      /* SPI baudrate is set to 12,5 MHz maximum (APB1/SPI_BaudRatePrescaler = 100/8 = 12,5 MHz) 
       to verify these constraints:
          - ST7735 LCD SPI interface max baudrate is 15MHz for write and 6.66MHz for read
            Since the provided driver doesn't use read capability from LCD, only constraint 
            on write baudrate is considered.
          - SD card SPI interface max baudrate is 25MHz for write/read
          - PCLK2 max frequency is 100 MHz 
       */ 
    hnucleo_Spi.Instance = SPI1;
		hnucleo_Spi.Init.Mode = SPI_MODE_MASTER;
		hnucleo_Spi.Init.Direction = SPI_DIRECTION_1LINE;
		hnucleo_Spi.Init.DataSize = SPI_DATASIZE_8BIT;
		hnucleo_Spi.Init.CLKPolarity = SPI_POLARITY_LOW;
		hnucleo_Spi.Init.CLKPhase = SPI_PHASE_1EDGE;
		hnucleo_Spi.Init.NSS = SPI_NSS_HARD_OUTPUT;
		hnucleo_Spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
		hnucleo_Spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
		hnucleo_Spi.Init.TIMode = SPI_TIMODE_DISABLED;
		hnucleo_Spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
		hnucleo_Spi.Init.CRCPolynomial = 10;
		HAL_SPI_Init(&hnucleo_Spi);

    SPIx_MspInit(&hnucleo_Spi);
    HAL_SPI_Init(&hnucleo_Spi);
 // }
	
}
#if 0
static void SPIx_WriteReadData(const uint8_t *DataIn, uint8_t *DataOut, uint16_t DataLegnth)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_SPI_TransmitReceive(&hnucleo_Spi, (uint8_t*) DataIn, DataOut, DataLegnth, SpixTimeout);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Execute user timeout callback */
   SPIx_Error();
  }
}
#endif
/**
  * @brief  SPI Write a byte to device.
  * @param  Value: value to be written
  */
static void SPIx_Write(uint8_t Value)
{
  HAL_StatusTypeDef status = HAL_OK;
  //uint8_t data;

  status = HAL_SPI_Transmit(&hnucleo_Spi, (uint8_t*) &Value, 1, SpixTimeout);
    
  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Execute user timeout callback */
    SPIx_Error();
  }
}

/**
  * @brief  SPI error treatment function.
  */
static void SPIx_Error (void)
{
  /* De-initialize the SPI communication BUS */
  HAL_SPI_DeInit(&hnucleo_Spi);
  
  /* Re-Initiaize the SPI communication BUS */
  SPIx_Init();
}

/********************************* LINK LCD ***********************************/
/**
  * @brief  Initializes the LCD.
  */
void LCD_IO_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
   
  /* LCD_CS_GPIO and LCD_DC_GPIO Periph clock enable */
  LCD_CS_GPIO_CLK_ENABLE();
  LCD_DC_GPIO_CLK_ENABLE();
	LCD_RST_GPIO_CLK_ENABLE();
  
  /* Configure LCD_CS_PIN pin: LCD Card CS pin */
  GPIO_InitStruct.Pin = LCD_CS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(LCD_CS_GPIO_PORT, &GPIO_InitStruct);

  /* Configure LCD_DC_PIN pin: LCD Card DC pin */
  GPIO_InitStruct.Pin = LCD_DC_PIN;
  HAL_GPIO_Init(LCD_DC_GPIO_PORT, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = LCD_BL_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(LCD_BL_GPIO_PORT, &GPIO_InitStruct);
	
	
	GPIO_InitStruct.Pin = LCD_RST_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(LCD_RST_GPIO_PORT, &GPIO_InitStruct);
  
  /* LCD chip select high */
  LCD_CS_HIGH();
  
  /* LCD SPI Config */
  SPIx_Init();
}

/**
  * @brief  Writes command to select the LCD register.
  * @param  LCDReg: Address of the selected register.
  */
void LCD_IO_WriteReg(uint8_t LCDReg)
{
  /* Reset LCD control line CS */
  LCD_CS_LOW();
  
  /* Set LCD data/command line DC to Low */
  LCD_DC_LOW();
    
  /* Send Command */
  SPIx_Write(LCDReg);
  
  /* Deselect : Chip Select high */
  LCD_CS_HIGH();
}

/**
  * @brief  Writes data to select the LCD register.
  *         This function must be used after st7735_WriteReg() function
  * @param  Data: data to write to the selected register.
  */
void LCD_IO_WriteData(uint8_t Data)
{
  /* Reset LCD control line CS */
  LCD_CS_LOW();
  
  /* Set LCD data/command line DC to High */
  LCD_DC_HIGH();

  /* Send Data */
  SPIx_Write(Data);
  
  /* Deselect : Chip Select high */
  LCD_CS_HIGH();
}

/**
  * @brief  Writes register value.
  * @param  pData: Pointer on the register value
  * @param  Size: Size of byte to transmit to the register
  */
void LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size)
{
  uint32_t counter = 0;
  __IO uint32_t data = 0;
  
  /* Reset LCD control line CS */
  LCD_CS_LOW();
  
  /* Set LCD data/command line DC to High */
  LCD_DC_HIGH();

  if (Size == 1)
  {
    /* Only 1 byte to be sent to LCD - general interface can be used */
    /* Send Data */
    SPIx_Write(*pData);
  }
  else
  {
    /* Several data should be sent in a raw */
    /* Direct SPI accesses for optimization */
    for (counter = Size; counter != 0; counter--)
    {
      while(((hnucleo_Spi.Instance->SR) & SPI_FLAG_TXE) != SPI_FLAG_TXE)
      {
      }
      /* Need to invert bytes for LCD*/
      *((__IO uint8_t*)&hnucleo_Spi.Instance->DR) = *(pData+1);
      
      while(((hnucleo_Spi.Instance->SR) & SPI_FLAG_TXE) != SPI_FLAG_TXE)
      {
      }
      *((__IO uint8_t*)&hnucleo_Spi.Instance->DR) = *pData;
      counter--;
      pData += 2;
      }
  
    /* Wait until the bus is ready before releasing Chip select */ 
    while(((hnucleo_Spi.Instance->SR) & SPI_FLAG_BSY) != RESET)
    {
    } 
  } 

  /* Empty the Rx fifo */
  data = *(&hnucleo_Spi.Instance->DR);
  UNUSED(data);

  /* Deselect : Chip Select high */
  LCD_CS_HIGH();
}

/**
  * @brief  Wait for loop in ms.
  * @param  Delay in ms.
  */
void LCD_Delay(uint32_t Delay)
{
 //HAL_Delay(Delay);
	RT_HAL_Delay(Delay);
}

void LCD_Reset(void)
{
	LCD_RST_LOW();
	LCD_Delay(100);
	LCD_RST_HIGH();
	LCD_Delay(50);
}

void  LCD_BL_light_On(void)
{
	LCD_BL_LOW();
}
void  LCD_BL_light_Off(void)
{
	LCD_BL_HIGH();
}

#endif /* HAL_SPI_MODULE_ENABLED */
