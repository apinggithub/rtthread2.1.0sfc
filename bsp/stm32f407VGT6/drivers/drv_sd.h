/**
  ******************************************************************************
  * @file    drv_sd.h
  * @author  mousie-yu
  * @version V1.1.1
  * @date    2011.11.22
  * @brief   SD卡驱动程序, 使用SPI模式. 支持MMC卡(未测), SD卡, SDHC卡
  *          V1.1.1 修改了等待应答的延时, 从而兼容了创见的SD_V2卡
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_SD_H
#define __DRV_SD_H

/* Includes ------------------------------------------------------------------*/
//#include "main.h"
#include "stm32f4xx_hal.h"
//#include "stm32f10x_gpio.h"
//#include "stm32f10x_rcc.h"

/** @addtogroup Drivers
  * @{
  */
/** @addtogroup DRV_SD
  * @{
  */



/**
  ******************************************************************************
  * @defgroup DRV_SD_Configure
  * @brief    用户配置
  ******************************************************************************
  * @{
  */

/** @defgroup SD_Pin_Assignment
  * @brief    SD引脚对应关系, 如下图:
  *           +-------------------------------------------------------+
  *           |                     Pin assignment                    |
  *           +-------------------------+---------------+-------------+
  *           | STM32 SD SPI Pins       |     SD        |    Pin      |
  *           +-------------------------+---------------+-------------+
  *           | SD_CS_PIN               |   CD DAT3     |    1        |
  *           | SD_SPI_MOSI_PIN         |   CMD         |    2        |
  *           |                         |   GND         |    3 (0 V)  |
  *           |                         |   VDD         |    4 (3.3 V)|
  *           | SD_SPI_SCK_PIN          |   CLK         |    5        |
  *           |                         |   GND         |    6 (0 V)  |
  *           | SD_SPI_MISO_PIN         |   DAT0        |    7        |
  *           |                         |   DAT1        |    8        |
  *           |                         |   DAT2        |    9        |
  *           | SD_DETECT_PIN           |   CD          |    10       |
  *           |                         |   GND         |    11 (0 V) |
  *           |                         |   WP          |    12       |
  *           +-------------------------+---------------+-------------+
  * @{
  */
//#define SD_SPI_NAME                     SD_SPI_HW                               ///< 该驱动需使用一个SPI, SPI的SCK, MISO, MOSI口在"drv_spi.h"中配置
//
//#define SD_CS_PIN                       GPIO_Pin_12                             ///< SD卡Pin1, 片选信号的PIN口
//#define SD_CS_GPIO_PORT                 GPIOB                                   ///< SD卡Pin1, 片选信号的PORT口
//#define SD_CS_GPIO_CLK                  RCC_APB2Periph_GPIOB                    ///< SD卡Pin1, 片选信号的时钟模块
//
//#define SD_DETECT_PIN                   GPIO_Pin_4                              ///< SD卡Pin10引脚, SD卡存在检测. PIN口
//#define SD_DETECT_GPIO_PORT             GPIOA                                   ///< SD卡Pin10引脚, SD卡存在检测. PORT口
//#define SD_DETECT_GPIO_CLK              RCC_APB2Periph_GPIOA                    ///< SD卡Pin10引脚, SD卡存在检测. 时钟模块
//
//#define SD_SPI_MULTI                    1                                       ///< SD的SPI是否复用, 1, 复用. 0, 不复用
/**
  * @}
  */

/**
  * @}
  */



/** @defgroup DRV_SD_Public_TypeDefine
  * @brief    公有类型定义
  * @{
  */
/// SD卡类型定义
typedef enum
{
  UNKNOW = (0x00),                                                              ///< 不明类型
  SD_V1  = (0x01),                                                              ///< V1.x版本的SD卡
  SD_V2  = (0x02),                                                              ///< V2.0版本的SD卡
  SDHC   = (0x03),                                                              ///< SDHC卡
  MMC    = (0x04),                                                              ///< MMC卡
} SdType_t;

/// SD卡信息类型定义
typedef struct
{
  SdType_t type;                                                                ///< SD卡类型
  uint8_t  manufacturer;                                                        ///< 生产厂商编号, 由SD卡组织分配
  uint8_t  oem[3];                                                              ///< OEM标示, 由SD卡组织分配
  uint8_t  product[6];                                                          ///< 生产厂商名称
  uint8_t  revision;                                                            ///< SD卡版本, 使用BCD码. 如0x32表示3.2
  uint32_t serial;                                                              ///< 序列号
  uint8_t  manufacturing_year;                                                  ///< 生产年份, 0表示2000年, 后类推
  uint8_t  manufacturing_month;                                                 ///< 生产月份
  uint64_t capacity;                                                            ///< SD卡容量, 单位为字节
} SdInfo_t;
/**
  * @}
  */

/** @defgroup DRV_SD_Public_MacroDefine
  * @brief    公有宏定义
  * @{
  */
#define SD_BLOCK_SIZE                   512                                     ///< SD卡块大小, 各种SD卡可兼容512的块大小
/**
  * @}
  */

/** @defgroup DRV_SD_Public_Variable
  * @brief    声明公有全局变量
  * @{
  */

/**
  * @}
  */

/** @defgroup DRV_SD_Public_Function
  * @brief    定义公有函数
  * @{
  */
void SdSpiPortInit(void);                                                       // SD卡的SPI和IO口初始化函数
void SdSpiPortDeInit(void);                                                     // SD卡的SPI和IO口禁用函数

uint8_t SdInit(void);                                                           // SD卡初始化. 0, 成功. 1, 失败
uint8_t SdPresent(void);                                                        // 检测SD卡是否存在. 0, 不存在. 1, 存在
uint8_t SdGetInfo(SdInfo_t *info);                                              // 读取SD卡信息. 0, 成功. 1, 失败

uint8_t SdReadBlock(uint8_t* pBuffer, uint32_t sector);                                   // SD卡读单个块
uint8_t SdReadMultiBlocks(uint8_t* pBuffer, uint32_t sector, uint32_t blockNum);          // SD卡写单个块
uint8_t SdWriteBlock(const uint8_t* pBuffer, uint32_t sector);                            // SD卡读多个块
uint8_t SdWriteMultiBlocks(const uint8_t* pBuffer, uint32_t sector, uint32_t blockNum);   // SD卡写多个块
/**
  * @}
  */



/**
  * @}
  */
/**
  * @}
  */

#endif
/* END OF FILE ---------------------------------------------------------------*/
