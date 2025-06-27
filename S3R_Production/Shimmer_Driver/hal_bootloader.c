/*
 * hal_bootloader.c
 *
 *  Created on: Jun 23, 2025
 *      Author: Ramesh Chhetri
 *      Author: Mark Nolan
 */

#include "hal_bootloader.h"

#include "stm32u5xx.h"

/*Based on the following example */
/* https://community.st.com/t5/stm32-mcus/how-to-jump-to-system-bootloader-from-application-code-on-stm32/ta-p/49424 */
void JumpToBootloader(void)
{
  uint32_t i = 0;
  void (*SysMemBootJump)(void);
  /* Set the address of the entry point to bootloader for U5 */
  /* The value 0x0BF90000 corresponds to the system memory bootloader entry point for STM32U5 microcontrollers. */
  /* Refer to the STM32 documentation and the example at https://community.st.com/t5/stm32-mcus/how-to-jump-to-system-bootloader-from-application-code-on-stm32/ta-p/49424
   * for more details. */
  volatile uint32_t BootAddr = 0x0BF90000;

  /* Disable all interrupts */
  __disable_irq();

  /* Disable Systick timer */
  SysTick->CTRL = 0;

  /* Set the clock to the default state */
  HAL_RCC_DeInit();

  /* Clear Interrupt Enable Register & Interrupt Pending Register */
  for (i = 0; i < 5; i++)
  {
    NVIC->ICER[i] = 0xFFFFFFFF;
    NVIC->ICPR[i] = 0xFFFFFFFF;
  }

  /* Re-enable all interrupts */
  __enable_irq();

  /* Set up the jump to boot loader address + 4 */
  SysMemBootJump = (void (*)(void))(*((uint32_t *) ((BootAddr + 4))));

  /* Set the main stack pointer to the boot loader stack */
  __set_MSP(*(uint32_t *) BootAddr);

  /* Call the function to jump to boot loader location */
  SysMemBootJump();

  /* Jump is done successfully */
  while (1)
  {
    /* Code should never reach this loop */
  }
}
