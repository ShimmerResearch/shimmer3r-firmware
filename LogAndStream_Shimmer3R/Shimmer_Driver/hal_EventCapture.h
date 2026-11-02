/*
 * hal_EventCapture.h
 *
 * The TTL input: GPIO_EXTERNAL (PE9) as TIM1_CH1, input-captured on both
 * edges at 1 MHz, for the event channel (log-and-stream-common
 * Events/shimmer_events.h). The functions are declared in
 * log_and_stream_externs.h, which is what calls them.
 *
 * TIM1 is set up here, register by register, rather than in the .ioc: it runs
 * only while armed, and PE9 has to stay a plain input otherwise. PE9 cannot be
 * an EXTI source at all - EXTI9 belongs to USB_VBUS (PA9) - which is why the
 * timer captures it instead.
 */

#ifndef HAL_EVENTCAPTURE_H_
#define HAL_EVENTCAPTURE_H_

#include <stdint.h>

void Board_evtTtlArm(uint8_t enable);
uint8_t Board_evtTtlLevel(void);

#endif /* HAL_EVENTCAPTURE_H_ */
