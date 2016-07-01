#include <stdint.h>
#include "stm32f.h"
#include "irq.h"

/*
    http://www.st.com/web/en/resource/technical/document/programming_manual/CD00228163.pdf
    http://infocenter.arm.com/help/topic/com.arm.doc.faqs/ka16220.html
    http://infocenter.arm.com/help/topic/com.arm.doc.dai0179b/CHDFDFIG.html
    http://infocenter.arm.com/help/topic/com.arm.doc.ddi0363g/Bgbcdeca.html
    https://blog.feabhas.com/2013/02/setting-up-the-cortex-m34-armv7-m-memory-protection-unit-mpu/
    https://libopencm3.github.io/docs/latest/cm3/html/mpu_8h.html
*/

  /* TEX:S:C:B   Description                      MEMATTRS[1:0]:HPROTS[3:2] */

  /* 000 0 0 0   Strongly Ordered                     10           00 */
  /* 000 0 0 1   Device, Shareable                    10           01 */
  /* 000 0 1 0   WT, Non-shareable                    01           10 */
  /* 000 0 1 1   WB, Non-shareable                    01           11 */
  /* 000 1 0 0   Strongly Ordered                     10           00 */
  /* 000 1 0 1   Device, Shareable                    10           01 */
  /* 000 1 1 0   WT, Shareable                        11           10 */
  /* 000 1 1 1   WB, Shareable                        11           11 */
  /* 001 0 0 0   Normal Non-cacheable, Non-shareable  00           10 */
  /* 001 0 1 0   Implementation Defined               10           10 */
  /* 001 0 1 1   WBWA, Non-shareable                  10           11 */
  /* 001 1 0 0   Normal non-cacheable, Shareable      10           10 */
  /* 001 1 1 0   Implementation Defined               10           10 */
  /* 001 1 1 1   WBWA, Shareable                      10           11 */
  /* 010 0 0 0   Device, Non-shareable                00           01 */
  /* 010 1 0 0   Device, Non-shareable                00           01 */
  /* 100 0 x x   Normal Non-cacheable, Non-shareable  00           10 */
  /* 100 1 x x   Normal Non-cacheable, Shareable      10           10 */
  /* 101 0 x x   WBWA, Non-shareable                  00           11 */
  /* 101 1 x x   WBWA, Shareable                      10           11 */
  /* 110 0 x x   WT, Non-shareable                    01           10 */
  /* 110 1 x x   WT, Shareable                        11           10 */
  /* 111 0 x x   WB, Non-shareable                    01           11 */
  /* 111 1 x x   WB, Shareable                        11           11 */

/* where */

/*   WT = Normal Cacheable, Write-Through, allocate on read miss */
/*   WB = Normal Cacheable, Write-Back, allocate on read miss */
/*   WBWA = Normal Cacheable, Write-Back, allocate on read and write miss */

/* Note: Where the caching attributes differ between "inner" (L1) and
   "outer" (L2), it is the outer policy which is indicated on the bus
   signals. */

/* Table B3-1 in the ARMv7-M ARM defines these as: */

/*   Address range          Name       Type and Attributes   */
/*   0x00000000-0x1FFFFFFF  Code       Normal Cacheable, Write-Through, Allocate on read miss */
/*   0x20000000-0x3FFFFFFF  SRAM       Normal Cacheable, Write-Back, Allocate on read and write miss */
/*   0x40000000-0x5FFFFFFF  Peripheral Device, Non-shareable */
/*   0x60000000-0x7FFFFFFF  RAM        Normal Cacheable, Write-Back, Allocate on read and write miss */
/*   0x80000000-0x9FFFFFFF  RAM        Normal Cacheable, Write-Through, Allocate on read miss */
/*   0xA0000000-0xBFFFFFFF  Device     Device, Shareable */
/*   0xC0000000-0xDFFFFFFF  Device     Device, Non-shareable */
/*   0xE0000000-0xFFFFFFFF  (System space, of which: */

/*     0xE0000000-0xE000FFFF   Private Peripheral Bus   Strongly Ordered */
/*     0xE0010000-0xFFFFFFFF   Vendor System            Device (Non-shareable)   */

/* The default Cacheable regions above are implicitly Non-shareable. */

/* Note: The default memory regions named "Device" and "Peripheral"
   are non-executable. This can be overridden by the MPU. The System
   space is always non-executable and this cannot be overridden. */

/* Shareable memory is considered to be accessible by agents other
   than the local processor, and therefore the memory system must take
   care to ensure that all observers of such memory will see a
   coherent view of the content of that memory. This is relevant to
   any levels of cache in the memory system. Shareable memory (if
   cacheable) must either not be cached, or must be cached in such a
   way that all observers see the same cached value. For simple,
   single processor systems and for systems not containing caches,
   Shareability has no relevance for Cortex-M3 and Cortex-M4. */

#define MPU_FLASH_RAM    (0b000010U << 16)
#define MPU_INTERNAL_RAM (0b000110U << 16)
#define MPU_EXTERNAL_RAM (0b000111U << 16)
#define MPU_PERIPHERIALS (0b000101U << 16)

//#define MEMMNGFAULTREG MMIO32(0xE000ED34) // MMAR
//#define MEMMNGFAULTSTAT MMIO8(0xE000ED28) // MMAS
//#define MMARVALID  (1 << 7)
//#define MSTKERR    (1 << 4)
//#define MUNSTKERR  (1 << 3)
//#define DACCVIOL   (1 << 1)
//#define IACCVIOL   (1 << 0)

/*  !!!!!!!!!
 *  IMPORTANT also update MPU_ACTIVE_REGIONS if you change the number of items in table
 *  !!!!!!!!!
 */
void mpu_init(void) {
  const uint32_t table[] = {
    // todo only allow exec on code area, not on storage area or system area
    // also only allow write on storage area, not code area
    // first 128K flash area is our firmware - RO
    (0x08000000 | MPU_REGION_Valid | 0), (MPU_REGION_Enabled | MPU_FLASH_RAM | MPU_REGION_1MB | MPU_RO | (0xfe <<8)),
    // 2nd 128K+ flash area is the key storage - NX + RW
    (0x08000000 | MPU_REGION_Valid | 7), (MPU_REGION_Enabled |MPU_NO_EXEC | MPU_FLASH_RAM | MPU_REGION_1MB | MPU_RW | (1 << 8)),
#ifdef RAMLOAD
    (0x20000000 | MPU_REGION_Valid | 1), (MPU_REGION_Enabled | MPU_INTERNAL_RAM | MPU_REGION_128KB | MPU_RW),
#else
    (0x20000000 | MPU_REGION_Valid | 1), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_INTERNAL_RAM | MPU_REGION_128KB | MPU_RW),
#endif //RAMLOAD
    (0x40000000 | MPU_REGION_Valid | 2), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_PERIPHERIALS | MPU_REGION_512MB | MPU_RW),
    (0x60000000 | MPU_REGION_Valid | 3), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_EXTERNAL_RAM | MPU_REGION_512MB | MPU_No_access),
    (0x80000000 | MPU_REGION_Valid | 4), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_EXTERNAL_RAM | MPU_REGION_512MB | MPU_No_access),
    (0xA0000000 | MPU_REGION_Valid | 5), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_PERIPHERIALS | MPU_REGION_512MB | MPU_No_access),
    //(0xE0000000 | MPU_REGION_Valid | 6), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_PERIPHERIALS | MPU_REGION_512MB | MPU_RW_No_access),
    (0xE0000000 | MPU_REGION_Valid | 6), (MPU_REGION_Enabled | MPU_NO_EXEC | MPU_PERIPHERIALS | MPU_REGION_512MB | MPU_RW),
  };

  int i;
  // not needed as irqs are initialized later
  //disable_irqs();
  /* Disable MPU */
  MPU->CTRL = 0;
  for(i=0;i<(sizeof(table)/sizeof(int));i+=2) {
    MPU->RBAR = table[i];
    MPU->RASR = table[i+1];
  }
  /* Enable MPU */
  SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
  MPU->CTRL = MPU_CTRL_HFNMIENA_Msk | MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
  __ISB();
  __DSB();
  // not needed as irqs are initialized later
  //enable_irqs();
}
