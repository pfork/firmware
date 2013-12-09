/**
  ******************************************************************************
  * @file    usb_dcd.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   Peripheral Device Interface Layer
  ******************************************************************************
  */

#include "usb_dcd.h"
#include "usb_bsp.h"

void DCD_Init(USB_OTG_CORE_HANDLE *pdev) {
  unsigned int i;
  USB_OTG_EP *ep;

  USB_OTG_SelectCore(pdev);

  pdev->dev.device_status = USB_OTG_DEFAULT;
  pdev->dev.device_address = 0;

  /* Init ep structure */
  for (i = 0; i < pdev->cfg.dev_endpoints; i++) {
    ep = &pdev->dev.in_ep[i];
    /* Init ep structure */
    ep->is_in = 1;
    ep->num = i;
    ep->tx_fifo_num = i;
    /* Control until ep is actvated */
    ep->type = EP_TYPE_CTRL;
    ep->maxpacket =  USB_OTG_MAX_EP0_SIZE;
    ep->xfer_buff = 0;
    ep->xfer_len = 0;
  }

  for (i = 0; i < pdev->cfg.dev_endpoints; i++) {
    ep = &pdev->dev.out_ep[i];
    /* Init ep structure */
    ep->is_in = 0;
    ep->num = i;
    ep->tx_fifo_num = i;
    /* Control until ep is activated */
    ep->type = EP_TYPE_CTRL;
    ep->maxpacket = USB_OTG_MAX_EP0_SIZE;
    ep->xfer_buff = 0;
    ep->xfer_len = 0;
  }

  USB_OTG_DisableGlobalInt(pdev);

  /*Init the Core (common init.) */
  USB_OTG_CoreInit(pdev);

  /* Init Device */
  USB_OTG_CoreInitDev(pdev);

  /* Enable USB Global interrupt */
  USB_OTG_EnableGlobalInt(pdev);
}


/**
* @brief  Configure an EP
* @param pdev : Device instance
* @param epdesc : Endpoint Descriptor
* @retval : status
*/
unsigned int DCD_EP_Open(USB_OTG_CORE_HANDLE *pdev ,
                     unsigned char ep_addr,
                     unsigned short ep_mps,
                     unsigned char ep_type)
{
  USB_OTG_EP *ep;

  if ((ep_addr & 0x80) == 0x80) {
    ep = &pdev->dev.in_ep[ep_addr & 0x7F];
  } else {
    ep = &pdev->dev.out_ep[ep_addr & 0x7F];
  }
  ep->num   = ep_addr & 0x7F;

  ep->is_in = (0x80 & ep_addr) != 0;
  ep->maxpacket = ep_mps;
  ep->type = ep_type;
  if (ep->is_in) {
    /* Assign a Tx FIFO */
    ep->tx_fifo_num = ep->num;
  }
  /* Set initial data PID. */
  if (ep_type == USB_OTG_EP_BULK ) {
    ep->data_pid_start = 0;
  }
  USB_OTG_EPActivate(pdev, ep );
  return 0;
}
/**
* @brief  called when an EP is disabled
* @param pdev: device instance
* @param ep_addr: endpoint address
* @retval : status
*/
unsigned int DCD_EP_Close(USB_OTG_CORE_HANDLE *pdev, unsigned char  ep_addr) {
  USB_OTG_EP *ep;

  if ((ep_addr&0x80) == 0x80) {
    ep = &pdev->dev.in_ep[ep_addr & 0x7F];
  } else {
    ep = &pdev->dev.out_ep[ep_addr & 0x7F];
  }
  ep->num   = ep_addr & 0x7F;
  ep->is_in = (0x80 & ep_addr) != 0;
  USB_OTG_EPDeactivate(pdev, ep );
  return 0;
}


/**
* @brief  DCD_EP_PrepareRx
* @param pdev: device instance
* @param ep_addr: endpoint address
* @param pbuf: pointer to Rx buffer
* @param buf_len: data length
* @retval : status
*/
unsigned int DCD_EP_PrepareRx(USB_OTG_CORE_HANDLE *pdev,
                               unsigned char ep_addr,
                              unsigned char *pbuf,
                              unsigned short buf_len) {
  USB_OTG_EP *ep;

  ep = &pdev->dev.out_ep[ep_addr & 0x7F];

  /*setup and start the Xfer */
  ep->xfer_buff = pbuf;
  ep->xfer_len = buf_len;
  ep->xfer_count = 0;
  ep->is_in = 0;
  ep->num = ep_addr & 0x7F;

  if (pdev->cfg.dma_enable == 1) {
    ep->dma_addr = (unsigned int)pbuf;
  }

  if ( ep->num == 0 ) {
    USB_OTG_EP0StartXfer(pdev, ep);
  } else {
    USB_OTG_EPStartXfer(pdev, ep );
  }
  return 0;
}

/**
* @brief  Transmit data over USB
* @param pdev: device instance
* @param ep_addr: endpoint address
* @param pbuf: pointer to Tx buffer
* @param buf_len: data length
* @retval : status
*/
unsigned int DCD_EP_Tx(USB_OTG_CORE_HANDLE *pdev,
                       unsigned char ep_addr,
                       unsigned char *pbuf,
                       unsigned int buf_len) {
  USB_OTG_EP *ep;

  ep = &pdev->dev.in_ep[ep_addr & 0x7F];

  /* Setup and start the Transfer */
  ep->is_in = 1;
  ep->num = ep_addr & 0x7F;
  ep->xfer_buff = pbuf;
  ep->dma_addr = (unsigned int)pbuf;
  ep->xfer_count = 0;
  ep->xfer_len  = buf_len;

  if ( ep->num == 0 ) {
    USB_OTG_EP0StartXfer(pdev, ep);
  } else {
    USB_OTG_EPStartXfer(pdev, ep );
  }
  return 0;
}


/**
* @brief  Stall an endpoint.
* @param pdev: device instance
* @param epnum: endpoint address
* @retval : status
*/
unsigned int  DCD_EP_Stall (USB_OTG_CORE_HANDLE *pdev, unsigned char   epnum) {
  USB_OTG_EP *ep;
  if ((0x80 & epnum) == 0x80) {
    ep = &pdev->dev.in_ep[epnum & 0x7F];
  } else {
    ep = &pdev->dev.out_ep[epnum];
  }

  ep->is_stall = 1;
  ep->num   = epnum & 0x7F;
  ep->is_in = ((epnum & 0x80) == 0x80);

  USB_OTG_EPSetStall(pdev, ep);
  return (0);
}


/**
* @brief  Clear stall condition on endpoints.
* @param pdev: device instance
* @param epnum: endpoint address
* @retval : status
*/
unsigned int  DCD_EP_ClrStall (USB_OTG_CORE_HANDLE *pdev, unsigned char epnum) {
  USB_OTG_EP *ep;
  if ((0x80 & epnum) == 0x80) {
    ep = &pdev->dev.in_ep[epnum & 0x7F];
  } else {
    ep = &pdev->dev.out_ep[epnum];
  }

  ep->is_stall = 0;
  ep->num   = epnum & 0x7F;
  ep->is_in = ((epnum & 0x80) == 0x80);

  USB_OTG_EPClearStall(pdev, ep);
  return (0);
}


/**
* @brief  This Function flushes the FIFOs.
* @param pdev: device instance
* @param epnum: endpoint address
* @retval : status
*/
unsigned int  DCD_EP_Flush (USB_OTG_CORE_HANDLE *pdev, unsigned char epnum) {

  if ((epnum & 0x80) == 0x80) {
    USB_OTG_FlushTxFifo(pdev, epnum & 0x7F);
  } else {
    USB_OTG_FlushRxFifo(pdev);
  }

  return (0);
}


/**
* @brief  This Function set USB device address
* @param pdev: device instance
* @param address: new device address
* @retval : status
*/
void DCD_EP_SetAddress (USB_OTG_CORE_HANDLE *pdev, unsigned char address) {
  USB_OTG_DCFG_TypeDef  dcfg;
  dcfg.d32 = 0;
  dcfg.b.devaddr = address;
  MODIFY_REG32( &pdev->regs.DREGS->DCFG, 0, dcfg.d32);
}

/**
* @brief  returns the EP Status
* @param  pdev : Selected device
*         epnum : endpoint address
* @retval : EP status
*/

unsigned int DCD_GetEPStatus(USB_OTG_CORE_HANDLE *pdev ,unsigned char epnum) {
  USB_OTG_EP *ep;
  unsigned int Status = 0;

  if ((0x80 & epnum) == 0x80) {
    ep = &pdev->dev.in_ep[epnum & 0x7F];
  } else {
    ep = &pdev->dev.out_ep[epnum];
  }

  Status = USB_OTG_GetEPStatus(pdev ,ep);

  /* Return the current status */
  return Status;
}

/**
* @brief  Set the EP Status
* @param  pdev : Selected device
*         Status : new Status
*         epnum : EP address
* @retval : None
*/
void DCD_SetEPStatus (USB_OTG_CORE_HANDLE *pdev, unsigned char epnum, unsigned int Status) {
  USB_OTG_EP *ep;

  if ((0x80 & epnum) == 0x80) {
    ep = &pdev->dev.in_ep[epnum & 0x7F];
  } else {
    ep = &pdev->dev.out_ep[epnum];
  }

   USB_OTG_SetEPStatus(pdev ,ep, Status);
}
