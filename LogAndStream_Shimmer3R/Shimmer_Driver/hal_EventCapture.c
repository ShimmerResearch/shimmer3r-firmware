/*
 * hal_EventCapture.c
 *
 *  Created on: 30 Oct 2026
 *      Author: MarkNolan
 */

#include "hal_EventCapture.h"

#include "main.h"

#include "log_and_stream_includes.h"

/* 1 us per count: TIM1 is 16-bit, so a capture can be up to 65.5 ms old when
 * its interrupt reads it */
#define TTL_TIMER_HZ 1000000UL
/* Older than half the counter's range, the age could have wrapped */
#define TTL_LATE_US  32768U

/* TIM1's kernel clock: PCLK2, doubled when APB2 is divided */
static uint32_t tim1ClockHz(void)
{
  uint32_t clk = HAL_RCC_GetPCLK2Freq();
  if ((RCC->CFGR2 & RCC_CFGR2_PPRE2_2) != 0U)
  {
    clk *= 2U;
  }
  return clk;
}

void Board_evtTtlArm(uint8_t enable)
{
  GPIO_InitTypeDef gpio = { 0 };

  gpio.Pin = GPIO_EXTERNAL_Pin;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  if (enable)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();
    TIM1->CR1 = 0;
    TIM1->DIER = 0;
    TIM1->CCER = 0;
    TIM1->PSC = (tim1ClockHz() / TTL_TIMER_HZ) - 1U;
    TIM1->ARR = 0xFFFFU;
    TIM1->TISEL = 0; /* TI1 from the CH1 pin */
    /* IC1 and IC2 both on TI1, filtered over 8 samples of the kernel clock;
     * IC1 takes rising edges and IC2 falling, so each edge's direction is
     * known from the channel that took it */
    TIM1->CCMR1 = (1UL << TIM_CCMR1_CC1S_Pos) | (2UL << TIM_CCMR1_CC2S_Pos)
        | (3UL << TIM_CCMR1_IC1F_Pos);
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC2P;
    TIM1->EGR = TIM_EGR_UG; /* load the prescaler */
    TIM1->SR = 0;
    TIM1->DIER = TIM_DIER_CC1IE | TIM_DIER_CC2IE;

    /* PE9 to TIM1_CH1, pulled down so an unconnected input stays low. An AF
     * mode, so EXTI9 stays with USB_VBUS. */
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIO_EXTERNAL_GPIO_Port, &gpio);

    HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
    TIM1->CR1 = TIM_CR1_CEN;
  }
  else
  {
    HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
    TIM1->CR1 = 0;
    TIM1->DIER = 0;
    TIM1->CCER = 0;
    TIM1->SR = 0;
    __HAL_RCC_TIM1_CLK_DISABLE();

    /* Back to a plain input with no pull, as MX_GPIO_Init leaves it - but not
     * its EXTI mode, which would take EXTI9 from USB_VBUS */
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIO_EXTERNAL_GPIO_Port, &gpio);
  }
}

uint8_t Board_evtTtlLevel(void)
{
  return (HAL_GPIO_ReadPin(GPIO_EXTERNAL_GPIO_Port, GPIO_EXTERNAL_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static void pushCapture(uint8_t level, uint16_t ageUs, uint64_t now, uint8_t flags)
{
  uint32_t ageTicks = ((uint32_t) ageUs * 32768UL + (TTL_TIMER_HZ / 2U)) / TTL_TIMER_HZ;

  if (ageUs > TTL_LATE_US)
  {
    flags |= EVT_FLAG_LATE;
  }
  (void) ShimEvt_isrPushTtlEdge(level, now - ageTicks, flags);
}

void TIM1_CC_IRQHandler(void)
{
  uint32_t sr = TIM1->SR;
  uint16_t cnt = (uint16_t) TIM1->CNT;
  uint64_t now = RTC_get64();
  uint8_t rise = (sr & TIM_SR_CC1IF) != 0U;
  uint8_t fall = (sr & TIM_SR_CC2IF) != 0U;
  /* Reading CCRx clears its CCxIF */
  uint16_t riseAge = rise ? (uint16_t) (cnt - (uint16_t) TIM1->CCR1) : 0U;
  uint16_t fallAge = fall ? (uint16_t) (cnt - (uint16_t) TIM1->CCR2) : 0U;
  uint8_t flags = EVT_FLAG_HW_CAPTURE;

  if (sr & (TIM_SR_CC1OF | TIM_SR_CC2OF))
  {
    /* A channel captured again before it was read: at least one edge lost */
    flags |= EVT_RAW_EDGES_LOST;
    TIM1->SR = ~(TIM_SR_CC1OF | TIM_SR_CC2OF); /* rc_w0: the rest unchanged */
  }

  /* Both pending: the older went first */
  if (rise && fall && fallAge > riseAge)
  {
    pushCapture(0, fallAge, now, flags);
    pushCapture(1, riseAge, now, EVT_FLAG_HW_CAPTURE);
  }
  else if (rise && fall)
  {
    pushCapture(1, riseAge, now, flags);
    pushCapture(0, fallAge, now, EVT_FLAG_HW_CAPTURE);
  }
  else if (rise)
  {
    pushCapture(1, riseAge, now, flags);
  }
  else if (fall)
  {
    pushCapture(0, fallAge, now, flags);
  }
}
