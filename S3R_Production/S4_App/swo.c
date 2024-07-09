/*
 * swo.c
 *
 *  Created on: Mar 29, 2024
 *      Author: MarkNolan
 */

#include "swo.h"
#include "stdio.h"
#include "stm32u5a5xx.h"
#include <stdint.h>

//struct __FILE
//{
//  int handle;/* Add whatever you need here */
//};
//
//FILE __stdout;
//FILE __stdin;
//
//int fputc(int ch, FILE *f)
//{
//  ITM_SendChar(ch);
//  return(ch);
//}

//int __io_putchar(int ch)
//{
//  ITM_SendChar(ch);
//  return ch;
//}

//#ifdef __GNUC__
///* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
//set to 'Yes') calls __io_putchar() */
//#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
//#else
//#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
//#endif /* __GNUC__ */
//
///**
//* @brief  Retargets the C library printf function to the USART.
//* @param  None
//* @retval None
//*/
//PUTCHAR_PROTOTYPE
//{
///* Place your implementation of fputc here */
///* e.g. write a character to the USART1 and Loop until the end of transmission */
////  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
//ITM_SendChar(ch);
//
//return ch;
//}

void log_error(char *p)
{
  printf("log Error: ");

  printf((char *) p);
  printf("\r\n");
}

void log_info(char *p)
{
  printf("log info: ");

  printf((char *) p);
  printf("\r\n");
}

void log_debug(char *p)
{
  printf("log debug: ");

  printf((char *) p);
  printf("\r\n");
}

void log_debug_array(char const *const label, void const *array, uint16_t const len)
{
  printf("log debug array: ");

  for (uint16_t i = 0; i < len; i++)
  {
    uint8_t val = *((uint8_t *) (array + i));
    printf("0x%02X", val);

    //Add ", " after all elements except the last one.
    if (i < len - 1)
    {
      printf(", ");
    }
  }
  printf("}\n");
}
