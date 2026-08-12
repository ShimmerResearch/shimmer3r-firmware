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
#include "version.h"
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

/* Firmware version marker embedded in the .version linker section for
 * traceability. Defined here (rather than in the auto-generated version.h) so
 * version.h can be included by multiple translation units without duplicating
 * this object across the .version section. */
__attribute__((section(".version"), used)) const firmware_version_t fw_version_struct
    = { .major = FW_VERSION_MAJOR, .minor = FW_VERSION_MINOR, .patch = FW_VERSION_PATCH };

volatile uint32_t time_start, time_end, time_diff;

/* DEV-866: LSE drive strength chosen by Lse_bringUp() at boot, stashed so
 * it can be printed from Init(). The print inside the escalation runs before
 * the core clock is configured, so its SWV/ITM output goes out at the wrong SWO
 * baud and is dropped by the host - hence a second, reliable print later. */
static const char *gLseDriveName = "not run";
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

  /* DEV-866: report the LSE drive strength the boot escalation settled on. Done
   * here rather than in Lse_bringUp() because that runs before the core
   * clock is configured (SWV output there is dropped); by Init() the clock is
   * up and SWV is reliable. */
  SHIMMER_PRINTF("LSE drive strength = %s\r\n", gLseDriveName);

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
  /* Detect which pressure sensor is fitted (BMP390 vs BMP581) and, for the
   * BMP390, read its calibration data. This is done before the configuration
   * is loaded so that any sensor specific validation (e.g. the pressure
   * oversampling range) is carried out against the correct sensor. */
  PressureSensor_initOnBoot();
  /* Calibration needs to be loaded after the chips have been detected in
   * order to know which default calib to set for attached chips.
   * It also needs to be loaded after the BT is initialised so that the
   * MAC ID can be used for default Shimmer name and calibration file names.*/
  ShimConfig_loadSensorConfigAndCalib();

  /* Sample both dock and USB-VBUS pins so the ownership decision below has
   * the complete picture.  LogAndStream_updateDockedStateAndCheckChanged() was
   * already called earlier (line 152) but we re-read here for consistency.  We
   * deliberately do NOT call LogAndStream_dockOrUsbStateUpdate() because it
   * fires LogAndStream_dockedStateChange() → TASK_SETUP_DOCK, which would cause
   * a redundant second pass through setupDock() from the main loop. */
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

      /* CDC TX: poll the write task whenever the CDC ACM instance is
       * valid and the device is configured, even if the host has
       * closed the COM port (!IsPortOpen). This still allows the
       * internal stall watchdog to abort transfers that were queued
       * while the port was open and then got stranded when the host
       * stopped polling / closed the port, without risking a NULL
       * dereference if the device disconnects or deactivates while a
       * TX is in flight. Gating only on IsPortOpen() (as the old code
       * did) meant a stale in-flight transfer could wedge tx_active=1
       * forever until the next port open, at which point every new
       * USBX_CDC_ACM_Transmit() would return usbx_busy. */
      if (USBX_CDC_ACM_IsPortConfigured())
      {
        cdc_acm_write_task();

        /* CDC RX: only touch once the host has opened the port (DTR
         * asserted). Before that, arming a bulk-OUT receive is wasted
         * work. */
        if (USBX_CDC_ACM_IsPortOpen())
        {
          cdc_acm_read_task();
        }
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
#define LSE_STOP_TIMEOUT_MS  100U
#define LSE_START_TIMEOUT_MS 1500U

/* Try one LSE drive level: fully restart the LSE + its system clock at `drive`,
 * then wait for LSESYSRDY - the LSE *system-clock* ready. Success is judged by
 * LSESYSRDY, NOT bare LSERDY: LSERDY is a loose startup counter that a marginal
 * / falsely-started oscillation can trip, whereas LSESYSRDY only sets when the
 * LSE is genuinely stable and system-usable (and it is exactly the flag
 * HAL_RCC_OscConfig() waits on afterward). LSEDRV is writable only while
 * LSEON = 0, so the full stop is required; no backup-domain reset is used, so
 * RTC time is preserved. Returns true if the level reached LSESYSRDY. */
static bool Lse_tryDriveLevel(uint32_t drive)
{
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSESYSEN | RCC_BDCR_LSEON);
  uint32_t start = HAL_GetTick();
  while (((RCC->BDCR & (RCC_BDCR_LSERDY | RCC_BDCR_LSESYSRDY)) != 0U)
      && ((HAL_GetTick() - start) < LSE_STOP_TIMEOUT_MS))
  {
  }

  MODIFY_REG(RCC->BDCR, RCC_BDCR_LSEDRV, drive);
  SET_BIT(RCC->BDCR, RCC_BDCR_LSEON | RCC_BDCR_LSESYSEN);

  start = HAL_GetTick();
  while ((HAL_GetTick() - start) < LSE_START_TIMEOUT_MS)
  {
    if ((RCC->BDCR & RCC_BDCR_LSESYSRDY) != 0U)
    {
      return true;
    }
  }
  return false;
}

/* DEV-866: bring up the 32.768 kHz LSE with a per-board minimum + margin.
 * The reworked crystals' effective load - and the drive needed to start - vary
 * board to board (hand-rework cap tolerance, PCB stray, ESR), so we escalate
 * LOW -> MEDIUMLOW -> MEDIUMHIGH to find the LOWEST level that reaches
 * LSESYSRDY, then run ONE LEVEL ABOVE that (capped at MEDIUMHIGH - per DEV-866
 * 5f372ce7, HIGH may exceed the CM315D's 0.5 uW max drive level). The +1 gives
 * temperature headroom: a 32 kHz fork's ESR rises at temperature extremes, so
 * it can need more drive mid-session than at boot, and the drive is fixed until
 * the next boot - the margin covers that swing without over-driving to the max.
 *
 * Caveat: the found minimum is only as trustworthy as the boot's start
 * condition. A warm reset (crystal still ringing) lets a low level "pass"
 * because it only has to sustain, not cold-start - so on a warm reset the
 * baseline can read low and the +1 margin is measured from there. It is fully
 * correct on a genuine cold boot; on warm resets the margin still adds a level
 * of headroom over whatever sustained.
 *
 * Returns the RCC_LSEDRIVE_* level actually applied, or 0xFFFFFFFF if no level
 * reached LSESYSRDY. */
static uint32_t Lse_walkDriveLadder(void)
{
  static const uint32_t levels[]
      = { RCC_LSEDRIVE_LOW, RCC_LSEDRIVE_MEDIUMLOW, RCC_LSEDRIVE_MEDIUMHIGH };
  static const char *const names[] = { "LOW", "MEDIUMLOW", "MEDIUMHIGH" };
  const uint32_t nLevels = (uint32_t) (sizeof(levels) / sizeof(levels[0]));

  for (uint32_t i = 0U; i < nLevels; i++)
  {
    if (Lse_tryDriveLevel(levels[i]))
    {
      /* This level is stable -> just run the next one up for margin (capped at
       * MEDIUMHIGH), applied blindly: a higher drive works if a lower one did. */
      uint32_t runIdx = ((i + 1U) < nLevels) ? (i + 1U) : i;
      if (runIdx != i)
      {
        (void) Lse_tryDriveLevel(levels[runIdx]);
      }
      gLseDriveName = names[runIdx];
      SHIMMER_PRINTF("LSE stable at %s, running at %s (+1 margin, "
                     "LSEDRV=%lu)\r\n",
          names[i], names[runIdx],
          (unsigned long) ((levels[runIdx] & RCC_BDCR_LSEDRV) >> RCC_BDCR_LSEDRV_Pos));
      return levels[runIdx];
    }
  }

  gLseDriveName = "NONE (LSE not stable at any drive)";
  SHIMMER_PRINTF("LSE did NOT reach LSESYSRDY at any drive level - "
                 "check 32k XTAL / rework\r\n");
  return 0xFFFFFFFFU;
}

/* DEV-866: set when every LSE recovery step failed and the RTC runs from the
 * LSI instead (see Boot_rtcIsOnLsiFallback in main.h). */
static uint8_t gRtcOnLsiFallback = 0U;

uint8_t Boot_rtcIsOnLsiFallback(void)
{
  return gRtcOnLsiFallback;
}

const char *Boot_getLseDriveName(void)
{
  return gLseDriveName;
}

/* DEV-866: full LSE bring-up ladder. Each rung only runs if the one before it
 * failed, so a healthy board pays nothing beyond the normal LSE start wait:
 *
 * 1. Adaptive drive escalation (Lse_walkDriveLadder above) - recovers a
 *    marginal / heavily-loaded crystal and, because each attempt toggles LSEON
 *    (required anyway: LSEDRV is write-locked while LSEON = 1), also recovers
 *    the battery-death case where a brown-out left LSEON = 1 latched in the
 *    backup domain with the crystal stopped and the drive write-locked.
 * 2. Backup-domain reset + one re-escalation - clears a poisoned RCC->BDCR
 *    that even rung 1 cannot untangle. This does in firmware exactly what
 *    removing all power does on the bench (the previous "fix" was a debugger
 *    session, whose recovery was really the power removal it included). The
 *    stored RTC time is the only cost, and it was already lost the moment the
 *    crystal stopped - which is the only way to reach this rung.
 * 3. LSI fallback - if the crystal is genuinely dead (broken part / rework
 *    fault), flag it and let SystemClock_Config/HAL_RTC_MspInit start the RTC
 *    from the LSI so the device still boots, connects and can report the
 *    fault (factory test prints 'FAIL ... LSE not ready') instead of hanging
 *    forever in Error_Handler. Timekeeping is degraded: LSI is a ~32 kHz RC
 *    (+/-5%) and the RTC prescalers + SSR tick math are deliberately left at
 *    their 32768 Hz values so every consumer stays self-consistent (rtc.c
 *    hardcodes the 0x8000 SSR wrap and ticks*32768 conversions) - the clock
 *    simply runs ~2.4% slow rather than lying differently in different places.
 *
 * Returns the RCC_LSEDRIVE_* level applied, or 0xFFFFFFFF when on rung 3. */
static uint32_t Lse_bringUp(void)
{
  uint32_t level = Lse_walkDriveLadder();
  if (level != 0xFFFFFFFFU)
  {
    return level;
  }

  SHIMMER_PRINTF("Forcing backup-domain reset (clears latched LSE "
                 "state; RTC time already lost) and retrying\r\n");
  __HAL_RCC_BACKUPRESET_FORCE();
  __HAL_RCC_BACKUPRESET_RELEASE();
  level = Lse_walkDriveLadder();
  if (level != 0xFFFFFFFFU)
  {
    return level;
  }

  /* Leave the dead crystal's cell off rather than burning max drive into it
   * forever; RCC_LSE_OFF in SystemClock_Config then matches this state. */
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSESYSEN | RCC_BDCR_LSEON);
  gRtcOnLsiFallback = 1U;
  gLseDriveName
      = "NONE - RTC on LSI fallback (32k XTAL dead, timekeeping degraded)";
  SHIMMER_PRINTF("LSE unrecoverable, booting with RTC on LSI - unit "
                 "needs 32k XTAL service\r\n");
  return 0xFFFFFFFFU;
}

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

  /* DEV-866: bring up the LSE - escalate LOW -> MEDIUMLOW -> MEDIUMHIGH to the
   * lowest level that reaches LSESYSRDY (genuinely stable, not a false LSERDY),
   * then run ONE LEVEL ABOVE it for temperature margin (capped at MEDIUMHIGH).
   * Per-board adaptive; the normal path does NOT touch the backup domain, so
   * RTC time is preserved. If the full recovery ladder fails (see
   * Lse_bringUp), the boot continues with the RTC on the LSI instead of
   * hanging: LSE_OFF below keeps HAL_RCC_OscConfig() from waiting on the dead
   * crystal, and HAL_RTC_MspInit (rtc.c) selects the matching RTC source. */
  (void) Lse_bringUp();

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_LSI
      | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = Boot_rtcIsOnLsiFallback() ? RCC_LSE_OFF : RCC_LSE_ON;
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

  /* DEV-866 boot bring-up diagnostic.
   * Error_Handler() is the catch-all for every failed HAL_xxx call during
   * boot. After the XTAL load-cap rework the common failure is the 32.768 kHz
   * LSE not oscillating, which makes MX_RTC_Init() time out waiting for the RTC
   * INITF flag (rtc.c). Dump the oscillator/RTC state to the SWV ITM console so
   * a board can be triaged from the console alone, without the register view.
   * All output is compiled out in release builds (printf -> _write is #ifdef
   * DEBUG and also requires an attached debugger with ITM enabled). */
  const uint32_t bdcr = RCC->BDCR;
  const uint32_t rccCr = RCC->CR;
  const uint32_t rtcIcsr = RTC->ICSR;

  SHIMMER_PRINTF("\r\n==== Error_Handler (caller=0x%08lX) ====\r\n",
      (unsigned long) (uint32_t) __builtin_return_address(0));
  SHIMMER_PRINTF("RCC->CR  =0x%08lX  HSE_RDY(16MHz)=%lu  MSIS_RDY=%lu\r\n",
      (unsigned long) rccCr, (unsigned long) ((rccCr & RCC_CR_HSERDY) != 0U),
      (unsigned long) ((rccCr & RCC_CR_MSISRDY) != 0U));
  SHIMMER_PRINTF("RCC->BDCR=0x%08lX  LSE_ON=%lu LSE_RDY(32k)=%lu LSE_DRV=%lu "
                 "RTCSEL=%lu LSE_CSSD=%lu\r\n",
      (unsigned long) bdcr, (unsigned long) ((bdcr & RCC_BDCR_LSEON) != 0U),
      (unsigned long) ((bdcr & RCC_BDCR_LSERDY) != 0U),
      (unsigned long) ((bdcr & RCC_BDCR_LSEDRV) >> RCC_BDCR_LSEDRV_Pos),
      (unsigned long) ((bdcr & RCC_BDCR_RTCSEL) >> RCC_BDCR_RTCSEL_Pos),
      (unsigned long) ((bdcr & RCC_BDCR_LSECSSD) != 0U));
  SHIMMER_PRINTF("RTC->ICSR=0x%08lX  INIT=%lu INITF=%lu INITS=%lu RSF=%lu\r\n",
      (unsigned long) rtcIcsr, (unsigned long) ((rtcIcsr & RTC_ICSR_INIT) != 0U),
      (unsigned long) ((rtcIcsr & RTC_ICSR_INITF) != 0U),
      (unsigned long) ((rtcIcsr & RTC_ICSR_INITS) != 0U),
      (unsigned long) ((rtcIcsr & RTC_ICSR_RSF) != 0U));

  /* Plain-language verdict for the rework techs. */
  if ((bdcr & RCC_BDCR_LSECSSD) != 0U)
  {
    SHIMMER_PRINTF(
        ">> LSE Clock Security System fired: 32k XTAL failure detected.\r\n");
  }
  if ((bdcr & RCC_BDCR_LSERDY) == 0U)
  {
    SHIMMER_PRINTF(">> 32.768kHz LSE NOT READY: crystal not oscillating. Check "
                   "32k XTAL load caps / rework.\r\n");
  }
  else if ((rtcIcsr & RTC_ICSR_INITF) == 0U)
  {
    SHIMMER_PRINTF(
        ">> LSE reports READY but RTC cannot enter INIT: marginal 32k XTAL "
        "(wrong load caps / high ESR). Try higher LSE drive.\r\n");
  }
  if ((rccCr & RCC_CR_HSERDY) == 0U)
  {
    SHIMMER_PRINTF(
        ">> 16MHz HSE NOT READY: check 16MHz XTAL load caps / rework.\r\n");
  }

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
