/**
  ******************************************************************************
  * @file    drv_spi_msd.c
  * @author  mousie-yu
  * @version V1.1.1
  * @date    2011.11.22
  * @brief   SD卡驱动程序, 使用SPI模式. 支持MMC卡(未测), SD卡, SDHC卡
  *          V1.1.1 修改了等待应答的延时, 从而兼容了创见的SD_V2卡
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "drv_spi_msd.h"

/** @addtogroup Drivers
  * @{
  */
/** @defgroup DRV_SD
  * @{
  */



/**
  ******************************************************************************
  * @addtogroup DRV_SD_Configure
  ******************************************************************************
  * @{
  */
#define SdSpiInitLowSpeed()             SpiInit(SD_SPI_NAME, SPI_TYPE_HIGH_EDGE2_MSB, SPI_BaudRatePrescaler_128)   ///< SD卡SPI初始化函数, 低速率. 必须小于400KHz, 读写SD卡指令用.
#define SdSpiInitHighSpeed()            SpiInit(SD_SPI_NAME, SPI_TYPE_HIGH_EDGE2_MSB, SPI_BaudRatePrescaler_2)     ///< SD卡SPI初始化函数, 高速率. 读写SD卡数据用.
#define SdSpiDeInit()                   SpiDeInit(SD_SPI_NAME)                            ///< SD卡SPI禁用函数宏定义
#define SdSpiTxRxByte(data)             SpiTxRxByte(SD_SPI_NAME, data)                    ///< SD卡SPI收发函数宏定义

/// SD 片选信号引脚初始化, IO口设置为推挽输出. SD 存在检测信号初始化, IO口设置为上拉输入
static __INLINE void SdPortInit(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  __SPI3_CLK_ENABLE();
  
    /**SPI3 GPIO Configuration    
    PC10     ------> SPI3_SCK
    PC11     ------> SPI3_MISO
    PC12     ------> SPI3_MOSI 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/// SD 片选信号引脚和存在检测信号禁用
static __INLINE void SdPortDeInit(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  __GPIOC_CLK_ENABLE();
  __GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_SET);

  /*Configure GPIO pins : PD2 PD3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;//GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/// SD 片选信号引脚使能, 将IO口电平置低
static __INLINE void SdCsEnable(void)
{
  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
}

/// SD 片选信号引脚禁止, 将IO口电平置高
static __INLINE void SdCsDisable(void)
{
  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
}

/// 获取SD卡检测IO口当前电平值. 0, 低电平. !0, 高电平
static __INLINE uint32_t SD_DETECT_PORT_LEVEL(void)
{
  return (GPIOD->IDR & GPIO_PIN_3);
}
/**
  * @}
  */



/** @defgroup DRV_SD_Private_TypeDefine
  * @brief    私有类型定义
  * @{
  */

/**
  * @}
  */

/** @defgroup DRV_SD_Private_MacroDefine
  * @brief    私有宏定义
  * @{
  */
#define CMD0                            0                                       ///< 命令0 , 卡复位
#define CMD1                            1                                       ///< 命令1 , CMD_SEND_OP_COND
#define CMD8                            8                                       ///< 命令8 , CMD_SEND_IF_COND
#define CMD9                            9                                       ///< 命令9 , 读CSD数据
#define CMD10                           10                                      ///< 命令10, 读CID数据
#define CMD12                           12                                      ///< 命令12, 停止数据传输
#define CMD16                           16                                      ///< 命令16, 设置SectorSize 应返回0x00
#define CMD17                           17                                      ///< 命令17, 读sector
#define CMD18                           18                                      ///< 命令18, 读Multi sector
#define ACMD23                          23                                      ///< 命令23, 设置多sector写入前预先擦除N个block
#define CMD24                           24                                      ///< 命令24, 写sector
#define CMD25                           25                                      ///< 命令25, 写Multi sector
#define ACMD41                          41                                      ///< 命令41, 应返回0x00
#define CMD55                           55                                      ///< 命令55, 应返回0x01
#define CMD58                           58                                      ///< 命令58, 读OCR信息
/**
  * @}
  */

/** @defgroup DRV_SD_Variables
  * @brief    定义全局变量(私有/公有)
  * @{
  */
static SdType_t sdType;                                                         ///< SD卡类型
/**
  * @}
  */

/** @defgroup DRV_SD_Private_Function
  * @brief    定义私有函数
  * @{
  */
static uint8_t SdSendCmd(uint8_t Cmd, uint32_t Arg);                            // SD指令发送函数
/**
  * @}
  */



/** @defgroup DRV_SD_Function
  * @brief    函数原文件
  * @{
  */

/**
  ******************************************************************************
  * @brief  SD卡的SPI通讯和相应的端口初始化函数
  * @param  None
  * @retval None
  ******************************************************************************
  */
void SdSpiPortInit(void)
{
  SdSpiInitLowSpeed();                                                          // 必须先初始化SS引脚为软件控制
  SdPortInit();                                                                 // 再初始化SS的GPIO口才有效
  SdCsDisable();
}

/**
  ******************************************************************************
  * @brief  禁用SD卡, SPI功能. 引脚设为悬空输入
  * @param  None
  * @retval None
  ******************************************************************************
  */
void SdSpiPortDeInit(void)
{
  SdSpiDeInit();
  SdPortDeInit();
}

/**
  ******************************************************************************
  * @brief  SD卡初始化
  * @param  None
  * @retval 返回值, 0, 成功. 1, 失败
  ******************************************************************************
  */
uint8_t SdInit(void)
{
  uint16_t i;
  uint8_t response;
  uint8_t rvalue = 1;

  sdType = UNKNOW;
  if (!SdPresent()) return 1;                                                   // SD卡不存在, 直接退出

  SdSpiInitLowSpeed();                                                          // 使能SPI, 设为低速模式可提高SD卡识别率
  for (i=10; i>0; i--) SdSpiTxRxByte(0xFF);                                     // 先产生>74个脉冲，让SD卡自己初始化完成

  SdCsEnable();                                                                 // SD卡片选信号使能
  for (i=10000; i>0; i--)
  {
    if (SdSendCmd(CMD0, 0) == 0x01) break;                                      // CMD0, 让SD卡进入空闲模式
  }
  if (i)                                                                        // SD卡成功进入空闲模式
  {
    // CMD8指令响应是识别SD1.x和SD2.0规范的关键
    response = SdSendCmd(8, 0x1AA);

    switch (response)
    {
      case 0x05:                                                                // CMD8应答0x05, 即不支持CMD8指令, 是SD1.x或者MMC卡
      {
        SdSpiTxRxByte(0xFF);                                                    // 发8个CLK, 让SD卡结束操作

        // 发送CMD55和ACMD41指令, 用于检测是SD卡还是MMC卡, MMC卡对ACMD41是没有应答的
        for (i=4000; i>0; i--)
        {
          response = SdSendCmd(CMD55, 0);
          if (response != 0x01) goto EXIT;                                      // 应答错误, 直接退出
          response = SdSendCmd(ACMD41, 0);
          if (response == 0x00) break;                                          // 正确应答, 跳出循环
        }
        if (i)
        {
          sdType = SD_V1;                                                       // 正确应答, 是SD卡V1.x
          rvalue = 0;                                                           // SD卡初始化成功标记
        }
        else                                                                    // 无应答, 可能是MMC卡
        {
          for (i=4000; i>0; i--)
          {
            response = SdSendCmd(CMD1, 0);
            if (response == 0x00) break;
          }
          if (i)                                                                // MMC卡初始化成功
          {
            sdType = MMC;                                                       // 是MMC卡
            rvalue = 0;                                                         // SD卡初始化成功标记
          }
        }

        if(SdSendCmd(CMD16, 512) != 0x00)                                       // 设置SD卡块大小
        {
          sdType = UNKNOW;
          rvalue = 1;
        }
      } break;

      case 0x01:                                                                // 应答0x01, 是V2.0的SD卡
      {
        SdSpiTxRxByte(0xFF);                                                    // V2.0的卡在CMD8命令后会传回4字节的应答数据
        SdSpiTxRxByte(0xFF);
        i   = SdSpiTxRxByte(0xFF);
        i <<= 8;
        i  |= SdSpiTxRxByte(0xFF);                                              // 4个字节应该是0x000001AA, 表示该卡是否支持2.7V-3.6V的电压范围

        if(i == 0x1AA)                                                          // SD卡支持2.7V-3.6V, 可以操作
        {
          for (i=4000; i>0; i--)
          {
            response = SdSendCmd(CMD55, 0);
            if (response != 0x01) goto EXIT;                                    // 应答错误, 直接退出
            response = SdSendCmd(ACMD41, 0x40000000);                           // SD V2.0要使用 0x40000000
            if (response == 0x00) break;                                        // 正确应答, 跳出循环
          }
          if (i)                                                                // CMD41正确应答
          {
            // 通过获取OCR信息, 识别是普通的 SD V2.0 还是 SDHC 卡
            if (SdSendCmd(CMD58, 0) == 0x00)
            {
              i = SdSpiTxRxByte(0xFF);                                          // 应答的4字节OCR信息
              SdSpiTxRxByte(0xFF);
              SdSpiTxRxByte(0xFF);
              SdSpiTxRxByte(0xFF);

              if (i & 0x40) sdType = SDHC;                                      // 通过检测CCS位确定SD卡类型
              else          sdType = SD_V2;
              rvalue = 0;                                                       // SD卡初始化成功标记
            }
          }
        }
      } break;
    }
  }

EXIT:
  SdCsDisable();
  SdSpiTxRxByte(0xFF);                                                          // 8个CLK, 让SD结束后续操作
  SdSpiInitHighSpeed();

  return rvalue;
}

/**
  ******************************************************************************
 * @brief  检测SD卡是否存在
 * @param  None
 * @retval 返回值, 0, 不存在. 1, 存在
 ******************************************************************************
 */
uint8_t SdPresent(void)
{
  if (SD_DETECT_PORT_LEVEL())
  {
    sdType = UNKNOW;
    return 0;
  }
  else
  {
    return 1;
  }
}

/**
  ******************************************************************************
  * @brief  读取SD卡信息
  * @param  info,   SD卡信息指针
  * @retval 返回值, 0, 成功. 1, 失败
  ******************************************************************************
  */
uint8_t SdGetInfo(SdInfo_t *info)
{
  uint16_t i;
  uint8_t rvalue = 1;
  uint8_t data;
  uint8_t csd_read_bl_len = 0;
  uint8_t csd_c_size_mult = 0;
  uint32_t csd_c_size = 0;

  assert_param(info != NULL);
  if((info == NULL) || (sdType == UNKNOW)) return rvalue;                       // 空指针 或 sd卡未初始化, 直接返回

#if (SD_SPI_MULTI == 1)
  SdSpiInitHighSpeed();
#endif

  SdCsEnable();
  if (SdSendCmd(CMD10, 0) == 0x00)                                              // 读取CID寄存器
  {
    for (i=4000; i>0; i--)
    {
      if (SdSpiTxRxByte(0xFF) == 0xFE) break;                                   // 准备好传输数据
    }
    if (i)
    {
      rvalue = 0;                                                               // 读取CID寄存器成功
      for (i=0; i<18; i++)                                                      // 16个数据, 2个CRC校验
      {
        data = SdSpiTxRxByte(0xFF);                                             // 接收CID数据
        switch(i)
        {
          case 0:
              info->manufacturer = data;
              break;
          case 1:
          case 2:
              info->oem[i - 1] = data;
              break;
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
              info->product[i - 3] = data;
              break;
          case 8:
              info->revision = data;
              break;
          case 9:
          case 10:
          case 11:
          case 12:
              info->serial |= (uint32_t)data << ((12 - i) * 8);
              break;
          case 13:
              info->manufacturing_year = data << 4;
              break;
          case 14:
              info->manufacturing_year |= data >> 4;
              info->manufacturing_month = data & 0x0f;
              break;
        }
      }
    }
  }
  if (rvalue) return rvalue;

  rvalue = 1;
  if (SdSendCmd(CMD9, 0) == 0x00)                                               // 读取CSD寄存器
  {
    for (i=4000; i>0; i--)
    {
      if (SdSpiTxRxByte(0xFF) == 0xFE) break;                                   // 准备好传输数据
    }
    if (i)
    {
      rvalue = 0;                                                               // 读取CID寄存器成功
      for (i=0; i<18; i++)                                                      // 16个数据, 2个CRC校验
      {
        data = SdSpiTxRxByte(0xFF);

        if (sdType == SDHC)                                                     // SDHC卡
        {
          switch(i)
          {
            case 7:
              csd_c_size = (data & 0x3f);
              csd_c_size <<= 8;
              break;
            case 8:
              csd_c_size |= data;
              csd_c_size <<= 8;
              break;
            case 9:
              csd_c_size |= data;
              csd_c_size++;
              info->capacity = csd_c_size;
              info->capacity <<= 19;                                            // *= (512 * 1025)
              break;
          }
        }
        else                                                                    // 普通SD卡 或 MMC 卡
        {
          switch(i)
          {
            case 5:
              csd_read_bl_len = data & 0x0f;
              break;
            case 6:
              csd_c_size = data & 0x03;
              csd_c_size <<= 8;
              break;
            case 7:
              csd_c_size |= data;
              csd_c_size <<= 2;
              break;
            case 8:
              csd_c_size |= data >> 6;
              ++csd_c_size;
              break;
            case 9:
              csd_c_size_mult = data & 0x03;
              csd_c_size_mult <<= 1;
              break;
            case 10:
              csd_c_size_mult |= data >> 7;
              info->capacity = csd_c_size;
              info->capacity <<= (csd_c_size_mult + csd_read_bl_len + 2);
              break;
          }
        }
      }
    }
  }

  info->type = sdType;

  return rvalue;
}

/**
  ******************************************************************************
  * @brief  读取SD卡的一个块,
  * @param  pBuffer, 数据存储指针
  * @param  sector,  读取块内容, 非物理地址. 块大小默认为512byte
  * @retval 返回值,  0, 成功. 1, 失败
  ******************************************************************************
  */
uint8_t SdReadBlock(uint8_t* pBuffer, uint32_t sector)
{
  uint16_t i;
  uint8_t  response;
  uint8_t  rvalue = 1;

  if ((sdType == UNKNOW) || (pBuffer == 0)) return rvalue;
  if (sdType != SDHC) sector <<= 9;                                             // 不是SDHC, 需要将块地址转为字节地址, sector *= 512

#if (SD_SPI_MULTI == 1)
  SdSpiInitHighSpeed();
#endif

  SdCsEnable();
  response = SdSendCmd(CMD17, sector);                                          // 发送 CMD17, 读单个块
  if (response == 0x00)                                                         // 应答正确
  {
    for (i=10000; i>0; i--)
    {
      if (SdSpiTxRxByte(0xFF) == 0xFE) break;                                   // 准备好传输数据
    }
    if (i)
    {
      rvalue = 0;
      for (i=512; i>0; i--) *pBuffer++ = SdSpiTxRxByte(0xFF);                   // 读一个块
      SdSpiTxRxByte(0xFF);                                                      // 读CRC校验
      SdSpiTxRxByte(0xFF);
    }
  }
  SdCsDisable();
  SdSpiTxRxByte(0xFF);

  return rvalue;
}

/**
  ******************************************************************************
  * @brief  读取SD卡的多个块,
  * @param  pBuffer,  读取地址指针
  * @param  sector,   读取块内容, 非物理地址. 块大小默认为512byte
  * @param  blockNum, 需要读取的SD块数量
  * @retval 返回值,         0, 成功. 1, 失败
  ******************************************************************************
  */
uint8_t SdReadMultiBlocks(uint8_t* pBuffer, uint32_t sector, uint32_t blockNum)
{
  uint32_t i;
  uint8_t  response;
  uint8_t  rvalue = 1;

  if ((sdType == UNKNOW) || (pBuffer == 0) || (blockNum == 0)) return rvalue;
  if (sdType != SDHC) sector <<= 9;                                             // 不是SDHC, 需要将块地址转为字节地址, sector *= 512

#if (SD_SPI_MULTI == 1)
  SdSpiInitHighSpeed();
#endif

  SdCsEnable();
  response = SdSendCmd(CMD18, sector);                                          // 发送 CMD18, 读多个块
  if (response == 0x00)                                                         // 应答正确
  {
    rvalue = 0;
    while (blockNum--)
    {
      for (i=10000; i>0; i--)
      {
        if (SdSpiTxRxByte(0xFF) == 0xFE) break;                                 // 准备好传输数据
      }
      if (i)
      {
        for (i=512; i>0; i--) *pBuffer++ = SdSpiTxRxByte(0xFF);                 // 读多个块
        SdSpiTxRxByte(0xFF);                                                    // 读CRC校验
        SdSpiTxRxByte(0xFF);
      }
      else
      {
        rvalue = 1;
      }
    }
  }
  SdSendCmd(CMD12, 0);                                                          // 发送停止命令
  SdCsDisable();
  SdSpiTxRxByte(0xFF);

  return rvalue;
}

/**
  ******************************************************************************
  * @brief  写入SD卡的一个块
  * @param  pBuffer, 写入数据指针
  * @param  sector,  写入块内容, 非物理地址. 块大小默认为512byte
  * @retval 返回值,  0, 成功. 1, 失败
  ******************************************************************************
  */
uint8_t SdWriteBlock(const uint8_t* pBuffer, uint32_t sector)
{
  uint32_t i;
  uint8_t  response;
  uint8_t  rvalue = 1;

  if ((sdType == UNKNOW) || (pBuffer == 0)) return rvalue;
  if (sdType != SDHC) sector <<= 9;                                             // 不是SDHC, 需要将块地址转为字节地址, sector *= 512

#if (SD_SPI_MULTI == 1)
  SdSpiInitHighSpeed();
#endif

  SdCsEnable();
  response = SdSendCmd(CMD24, sector);                                          // 发送 CMD24, 写单个块
  if (response == 0x00)                                                         // 应答正确
  {
    SdSpiTxRxByte(0xFF);                                                        // 若干CLK, 等待SD卡准备好
    SdSpiTxRxByte(0xFE);                                                        // 起始令牌0xFE
    for (i = 512; i>0 ; i--) SdSpiTxRxByte(*pBuffer++);                         // 写一个块
    SdSpiTxRxByte(0xFF);                                                        // 读CRC校验
    SdSpiTxRxByte(0xFF);

    if ((SdSpiTxRxByte(0xFF) & 0x1F) == 0x05)                                   // 读数据应答
    {
      for (i=100000; i>0; i--)                                                  // 写入需要时间, V1卡较慢, 延时不能太小.
      {
        if (SdSpiTxRxByte(0xFF) == 0xFF)                                        // 等待数据写入结束
        {
          rvalue = 0;                                                           // 标记为写成功
          break;
        }
      }
    }
  }
  SdCsDisable();
  SdSpiTxRxByte(0xFF);

  return rvalue;
}

/**
  ******************************************************************************
  * @brief  写入SD卡的多个块
  * @param  pBuffer,  写入地址指针
  * @param  sector,   写入块内容, 非物理地址. 块大小默认为512byte
  * @param  blockNum, 需要写入的SD块数量
  * @retval 返回值,  0, 成功. 1, 失败
  ******************************************************************************
  */
uint8_t  SdWriteMultiBlocks(const uint8_t* pBuffer, uint32_t sector, uint32_t blockNum)
{
  uint32_t i;
  uint8_t  response;
  uint8_t  rvalue = 1;

  if ((sdType == UNKNOW) || (pBuffer == 0) || (blockNum == 0)) return rvalue;
  if (sdType != SDHC) sector <<= 9;                                             // 不是SDHC, 需要将块地址转为字节地址, sector *= 512

#if (SD_SPI_MULTI == 1)
  SdSpiInitHighSpeed();
#endif

  SdCsEnable();
  if(sdType != MMC) SdSendCmd(ACMD23, sector);                                  // 使用ACMD23预擦除SD卡内容
  response = SdSendCmd(CMD25, sector);                                          // 发送 CMD25, 写多个块
  if (response == 0x00)                                                         // 应答正确
  {
    rvalue = 0;
    for (; blockNum>0; blockNum--)
    {
      SdSpiTxRxByte(0xFF);                                                      // 若干CLK, 等待SD卡准备好
      SdSpiTxRxByte(0xFC);                                                      // 放起始令牌0xFC, 表明是多块写入

      for(i=512; i>0; i--) SdSpiTxRxByte(*pBuffer++);                           // 写入一个块
      SdSpiTxRxByte(0xFF);                                                      // 读取CRC信息
      SdSpiTxRxByte(0xFF);

      if ((SdSpiTxRxByte(0xFF) & 0x1F) == 0x05)                                 // 读取SD卡应答信息
      {
        for (i=100000; i>0; i--)                                                // 写入需要时间, V1卡较慢, 延时不能太小.
        {
          if (SdSpiTxRxByte(0xFF) == 0xFF) break;                               // 等待数据写入结束
        }
      }
      else
      {
        i = 0;
      }
      if (i == 0) rvalue = 1;                                                   // 有一个块写入错误, 就标记为错误
    }
  }
  SdSpiTxRxByte(0xFD);                                                          // 结束SD卡多块写
  for (i=100000; i>0; i--)                                                      // 等待多块写入结束
  {
    if (SdSpiTxRxByte(0xFF) == 0xFF) break;
  }
  SdCsDisable();
  SdSpiTxRxByte(0xFF);

  return rvalue;
}

/**
  ******************************************************************************
  * @brief  发送5字节的SD卡指令
  * @param  Cmd, 要发送的指令
  * @param  Arg, 指令参数
  * @retval 返回值, 指令应答
  ******************************************************************************
  */
uint8_t SdSendCmd(uint8_t cmd, uint32_t arg)
{
  uint8_t response;
  ubase_t i;

  SdSpiTxRxByte(0xFF);

  SdSpiTxRxByte(0x40 | cmd);                                                    // 发送指令
  SdSpiTxRxByte(arg >> 24);
  SdSpiTxRxByte(arg >> 16);
  SdSpiTxRxByte(arg >> 8);
  SdSpiTxRxByte(arg >> 0);
  switch(cmd)
  {
    case CMD0:
      SdSpiTxRxByte(0x95);                                                      // CMD0的校验为0x95
      break;
    case CMD8:
      SdSpiTxRxByte(0x87);                                                      // CMD8的校验为0x87
      break;
    default:
      SdSpiTxRxByte(0xff);                                                      // 其它指令在SPI模式下无需校验
      break;
  }

  for (i=10; i>0; i--)                                                          // 接收应答
  {
    response = SdSpiTxRxByte(0xFF);
    if(response != 0xFF) break;
  }

  return response;
}
/**
  * @}
  */



/**
  * @}
  */
/**
  * @}
  */

/* END OF FILE ---------------------------------------------------------------*/
