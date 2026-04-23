/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_usbx_device.h"
#include "dcache.h"
#include "gpdma.h"
#include "gpio.h"
#include "icache.h"
#include "rng.h"
#include "rtc.h"
#include "tim.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_usbx_device.h"
#include "log_and_stream_globals.h"
#include "shimmer_definitions.h"
#include "shimmer_include.h"
#include "usb_otg.h"
#include "ux_device_cdc_acm.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

#define TIM_MEASURE_START time_start = SysTick->VAL
#define TIM_MEASURE_END    \
  time_end = SysTick->VAL; \
  time_diff = time_start - time_end
#define BOOTLOADER_ENTRY_THRESHOLD_MS 3000

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

volatile uint32_t time_start, time_end, time_diff;
#define BLOCK_START_ADDR 0 /* Block start address      */
#define NUM_OF_BLOCKS    5 /* Total number of blocks   */
#define BUFFER_WORDS_SIZE \
  ((MMC_BLOCKSIZE * NUM_OF_BLOCKS) >> 2) /* Total data size in bytes */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
/* USER CODE BEGIN PFP */

void Init(void);
void InitialiseBt(void);
void InitialiseBtAfterBoot(void);
void btFactoryResetViaFw(void);
void btCommWithDiffBaudRates(void);
void BtStartDone(void);
void setBtConnectionState(bool state);
bool isBtConnected(void);
#if USE_CUSTOM_HAL_DELAY
void HAL_Delay(uint32_t Delay);
#endif

void BtStart(void);
void BtStop(uint8_t isCalledFromMain);
float samplingClockFreqGet(void);
uint8_t getDefaultBaudForBtVersion(void);
void JumpToBootloaderIfRequired(void);
HAL_StatusTypeDef checknBoot0OptionByte(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int _write(int file, char *ptr, int len)
{
#ifdef DEBUG
  int DataIdx;
  //Only send if debugger attached and ITM enabled
  if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
      && (ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & 1U))
  {
    for (DataIdx = 0; DataIdx < len; DataIdx++)
    {
      ITM_SendChar(*ptr++);
    }
    //HAL_UART_Transmit(huartBsl, (uint8_t *) ptr++, (uint16_t) len, 0xFFFF);
  }
#endif
  return len;
}

void Init()
{
  LogAndStream_init();
  shimmerStatus.booting = 1; /* led flag, in initialisation period */

#if defined(SHIMMER3R)
  Board_ledTimersStart(&htim3, &htim2, &htim6);
#endif

  JumpToBootloaderIfRequired();

  LogAndStream_setBootStage(BOOT_STAGE_START);

  ShimBrd_setHwId(DEVICE_VER);

#if defined(SHIMMER4_SDK)
  TIM_init();
#endif

  //GPIO_init();
  S4_ADC_init();

  LogAndStream_setBootStage(BOOT_STAGE_I2C);
  //TODO Shimmer3 performs bus scan on boot - not needed for Shimmer3r?
  ShimEeprom_setIsPresent(1);
  LogAndStream_processDaughterCardId();

  setUartPeripheralPointers();

  LogAndStream_updateDockedStateAndCheckChanged();
  LogAndStream_checkSdInSlot();
  if (shimmerStatus.sdInserted)
  {
    //Take control of SD card so configuration & calibration can be loaded later
    LogAndStream_setupUndock();
  }

  //(void) ShimBtn_pressReleaseAction();

#if defined(SHIMMER3R)
  LogAndStream_setBootStage(BOOT_STAGE_BLUETOOTH);
  ShimBt_btCommsProtocolInit();
  ShimSdSync_init(ShimTask_setInitialiseBluetooth, BtStop);
  //btFactoryResetViaFw();
  InitialiseBt();
  ShimBt_macIdSetFromBytes(BT_getCyw20820MacAddressPtr());
  BT_generateCyw20820FirmwareVersionStr(ShimBt_getBtVerStrPtr());

  /* Check if radio details in EEPROM are correct and, if not, update them
   * and write them to EEPROM for the SHIMMER3R boot path. */
  if (ShimEeprom_areRadioDetailsIncorrect())
  {
    ShimEeprom_updateRadioDetails();
    ShimEeprom_writeSensorSettingsPage();
  }

  //BtStop(true);

#elif defined(SHIMMER4_SDK)
  BtUart_init();
#endif

  LogAndStream_setBootStage(BOOT_STAGE_CONFIGURATION);
  /* Calibration needs to be loaded after the chips have been detected in
   * order to know which default calib to set for attached chips.
   * It also needs to be loaded after the BT is initialised so that the
   * MAC ID can be used for default Shimmer name and calibration file names.*/
  ShimConfig_loadSensorConfigAndCalib();
  bmp3_readCalibrationDataOnBoot();

  /* Sample both dock and USB-VBUS pins so the ownership decision below has
   * the complete picture.  Board_checkDockedDetectState() was already called
   * earlier (line 152) but we re-read here for consistency.  We deliberately
   * do NOT call LogAndStream_dockOrUsbStateUpdate() because it fires
   * LogAndStream_dockedStateChange() → TASK_SETUP_DOCK, which would cause a
   * redundant second pass through setupDock() from the main loop. */
  GPIO_usbVbusIntInit(1);
  LogAndStream_updateDockedStateAndCheckChanged();
  shimmerStatus.usbPluggedIn = Board_isUsbPluggedIn();

  //Pass control of SD card to USB or dock (USB has priority)
  if (LogAndStream_isDockedOrUsbIn())
  {
    LogAndStream_setupDock();
  }

  //Disable dock comms until sensor is ready to communicate
  DockUart_disable();

  //==== 13.8ma ====
#if FULL_TEST_MODE
  FullTest();
#endif
  //S4Sens_stopPeripherals();
  RTC_wakeUpOff();
#if defined(SHIMMER4_SDK)
  S4_RTC_WakeUpSetSlow();
#endif

  /* Take initial measurement to update LED state */
  manageReadBatt(1);

  //Enable dock comms now that sensor is ready to communicate
  DockUart_enable();

  shimmerStatus.booting = 0;
  LogAndStream_setBootStage(BOOT_STAGE_END);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  uint32_t i = 0;
  while (i++ < 1000000)
    ;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the System Power */
  SystemPower_Config();

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_ICACHE_Init();
  MX_RNG_Init();
  MX_RTC_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_DCACHE1_Init();
  /* USER CODE BEGIN 2 */

  //MX_IWDG_Init();

#if !IS_CONNECTED_EEPROM
  setMockExpansionBrdDetails();
#endif

  Init();

  /* Check nBOOT0 option byte is configured correctly */
  checknBoot0OptionByte();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Let USBX progress enumeration/state machine */
    if (USBX_IsInitialised())
    {
      ux_device_stack_tasks_run();

      /* CDC TX: always poll the write task. It has its own guards for
       * (cdc_acm == NULL || device_state != CONFIGURED) and will early-
       * return when there's no work. We MUST poll this even when the
       * host has closed the COM port (!IsPortOpen) so the internal
       * stall watchdog gets a chance to abort transfers that were
       * queued while the port was open and then got stranded when the
       * host stopped polling / closed the port. Gating it behind
       * IsPortOpen() (as the old code did) meant a stale in-flight
       * transfer could wedge tx_active=1 forever until the next port
       * open, at which point every new USBX_CDC_ACM_Transmit() would
       * return usbx_busy. */
      cdc_acm_write_task();

      /* CDC RX: only touch once the host has opened the port (DTR
       * asserted). Before that, arming a bulk-OUT receive is wasted
       * work. */
      if (USBX_CDC_ACM_IsPortOpen())
      {
        cdc_acm_read_task();
      }
    }

    ShimTask_manage();
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
  RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
   */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_LSI
      | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief Power Configuration
 * @retval None
 */
static void SystemPower_Config(void)
{
  HAL_PWREx_EnableVddIO2();

  /*
   * Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
   */
  HAL_PWREx_DisableUCPDDeadBattery();

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN PWR */
  /* USER CODE END PWR */
}

/* USER CODE BEGIN 4 */

STATTypeDef *GetStatus()
{
  return &shimmerStatus;
}

//TODO move out of here
#if defined(SHIMMER3R)
void InitialiseBt(void)
{
  SHIMMER_PRINTF("\r\nBT init start\r\n");

  setBtBootModeFirstBoot();

  btCommWithDiffBaudRates();
}

void InitialiseBtAfterBoot(void)
{
  SHIMMER_PRINTF("\r\nBT init after boot start\r\n");

  setBtBootModeSubsequentBoot();

  BtStart();
}

void btFactoryResetViaFw(void)
{
  SHIMMER_PRINTF("\r\nBT factory reset start\r\n");

  setBtBootModeFactoryReset();

  btCommWithDiffBaudRates();

  //Abort transfer operations to release UART for subsequent requests.
  HAL_StatusTypeDef status = HAL_UART_Abort(&huart3);

  SHIMMER_PRINTF("BT factory reset end\r\n");
}

void btCommWithDiffBaudRates(void)
{
  uint8_t failCount = 0U;
  //50 * 100ms = 5s per baud rate attempt
  uint8_t resetCnt = 50U;
  uint8_t resetCntCurrent = resetCnt;
  uint32_t baudToTry = BAUD_TO_USE;

#if SUPPORT_SR48_6_0
  if (ShimBrd_isBoardSr48_6_0())
  {
    baudToTry = BAUD_TO_USE_SR48_6_0;
  }
#endif //SUPPORT_SR48_6_0

  BT_startDone_cb(BtStartDone);

  ShimBt_setBtBaudRateToUse(baudToTry);
  BtStart();

  if (resetCnt > 0U)
  {
    while ((isBtInitCmdsRunning() && !shimmerStatus.btIsInitialised)
        || (isBtFactoryResetCmdsRunning() && !isBtIsFactoryResetted()))
    {
      //HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
      /* Insert delay 100 ms */
      HAL_Delay(100);

      if (isEzsBaudRateDelayPending())
      {
        /* Delay or arbitrary value. As a guide, the EZ-Serial user guide states that it takes ~150 ms for a "chipset reset and boot process". */
        HAL_Delay(500);
        incrementBtInitCmdsStep();
        btInitCommands();
      }
      else if (isEzsFactoryRebootDelayPending())
      {
        //TODO move away from fixed delay now that we're able to parse the boot message
        /* Experimentally found to be ~ 2.75s. */
        HAL_Delay(3000);
        incrementBtInitCmdsStep();
        btInitCommands();
      }

      if (!(resetCntCurrent--))
      {
        failCount++;

        BtStop(1);
        HAL_Delay(500);

        if (failCount <= 4)
        {
          if (failCount == 1)
          {
            baudToTry = 115200;
          }
          else if (failCount == 2)
          {
            baudToTry = 460800;
          }
          else if (failCount == 3)
          {
            baudToTry = 2000000;
          }
          else if (failCount == 4)
          {
            baudToTry = 500000;
          }

          ShimBt_setBtBaudRateToUse(baudToTry);
          BtStart();

          resetCntCurrent = resetCnt;
        }
        else
        {
          //SHIMMER_PRINTF("Operation failed, performing system reset\r\n");
          ////software POR reset
          //NVIC_SystemReset();
          LogAndStream_setBootStage(BOOT_STAGE_BLUETOOTH_FAILURE);
          break;
        }
      }
    }
  }
}

void BtStartDone(void)
{
  initBtInterrupts();
  shimmerStatus.btIsInitialised = true;

  /* TODO LP_MODE feature provides a noticable drop in current consumption but
   * Consensys is having difficulty communicating after connection is
   * established (could be due to the lack of CTS/RTS in prototype boards?) */
  //Allow LP Mode after configuring
  //Board_BT_LP_MODE(0);

  SHIMMER_PRINTF("BT init end\r\n");
}

void setBtConnectionState(bool state)
{
  shimmerStatus.btConnected = state;
  //HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, state? GPIO_PIN_SET:GPIO_PIN_RESET);

  ShimBt_handleBtRfCommStateChange(shimmerStatus.btConnected);
}

bool isBtConnected(void)
{
  return shimmerStatus.btConnected;
}

#endif

#if USE_CUSTOM_HAL_DELAY
/* TODO: Overriding HAL_DELAY() with this because USB peripheral init based
 * on VBUS interrupt was cause HAL_DELAY() to be stuck in a loop changing the
 * interrupt priority didn't help */
void HAL_Delay(uint32_t Delay)
{
  /* Delay for amount of milliseconds */
  if (__get_IPSR() == 0)
  {
    uint32_t tickstart = HAL_GetTick();
    while ((HAL_GetTick() - tickstart) < Delay)
    {
      __WFI();
    }
  }
  else
  {
    while (Delay)
    {
      if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
      {
        Delay--;
      }
    }
  }
}
#endif

void platform_sleepWhenNoTask(void)
{
  if (USBX_IsInitialised())
  {
    /* idle: sleep until next IRQ (SOF, UART RX, HAL_GetTick SysTick, etc.) */
    __WFI();
  }
  else
  {
    /* Only wake MCU when new Task is set. See corresponding
     * HAL_PWR_DisableSleepOnExit() in ShimTask_set() */
    HAL_PWR_EnableSleepOnExit();

    Power_SleepUntilInterrupt();
  }
}

void BtStart(void)
{
  //Best to check if BT is powered on as it could be on but not yet initialised
  if (!shimmerStatus.btPowerOn)
  {
    ShimBt_startCommon();
    btInit();
  }
}

void BtStop(uint8_t isCalledFromMain)
{
  if (shimmerStatus.btPowerOn)
  {
    SHIMMER_PRINTF("\r\nBT Stop\r\n");

    //BT_disable
    btDeinit();

    ShimBt_stopCommon(isCalledFromMain);
  }
}

float samplingClockFreqGet(void)
{
  return 32768.0f;
}

uint8_t getDefaultBaudForBtVersion(void)
{
  return 12; //To represent 2M baud rate
}

void stopSensingWrapup(void)
{
}

void JumpToBootloaderIfRequired(void)
{
  if (USER_BTN_PRESSED)
  {
    for (shimmerStatus.bslCheckTimeMs = 100U;
         shimmerStatus.bslCheckTimeMs <= BOOTLOADER_ENTRY_THRESHOLD_MS;
         shimmerStatus.bslCheckTimeMs += 100)
    {
      HAL_Delay(100U); //Wait 100ms before checking again

      if (!USER_BTN_PRESSED)
      {
        shimmerStatus.bslCheckTimeMs = 0;
        //Button released, exit BSL check
        break;
      }

      if (shimmerStatus.bslCheckTimeMs == BOOTLOADER_ENTRY_THRESHOLD_MS)
      {
        //Button pressed for long enough, jump to bootloader
        JumpToBootloader();
      }
    }
  }
}

HAL_StatusTypeDef checknBoot0OptionByte(void)
{
  FLASH_OBProgramInitTypeDef OB;
  HAL_FLASHEx_OBGetConfig(&OB);

  uint32_t nBoot0State = ShimBrd_checkCorrectStateForBoot0() ? FLASH_OPTR_nBOOT0_Msk : 0U;

  /* OB.USERConfig returns the FLASH_OPTR register */
  //Use it to check if OB programming is necessary
  if ((OB.USERConfig & FLASH_OPTR_nBOOT0_Msk) != nBoot0State)
  {

    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();

    OB.OptionType = OPTIONBYTE_USER;
    OB.USERType = OB_USER_NBOOT0;
    OB.USERConfig = nBoot0State ? OB_NBOOT0_SET : OB_NBOOT0_RESET;

    if (HAL_FLASHEx_OBProgram(&OB) != HAL_OK)
    {
      HAL_FLASH_OB_Lock();
      HAL_FLASH_Lock();
      return HAL_ERROR;
    }

    /* This should cause a reboot */
    HAL_FLASH_OB_Launch();

    /* We should not make it past the Launch, so lock
     * flash memory and return an error from function
     */
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    return HAL_ERROR;
  }

  return HAL_OK;
}

void platform_delayMs(const uint32_t delay_time_ms)
{
  HAL_Delay(delay_time_ms);
}

void platform_reset(void)
{
  NVIC_SystemReset();
}

uint32_t platform_getTick(void)
{
  return HAL_GetTick();
}

bool platform_isDockUartInitialised(void)
{
  return DockUart_isInitialised();
}

bool platform_isUsbUartInitialised(void)
{
  return USBX_IsInitialised();
}

//Overrides weak function in LogAndStream driver
uint8_t ShimBrd_doesDeviceSupportBle(void)
{
  return 1;
}

//Overrides weak function in LogAndStream driver
uint8_t ShimBrd_doesDeviceSupportBtClassic(void)
{
  return 1;
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
