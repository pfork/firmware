#include "stm32f.h"

/**
* @brief  USB_OTG_BSP_Init
*         Initilizes BSP configurations
* @param  None
*/

void USB_OTG_BSP_Init(void) {
   GPIO_Regs *greg;
   // enable gpioa
   MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOA;

   greg = (GPIO_Regs *) GPIOA_BASE;
   // enable gpioa_[9, 11, 12]
   greg->MODER |= (GPIO_Mode_AF << (8 << 1)) |
                  (GPIO_Mode_AF << (9 << 1)) |
                  (GPIO_Mode_OUT << (10 << 1)) |
                  (GPIO_Mode_AF << (11 << 1)) |
                  (GPIO_Mode_AF << (12 << 1));
   greg->OTYPER |= (GPIO_OType_OD << 10 );
   greg->PUPDR |= (GPIO_PuPd_NOPULL << (8 << 1)) |
                  (GPIO_PuPd_NOPULL << (9 << 1)) |
                  (GPIO_PuPd_UP << (10 << 1)) |
                  (GPIO_PuPd_NOPULL << (11 << 1)) |
                  (GPIO_PuPd_NOPULL << (12 << 1));
   greg->OSPEEDR |= (GPIO_Speed_100MHz << (8 << 1)) |
                    (GPIO_Speed_100MHz << (9 << 1)) |
                    (GPIO_Speed_100MHz << (10 << 1)) |
                    (GPIO_Speed_100MHz << (11 << 1)) |
                    (GPIO_Speed_100MHz << (12 << 1));
   greg->MODER |= (GPIO_Mode_AF << (10 << 1));
   greg->AFR[1] |= (GPIO_AF_OTG_FS << (0 << 2)) |
                   (GPIO_AF_OTG_FS << (1 << 2)) |
                   (GPIO_AF_OTG_FS << (2 << 2)) |
                   (GPIO_AF_OTG_FS << (3 << 2)) |
                   (GPIO_AF_OTG_FS << (4 << 2));

   // enable otgfsen
   MMIO32(RCC_AHB2ENR) |= RCC_AHB2ENR_OTGFSEN;
   // enable syscfg
   MMIO32(RCC_APB2ENR) |= RCC_APB2ENR_SYSCFG;

   //usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
   //usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
}
/**
* @brief  USB_OTG_BSP_EnableInterrupt
*         Enabele USB Global interrupt
* @param  None
*/
void USB_OTG_BSP_EnableInterrupt(void) {
  // enable IRQ
  NVIC_IPR(NVIC_OTG_FS_IRQ) = 3 << 4;
  irq_enable(NVIC_OTG_FS_IRQ);
}
