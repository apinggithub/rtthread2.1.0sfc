#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include <rthw.h>

#ifdef RT_USING_LWIP
//#include "stm32_eth.h"
#endif /* RT_USING_LWIP */

#ifdef RT_USING_SPI
#include "rt_stm32f40x_spi.h"

#if defined(RT_USING_DFS) && defined(RT_USING_DFS_ELMFAT)
#include "msd.h"
#include "spi_flash_w25qxx.h"
#endif /* RT_USING_DFS */

/*
 * SPI1_MOSI: PA7
 * SPI1_MISO: PA6
 * SPI1_SCK : PA5
 *
 * CS0: PA4  SD card.
*/
static void rt_hw_spi_init(void)
{

#ifdef RT_USING_SPI
    /* register spi bus */
    {
			#ifdef RT_USING_SPI3
        static struct stm32_spi_bus stm32_spi3;
        GPIO_InitTypeDef GPIO_InitStruct;

					/* Enable GPIO clock */
				__HAL_RCC_GPIOA_CLK_ENABLE();
				__HAL_RCC_GPIOC_CLK_ENABLE();
				__SPI3_CLK_ENABLE();
				/*nss  PA15 SCK PC10 MOSI PC12 MIso PC11*/
				GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
				GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
				GPIO_InitStruct.Pull = GPIO_PULLUP;
				GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
				GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
				HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        stm32_spi_register(SPI3, &stm32_spi3, "spi3");
			#endif
    }

    /* attach cs */
    {
        static struct rt_spi_device spi_device;
        static struct stm32_spi_cs  spi_cs;

        GPIO_InitTypeDef GPIO_InitStruct;
			 #ifdef	RT_USING_MSD
				__HAL_RCC_GPIOD_CLK_ENABLE();
			
				GPIO_InitStruct.Pin = GPIO_PIN_2;
				GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
				GPIO_InitStruct.Pull = GPIO_PULLUP;
				GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
				//GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
			
        /* spi21: PA15 */
        spi_cs.GPIOx = GPIOD;
        spi_cs.GPIO_Pin = GPIO_PIN_2;
        
        GPIO_InitStruct.Pin = spi_cs.GPIO_Pin;
				HAL_GPIO_WritePin(spi_cs.GPIOx, spi_cs.GPIO_Pin, GPIO_PIN_SET);
        HAL_GPIO_Init(spi_cs.GPIOx, &GPIO_InitStruct);
				__HAL_RCC_GPIOD_CLK_ENABLE();
        rt_spi_bus_attach_device(&spi_device, "spi_msd", "spi3", (void*)&spi_cs);
      #endif 	
			#ifdef  RT_USING_SPI_FLASH
				__HAL_RCC_GPIOA_CLK_ENABLE();
			
				GPIO_InitStruct.Pin = GPIO_PIN_15;
				GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
				GPIO_InitStruct.Pull = GPIO_NOPULL;
				GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
				//GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
			
        /* spi21: PA15 */
        spi_cs.GPIOx = GPIOA;
        spi_cs.GPIO_Pin = GPIO_PIN_15;
        
        GPIO_InitStruct.Pin = spi_cs.GPIO_Pin;
				HAL_GPIO_WritePin(spi_cs.GPIOx, spi_cs.GPIO_Pin, GPIO_PIN_SET);
        HAL_GPIO_Init(spi_cs.GPIOx, &GPIO_InitStruct);

        rt_spi_bus_attach_device(&spi_device, "spi_flash", "spi3", (void*)&spi_cs);
			#endif
     		
    }
#endif /* RT_USING_SPI1 */
}
#endif /* RT_USING_SPI */


void rt_platform_init(void)
{
#ifdef RT_USING_SPI
    rt_hw_spi_init();

	#if defined(RT_USING_DFS) && defined(RT_USING_DFS_ELMFAT)
			/* init sdcard driver */
			{
				 /* extern void rt_hw_msd_init(void);
					GPIO_InitTypeDef  GPIO_InitStruct;

					 GPIO_InitStruct.Pin = GPIO_PIN_15;
					GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
					GPIO_InitStruct.Pull = GPIO_NOPULL;
					GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
					HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
					
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
					//rt_thread_delay(2);*/
				
				#ifdef	RT_USING_MSD
					extern void rt_hw_msd_init(void);
					GPIO_InitTypeDef  GPIO_InitStruct;

					GPIO_InitStruct.Pin = GPIO_PIN_3;
					GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
					GPIO_InitStruct.Pull = GPIO_NOPULL;
					GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
					HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
					
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
					rt_thread_delay(2);
					msd_init("sd1", "spi_msd");
				#endif
				#ifdef RT_USING_SPI_FLASH
					w25qxx_init("sd0", "spi_flash");
				#endif
				
			}
	#endif /* RT_USING_DFS && RT_USING_DFS_ELMFAT */

#endif // RT_USING_SPI

#ifdef RT_USING_LWIP
    /* initialize eth interface */
    rt_hw_stm32_eth_init();
#endif /* RT_USING_LWIP */

		

}
