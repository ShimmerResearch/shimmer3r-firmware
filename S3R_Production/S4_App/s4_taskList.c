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

#include "s4_taskList.h"

uint32_t taskList = 0;
uint32_t taskCurrent;

void S4_NORM_Task_init(void)
{
  taskList = 0;
  //taskList = TASK_STARTSENSING;
}

void S4_NORM_Task_manage(void)
{
  taskCurrent = S4_Task_getCurrent();

#if USE_USBX
  USBX_Device_Process();
#endif

  if (!taskCurrent)
  {
    Power_SleepUntilInterrupt();
    //if(stat.isBtConnected && !stat.isSensing){
    //   Power_SleepUntilInterrupt();
    //
    //   __NOP();
    //   __NOP();
    //   __NOP();
    //}else{
    //   if(stat.periStat == 0)
    //   {
    ////            static uint8_t green1_cnt = 0;
    ////            if(!green1_cnt++){
    ////               Board_ledToggle(LED_GREEN1);
    ////            }
    //Power_StopUntilInterrupt();
    //}
    //else
    //{
    //static uint8_t blue_cnt = 0;
    //if(!blue_cnt++){
    //   Board_ledToggle(LED_BLUE);
    //}
    //__NOP();
    //__NOP();
    //__NOP();
    //Power_SleepUntilInterrupt();
    //}
    //}
  }
  else
  {
    switch (taskCurrent)
    {
    case TASK_DOCKSETUP:    DockUart_setup(); break;
    case TASK_UARTPROCESS:  DockUart_processCmd(); break;
    case TASK_UARTRESPONSE: DockUart_sendRsp(); break;
    case TASK_BTPROCESS:    BtUart_processCmd(); break;
    case TASK_BTRESPONSE:   BtUart_sendRsp(); break;
    case TASK_RCCENTERR1:   SyncCenterR1(); break;
    case TASK_RCNODER10:    SyncNodeR10(); break;
    case TASK_STREAMDATA:   S4Sens_streamData(); break;
#if defined(SHIMMER3R)
    case TASK_SAVEDATA: saveData(); break;
#endif
    case TASK_STARTSENSING: S4Sens_startSensing(); break;
    case TASK_STOPSENSING:  S4Sens_stopSensing(); break;
    //case TASK_NEXTSENSOR:      I2C_sensorNext();       break;
    case TASK_SDWRITE:      SD_writeToCard(); break;
    case TASK_SDLOG_CFG_UPDATE:
      if (!stat.isDocked && !stat.isSensing && stat.isSdInserted && GetSdCfgFlag())
      {
        stat.isConfiguring = 1;
        IniReadInfoMem();
        UpdateSdConfig();
        SetSdCfgFlag(0);
        stat.isConfiguring = 0;
      }
      break;
    case TASK_BATTREAD:
#if defined(SHIMMER3R)
      manageReadBatt();
#elif defined(SHIMMER4_SDK)
      S4_ADC_readBatt();
      I2C_readBatt();
#endif
      break;
    default: break;
    }
  }
}

uint32_t S4_NORM_Task_getCurrent()
{
  uint8_t i;
  uint32_t task;
  if (taskList)
  {
    for (i = 0; i < TASK_SIZE; i++)
    {
      task = 0x1 << i;
      if (taskList & task)
      {
        S4_Task_clear(task);
        return task;
      }
    }
  }
  return 0;
}

void S4_NORM_Task_clear(uint32_t task_id)
{
  taskList &= ~task_id;
}

uint8_t S4_NORM_Task_set(uint32_t task_id)
{
  uint8_t is_sleeping = 0;
  //if(!taskList && !TaskCurrentGet())
  if (!taskList && !taskCurrent)
    is_sleeping = 1;
  taskList |= task_id;
  return is_sleeping;
}

uint32_t S4_NORM_Task_getList()
{
  return taskList;
}

uint8_t setTaskNewBtCmdToProcess(void)
{
  return S4_Task_set(TASK_BTPROCESS);
}

void setStartSensing(void)
{
  S4_NORM_Task_set(TASK_SDLOG_CFG_UPDATE);
  S4_NORM_Task_set(TASK_STARTSENSING);
}

void setStopSensing(void)
{
  S4_NORM_Task_set(TASK_STOPSENSING);
}
