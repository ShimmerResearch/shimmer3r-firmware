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
  /* Refer to the STM32 documentation and the example at
   * https://community.st.com/t5/stm32-mcus/how-to-jump-to-system-bootloader-from-application-code-on-stm32/ta-p/49424
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

/* DEV-1316: put back the core and USB state a reset would have left, for an
 * application started by a jump instead of a reset.
 *
 * ST's system bootloader starts the application by branching to its reset
 * vector (STM32_Programmer_CLI -s / -g after a USB DFU write, which is how
 * both the HIL rig and Consensys finish an update). Nothing is reset on the
 * way: NVIC enables and pendings, SysTick, PRIMASK and the OTG_HS core with
 * its embedded PHY (clocked, powered, D+ pull-up on) all stay as the
 * bootloader left them. This application assumes reset state, and does not
 * initialise USB until LogAndStream_setupDock() at the end of Init(), seconds
 * later. An OTG_HS interrupt in that window lands in HAL_PCD_IRQHandler()
 * with hpcd_USB_OTG_HS still zero, and the unit hangs with the bootloader's
 * pull-up still on the bus: "Device Descriptor Request Failed" until a reset.
 *
 * Called first thing in SystemInit(), before .data/.bss are set up, so it
 * touches registers only. After a real reset every step is a no-op. */
void Bootloader_resetStateLeftByJump(void)
{
  uint32_t i;
  const uint32_t pwrClkWasOff = READ_BIT(RCC->AHB3ENR, RCC_AHB3ENR_PWREN) == 0U;
  const uint32_t syscfgClkWasOff = READ_BIT(RCC->APB3ENR, RCC_APB3ENR_SYSCFGEN) == 0U;

  __disable_irq();

  SysTick->CTRL = 0U;
  SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;

  for (i = 0U; i < (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); i++)
  {
    NVIC->ICER[i] = 0xFFFFFFFFU;
    NVIC->ICPR[i] = 0xFFFFFFFFU;
  }

  /* Detach from the host first, so it sees the bootloader's device leave
   * rather than a device that stops answering. */
  if (READ_BIT(RCC->AHB2ENR1, RCC_AHB2ENR1_OTGEN) != 0U)
  {
    ((USB_OTG_DeviceTypeDef *) (USB_OTG_HS_BASE + USB_OTG_DEVICE_BASE))->DCTL |= USB_OTG_DCTL_SDIS;
  }

  /* Then the same teardown as USB_OTG_HS_PhyTeardown() (usb_otg.c): reset the
   * core and PHY, disable the PHY, remove its supplies, gate the clocks. */
  SET_BIT(RCC->AHB2RSTR1, RCC_AHB2RSTR1_OTGRST);
  __DSB();
  CLEAR_BIT(RCC->AHB2RSTR1, RCC_AHB2RSTR1_OTGRST);
  __DSB();

  SET_BIT(RCC->APB3ENR, RCC_APB3ENR_SYSCFGEN);
  (void) READ_BIT(RCC->APB3ENR, RCC_APB3ENR_SYSCFGEN);
  CLEAR_BIT(SYSCFG->OTGHSPHYCR, SYSCFG_OTGHSPHYCR_EN);

  SET_BIT(RCC->AHB3ENR, RCC_AHB3ENR_PWREN);
  (void) READ_BIT(RCC->AHB3ENR, RCC_AHB3ENR_PWREN);
  CLEAR_BIT(PWR->VOSR, (PWR_VOSR_USBPWREN | PWR_VOSR_USBBOOSTEN));
  CLEAR_BIT(PWR->SVMCR, PWR_SVMCR_USV);

  CLEAR_BIT(RCC->AHB2ENR1, RCC_AHB2ENR1_OTGEN | RCC_AHB2ENR1_USBPHYCEN);
  if (pwrClkWasOff)
  {
    CLEAR_BIT(RCC->AHB3ENR, RCC_AHB3ENR_PWREN);
  }
  if (syscfgClkWasOff)
  {
    CLEAR_BIT(RCC->APB3ENR, RCC_APB3ENR_SYSCFGEN);
  }

  /* PRIMASK is clear out of reset, and HAL_Init() relies on it. */
  __DSB();
  __ISB();
  __enable_irq();
}
