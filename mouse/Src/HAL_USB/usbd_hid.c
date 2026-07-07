/**
  ******************************************************************************
  * @file    usbd_hid.c
  * @author  MCD Application Team
  * @brief   This file provides the HID core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                HID Class  Description
  *          ===================================================================
  *           This module manages the HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse protocol
  *             - Usage Page : Generic Desktop
  *             - Usage : Joystick
  *             - Collection : Application
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_hid.h"
#include "usbd_ctlreq.h"
#include "board.h"
#include "config.h"
#include "main.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_HID
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_HID_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_HID_Private_Defines
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_HID_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_HID_Private_FunctionPrototypes
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_HID_Private_Variables
  * @{
  */

USBD_ClassTypeDef USBD_HID = {
  USBD_HID_Init,
  USBD_HID_DeInit,
  USBD_HID_Setup,
  NULL,                 /* EP0_TxSent */
  USBD_HID_EP0_RxReady, /* EP0_RxReady */
  USBD_HID_DataIn,      /* DataIn */
  NULL,                 /* DataOut */
  NULL,                 /* SOF */
  NULL,
  NULL,
  USBD_HID_GetHSCfgDesc,
  NULL, //USBD_HID_GetFSCfgDesc,
  NULL, //USBD_HID_GetOtherSpeedCfgDesc,
  USBD_HID_GetDeviceQualifierDesc,
};

/* USB HID device HS Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_HID_CfgHSDesc[USB_HID_CONFIG_DESC_SIZ] __ALIGN_END = {
  0x09,                                               /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION,                        /* bDescriptorType: Configuration */
  USB_HID_CONFIG_DESC_SIZ,
                                                      /* wTotalLength: Bytes returned */
  0x00,
  0x01,                                               /* bNumInterfaces: 1 interface */
  0x01,                                               /* bConfigurationValue: Configuration value */
  0x00,                                               /* iConfiguration: Index of string descriptor describing the configuration */
  0x80,                                               /* bmAttributes: Is not bus powered and does not support remote wakeup */
  0x32,                                               /* MaxPower 100 mA: this current is used for detecting Vbus */

  /************** Descriptor of Joystick Mouse interface ****************/
  /* 09 */
  0x09,                                               /* bLength: Interface Descriptor size */
  USB_DESC_TYPE_INTERFACE,                            /* bDescriptorType: Interface descriptor type */
  0x00,                                               /* bInterfaceNumber: Number of Interface */
  0x00,                                               /* bAlternateSetting: Alternate setting */
  0x01,                                               /* bNumEndpoints */
  0x03,                                               /* bInterfaceClass: HID */
  0x01,                                               /* bInterfaceSubClass : 1=BOOT, 0=no boot */
  0x02,                                               /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
  0,                                                  /* iInterface: Index of string descriptor */
  /******************** Descriptor of Joystick Mouse HID ********************/
  /* 18 */
  0x09,                                               /* bLength: HID Descriptor size */
  HID_DESCRIPTOR_TYPE,                                /* bDescriptorType: HID */
  0x11,                                               /* bcdHID: HID Class Spec release number */
  0x01,
  0x00,                                               /* bCountryCode: Hardware target country */
  0x01,                                               /* bNumDescriptors: Number of HID class descriptors to follow */
  0x22,                                               /* bDescriptorType */
  HID_MOUSE_REPORT_DESC_SIZE,                         /* wItemLength: Total length of Report descriptor */
  0x00,
  /******************** Descriptor of Mouse endpoint ********************/
  /* 27 */
  0x07,                                               /* bLength: Endpoint Descriptor size */
  USB_DESC_TYPE_ENDPOINT,                             /* bDescriptorType: */

  HID_EPIN_ADDR,                                      /* bEndpointAddress: Endpoint Address (IN) */
  0x03,                                               /* bmAttributes: Interrupt endpoint */
  HID_EPIN_SIZE,                                      /* wMaxPacketSize: 6 Byte max */
  0x00,
  HID_HS_BINTERVAL,                                   /* bInterval: Polling Interval */
  /* 34 */
};

/* USB HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_HID_Desc[USB_HID_DESC_SIZ] __ALIGN_END = {
  /* 18 */
  0x09,                                               /* bLength: HID Descriptor size */
  HID_DESCRIPTOR_TYPE,                                /* bDescriptorType: HID */
  0x11,                                               /* bcdHID: HID Class Spec release number */
  0x01,
  0x00,                                               /* bCountryCode: Hardware target country */
  0x01,                                               /* bNumDescriptors: Number of HID class descriptors to follow */
  0x22,                                               /* bDescriptorType */
  HID_MOUSE_REPORT_DESC_SIZE,                         /* wItemLength: Total length of Report descriptor */
  0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_HID_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

__ALIGN_BEGIN static uint8_t HID_MOUSE_ReportDesc[HID_MOUSE_REPORT_DESC_SIZE] __ALIGN_END = {
	// Mouse Collection (Input Report with Report ID 1)
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop) - Context set to Generic Desktop controls (0x01)
	0x09, 0x02,                    // USAGE (Mouse) - This collection represents a Mouse device (0x02)
	0xA1, 0x01,                    // COLLECTION (Application) - Starts an Application collection for mouse input
	0x85, 0x01,                    //   REPORT_ID (1) - Assigns Report ID 1 to this input report (0x01)
	0x05, 0x09,                    //   USAGE_PAGE (Button) - Switches context to Button controls (0x09)
	0x19, 0x01,                    //   USAGE_MINIMUM (Button 1) - Defines the first button (0x01)
	0x29, 0x05,                    //   USAGE_MAXIMUM (Button 5) - Defines the fifth button (0x05), total 5 buttons
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0) - Button state: 0 = released
	0x25, 0x01,                    //   LOGICAL_MAXIMUM (1) - Button state: 1 = pressed
	0x95, 0x05,                    //   REPORT_COUNT (5) - 5 button fields
	0x75, 0x01,                    //   REPORT_SIZE (1) - Each button is 1 bit
	0x81, 0x02,                    //   INPUT (Data,Var,Abs) - 5 bits of variable, absolute button data
	0x95, 0x01,                    //   REPORT_COUNT (1) - 1 padding field
	0x75, 0x03,                    //   REPORT_SIZE (3) - 3 bits of padding to align to 1 byte
	0x81, 0x03,                    //   INPUT (Cnst,Var,Abs) - 3 constant bits (padding, typically 0)
	0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop) - Returns context to Generic Desktop
	0x09, 0x38,                    //   USAGE (Wheel) - Wheel control (0x38)
	0x15, 0x81,                    //   LOGICAL_MINIMUM (-127) - Wheel range: -127 (scroll down)
	0x25, 0x7F,                    //   LOGICAL_MAXIMUM (127) - to 127 (scroll up)
	0x35, 0x81,                    //   PHYSICAL_MINIMUM (-127) - Physical range matches logical (optional)
	0x45, 0x7F,                    //   PHYSICAL_MAXIMUM (127) - Physical range endpoint (optional)
	0x75, 0x08,                    //   REPORT_SIZE (8) - Wheel data is 8 bits
	0x95, 0x01,                    //   REPORT_COUNT (1) - 1 wheel field
	0x81, 0x06,                    //   INPUT (Data,Var,Rel) - 1 byte of variable, relative wheel data
	0x09, 0x30,                    //   USAGE (X) - X-axis coordinate (0x30)
	0x09, 0x31,                    //   USAGE (Y) - Y-axis coordinate (0x31)
	0x16, 0x01, 0x80,              //   LOGICAL_MINIMUM (-32767) - X/Y range: -32767 (2-byte signed)
	0x26, 0xFF, 0x7F,              //   LOGICAL_MAXIMUM (32767) - to 32767
	0x36, 0x01, 0x80,              //   PHYSICAL_MINIMUM (-32767) - Physical range start (optional)
	0x46, 0xFF, 0x7F,              //   PHYSICAL_MAXIMUM (32767) - Physical range end (optional)
	0x75, 0x10,                    //   REPORT_SIZE (16) - Each coordinate is 16 bits
	0x95, 0x02,                    //   REPORT_COUNT (2) - 2 fields (X and Y)
	0x81, 0x06,                    //   INPUT (Data,Var,Rel) - 4 bytes of variable, relative X/Y data
	0xC0,                          // END_COLLECTION - Closes the mouse Application collection
	// 71 Bytes

	// Vendor-Defined Collection (Feature Report with Report ID 2)
	0x06, 0x00, 0xFF,              // USAGE_PAGE (Vendor-Defined 1) - Vendor-defined context (0xFF00)
	0x09, 0x01,                    // USAGE (Vendor Usage 1) - Custom usage ID for this collection (0x01)
	0xA1, 0x01,                    // COLLECTION (Application) - Starts an Application collection for config
    0x85, 0x02,                    //   REPORT_ID (2) Get version
    0x09, 0x01,                    //   USAGE (Vendor Usage 1)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x3F,                    //   REPORT_COUNT (63) // id + 63 = 64
    0xb1, 0x02,                    //   FEATURE (Data,Var,Abs)
	0x85, 0x03,                    //   REPORT_ID (3) Get and set config
	0x09, 0x02,                    //   USAGE (Vendor Usage 2) - Custom usage ID for config data (0x02)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0) - Config values start at 0
	0x27, 0xFF, 0xFF, 0x00, 0x00,  //   LOGICAL_MAXIMUM (65535) - Config values up to 65535 (16-bit unsigned)
	0x75, 0x10,                    //   REPORT_SIZE (16) - Each config field is 16 bits
	0x95, 0x02,                    //   REPORT_COUNT (2)
	0xB1, 0x02,                    //   FEATURE (Data,Var,Abs) - 64 bytes of variable, absolute config data
	0xC0                           // END_COLLECTION - Closes the vendor-defined Application collection
	// 40 Bytes
};

/** @defgroup USBD_HID_Private_Functions
  * @{
  */
USBD_HID_HandleTypeDef _hhid;
/**
  * @brief  USBD_HID_Init
  *         Initialize the HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t USBD_HID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);

  USBD_HID_HandleTypeDef *hhid = &_hhid;

//  hhid = USBD_malloc(sizeof(USBD_HID_HandleTypeDef));
//
//  if (hhid == NULL)
//  {
//    pdev->pClassData = NULL;
//    return (uint8_t)USBD_EMEM;
//  }

  pdev->pClassData = (void *)hhid;

  if (pdev->dev_speed == USBD_SPEED_HIGH)
  {
    pdev->ep_in[HID_EPIN_ADDR & 0xFU].bInterval = HID_HS_BINTERVAL;
  }
  else   /* LOW and FULL-speed endpoints */
  {
    pdev->ep_in[HID_EPIN_ADDR & 0xFU].bInterval = HID_FS_BINTERVAL;
  }

    /* Open EP IN */
  (void)USBD_LL_OpenEP(pdev, HID_EPIN_ADDR, USBD_EP_TYPE_INTR, HID_EPIN_SIZE);

  pdev->ep_in[HID_EPIN_ADDR & 0xFU].is_used = 1U;

  hhid->state = HID_IDLE;

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_HID_DeInit
  *         DeInitialize the HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t USBD_HID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);

  return (uint8_t)USBD_OK;
  /* Close HID EPs */
  (void)USBD_LL_CloseEP(pdev, HID_EPIN_ADDR);
  pdev->ep_in[HID_EPIN_ADDR & 0xFU].is_used = 0U;
  pdev->ep_in[HID_EPIN_ADDR & 0xFU].bInterval = 0U;

  /* FRee allocated memory */
  if (pdev->pClassData != NULL)
  {
//    (void)USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_HID_Setup
  *         Handle the HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
uint8_t USBD_HID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef *)pdev->pClassData;
  USBD_StatusTypeDef ret = USBD_OK;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {
    case HID_REQ_SET_REPORT:
        if ((req->wValue >> 8) == 0x03 && ((req->wValue & 0xFF) == 0x02 || (req->wValue & 0xFF) == 0x03))
        {
        	hhid->state = HID_SET_REPORT_PENDING;

            USBD_CtlPrepareRx(pdev, hhid->set_report_buffer, 64);
        } else {
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
        }
        break;

    case HID_REQ_GET_REPORT:
    	if ((req->wValue >> 8) == 0x03 && (req->wValue & 0xFF) == 0x02)
		{
		    uint8_t report_buffer[64];
			memset(report_buffer, 0, sizeof(report_buffer)); // Zero all bytes

			report_buffer[0] = 0x02U;
			memcpy(&report_buffer[1], FW_VERSION, sizeof(FW_VERSION)); // Copy including null terminator

			// Send Report ID + 64 bytes of firmware version
			USBD_CtlSendData(pdev,  (uint8_t *)report_buffer, 64);
		}
		else if ((req->wValue >> 8) == 0x03 && (req->wValue & 0xFF) == 0x03)
		{
			uint8_t report_buffer[5];
			memset(report_buffer, 0, sizeof(report_buffer)); // Zero all bytes

			report_buffer[0] = 0x03U;

			// Read config
			Config cfg = config_read();
			uint8_t cfg_bytes[2];
			cfg_bytes[0] = cfg & 0xFF;        // Low byte
			cfg_bytes[1] = (cfg >> 8) & 0xFF; // High byte

			for(uint8_t i = 0; i < 2; i++) {
				report_buffer[i+1] = cfg_bytes[i];
			}

			// Send Report ID + 32 bytes of config
			USBD_CtlSendData(pdev,  (uint8_t *)report_buffer, 5);
		} else {
	      USBD_CtlError(pdev, req);
	      ret = USBD_FAIL;
	    }
		break;

    case HID_REQ_SET_PROTOCOL:
      hhid->Protocol = (uint8_t)(req->wValue);
      break;

    case HID_REQ_GET_PROTOCOL:
      (void)USBD_CtlSendData(pdev, (uint8_t *)&hhid->Protocol, 1U);
      break;

    case HID_REQ_SET_IDLE:
      hhid->IdleState = (uint8_t)(req->wValue >> 8);
      break;

    case HID_REQ_GET_IDLE:
      (void)USBD_CtlSendData(pdev, (uint8_t *)&hhid->IdleState, 1U);
      break;

    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
      }
      else
      {
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_DESCRIPTOR:
      if ((req->wValue >> 8) == HID_REPORT_DESC)
      {
        len = MIN(HID_MOUSE_REPORT_DESC_SIZE, req->wLength);
        pbuf = HID_MOUSE_ReportDesc;
      }
      else if ((req->wValue >> 8) == HID_DESCRIPTOR_TYPE)
      {
        pbuf = USBD_HID_Desc;
        len = MIN(USB_HID_DESC_SIZ, req->wLength);
      }
      else
      {
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
        break;
      }
      (void)USBD_CtlSendData(pdev, pbuf, len);
      break;

    case USB_REQ_GET_INTERFACE :
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        (void)USBD_CtlSendData(pdev, (uint8_t *)&hhid->AltSetting, 1U);
      }
      else
      {
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        hhid->AltSetting = (uint8_t)(req->wValue);
      }
      else
      {
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_CLEAR_FEATURE:
      break;

    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;

  default:
    USBD_CtlError(pdev, req);
    ret = USBD_FAIL;
    break;
  }

  return (uint8_t)ret;
}

/**
  * @brief  USBD_HID_SendReport
  *         Send HID Report
  * @param  pdev: device instance
  * @param  buff: pointer to report
  * @retval status
  */
uint8_t USBD_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len)
{
  USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef *)pdev->pClassData;

  if (pdev->dev_state == USBD_STATE_CONFIGURED)
  {
    //if (hhid->state == HID_IDLE)
    //{
      hhid->state = HID_BUSY;
      (void)USBD_LL_Transmit(pdev, HID_EPIN_ADDR, report, len);
    //}
  }

  return (uint8_t)USBD_OK;
}
#if 0
/**
  * @brief  USBD_HID_GetCfgFSDesc
  *         return FS configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_HID_GetFSCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_HID_CfgFSDesc);

  return USBD_HID_CfgFSDesc;
}
#endif
/**
  * @brief  USBD_HID_GetCfgHSDesc
  *         return HS configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_HID_GetHSCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_HID_CfgHSDesc);

  return USBD_HID_CfgHSDesc;
}


/**
  * @brief  USBD_HID_EP0_RxReady
  *         Handle EP0 Rx Ready event (data received after USBD_CtlPrepareRx)
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_HID_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef *)pdev->pClassData;

    if (hhid->state == HID_SET_REPORT_PENDING)
    {
		if (hhid->set_report_buffer[0] == 2)
		{
			// NOP
		}
		else if (hhid->set_report_buffer[0] == 3)
		{
			// Data has been received into hhid->set_report_buffer
			// Copy to feature_report.words (32 x 16-bit)
			for (uint8_t i = 1; i < 5; i++)
			{
				// extern from main.h
				cfg_bytes[i - 1] = hhid->set_report_buffer[i];
			}

			// Tell main loop to update cfg
			update_cfg = 1;
		}

        hhid->state = HID_IDLE;
    }

    return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t USBD_HID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  UNUSED(epnum);
  /* Ensure that the FIFO is empty before a new transfer, this condition could
  be caused by  a new transfer before the end of the previous transfer */
  ((USBD_HID_HandleTypeDef *)pdev->pClassData)->state = HID_IDLE;

  return (uint8_t)USBD_OK;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t *USBD_HID_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_HID_DeviceQualifierDesc);

  return USBD_HID_DeviceQualifierDesc;
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
