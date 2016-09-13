/**
  ******************************************************************************
  * @file    rt_sd.c
  * @author  mousie-yu
  * @version V1.0.0
  * @date    2011.12.7
  * @brief   RT-Thread的SD卡文件系统驱动接口
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "rt_sd.h"
#include "stm32_eval_spi_sd.h"
#include "rtthread.h"
#include "dfs_fs.h"

/** @addtogroup Drivers
  * @{
  */
/** @defgroup RT_SD
  * @{
  */



/**
  ******************************************************************************
  * @addtogroup RT_SD_Configure
  ******************************************************************************
  * @{
  */

/**
  * @}
  */



/** @defgroup RT_SD_Private_TypeDefine
  * @brief    私有类型定义
  * @{
  */

/**
  * @}
  */

/** @defgroup RT_SD_Private_MacroDefine
  * @brief    私有宏定义
  * @{
  */

/**
  * @}
  */

/** @defgroup RT_SD_Variables
  * @brief    定义全局变量(私有/公有)
  * @{
  */
static struct rt_device sdcard_device;
static struct dfs_partition part;
/**
  * @}
  */

/** @defgroup RT_SD_Private_Function
  * @brief    定义私有函数
  * @{
  */

/**
  * @}
  */



/** @defgroup RT_SD_Function
  * @brief    函数原文件
  * @{
  */
/**
  ******************************************************************************
 * @brief  RT_Thread 设备驱动接口, 初始化
 * @param  RT_Thread 设备名称
 * @retval 处理结果
 ******************************************************************************
 */
static rt_err_t rt_sd_init(rt_device_t dev)
{
  return RT_EOK;
}

static rt_err_t rt_sd_open(rt_device_t dev, rt_uint16_t oflag)
{
  return RT_EOK;
}

static rt_err_t rt_sd_close(rt_device_t dev)
{
  return RT_EOK;
}

static rt_size_t rt_sd_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
  if (SD_ReadBlock((rt_uint8_t*)buffer, pos / SD_BLOCK_SIZE, size) == 0)
    return size;
  else
    return 0;
}

static rt_size_t rt_sd_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
  if (SD_WriteBlock((rt_uint8_t*)buffer, pos*512,512*size) == 0)
    return size;
  else
    return 0;
}

static rt_err_t rt_sd_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
     return RT_EOK;
}

void rt_hw_sd_init(void)
{
  //SdSpiPortInit();

  SD_Init();
  {
    rt_uint8_t status;
    rt_uint8_t *sector;
    uint8_t sendbuf[512];
		uint8_t recvbuf[512];
		uint16_t i;
		for(i = 0;i<512;i++)
	{
		sendbuf[i] =(0xba)&0xff;
		recvbuf[i] = 0;
	}
		//SD_WriteBlock(sendbuf,0, 256);
	//rt_thread_delay(2000);
	  SD_ReadBlock(recvbuf, 0, 256);
		sdcard_device.type    = RT_Device_Class_Block;
    sdcard_device.init  = rt_sd_init;
    sdcard_device.open  = rt_sd_open;
    sdcard_device.close = rt_sd_close;
    sdcard_device.read  = rt_sd_read;
    sdcard_device.write = rt_sd_write;
    sdcard_device.control = rt_sd_control;
    sdcard_device.user_data = RT_NULL;
    sector = (rt_uint8_t*)rt_malloc(512);
    if (SD_ReadBlock(sector, 0,512) == 0)
    {
      status = dfs_filesystem_get_partition(&part, sector, 0);
      if (status != RT_EOK)
      {
        part.offset = 0;
        part.size   = 0;
      }
    }
    else
    {
      part.offset = 0;
      part.size   = 0;
    }
    rt_free(sector);

    rt_device_register(&sdcard_device, "sd1",
       RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);
 
}
}


/* END OF FILE ---------------------------------------------------------------*/
