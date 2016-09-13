#include "rt_stm32f40x_spi.h"

#define CS_HIGH()  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET)
#define CS_LOW()  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET)
static rt_err_t configure(struct rt_spi_device* device, struct rt_spi_configuration* configuration);
static rt_uint32_t xfer(struct rt_spi_device* device, struct rt_spi_message* message);

static struct rt_spi_ops stm32_spi_ops =
{
    configure,
    xfer
};

#ifdef RT_USING_SPI1
static struct stm32_spi_bus stm32_spi_bus_1;
#endif /* #ifdef USING_SPI1 */

#ifdef RT_USING_SPI2
static struct stm32_spi_bus stm32_spi_bus_2;
#endif /* #ifdef USING_SPI2 */

#ifdef RT_USING_SPI3
//static struct stm32_spi_bus stm32_spi_bus_3;

#endif /* #ifdef USING_SPI3 */
SPI_HandleTypeDef hspi3;
//------------------ DMA ------------------
#ifdef SPI_USE_DMA
static uint8_t dummy = 0xFF;
#endif



#ifdef SPI_USE_DMA
static void DMA_Configuration(struct stm32_spi_bus * stm32_spi_bus, const void * send_addr, void * recv_addr, rt_size_t size)
{
    DMA_InitTypeDef DMA_InitStructure;

    DMA_ClearFlag(stm32_spi_bus->DMA_Channel_RX_FLAG_TC
                  | stm32_spi_bus->DMA_Channel_RX_FLAG_TE
                  | stm32_spi_bus->DMA_Channel_TX_FLAG_TC
                  | stm32_spi_bus->DMA_Channel_TX_FLAG_TE);

    /* RX channel configuration */
    DMA_Cmd(stm32_spi_bus->DMA_Channel_RX, DISABLE);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&(stm32_spi_bus->SPI->DR));
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_InitStructure.DMA_BufferSize = size;

    if(recv_addr != RT_NULL)
    {
        DMA_InitStructure.DMA_MemoryBaseAddr = (u32) recv_addr;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
        DMA_InitStructure.DMA_MemoryBaseAddr = (u32) (&dummy);
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }

    DMA_Init(stm32_spi_bus->DMA_Channel_RX, &DMA_InitStructure);

    DMA_Cmd(stm32_spi_bus->DMA_Channel_RX, ENABLE);

    /* TX channel configuration */
    DMA_Cmd(stm32_spi_bus->DMA_Channel_TX, DISABLE);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&(stm32_spi_bus->SPI->DR));
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_InitStructure.DMA_BufferSize = size;

    if(send_addr != RT_NULL)
    {
        DMA_InitStructure.DMA_MemoryBaseAddr = (u32)send_addr;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    }
    else
    {
        DMA_InitStructure.DMA_MemoryBaseAddr = (u32)(&dummy);;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    }

    DMA_Init(stm32_spi_bus->DMA_Channel_TX, &DMA_InitStructure);

    DMA_Cmd(stm32_spi_bus->DMA_Channel_TX, ENABLE);
}
#endif

rt_inline uint16_t get_spi_BaudRatePrescaler(rt_uint32_t max_hz)
{
    uint16_t SPI_BaudRatePrescaler;

    /* STM32F10x SPI MAX 18Mhz */
    if(max_hz >= SystemCoreClock/2 && SystemCoreClock/2 <= 18000000)
    {
        SPI_BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    }
    else if(max_hz >= SystemCoreClock/4)
    {
        SPI_BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    }
    else if(max_hz >= SystemCoreClock/8)
    {
        SPI_BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    }
    else if(max_hz >= SystemCoreClock/16)
    {
        SPI_BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    }
    else if(max_hz >= SystemCoreClock/32)
    {
        SPI_BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    }
    else if(max_hz >= SystemCoreClock/64)
    {
        SPI_BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    }
    else if(max_hz >= SystemCoreClock/128)
    {
        SPI_BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    }
    else
    {
        /* min prescaler 256 */
        SPI_BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    }

    return SPI_BaudRatePrescaler;
}

static rt_err_t configure(struct rt_spi_device* device, struct rt_spi_configuration* configuration)
{
    struct stm32_spi_bus * stm32_spi_bus = (struct stm32_spi_bus *)device->bus;
    //SPI_InitTypeDef hspi.Init;
	  SPI_HandleTypeDef hspi;
    SPI_TypeDef *  SPIx;
	
    //SPI_StructInit(&hspi.Init);
    /* data_width */
	
		
	#if 1
    if(configuration->data_width <= 8)
    {
        hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    }
    else if(configuration->data_width <= 16)
    {
        hspi.Init.DataSize = SPI_DATASIZE_16BIT;
    }
    else
    {
        return RT_EIO;
    }
    /* baudrate */
    hspi.Init.BaudRatePrescaler = get_spi_BaudRatePrescaler(configuration->max_hz);
    /* CPOL */
    if(configuration->mode & RT_SPI_CPOL)
    {
        hspi.Init.CLKPolarity = SPI_POLARITY_HIGH;
    }
    else
    {
        hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    }
    /* CPHA */
    if(configuration->mode & RT_SPI_CPHA)
    {
        hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
    }
    else
    {
        hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
    }
    /* MSB or LSB */
    if(configuration->mode & RT_SPI_MSB)
    {
        hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    }
    else
    {
        hspi.Init.FirstBit = SPI_FIRSTBIT_LSB;
    }
		hspi.Init.Mode = SPI_MODE_MASTER;
		hspi.Init.Direction = SPI_DIRECTION_2LINES;
		hspi.Init.NSS = SPI_NSS_SOFT;
		//hspi.Init.TIMode = SPI_TIMODE_ENABLE;
		hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	  hspi.Init.CRCPolynomial = 7;
     
		hspi.Instance = stm32_spi_bus->SPI;
		hspi3 = hspi;
    /* init SPI */
    //HAL_SPI_DeInit(&hspi);
		HAL_SPI_Init(&hspi3);
		//__HAL_SPI_DISABLE(&hspi3);
    __HAL_SPI_ENABLE(&hspi3);
		SPIx = stm32_spi_bus->SPI;
		if(SPI1 == SPIx)
			__HAL_RCC_SPI1_CLK_ENABLE();
		else if(SPI2 == SPIx)
			__HAL_RCC_SPI2_CLK_ENABLE();
		else if(SPI3 == SPIx)
			__HAL_RCC_SPI3_CLK_ENABLE();
		
		#endif
			
			

    return RT_EOK;
};

static rt_uint32_t xfer(struct rt_spi_device* device, struct rt_spi_message* message)
{
    struct stm32_spi_bus * stm32_spi_bus = (struct stm32_spi_bus *)device->bus;
    struct rt_spi_configuration * config = &device->config;
    //SPI_TypeDef * SPI = stm32_spi_bus->SPI->Instance;
    struct stm32_spi_cs * stm32_spi_cs = device->parent.user_data;
    rt_uint32_t size = message->length;
		SPI_HandleTypeDef hspi;
	  if(SPI3 == stm32_spi_bus->SPI)
			hspi = hspi3;
    
    /* take CS */
    if(message->cs_take)
    {
			  //__HAL_SPI_ENABLE(&hspi);
        HAL_GPIO_WritePin(stm32_spi_cs->GPIOx, stm32_spi_cs->GPIO_Pin,GPIO_PIN_RESET);
			  
    }
    
#ifdef SPI_USE_DMA
    if(message->length > 32)
    {
        if(config->data_width <= 8)
        {
            /*DMA_Configuration(stm32_spi_bus, message->send_buf, message->recv_buf, message->length);
            SPI_I2S_DMACmd(SPI, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE);
            while (DMA_GetFlagStatus(stm32_spi_bus->DMA_Channel_RX_FLAG_TC) == RESET
                    || DMA_GetFlagStatus(stm32_spi_bus->DMA_Channel_TX_FLAG_TC) == RESET);
            SPI_I2S_DMACmd(SPI, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE);*/
					//SPI_DMAEndTransmitReceive(stm32_spi_bus->SPI);
					HAL_SPI_TransmitReceive_DMA(hspi,message->send_buf,message->recv_buf, message->length);
        }
//        rt_memcpy(buffer,_spi_flash_buffer,DMA_BUFFER_SIZE);
//        buffer += DMA_BUFFER_SIZE;
    }
    else
#endif
    {
        if(config->data_width <= 8)
        {
            rt_uint8_t* send_ptr = (rt_uint8_t*)(message->send_buf);
					  //const rt_uint8_t  send_ptr1 = message->send_buf;
            rt_uint8_t*  recv_ptr =    message->recv_buf;
              #if 0
					   
					    if((send_ptr != RT_NULL)&&(recv_ptr != RT_NULL))
									HAL_SPI_TransmitReceive(&hspi,send_ptr,message->recv_buf,size, 1000);
							else if(send_ptr != RT_NULL)
							{
								HAL_SPI_Transmit(&hspi,send_ptr,size, 1000);
									//HAL_SPI_TransmitReceive(&hspi,(uint8_t*)send_ptr,recv,5, 1000);//size
							}
							else if(recv_ptr != RT_NULL)
							{
								HAL_SPI_Receive(&hspi,recv_ptr,size, 1000);
							}
							#else
					   // __HAL_SPI_ENABLE(&hspi);
					    while(size--)
							{
									rt_uint8_t data = 0xFF;

									if(send_ptr != RT_NULL)
									{
											data = *send_ptr++;
									}

									//Wait until the transmit buffer is empty
									while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_TXE) == RESET);
									// Send the byte
									(&hspi)->Instance->DR = data;

									//Wait until a data is received
									while (__HAL_SPI_GET_FLAG(&hspi, SPI_FLAG_RXNE) == RESET);
									// Get the received data
									data = (&hspi)->Instance->DR;

									if(recv_ptr != RT_NULL)
									{
											*recv_ptr++ = data;
									}
							}
							#endif

							
        }
        else if(config->data_width <= 16)
        {
            const rt_uint8_t * send_ptr = message->send_buf;
            rt_uint8_t * recv_ptr = message->recv_buf;
						
						 if(send_ptr != RT_NULL)
								HAL_SPI_Transmit(&hspi,(uint8_t*)send_ptr,size, 1000);
									//HAL_SPI_TransmitReceive(&hspi,(uint8_t*)send_ptr,recv,5, 1000);//size
							if(recv_ptr != RT_NULL)
							{
								HAL_SPI_Receive(&hspi,recv_ptr,size, 1000);
							}
        }
    }

    /* release CS */
    if(message->cs_release)
    {
        HAL_GPIO_WritePin(stm32_spi_cs->GPIOx, stm32_spi_cs->GPIO_Pin,GPIO_PIN_SET);
			 //__HAL_SPI_DISABLE(&hspi);
    }

    return message->length;
};

/** \brief init and register stm32 spi bus.
 *
 * \param SPI: STM32 SPI, e.g: SPI1,SPI2,SPI3.
 * \param stm32_spi: stm32 spi bus struct.
 * \param spi_bus_name: spi bus name, e.g: "spi1"
 * \return
 *
 */
rt_err_t stm32_spi_register(SPI_TypeDef * SPI,
                            struct stm32_spi_bus * stm32_spi,
                            const char * spi_bus_name)
{
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    SPI_HandleTypeDef hspix;
    if(SPI == SPI1)
    {
    	stm32_spi->SPI = SPI1;
#ifdef SPI_USE_DMA
        /* Enable the DMA1 Clock */
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

        stm32_spi->DMA_Channel_RX = DMA1_Channel2;
        stm32_spi->DMA_Channel_TX = DMA1_Channel3;
        stm32_spi->DMA_Channel_RX_FLAG_TC = DMA1_FLAG_TC2;
        stm32_spi->DMA_Channel_RX_FLAG_TE = DMA1_FLAG_TE2;
        stm32_spi->DMA_Channel_TX_FLAG_TC = DMA1_FLAG_TC3;
        stm32_spi->DMA_Channel_TX_FLAG_TE = DMA1_FLAG_TE3;
#endif
        //__HAL_RCC_SPI1_CLK_ENABLE();
    }
    else if(SPI == SPI2)
    {
        stm32_spi->SPI = SPI2;
#ifdef SPI_USE_DMA
        /* Enable the DMA1 Clock */
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

        stm32_spi->DMA_Channel_RX = DMA1_Channel4;
        stm32_spi->DMA_Channel_TX = DMA1_Channel5;
        stm32_spi->DMA_Channel_RX_FLAG_TC = DMA1_FLAG_TC4;
        stm32_spi->DMA_Channel_RX_FLAG_TE = DMA1_FLAG_TE4;
        stm32_spi->DMA_Channel_TX_FLAG_TC = DMA1_FLAG_TC5;
        stm32_spi->DMA_Channel_TX_FLAG_TE = DMA1_FLAG_TE5;
#endif
       //__HAL_RCC_SPI2_CLK_ENABLE();
    }
    else if(SPI == SPI3)
    {
    	stm32_spi->SPI = SPI3;
#ifdef SPI_USE_DMA
        /* Enable the DMA2 Clock */
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

        stm32_spi->DMA_Channel_RX = DMA2_Channel1;
        stm32_spi->DMA_Channel_TX = DMA2_Channel2;
        stm32_spi->DMA_Channel_RX_FLAG_TC = DMA2_FLAG_TC1;
        stm32_spi->DMA_Channel_RX_FLAG_TE = DMA2_FLAG_TE1;
        stm32_spi->DMA_Channel_TX_FLAG_TC = DMA2_FLAG_TC2;
        stm32_spi->DMA_Channel_TX_FLAG_TE = DMA2_FLAG_TE2;
#endif
        
    }
    else
    {
        return RT_ENOSYS;
    }
		hspix.Instance = stm32_spi->SPI;
   __HAL_SPI_ENABLE(&hspix);
    return rt_spi_bus_register(&stm32_spi->parent, spi_bus_name, &stm32_spi_ops);
}
