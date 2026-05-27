/**
  ******************************************************************************
  * @file    USB_Device/DFU_Standalone/Src/usbd_desc.c
  * @author  MCD Application Team
  * @brief   This file provides the USBD descriptors and string formatting method.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define USBD_VID                      USB_VID
#define USBD_PID                      USB_PID
#define USBD_LANGID_STRING            0x409
#define USBD_MANUFACTURER_STRING      USB_MFG
#define USBD_PRODUCT_HS_STRING        USB_NAME
#define USBD_PRODUCT_FS_STRING        USB_NAME
#define USBD_CONFIGURATION_HS_STRING  "DFU Config"
#define USBD_INTERFACE_HS_STRING      "DFU Interface"
#define USBD_CONFIGURATION_FS_STRING  "DFU Config"
#define USBD_INTERFACE_FS_STRING      "DFU Interface"

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
uint8_t *USBD_DFU_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_DFU_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_DFU_ManufacturerStrDescriptor (USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_DFU_ProductStrDescriptor (USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_DFU_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_DFU_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_DFU_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_DFU_MSOS1StrDescriptor(USBD_SpeedTypeDef speed , uint16_t *length);
uint8_t *USBD_DFU_BOSDescriptor(USBD_SpeedTypeDef speed , uint16_t *length);
uint8_t *USBD_DFU_MSOS2Descriptor(USBD_SpeedTypeDef speed , uint16_t *length);

/* Private variables ---------------------------------------------------------*/
USBD_DescriptorsTypeDef DFU_Desc = {
  USBD_DFU_DeviceDescriptor,
  USBD_DFU_LangIDStrDescriptor, 
  USBD_DFU_ManufacturerStrDescriptor,
  USBD_DFU_ProductStrDescriptor,
  USBD_DFU_SerialStrDescriptor,
  USBD_DFU_ConfigStrDescriptor,
  USBD_DFU_InterfaceStrDescriptor,
  USBD_DFU_MSOS1StrDescriptor,
  USBD_DFU_BOSDescriptor,
  USBD_DFU_MSOS2Descriptor
};

/* USB Device DFU BOS descriptor */
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma data_alignment=4
#endif /* __ICCARM__ */
__ALIGN_BEGIN  uint8_t USBD_BOSDesc[USB_SIZ_BOS_DESC] __ALIGN_END =
{
  // BOS Descriptor Header
	    0x05,                   // bLength = 5 bytes
	    0x0F,                   // bDescriptorType = BOS
	    0x21, 0x00,             // wTotalLength = 33 bytes
	    0x01,                   // bNumDeviceCaps = 1 (MS OS 2.0 Capability only)

	    0x1C,                   // bLength = 28 bytes
	    0x10,                   // bDescriptorType = DEVICE_CAPABILITY
	    0x05,                   // bDevCapabilityType = PLATFORM
	    0x00,                   // bReserved
	    0xDF, 0x60, 0xDD, 0xD8, // MS OS 2.0 Platform Capability UUID:
	    0x89, 0x45, 0xC7, 0x4C, // {D8DD60DF-4589-4CC7-9CD2-659D9E648A9F} (with 9CD2 swapped)
	    0x9C, 0xD2, 0x65, 0x9D, // Changed back to 9C D2 to match reference
	    0x9E, 0x64, 0x8A, 0x9F,
	    0x00, 0x00, 0x03, 0x06, // Windows version (8.1+)
		0x1E, 0x00,             // Length of MS OS 2.0 Descriptor Set (158 bytes, matching your actual descriptor set)
	    0xEE,                   // bMS_VendorCode
		0x00                    // bAltEnumCmd
};

/* USB Device DFU MS OS 2.0 descriptor */
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma data_alignment=4
#endif /* __ICCARM__ */
__ALIGN_BEGIN  uint8_t USBD_MSOS2Desc[USB_SIZ_MSOS2_DESC] __ALIGN_END =
{
	    // Descriptor Set Header
	    0x0A, 0x00, // wLength (10 bytes)
	    0x00, 0x00, // wDescriptorType (MS_OS_20_SET_HEADER_DESCRIPTOR)
	    0x00, 0x00, 0x03, 0x06, // dwWindowsVersion (0x06030000)
	    0x1E, 0x00, // wTotalLength (30 bytes)
		//0xA2, 0x00, // wTotalLength (160 bytes)

	    // Compatible ID Feature Descriptor
	    0x14, 0x00, // wLength (20 bytes)
	    0x03, 0x00, // wDescriptorType (MS_OS_20_FEATURE_COMPATIBLE_ID)
	    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, // CompatibleID: "WINUSB\0\0"
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // SubCompatibleID: all zeros
};

// MS OS 1.0
// This is the old way via 0xEE string descriptor.
// Not currently used but worked fine. Leaving here for future reference
/* USB Device DFU MS OS 1.0 string descriptor */
#if defined ( __ICCARM__ ) //!< IAR Compiler */
#pragma data_alignment=4
#endif /* __ICCARM__ */
__ALIGN_BEGIN  uint8_t USBD_MSOS1StrDesc[USB_SIZ_MSOS1_STR_DESC] __ALIGN_END =
{
  0x12, 0x03, // Length = 18, Type = STRING
  'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00, // "MSFT100"
  '1', 0x00, '0', 0x00, '0', 0x00,
  0xEE, 0x00  // bMS_VendorCode = 0xEE, bPad
};

/* USB Standard Device Descriptor */
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN uint8_t USBD_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
  0x12,                       /* bLength */
  USB_DESC_TYPE_DEVICE,       /* bDescriptorType */
  0x10,                       /* bcdUSB */
  0x02,
  0x00,                       /* bDeviceClass */
  0x00,                       /* bDeviceSubClass */
  0x00,                       /* bDeviceProtocol */
  USB_MAX_EP0_SIZE,           /* bMaxPacketSize */
  LOBYTE(USBD_VID),           /* idVendor */
  HIBYTE(USBD_VID),           /* idVendor */
  LOBYTE(USBD_PID),           /* idVendor */
  HIBYTE(USBD_PID),           /* idVendor */
  0x00,                       /* bcdDevice rel. 2.00 */
  0x02,
  USBD_IDX_MFC_STR,           /* Index of manufacturer string */
  USBD_IDX_PRODUCT_STR,       /* Index of product string */
  USBD_IDX_SERIAL_STR,        /* Index of serial number string */
  USBD_MAX_NUM_CONFIGURATION  /* bNumConfigurations */
}; /* USB_DeviceDescriptor */

/* USB Standard Device Descriptor */
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
  USB_LEN_LANGID_STR_DESC,         
  USB_DESC_TYPE_STRING,       
  LOBYTE(USBD_LANGID_STRING),
  HIBYTE(USBD_LANGID_STRING), 
};

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END =
{
  USB_SIZ_STRING_SERIAL,      
  USB_DESC_TYPE_STRING,    
};

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

/* Private functions ---------------------------------------------------------*/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);
static void Get_SerialNum(void);

/**
  * @brief  Returns the device descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_DFU_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = sizeof(USBD_DeviceDesc);
  return (uint8_t*)USBD_DeviceDesc;
}

/**
  * @brief  Returns the LangID string descriptor.        
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_DFU_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = sizeof(USBD_LangIDDesc);  
  return (uint8_t*)USBD_LangIDDesc;
}

/**
  * @brief  Returns the product string descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_DFU_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  if(speed == USBD_SPEED_HIGH)
  {   
    USBD_GetString((uint8_t *)USBD_PRODUCT_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString((uint8_t *)USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);    
  }
  return USBD_StrDesc;
}

/**
  * @brief  Returns the manufacturer string descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_DFU_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
  * @brief  Returns the serial number string descriptor.        
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_DFU_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = USB_SIZ_STRING_SERIAL;
  
  /* Update the serial number string descriptor with the data from the unique ID */
  Get_SerialNum();
  
  return (uint8_t*)USBD_StringSerial;
}

/**
  * @brief  Returns the configuration string descriptor.    
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_DFU_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  if(speed == USBD_SPEED_HIGH)
  {  
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length); 
  }
  return USBD_StrDesc;  
}

/**
  * @brief  Returns the interface string descriptor.        
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_DFU_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  if(speed == USBD_SPEED_HIGH)
  {
    USBD_GetString((uint8_t *)USBD_INTERFACE_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString((uint8_t *)USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;  
}

/**
  * @brief  USBD_DFU_BOSDescriptor
  *         return the BOS descriptor
  * @param  speed : current device speed
  * @param  length : pointer to data length variable
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_DFU_BOSDescriptor(USBD_SpeedTypeDef speed , uint16_t *length)
{
  *length = sizeof(USBD_BOSDesc);
  return (uint8_t*)USBD_BOSDesc;
}

/**
  * @brief  USBD_DFU_MSOS1StrDescriptor
  *         return the MS OS 1.0 string descriptor
  * @param  speed : current device speed
  * @param  length : pointer to data length variable
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_DFU_MSOS1StrDescriptor(USBD_SpeedTypeDef speed , uint16_t *length)
{
  *length = sizeof(USBD_MSOS1StrDesc);
  return (uint8_t*)USBD_MSOS1StrDesc;
}


/**
  * @brief  USBD_DFU_MSOS2Descriptor
  *         return the MS OS 2.0 descriptor
  * @param  speed : current device speed
  * @param  length : pointer to data length variable
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_DFU_MSOS2Descriptor(USBD_SpeedTypeDef speed , uint16_t *length)
{
  *length = sizeof(USBD_MSOS2Desc);
  return (uint8_t*)USBD_MSOS2Desc;
}


/**
  * @brief  Create the serial number string descriptor 
  * @param  None 
  * @retval None
  */
static void Get_SerialNum(void)
{
  uint32_t deviceserial0, deviceserial1, deviceserial2;
  
  deviceserial0 = *(uint32_t*)DEVICE_ID1;
  deviceserial1 = *(uint32_t*)DEVICE_ID2;
  deviceserial2 = *(uint32_t*)DEVICE_ID3;
  
  deviceserial0 += deviceserial2;
  
  if (deviceserial0 != 0)
  {
    IntToUnicode (deviceserial0, &USBD_StringSerial[2] ,8);
    IntToUnicode (deviceserial1, &USBD_StringSerial[18] ,4);
  }
}

/**
  * @brief  Convert Hex 32Bits value into char 
  * @param  value: value to convert
  * @param  pbuf: pointer to the buffer 
  * @param  len: buffer length
  * @retval None
  */
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
  uint8_t idx = 0;
  
  for( idx = 0; idx < len; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10; 
    }
    
    value = value << 4;
    
    pbuf[ 2* idx + 1] = 0;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

