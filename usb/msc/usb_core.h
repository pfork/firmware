/**
  ******************************************************************************
  * @file    usb_core.h
  * @date    05-December-2013
  * @brief   Header of the Core Layer
  ******************************************************************************
  */

#ifndef usb_core_h
#define usb_core_h

#include "usb_conf.h"
#include "usb_regs.h"
#include "usb_defines.h"

#define USB_OTG_EP0_IDLE                          0
#define USB_OTG_EP0_SETUP                         1
#define USB_OTG_EP0_DATA_IN                       2
#define USB_OTG_EP0_DATA_OUT                      3
#define USB_OTG_EP0_STATUS_IN                     4
#define USB_OTG_EP0_STATUS_OUT                    5
#define USB_OTG_EP0_STALL                         6

#define USB_OTG_EP_TX_DIS       0x0000
#define USB_OTG_EP_TX_STALL     0x0010
#define USB_OTG_EP_TX_NAK       0x0020
#define USB_OTG_EP_TX_VALID     0x0030

#define USB_OTG_EP_RX_DIS       0x0000
#define USB_OTG_EP_RX_STALL     0x1000
#define USB_OTG_EP_RX_NAK       0x2000
#define USB_OTG_EP_RX_VALID     0x3000

#define   MAX_DATA_LENGTH                        0xFF

typedef enum {
  USB_OTG_OK = 0,
  USB_OTG_FAIL
}USB_OTG_STS;

typedef enum {
  HC_IDLE = 0,
  HC_XFRC,
  HC_HALTED,
  HC_NAK,
  HC_NYET,
  HC_STALL,
  HC_XACTERR,
  HC_BBLERR,
  HC_DATATGLERR,
}HC_STATUS;

typedef enum {
  URB_IDLE = 0,
  URB_DONE,
  URB_NOTREADY,
  URB_ERROR,
  URB_STALL
}URB_STATE;

typedef enum {
  CTRL_START = 0,
  CTRL_XFRC,
  CTRL_HALTED,
  CTRL_NAK,
  CTRL_STALL,
  CTRL_XACTERR,
  CTRL_BBLERR,
  CTRL_DATATGLERR,
  CTRL_FAIL
}CTRL_STATUS;

typedef struct USB_OTG_hc {
  unsigned char       dev_addr ;
  unsigned char       ep_num;
  unsigned char       ep_is_in;
  unsigned char       speed;
  unsigned char       do_ping;
  unsigned char       ep_type;
  unsigned short      max_packet;
  unsigned char       data_pid;
  unsigned char       *xfer_buff;
  unsigned int        xfer_len;
  unsigned int        xfer_count;
  unsigned char       toggle_in;
  unsigned char       toggle_out;
  unsigned int        dma_addr;
} USB_OTG_HC, *PUSB_OTG_HC;

typedef struct USB_OTG_ep {
  unsigned char        num;
  unsigned char        is_in;
  unsigned char        is_stall;
  unsigned char        type;
  unsigned char        data_pid_start;
  unsigned char        even_odd_frame;
  unsigned short       tx_fifo_num;
  unsigned int         maxpacket;
  /* transaction level variables*/
  unsigned char        *xfer_buff;
  unsigned int         dma_addr;
  unsigned int         xfer_len;
  unsigned int         xfer_count;
  /* Transfer level variables*/
  unsigned int         rem_data_len;
  unsigned int         total_data_len;
  unsigned int         ctl_data_len;
} USB_OTG_EP, *PUSB_OTG_EP;

typedef struct USB_OTG_core_cfg {
  unsigned char       host_channels;
  unsigned char       dev_endpoints;
  unsigned char       speed;
  unsigned char       dma_enable;
  unsigned short      mps;
  unsigned short      TotalFifoSize;
  unsigned char       phy_itface;
  unsigned char       Sof_output;
  unsigned char       low_power;

} USB_OTG_CORE_CFGS, *PUSB_OTG_CORE_CFGS;

typedef  struct  usb_setup_req {
    unsigned char   bmRequest;
    unsigned char   bRequest;
    unsigned short  wValue;
    unsigned short  wIndex;
    unsigned short  wLength;
} USB_SETUP_REQ;

typedef struct _Device_TypeDef {
  unsigned char  *(*GetDeviceDescriptor)( unsigned char speed, unsigned short *length);
  unsigned char  *(*GetLangIDStrDescriptor)( unsigned char speed, unsigned short *length);
  unsigned char  *(*GetManufacturerStrDescriptor)( unsigned char speed, unsigned short *length);
  unsigned char  *(*GetProductStrDescriptor)( unsigned char speed, unsigned short *length);
  unsigned char  *(*GetSerialStrDescriptor)( unsigned char speed, unsigned short *length);
  unsigned char  *(*GetConfigurationStrDescriptor)( unsigned char speed, unsigned short *length);
  unsigned char  *(*GetInterfaceStrDescriptor)( unsigned char speed, unsigned short *length);
} USBD_DEVICE, *pUSBD_DEVICE;

typedef struct USB_OTG_hPort {
  void (*Disconnect) (void *phost);
  void (*Connect) (void *phost);
  unsigned char ConnStatus;
  unsigned char DisconnStatus;
  unsigned char ConnHandled;
  unsigned char DisconnHandled;
} USB_OTG_hPort_TypeDef;

typedef struct _Device_cb {
  unsigned char  (*Init)         (void *pdev, unsigned char cfgidx);
  unsigned char  (*DeInit)       (void *pdev, unsigned char cfgidx);
 /* Control Endpoints*/
  unsigned char  (*Setup)        (void *pdev, USB_SETUP_REQ  *req);
  unsigned char  (*EP0_TxSent)   (void *pdev );
  unsigned char  (*EP0_RxReady)  (void *pdev );
  /* Class Specific Endpoints*/
  unsigned char  (*DataIn)       (void *pdev, unsigned char epnum);
  unsigned char  (*DataOut)      (void *pdev, unsigned char epnum);
  unsigned char  (*SOF)          (void *pdev);
  unsigned char  (*IsoINIncomplete)  (void *pdev);
  unsigned char  (*IsoOUTIncomplete)  (void *pdev);

  unsigned char  *(*GetConfigDescriptor)( unsigned char speed, unsigned short *length);
#ifdef USB_SUPPORT_USER_STRING_DESC
  unsigned char  *(*GetUsrStrDescriptor)( unsigned char speed,unsigned char index,  unsigned short *length);
#endif
} USBD_Class_cb_TypeDef;

typedef struct _USBD_USR_PROP {
  void (*Init)(void);
  void (*DeviceReset)(unsigned char speed);
  void (*DeviceConfigured)(void);
  void (*DeviceSuspended)(void);
  void (*DeviceResumed)(void);

  void (*DeviceConnected)(void);
  void (*DeviceDisconnected)(void);
} USBD_Usr_cb_TypeDef;

typedef struct _DCD {
  unsigned char        device_config;
  unsigned char        device_state;
  unsigned char        device_status;
  unsigned char        device_address;
  unsigned int       DevRemoteWakeup;
  USB_OTG_EP     in_ep   [USB_OTG_MAX_TX_FIFOS];
  USB_OTG_EP     out_ep  [USB_OTG_MAX_TX_FIFOS];
  unsigned char        setup_packet [8*3];
  USBD_Class_cb_TypeDef         *class_cb;
  USBD_Usr_cb_TypeDef           *usr_cb;
  USBD_DEVICE                   *usr_device;
  unsigned char        *pConfig_descriptor;
} DCD_DEV, *DCD_PDEV;


typedef struct _HCD {
  unsigned char                  Rx_Buffer [MAX_DATA_LENGTH];
  volatile unsigned int            ConnSts;
  volatile unsigned int            ErrCnt[USB_OTG_MAX_TX_FIFOS];
  volatile unsigned int            XferCnt[USB_OTG_MAX_TX_FIFOS];
  volatile HC_STATUS           HC_Status[USB_OTG_MAX_TX_FIFOS];
  volatile URB_STATE           URB_State[USB_OTG_MAX_TX_FIFOS];
  USB_OTG_HC               hc [USB_OTG_MAX_TX_FIFOS];
  unsigned short                 channel [USB_OTG_MAX_TX_FIFOS];
  USB_OTG_hPort_TypeDef    *port_cb;
} HCD_DEV, *USB_OTG_USBH_PDEV;


typedef struct _OTG {
  unsigned char    OTG_State;
  unsigned char    OTG_PrevState;
  unsigned char    OTG_Mode;
} OTG_DEV, *USB_OTG_USBO_PDEV;

typedef struct USB_OTG_handle {
  USB_OTG_CORE_CFGS    cfg;
  USB_OTG_CORE_REGS    regs;
  DCD_DEV     dev;
} USB_OTG_CORE_HANDLE, *PUSB_OTG_CORE_HANDLE;

USB_OTG_STS  USB_OTG_CoreInit        (USB_OTG_CORE_HANDLE *pdev);
USB_OTG_STS  USB_OTG_SelectCore      (USB_OTG_CORE_HANDLE *pdev);
USB_OTG_STS  USB_OTG_EnableGlobalInt (USB_OTG_CORE_HANDLE *pdev);
USB_OTG_STS  USB_OTG_DisableGlobalInt(USB_OTG_CORE_HANDLE *pdev);
void*        USB_OTG_ReadPacket      (USB_OTG_CORE_HANDLE *pdev,
                                      unsigned char *dest,
                                      unsigned short len);
USB_OTG_STS  USB_OTG_WritePacket     (USB_OTG_CORE_HANDLE *pdev,
                                      unsigned char *src,
                                      unsigned char ch_ep_num,
                                      unsigned short len);
USB_OTG_STS  USB_OTG_FlushTxFifo     (USB_OTG_CORE_HANDLE *pdev, unsigned int num);
USB_OTG_STS  USB_OTG_FlushRxFifo     (USB_OTG_CORE_HANDLE *pdev);

unsigned int USB_OTG_ReadCoreItr     (USB_OTG_CORE_HANDLE *pdev);
unsigned int USB_OTG_ReadOtgItr      (USB_OTG_CORE_HANDLE *pdev);
unsigned char USB_OTG_IsDeviceMode   (USB_OTG_CORE_HANDLE *pdev);
unsigned int USB_OTG_GetMode         (USB_OTG_CORE_HANDLE *pdev);
USB_OTG_STS  USB_OTG_PhyInit         (USB_OTG_CORE_HANDLE *pdev);
USB_OTG_STS  USB_OTG_SetCurrentMode  (USB_OTG_CORE_HANDLE *pdev, unsigned char mode);

USB_OTG_STS  USB_OTG_CoreInitDev         (USB_OTG_CORE_HANDLE *pdev);
USB_OTG_STS  USB_OTG_EnableDevInt        (USB_OTG_CORE_HANDLE *pdev);
unsigned int USB_OTG_ReadDevAllInEPItr           (USB_OTG_CORE_HANDLE *pdev);
USB_OTG_STS  USB_OTG_EP0Activate (USB_OTG_CORE_HANDLE *pdev);
USB_OTG_STS  USB_OTG_EPActivate  (USB_OTG_CORE_HANDLE *pdev, USB_OTG_EP *ep);
USB_OTG_STS  USB_OTG_EPDeactivate(USB_OTG_CORE_HANDLE *pdev, USB_OTG_EP *ep);
USB_OTG_STS  USB_OTG_EPStartXfer (USB_OTG_CORE_HANDLE *pdev, USB_OTG_EP *ep);
USB_OTG_STS  USB_OTG_EP0StartXfer(USB_OTG_CORE_HANDLE *pdev, USB_OTG_EP *ep);
USB_OTG_STS  USB_OTG_EPSetStall          (USB_OTG_CORE_HANDLE *pdev, USB_OTG_EP *ep);
USB_OTG_STS  USB_OTG_EPClearStall        (USB_OTG_CORE_HANDLE *pdev, USB_OTG_EP *ep);
unsigned int USB_OTG_ReadDevAllOutEp_itr (USB_OTG_CORE_HANDLE *pdev);
unsigned int USB_OTG_ReadDevOutEP_itr    (USB_OTG_CORE_HANDLE *pdev, unsigned char epnum);
unsigned int USB_OTG_ReadDevAllInEPItr   (USB_OTG_CORE_HANDLE *pdev);
unsigned char USBH_IsEvenFrame (USB_OTG_CORE_HANDLE *pdev);
void         USB_OTG_EP0_OutStart(USB_OTG_CORE_HANDLE *pdev);
void         USB_OTG_SetEPStatus (USB_OTG_CORE_HANDLE *pdev, USB_OTG_EP *ep, unsigned int Status);
unsigned int USB_OTG_GetEPStatus(USB_OTG_CORE_HANDLE *pdev,USB_OTG_EP *ep);

#endif  /* usb_core_h */
