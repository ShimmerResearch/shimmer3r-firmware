/*
 * Copyright (c) 2016, Shimmer Research, Ltd.
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of Shimmer Research, Ltd. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *    * You may not use or distribute this Software or any derivative works
 *      in any form for commercial purposes with the exception of commercial
 *      purposes when used in conjunction with Shimmer products purchased
 *      from Shimmer or their designated agent or with permission from
 *      Shimmer.
 *      Examples of commercial purposes would be running business
 *      operations, licensing, leasing, or selling the Software, or
 *      distributing the Software for use with commercial products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author Weibo Pan
 * @date May, 2016
 */
 
#include "s4_led.h"

uint8_t cntBlink = 0;
uint8_t cnt1 = 0;

void S4Led_Blink(void) {
#if USE_DEFAULT_LED
      // batt leds:   
      uint8_t batt_led = 0;
      if(!stat.isDocked){
         if(cntBlink == 0){
            batt_led = stat.battStatLed;
         }    
      } else{
         if(((stat.battVal[2]&0x80) == 0) && ((stat.battVal[2]&0x40) != 0)){// charge done
            batt_led = LED_GREEN0;
         }else if(((stat.battVal[2]&0x80) != 0) && ((stat.battVal[2]&0x40) == 0)){// charge in progress
            batt_led = LED_YELLOW;
         }else if(((stat.battVal[2]&0x80) != 0) && ((stat.battVal[2]&0x40) != 0)){// stand by
            batt_led = LED_ALL_BATT;
         }else{// bad battery
            batt_led = LED_RED;
         }
      }
      Board_ledOn(batt_led);
      Board_ledOff(LED_ALL_BATT - batt_led);      
//      //yellow = stat1
//      //green0 = stat2
//      Board_ledOff(LED_RED + LED_YELLOW + LED_GREEN0);
//      if((stat.battVal[2]&0x80) == 0){
//         Board_ledOn(LED_GREEN0);
//      }
//      if((stat.battVal[2]&0x40) == 0){
//         Board_ledOn(LED_YELLOW);
//      } 
   
      if(stat.testResult){ 
         if(cntBlink%2){
            Board_ledOff(LED_RED);
         }else{
            Board_ledOn(LED_RED);
         }          
      }
      if(stat.isButtonPressed){
         Board_ledOn(LED_GREEN0);
      }
      
      if(stat.badFile){      
         if(cntBlink%2){
            Board_ledOn(LED_BLUE);
            Board_ledOff(LED_GREEN1);
         }else{
            Board_ledOff(LED_BLUE);
            Board_ledOn(LED_GREEN1);
         }         
      }else{
         // green1
         if(stat.isConfiguring){    
            if(cntBlink%2){
               Board_ledOff(LED_GREEN1);
            }else{
               Board_ledOn(LED_GREEN1);
            }          
         }else if(stat.isSensing){
            if(cntBlink >= 10){
               Board_ledOff(LED_GREEN1);
            }else{
               Board_ledOn(LED_GREEN1);
            }          
         }else if(stat.isSdInserted){
            if(cntBlink == 0){
               Board_ledOn(LED_GREEN1);
            }else{
               Board_ledOff(LED_GREEN1);
            }         
         }else{
            Board_ledOff(LED_GREEN1);
         }
         
         //blue
         if(stat.isBtPoweredOn){
            if(!stat.isBtConnected){
               if(cntBlink == 0){
                  Board_ledOn(LED_BLUE);
               }else{
                  Board_ledOff(LED_BLUE);
               }         
            }else{
               Board_ledOn(LED_BLUE);
            }
         }else{
            Board_ledOff(LED_BLUE);
         }
      }
      
      if(++cntBlink >= 20){
         cntBlink = 0;  
         cnt1++;
         if(cnt1 == 10){
            cnt1 = 0;
            __NOP();
         }
      }
      if(!(cntBlink % 10)){         
#if USE_VBATT_ALWAYS   
         if(!(stat.isSensing || stat.isConfiguring))
#endif        
         {         
            S4_Task_set(TASK_BATTREAD);
         }
      }
      //DockedCheck();//every 0.1s
      
#endif //USE_DEFAULT_LED
}



