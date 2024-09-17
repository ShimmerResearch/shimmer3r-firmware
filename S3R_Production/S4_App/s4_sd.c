/*
 * s4sd.cpp
 *
 *  Created on: Mar 21, 2024
 *      Author: MarkNolan
 */

#include <s4_sd.h>

#include "main.h"
#include "s4.h"
#include "s4__cfg.h"
#include "stm32u5xx_hal.h"

#if USE_FATFS
#include "fatfs.h"
#include "ff.h"
#else
#include "fx_api.h"
#endif

#include "Bluetooth/sd_sync.h"

uint8_t test_cnt;
uint8_t fileName[64], dirName[64], expDirName[32],
    dataBuf[100], //storedConfig[512], sdHeadText[SD_HEAD_SIZE],
    sdWrBuf[NUM_SDWRBUF][SD_WRITE_BUF_SIZE], //btMacHex[6], btMacAscii[14],
    sdBufInQ, expIdName[MAX_CHARS], shimmerName[MAX_CHARS],
    configTimeText[UINT32_LEN], //sdBufInQMax,
    dirLen, sdBufSens = 0, sdBufWr = 0;
uint16_t fileNum, dirCounter, sdWrLen[NUM_SDWRBUF];
uint64_t sdFileCrTs, sdFileSyncTs;
#if USE_FATFS
FRESULT file_status;
//char file_name_current[128];
FATFS fatfs;
DIR dir;
FIL dataFile;
FILINFO dataFileInfo;

extern uint8_t retSD;  /* Return value for SD */
extern char SDPath[4]; /* SD logical drive path */
extern FATFS SDFatFS;  /* File system object for SD logical drive */
extern FIL SDFile;     /* File object for SD */

static uint8_t all0xff[7U] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint8_t sdInfoSyncDelayed = 0;

#endif

void SD_init(void)
{
  sdBufSens = sdBufWr = 0;
  //fileNum = 0;

  //*configTimeText = '\0';
  //*fileName = '\0';
}

#define TEST_TEXT_LEN 40

uint8_t SD_test(void)
{
#if USE_FATFS
  FIL test_file;
#endif
  char file_name[] = "test.txt";
  char test_text1[TEST_TEXT_LEN] = "This is the 1st line of the test file.\n";
  char test_text2[TEST_TEXT_LEN] = "This is the 2nd line of the test file.\n";
  char test_text3[TEST_TEXT_LEN];
#if USE_FATFS
  UINT bw;
#endif

  Board_sd2Arm();
  stat.pinPvSd = 1;
  SD_mount(0);
  SD_mount(1);

#if USE_FATFS
  stat.badFile += f_open(&test_file, file_name, FA_CREATE_ALWAYS | FA_WRITE);
  stat.badFile += f_write(&test_file, test_text1, TEST_TEXT_LEN - 1, &bw);
  stat.badFile += f_write(&test_file, test_text2, TEST_TEXT_LEN - 1, &bw);
  stat.badFile += f_close(&test_file);

  memset(test_text3, 0, 40);
  stat.badFile += f_open(&test_file, file_name, FA_OPEN_EXISTING | FA_READ);
  stat.badFile += f_read(&test_file, test_text3, TEST_TEXT_LEN - 1, &bw);
  stat.badFile += f_close(&test_file);
#endif

  stat.badFile += strcmp(test_text1, test_text3);

  return stat.badFile;
}

uint8_t SD_test_alternative(void)
{
  FRESULT res = FR_OK; /* FatFs function common result code */
#if USE_FATFS
  uint32_t byteswritten; //, bytesread; /* File write/read counts */
  uint8_t wtext[] = "STM32 FATFS works great!"; /* File write buffer */
  uint8_t rtext[_MAX_SS];                       /* File read buffer */
  FIL SDFile;

  if (f_mount(&fatfs, (TCHAR const *) SDPath, 0) != FR_OK)
  {
    Error_Handler();
  }
  else
  {
    if (f_mkfs((TCHAR const *) SDPath, FM_ANY, 0, rtext, sizeof(rtext)) != FR_OK)
    {
      Error_Handler();
    }
    else
    {
      //Open file for writing (Create)
      if (f_open(&SDFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
      {
        Error_Handler();
      }
      else
      {

        //Write to the text file
        res = f_write(&SDFile, wtext, strlen((char *) wtext), (void *) &byteswritten);
        if ((byteswritten == 0) || (res != FR_OK))
        {
          Error_Handler();
        }
        else
        {

          f_close(&SDFile);
        }
      }
    }
  }
  f_mount(&SDFatFS, (TCHAR const *) NULL, 0);
#endif
  return res;
}

void SD_setShimmerName(void)
{
  uint8_t i;
  gConfigBytes *configBytes = S4Ram_getStoredConfig();

  memset(&shimmerName[0], 0x00, sizeof(shimmerName));

  for (i = 0; (i < MAX_CHARS - 1) && isprint((uint8_t) configBytes->shimmerName[i]); i++)
    ;
  if (i == 0)
  {
    setDefaultShimmerName();
  }
  memcpy((char *) shimmerName, &(configBytes->shimmerName[0]),
      sizeof(configBytes->shimmerName));
}

void SD_setExpIdName(void)
{
  uint8_t i;
  gConfigBytes *configBytes = S4Ram_getStoredConfig();

  memset(&expIdName[0], 0x00, sizeof(expIdName));

  for (i = 0; (i < MAX_CHARS - 1) && (isprint((uint8_t) configBytes->expIdName[i])); i++)
    ;
  if (i == 0)
  {
    setDefaultTrialId();
  }
  memcpy((char *) expIdName, &(configBytes->expIdName[0]), sizeof(configBytes->expIdName));
  //strcpy((char*)expIdName,"DefaultTrial");
}

void SD_setCfgTime(void)
{
  uint32_t cfg_time_temp = 0;
  uint8_t i;
  gConfigBytes *configBytes = S4Ram_getStoredConfig();

  //MSB order
  for (i = 0; i < 4; i++)
  {
    cfg_time_temp <<= 8;
    cfg_time_temp |= configBytes->rawBytes[NV_SD_CONFIG_TIME + i];
  }
  if (cfg_time_temp)
  {
    S4Calc_itoaNo0((uint64_t) cfg_time_temp, configTimeText, UINT32_LEN);
  }
  else
  {
    strcpy((char *) configTimeText, "0");
  }
}

void SetName(void)
{
  if (strlen((char *) configTimeText) == 0)
  {
    strcpy((char *) configTimeText, "0");
  }

  if (strlen((char *) fileName) == 0)
    strcpy((char *) fileName, "no_file   ");
}

void SD_infomem2Names(void)
{
  SD_setShimmerName();
  SD_setExpIdName();
  SD_setCfgTime();
}

uint8_t SD_setBasedir(void)
{
#if USE_FATFS
  FILINFO fno;
  //volatile uint8_t res;
  uint16_t tmp_counter = 0;
  char lfn[_MAX_LFN + 1], *fname, *scout, *dash, dirnum[8];

  SD_infomem2Names();
  //SetName();

  file_status = f_opendir(&dir, "/data");
  if (file_status)
  {
    if (file_status == FR_NO_PATH) //we'll have to make /data first
      file_status = f_mkdir("/data");
    if (file_status) //in every case, we're toast
      return 0;      //FAIL;

    //try one more time
    file_status = f_opendir(&dir, "/data");
    if (file_status)
      return 0; //FAIL;
  }
  file_status = f_closedir(&dir);

  strcpy((char *) expDirName, "data/");
  strcat((char *) expDirName, (char *) expIdName);
  strcat((char *) expDirName, "_");
  strcat((char *) expDirName, (char *) configTimeText);

  file_status = f_opendir(&dir, (char *) expDirName);
  if (file_status)
  {
    if (file_status == FR_NO_PATH) //we'll have to make the experiment folder first
      file_status = f_mkdir((char *) expDirName);
    if (file_status) //in every case, we're toast
      return 0;      //FAIL;

    //try one more time
    file_status = f_opendir(&dir, (char *) expDirName);
    if (file_status)
      return 0; //FAIL;
  }

  dirCounter = 0; //this might be the first log for this shimmer

  //file name format
  //shimmername    as defined in sdlog.cfg
  //-              separator
  //000
  //we want to create a new directory with a sequential run number each power-up/reset for each shimmer
  while (f_readdir(&dir, &fno) == FR_OK)
  {
    if (*fno.fname == 0)
      break;
    else if (fno.fattrib & AM_DIR)
    {
      fname = (*lfn) ? lfn : fno.fname;

      if (!strncmp(fname, (char *) shimmerName, strlen(fname) - 4))
      { //-4 because of the -000 etc.
        scout = strchr(fname, '-');
        if (scout)
        { //if not, something is seriously wrong!
          scout++;
          //while(dash = strchr(scout, '-'))  // In case the shimmer name contains '-'
          //   scout = dash + 1;
          dash = strchr(scout, '-');
          while (dash)
          { //In case the shimmer name contains '-'
            scout = dash + 1;
            dash = strchr(scout, '-');
          }
          strcpy(dirnum, scout);
          tmp_counter = atoi(dirnum);
          if (tmp_counter >= dirCounter)
          {
            dirCounter = tmp_counter;
            dirCounter++; //start with next in numerical sequence
          }
        }
        else
          return 0; //FAIL;
      }
    }
  }
  file_status = f_closedir(&dir);

  //at this point, we have the id string and the counter, so we can make a directory name
  return 1; //SUCCESS;
#endif
}

uint8_t SD_makeBasedir(void)
{
  memset(dirName, 0, 64);

  char dir_counter_text[4];
  S4Calc_itoaWith0((uint64_t) dirCounter, (uint8_t *) dir_counter_text, 4);

  strcpy((char *) dirName, (char *) expDirName);
  strcat((char *) dirName, "/");
  strcat((char *) dirName, (char *) shimmerName);
  strcat((char *) dirName, "-");
  strcat((char *) dirName, dir_counter_text);

#if USE_FATFS
  file_status = f_mkdir((char *) dirName);
  if (file_status)
  {
    //FindError(fileBad, dirName);
    return 0; //FAIL;
  }
#endif

  memset(fileName, 0, 64);
  strcpy((char *) fileName, (char *) dirName);
  dirLen = strlen((char *) dirName);
  strcat((char *) fileName, "/000");
  fileNum = 0;
  //sprintf((char*)fileName, "/%03d", fileNum++);

  return 1; //SUCCESS;
}

void SD_makeFileName(char *name_buf)
{
  //strcpy(file_name, "this_is_a_very_long_name_for_the_dataFile");
  uint8_t temp_str[7];
  sprintf((char *) temp_str, "/%03d", fileNum++);

  strcpy((char *) name_buf, (char *) dirName);
  strcat((char *) name_buf, (char *) temp_str);
}

void SD_fileInit(void)
{
#if USE_SD
  char file_name[128];
  //char file_name[128];
#if USE_FATFS
  UINT bw;
#endif
  uint8_t temp_sdHeadText[SD_HEAD_SIZE];

  //SD_config2SdHead();
  //Board_sdPowerCycle();
  S4Ram_config2SdHead();

  sensing.isSdOperating = 1;
  //SD_mount(0);
  //SD_mount(1);
  SD_setBasedir();
  SD_makeBasedir();
  SD_makeFileName(file_name);

  S4Ram_sdHeadTextGet(temp_sdHeadText, 0, SD_HEAD_SIZE);

#if USE_FATFS
  file_status = f_open(&dataFile, file_name, FA_CREATE_ALWAYS | FA_WRITE);
  f_stat(file_name, &dataFileInfo);
#endif
  sdFileSyncTs = sdFileCrTs = RTC_get64();
#if USE_8BYTES_INIT_TS
  *(uint64_t *) (temp_sdHeadText + SDH_MY_LOCALTIME_0TH) = sdFileSyncTs;
#else
  temp_sdHeadText[SDH_MY_LOCALTIME_5TH] = (sdFileSyncTs >> 32) & 0xff;
  *(uint32_t *) (temp_sdHeadText + SDH_MY_LOCALTIME)
      = (uint32_t) (sdFileSyncTs & 0xffffffff);
#endif
#if USE_FATFS
  file_status = f_write(&dataFile, temp_sdHeadText, SD_HEAD_SIZE, &bw);
#endif
  sensing.isSdOperating = 0;
  sensing.isFileCreated = 1;
  memset(sdWrLen, 0, NUM_SDWRBUF * sizeof(sdWrLen[0]));
  sdBufInQ = sdBufSens = sdBufWr = 0;
#endif //USE_SD
}

void SD_close(void)
{
#if USE_SD
#if USE_FATFS
  f_sync(&dataFile);
  f_close(&dataFile);
  file_status = FR_OK;
#endif
  //Board_sdPower(0);
  stat.badFile = 0;
  sensing.isFileCreated = 0;
#endif //USE_SD
}

void SD_writeToBuff(uint8_t *buf, uint16_t len)
{
#if USE_SD
  //uint8_t *sensing_buf;
  //uint16_t *sensing_buf_len;
  if ((NUM_SDWRBUF == sdBufInQ) || (0 == sensing.isFileCreated))
  {
    __NOP();
    return;
  }
  //sdWrBuf[NUM_SDWRBUF][SD_WRITE_BUF_SIZE], sdBufSens, sdBufWr, sdBufInQ;

  memcpy(sdWrBuf[sdBufSens] + sdWrLen[sdBufSens], buf, len);
  sdWrLen[sdBufSens] += len;
  if (sdWrLen[sdBufSens] + len > SD_WRITE_BUF_SIZE)
  {
    S4_Task_set(TASK_SDWRITE);
    sdBufInQ++;
    sdBufSens++;
    if (sdBufSens >= NUM_SDWRBUF)
    {
      sdBufSens = 0;
    }
  }
#endif //USE_SD
}

void SD_writeToCard(void)
{
  //__disable_irq();
#if USE_FATFS
  UINT bw;
#endif
  uint8_t *writing_buf;
  uint16_t *writing_buf_len;

  writing_buf = sdWrBuf[sdBufWr];
  writing_buf_len = sdWrLen + sdBufWr;

  __NOP();

  if ((0 == *writing_buf_len) || (0 == sdBufInQ))
  {
    return;
  }

  sensing.inSdWr = 1;
  //HAL_Delay(5);

  sensing.isSdOperating = 1;

#if USE_FATFS
  file_status = f_lseek(&dataFile,
      f_size(&dataFile)); //dataFileInfo.fsize was not incrementing the file size.
  assert_param(file_status == FR_OK);
  file_status = f_write(&dataFile, writing_buf, *writing_buf_len, &bw);
  assert_param(file_status == FR_OK);
#endif

  __NOP();
  __NOP();

  /*split file every hour upwards from 000*/
  if ((sensing.latestTs - sdFileCrTs) >= BIN_FILE_SPLIT_TIME_TICKS)
  { //(&& (test_cnt < 15))
    //sdFileCrTs = sensing.latestTs;
    sdFileSyncTs = sdFileCrTs = RTC_get64();
    char file_name[256];
#if USE_FATFS
    file_status = f_sync(&dataFile);
    assert_param(file_status == FR_OK);
    file_status = f_close(&dataFile);
    assert_param(file_status == FR_OK);
#endif
    SD_makeFileName(file_name);
#if USE_FATFS
    file_status = f_open(&dataFile, file_name, FA_CREATE_ALWAYS | FA_WRITE);
    assert_param(file_status == FR_OK);
#endif
    //S4_RTC_GetDateTime(&hrtc, &rtc_data);
    //sdFileSyncTs = sdFileCrTs = rtc_data.ticks;
    //memcpy(sdHeadText, (uint8_t*)&time_diff, 4);
    uint8_t temp_sdHeadText[SD_HEAD_SIZE];
    S4Ram_sdHeadTextGet(temp_sdHeadText, 0, SD_HEAD_SIZE);
#if USE_8BYTES_INIT_TS
    *(uint64_t *) (temp_sdHeadText + SDH_MY_LOCALTIME_0TH) = sdFileSyncTs;
#else
    temp_sdHeadText[SDH_MY_LOCALTIME_5TH] = (sdFileSyncTs >> 32) & 0xff;
    *(uint32_t *) (temp_sdHeadText + SDH_MY_LOCALTIME)
        = (uint32_t) (sdFileSyncTs & 0xffffffff);
#endif

#if USE_FATFS
    file_status = f_write(&dataFile, temp_sdHeadText, SD_HEAD_SIZE, &bw);
    assert_param(file_status == FR_OK);
#endif
  }
  /*Sync file every minute*/
  else if (sensing.latestTs - sdFileSyncTs >= BIN_FILE_SYNC_TIME_TICKS)
  {
#if USE_FATFS
    file_status = f_sync(&dataFile);
    assert_param(file_status == FR_OK);
#endif
    sdFileSyncTs = RTC_get64();
  }
  //else if (test_cnt >= 15) {
  //   f_close(&dataFile);
  //}

  sensing.isSdOperating = 0;

  *writing_buf_len = 0;
  //sdBufWr++;
  if (++sdBufWr >= NUM_SDWRBUF)
  {
    sdBufWr = 0;
  }
  //sdBufInQ--;
  if (--sdBufInQ)
  {
    S4_Task_set(TASK_SDWRITE);
  }

#if USE_FATFS
  if (file_status != 0)
  {
    stat.badFile = 1;
  }
  else
  {
    stat.badFile = 0;
  }
#endif

  sensing.inSdWr = 0;
  sensing.inSdWrCnt = 0;
  //__enable_irq();
}

void SD_mount(uint8_t val)
{
#if USE_FATFS
  if (1 == val)
  {
    f_mount(&fatfs, (TCHAR const *) SDPath, 0);
  }
  else
  {
    f_mount(0, (TCHAR const *) SDPath, 0);
  }
#endif
}

void UpdateSdConfig(void)
{
  FIL cfgFile;

  if (!stat.isDocked && CheckSdInslot() && !stat.badFile)
  {
    uint8_t sd_power_state;
    if (!isSdPowerOn())
    {
      SdPowerOn();
      sd_power_state = 0;
    }
    else
    {
      sd_power_state = 1;
    }

    char buffer[66], val_char[21];
    float val_num;
    uint16_t val_int, val_f;
    //uint32_t temp32;
    uint64_t temp64;

    uint8_t i;
    //uint16_t temp16;
    resetSyncVariablesBeforeParseConfig();

    UINT bw;

    memset(shimmerName, 0, sizeof(shimmerName));
    memset(expIdName, 0, sizeof(expIdName));
    memset(configTimeText, 0, sizeof(configTimeText));

    char cfgname[] = "sdlog.cfg";

    gConfigBytes *storedConfig = S4Ram_getStoredConfig();

    if (memcmp(all0xff, S4Ram_getStoredConfig(), 6))
    {
      file_status = f_open(&cfgFile, cfgname, FA_WRITE | FA_CREATE_ALWAYS);

      //sensor0
      sprintf(buffer, "accel=%d\r\n", storedConfig->chEnLnAccel);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "gyro=%d\r\n", storedConfig->chEnGyro);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "mag=%d\r\n", storedConfig->chEnMag);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "exg1_24bit=%d\r\n", storedConfig->chEnExg1_24Bit);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "exg2_24bit=%d\r\n", storedConfig->chEnExg2_24Bit);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "gsr=%d\r\n", storedConfig->chEnGsr);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "extch7=%d\r\n", storedConfig->chEnExtADC0);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "extch6=%d\r\n", storedConfig->chEnExtADC1);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      //sensor1
      sprintf(buffer, "br_amp=%d\r\n", storedConfig->chEnBridgeAmp);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "vbat=%d\r\n", storedConfig->chEnVBattery);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "accel_d=%d\r\n", storedConfig->chEnWrAccel);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "extch15=%d\r\n", storedConfig->chEnExtADC2);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "intch1=%d\r\n", storedConfig->chEnIntADC3);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "intch12=%d\r\n", storedConfig->chEnIntADC0);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "intch13=%d\r\n", storedConfig->chEnIntADC1);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      //sensor2
      sprintf(buffer, "intch14=%d\r\n", storedConfig->chEnIntADC2);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "accel_mpu=%d\r\n", storedConfig->chEnAltAccel);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "mag_mpu=%d\r\n", storedConfig->chEnAltMag);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "exg1_16bit=%d\r\n", storedConfig->chEnExg1_16Bit);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "exg2_16bit=%d\r\n", storedConfig->chEnExg2_16Bit);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "pres=%d\r\n", storedConfig->chEnPressureAndTemperature);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      //sample_rate
      val_num = 32768.0 / storedConfig->samplingRateTicks;
      val_int = (uint16_t) floor(val_num); //the compiler can't handle sprintf %f here
      val_f = (uint16_t) round((val_num - floor(val_num)) * 100);
      if (val_f == 100)
      {
        val_f = 0;
        val_int++;
      }
      if (val_f)
      {
        sprintf(val_char, "%d.%d", val_int, val_f);
      }
      else
      {
        sprintf(val_char, "%d", val_int);
      }
      sprintf(buffer, "sample_rate=%s\r\n", val_char);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      //setup config
      sprintf(buffer, "mg_internal_rate=%d\r\n", storedConfig->magRate);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "mg_range=%d\r\n", storedConfig->magRange);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "acc_internal_rate=%d\r\n", storedConfig->wrAccelRate);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "accel_mpu_range=%d\r\n", storedConfig->altAccelRange);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "pres_bmp390_prec=%d\r\n", storedConfig->pressurePrecision);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "gsr_range=%d\r\n", storedConfig->gsrRange);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "exp_power=%d\r\n", storedConfig->expansionBoardPower);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "gyro_range=%d\r\n", storedConfig->gyroRange);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "gyro_samplingrate=%d\r\n", storedConfig->gyroRate);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "acc_range=%d\r\n", storedConfig->wrAccelRange);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "acc_lpm=%d\r\n", storedConfig->wrAccelLPM);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "acc_hrm=%d\r\n", storedConfig->wrAccelHRM);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      //trial config
      sprintf(buffer, "user_button_enable=%d\r\n", storedConfig->userButtonEnable);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "rtc_error_enable=%d\r\n", storedConfig->rtcErrorEnable);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "sd_error_enable=%d\r\n", storedConfig->sdErrorEnable);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "iammaster=%d\r\n", storedConfig->master);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "sync=%d\r\n", storedConfig->sync);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "low_battery_autostop=%d\r\n", storedConfig->lowBatteryCutOut);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "interval=%d\r\n", storedConfig->btInterval);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);

      sprintf(buffer, "max_exp_len=%d\r\n", storedConfig->experimentLengthMaxInMinutes);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);

      sprintf(buffer, "est_exp_len=%d\r\n", storedConfig->experimentLengthEstimatedInSec);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);

      parseSyncNodeNamesFromConfig(&storedConfig->rawBytes[0]);
      for (i = 0; i < getSyncNodeNum(); i++)
      {
        sprintf(buffer, "node=%s\r\n", (char *) getSyncNodeNamePtrForIndex(i));
        f_write(&cfgFile, buffer, strlen(buffer), &bw);
      }

      if (memcmp(all0xff, storedConfig + NV_CENTER, 6))
      {
        parseSyncCenterNameFromConfig(&storedConfig->rawBytes[0]);
        sprintf(buffer, "center=%s\r\n", (char *) getSyncCenterNamePtr());
        f_write(&cfgFile, buffer, strlen(buffer), &bw);
      }

      sprintf(buffer, "singletouch=%d\r\n", storedConfig->singleTouchStart);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);

      sprintf(buffer, "myid=%d\r\n", storedConfig->myTrialID);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "Nshimmer=%d\r\n", storedConfig->numberOfShimmers);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);

      SD_infomem2Names();
      sprintf(buffer, "shimmername=%s\r\n", shimmerName);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "experimentid=%s\r\n", expIdName);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "configtime=%s\r\n", configTimeText);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);

      sprintf(buffer, "baud_rate=%d\r\n", storedConfig->btCommsBaudRate);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      //temp32 = storedConfig[NV_DERIVED_CHANNELS_0]
      //       + (((uint32_t)storedConfig[NV_DERIVED_CHANNELS_1])<<8)
      //       + (((uint32_t)storedConfig[NV_DERIVED_CHANNELS_2])<<16);
      //ItoaNo0((uint64_t)temp32, (uint8_t*)val_char, 9);
      temp64 = storedConfig->rawBytes[NV_DERIVED_CHANNELS_0]
          + (((uint64_t) storedConfig->rawBytes[NV_DERIVED_CHANNELS_1]) << 8)
          + (((uint64_t) storedConfig->rawBytes[NV_DERIVED_CHANNELS_2]) << 16)
          + (((uint64_t) storedConfig->rawBytes[NV_DERIVED_CHANNELS_3]) << 24)
          + (((uint64_t) storedConfig->rawBytes[NV_DERIVED_CHANNELS_4]) << 32)
          + (((uint64_t) storedConfig->rawBytes[NV_DERIVED_CHANNELS_5]) << 40)
          + (((uint64_t) storedConfig->rawBytes[NV_DERIVED_CHANNELS_6]) << 48)
          + (((uint64_t) storedConfig->rawBytes[NV_DERIVED_CHANNELS_7]) << 56);
      ItoaNo0(temp64, (uint8_t *) val_char, 21);
      sprintf(buffer, "derived_channels=%s\r\n", val_char); //todo: got value 0?
      f_write(&cfgFile, buffer, strlen(buffer), &bw);

      sprintf(buffer, "EXG_ADS1292R_1_CONFIG1=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_CONFIG1]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_CONFIG2=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_CONFIG2]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_LOFF=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_LOFF]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_CH1SET=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_CH1SET]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_CH2SET=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_CH2SET]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_RLD_SENS=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_RLD_SENS]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_LOFF_SENS=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_LOFF_SENS]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_LOFF_STAT=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_LOFF_STAT]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_RESP1=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_RESP1]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_1_RESP2=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_1_RESP2]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_CONFIG1=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_CONFIG1]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_CONFIG2=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_CONFIG2]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_LOFF=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_LOFF]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_CH1SET=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_CH1SET]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_CH2SET=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_CH2SET]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_RLD_SENS=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_RLD_SENS]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_LOFF_SENS=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_LOFF_SENS]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_LOFF_STAT=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_LOFF_STAT]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_RESP1=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_RESP1]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);
      sprintf(buffer, "EXG_ADS1292R_2_RESP2=%d\r\n",
          storedConfig->rawBytes[NV_EXG_ADS1292R_2_RESP2]);
      f_write(&cfgFile, buffer, strlen(buffer), &bw);

      file_status = f_close(&cfgFile);

      HAL_Delay(100); //100ms @ 24MHz
    }
    else
    {
      file_status = FR_DISK_ERR;
    }
    if (!sd_power_state)
    {
      SdPowerOff();
    }
  }
}

void ParseConfig(void)
{
  FIL cfgFile;

  char buffer[66], *equals;
  uint8_t string_length = 0;
  float sample_rate = 51.2;
  uint16_t sample_period = 0;
  uint64_t derived_channels_val = 0;
  uint8_t broadcast_interval;
  uint8_t gsr_range = 0;

  uint8_t triggerSdCardUpdate = 0;

  CheckSdInslot();
  SD_insertedCheck();
  char cfgname[] = "sdlog.cfg";
  file_status = f_open(&cfgFile, cfgname, FA_READ | FA_OPEN_EXISTING);
  if (file_status == FR_NO_FILE)
  {
    IniReadInfoMem();
    UpdateSdConfig();
    //fileBad = 0;
  }
  else if (file_status != FR_OK)
  {
    stat.badFile = 0;
    //fileBad = (initializing) ? 0 : 1;
    return;
  }
  else
  {
    gConfigBytes stored_config_temp;

    resetSyncVariablesBeforeParseConfig();
    resetSyncNodeArray();

    broadcast_interval = SYNC_INT_C;

    memset((uint8_t *) (stored_config_temp.rawBytes), 0, NV_A_ACCEL_CALIBRATION); //0
    memset((uint8_t *) (stored_config_temp.rawBytes + NV_A_ACCEL_CALIBRATION), 0xff, 84);
    memset((uint8_t *) (stored_config_temp.rawBytes + NV_DERIVED_CHANNELS_3), 0, 5); //0
    memset((uint8_t *) (stored_config_temp.rawBytes + NV_SENSORS3), 0, 5); //0
    memset((uint8_t *) (stored_config_temp.rawBytes + NV_MPL_ACCEL_CALIBRATION), 0xff, 82);
    memset((uint8_t *) (stored_config_temp.rawBytes + NV_SD_MYTRIAL_ID), 0, 9); //0
    InfoMem_readRam(stored_config_temp.rawBytes + NV_MAC_ADDRESS, NV_MAC_ADDRESS, 7);
    memset((uint8_t *) (stored_config_temp.rawBytes + NV_BT_SET_PIN + 1), 0xff, 24);
    memset((uint8_t *) (stored_config_temp.rawBytes + NV_CENTER), 0xff, 128);

    stored_config_temp.rawBytes[NV_SD_TRIAL_CONFIG0] &= ~SDH_SET_PMUX; //PMUX reserved as 0
    stored_config_temp.rawBytes[NV_SD_TRIAL_CONFIG0] |= SDH_TIME_STAMP; //TIME_STAMP always = 1
    stored_config_temp.gsrRange = GSR_AUTORANGE;
    stored_config_temp.bufferSize = 1;
    stored_config_temp.btCommsBaudRate = 0xFF;
    //changeBtBaudRate = BAUD_NO_CHANGE_NEEDED; // set this flag to no_event as every sdlog.cfg read is followed by a baudrate setting

    memset(shimmerName, 0, sizeof(shimmerName));
    memset(expIdName, 0, sizeof(expIdName));
    memset(configTimeText, 0, sizeof(configTimeText));

    stored_config_temp.shimmerName[0] = '\0';
    stored_config_temp.expIdName[0] = '\0';
    *configTimeText = '\0';
    stored_config_temp.configTime = 0;

    while (f_gets(buffer, 64, &cfgFile))
    {
      if (!(equals = strchr(buffer, '=')))
        continue;
      equals++; //this is the value
      if (strstr(buffer, "accel="))
        stored_config_temp.chEnLnAccel = atoi(equals);
      else if (strstr(buffer, "gyro="))
        stored_config_temp.chEnGyro = atoi(equals);
      else if (strstr(buffer, "mag="))
        stored_config_temp.chEnMag = atoi(equals);
      else if (strstr(buffer, "exg1_24bit="))
        stored_config_temp.chEnExg1_24Bit = atoi(equals);
      else if (strstr(buffer, "exg2_24bit="))
        stored_config_temp.chEnExg2_24Bit = atoi(equals);
      else if (strstr(buffer, "gsr="))
        stored_config_temp.chEnGsr = atoi(equals);
      else if (strstr(buffer, "extch7="))
        stored_config_temp.chEnExtADC0 = atoi(equals);
      else if (strstr(buffer, "extch6="))
        stored_config_temp.chEnExtADC1 = atoi(equals);
      else if (strstr(buffer, "str=") || strstr(buffer, "br_amp="))
        stored_config_temp.chEnBridgeAmp = atoi(equals);
      else if (strstr(buffer, "vbat="))
        stored_config_temp.chEnVBattery = atoi(equals);
      else if (strstr(buffer, "accel_d="))
        stored_config_temp.chEnWrAccel = atoi(equals);
      else if (strstr(buffer, "extch15="))
        stored_config_temp.chEnExtADC2 = atoi(equals);
      else if (strstr(buffer, "intch1="))
        stored_config_temp.chEnIntADC3 = atoi(equals);
      else if (strstr(buffer, "intch12="))
        stored_config_temp.chEnIntADC0 = atoi(equals);
      else if (strstr(buffer, "intch13="))
        stored_config_temp.chEnIntADC1 = atoi(equals);
      else if (strstr(buffer, "intch14="))
        stored_config_temp.chEnIntADC2 = atoi(equals);
      else if (strstr(buffer, "accel_mpu="))
        stored_config_temp.chEnAltAccel = atoi(equals);
      else if (strstr(buffer, "mag_mpu="))
        stored_config_temp.chEnAltMag = atoi(equals);
      else if (strstr(buffer, "exg1_16bit="))
        stored_config_temp.chEnExg1_16Bit = atoi(equals);
      else if (strstr(buffer, "exg2_16bit="))
        stored_config_temp.chEnExg2_16Bit = atoi(equals);
      else if (strstr(buffer, "pres="))
        stored_config_temp.chEnPressureAndTemperature = atoi(equals);
      else if (strstr(buffer, "sample_rate="))
      {
        sample_rate = atof(equals);
      }
      else if (strstr(buffer, "mg_internal_rate="))
      {
        stored_config_temp.magRate = atoi(equals);
      }
      else if (strstr(buffer, "mg_range="))
      {
        stored_config_temp.magRange = atoi(equals);
      }
      else if (strstr(buffer, "acc_internal_rate="))
      {
        stored_config_temp.wrAccelRate = atoi(equals);
      }
      else if (strstr(buffer, "accel_mpu_range="))
      {
        stored_config_temp.altAccelRange = atoi(equals);
      }
      else if (strstr(buffer, "acc_range="))
      {
        stored_config_temp.wrAccelRange = atoi(equals);
      }
      else if (strstr(buffer, "acc_lpm="))
      {
        stored_config_temp.wrAccelLPM = atoi(equals);
      }
      else if (strstr(buffer, "acc_hrm="))
      {
        stored_config_temp.wrAccelHRM = atoi(equals);
      }
      else if (strstr(buffer, "gsr_range="))
      { //or "gsr_range="?
        gsr_range = atoi(equals);
        if (gsr_range > 4)
          gsr_range = 4;

        stored_config_temp.gsrRange = gsr_range;
      }
      else if (strstr(buffer, "gyro_samplingrate="))
        stored_config_temp.gyroRate = atoi(equals);
      else if (strstr(buffer, "gyro_range="))
      {
        stored_config_temp.gyroRange = atoi(equals);
      }
      else if (strstr(buffer, "pres_bmp390_prec="))
      {
        stored_config_temp.pressurePrecision = atoi(equals);
      }
#if !RTC_OFF
      else if (strstr(buffer, "rtc_error_enable="))
      {
        stored_config_temp.rtcErrorEnable = atoi(equals);
      }
#endif
      else if (strstr(buffer, "sd_error_enable="))
      {
        stored_config_temp.sdErrorEnable = atoi(equals);
      }
      else if (strstr(buffer, "user_button_enable="))
      {
        stored_config_temp.userButtonEnable = atoi(equals);
      }
      else if (strstr(buffer, "iammaster="))
      { //0=slave=node
        stored_config_temp.master = atoi(equals);
      }
      else if (strstr(buffer, "sync="))
      {
        stored_config_temp.sync = atoi(equals);
      }
      else if (strstr(buffer, "low_battery_autostop="))
      {
        stored_config_temp.lowBatteryCutOut = atoi(equals);
      }
#if defined(SHIMMER3)
#if IS_SUPPORTED_TCXO
      else if (strstr(buffer, "tcxo="))
      {
        tcxo = (atoi(equals) == 0) ? FALSE : TRUE;
        stored_config_temp[NV_SD_TRIAL_CONFIG1] |= tcxo * SDH_TCXO;
      }
#endif
#endif
      else if (strstr(buffer, "interval="))
      {
        broadcast_interval = atoi(equals) > 255 ? 255 : atoi(equals);
      }
      else if (strstr(buffer, "exp_power="))
      {
        stored_config_temp.expansionBoardPower = atoi(equals);
      }
      else if (strstr(buffer, "center="))
      {
        parseSyncCenterNameFromCfgFile(&stored_config_temp.rawBytes[0], equals);
      }
      else if (strstr(buffer, "node"))
      {
        parseSyncNodeNameFromCfgFile(&stored_config_temp.rawBytes[0], equals);
      }
      else if (strstr(buffer, "est_exp_len="))
      {
        stored_config_temp.experimentLengthEstimatedInSec = atoi(equals);
      }
      else if (strstr(buffer, "max_exp_len="))
      {
        stored_config_temp.experimentLengthMaxInMinutes = atoi(equals);
      }
      else if (strstr(buffer, "myid="))
      {
        stored_config_temp.myTrialID = atoi(equals);
      }
      else if (strstr(buffer, "Nshimmer="))
      {
        stored_config_temp.numberOfShimmers = atoi(equals);
      }
      else if (strstr(buffer, "shimmername="))
      {
        string_length = strlen(equals);
        if (string_length > MAX_CHARS)
          string_length = MAX_CHARS - 1;
        else if (string_length >= 2)
          string_length -= 2;
        else
          string_length = 0;
        memcpy(&stored_config_temp.shimmerName[0], equals, string_length);
        if (!memcmp(&stored_config_temp.shimmerName[0], "ID", 2))
          memcpy(&stored_config_temp.shimmerName[0], "id", 2);
        memcpy((char *) shimmerName, &stored_config_temp.shimmerName[0], MAX_CHARS - 1);
        shimmerName[string_length] = 0;
      }
      else if (strstr(buffer, "experimentid="))
      {
        string_length = strlen(equals);
        if (string_length > MAX_CHARS)
          string_length = MAX_CHARS - 1;
        else if (string_length >= 2)
          string_length -= 2;
        else
          string_length = 0;
        memcpy(&stored_config_temp.expIdName[0], equals, string_length);
        memcpy((char *) expIdName, &stored_config_temp.expIdName[0], MAX_CHARS - 1);
        expIdName[string_length] = 0;
      }
      else if (strstr(buffer, "configtime="))
      {
        stored_config_temp.configTime = atol(equals);
        string_length = MAX_CHARS < strlen(equals) ? MAX_CHARS : strlen(equals) - 1;
        memcpy((char *) configTimeText, equals, string_length - 1);
        *(configTimeText + string_length - 1) = 0;
      }
      else if (strstr(buffer, "EXG_ADS1292R_1_CONFIG1="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_CONFIG1] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_CONFIG2="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_CONFIG2] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_LOFF="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_LOFF] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_CH1SET="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_CH1SET] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_CH2SET="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_CH2SET] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_RLD_SENS="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_RLD_SENS] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_LOFF_SENS="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_LOFF_SENS] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_LOFF_STAT="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_LOFF_STAT] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_RESP1="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_RESP1] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_1_RESP2="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_1_RESP2] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_CONFIG1="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_CONFIG1] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_CONFIG2="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_CONFIG2] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_LOFF="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_LOFF] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_CH1SET="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_CH1SET] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_CH2SET="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_CH2SET] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_RLD_SENS="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_RLD_SENS] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_LOFF_SENS="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_LOFF_SENS] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_LOFF_STAT="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_LOFF_STAT] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_RESP1="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_RESP1] = atoi(equals);
      else if (strstr(buffer, "EXG_ADS1292R_2_RESP2="))
        stored_config_temp.rawBytes[NV_EXG_ADS1292R_2_RESP2] = atoi(equals);
#if defined(SHIMMER3)
      else if (strstr(buffer, "baud_rate="))
      {
        config_baudrate = atoi(equals);
        if (config_baudrate != getCurrentBtBaudRate())
        {
#if BT_ENABLE_BAUD_RATE_CHANGE
          if (config_baudrate <= BAUD_1000000)
          {
            changeBtBaudRate = config_baudrate;
          }
#else
          triggerSdCardUpdate = 1;
#endif
        }
        stored_config_temp[NV_BT_COMMS_BAUD_RATE] = getCurrentBtBaudRate();
      }
#endif
      else if (strstr(buffer, "derived_channels="))
      {
        //derived_channels_val = atol(equals);
        //stored_config_temp[NV_DERIVED_CHANNELS_0] = derived_channels_val & 0xff;
        //stored_config_temp[NV_DERIVED_CHANNELS_1] = (derived_channels_val >> 8) & 0xff;
        //stored_config_temp[NV_DERIVED_CHANNELS_2] = (derived_channels_val >> 16) & 0xff;
        derived_channels_val = atoll(equals);
        stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_0] = derived_channels_val & 0xff;
        stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_1]
            = (derived_channels_val >> 8) & 0xff;
        stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_2]
            = (derived_channels_val >> 16) & 0xff;
        stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_3]
            = (derived_channels_val >> 24) & 0xff;
        stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_4]
            = (derived_channels_val >> 32) & 0xff;
        stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_5]
            = (derived_channels_val >> 40) & 0xff;
        stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_6]
            = (derived_channels_val >> 48) & 0xff;
        stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_7]
            = (derived_channels_val >> 56) & 0xff;
      }
    }
    file_status = f_close(&cfgFile);
    HAL_Delay(50); //50ms

    sample_period = (uint16_t) round(((float) 32768.0) / sample_rate);
    stored_config_temp.samplingRateTicks = sample_period;

    if (stored_config_temp.chEnGsr)
    { //they are sharing adc1, so ban intch1 when gsr is on
      stored_config_temp.chEnIntADC3 = 0;
      triggerSdCardUpdate = 1;
    }
    if (stored_config_temp.chEnBridgeAmp)
    { //they are sharing adc13 and adc14
      stored_config_temp.chEnIntADC1 = 0;
      stored_config_temp.chEnIntADC2 = 0;
      triggerSdCardUpdate = 1;
    }
    if (stored_config_temp.chEnExg1_24Bit)
    {
      stored_config_temp.chEnExg1_16Bit = 0;
      triggerSdCardUpdate = 1;
    }
    if (stored_config_temp.chEnExg2_24Bit)
    {
      stored_config_temp.chEnExg2_16Bit = 0;
      triggerSdCardUpdate = 1;
    }
    if (stored_config_temp.chEnExg1_24Bit || stored_config_temp.chEnExg2_24Bit
        || stored_config_temp.chEnExg1_16Bit || stored_config_temp.chEnExg2_16Bit)
    {
      stored_config_temp.chEnIntADC3 = 0;
      stored_config_temp.chEnIntADC2 = 0;
      triggerSdCardUpdate = 1;
    }

    if (stored_config_temp.gsrRange > 4)
    { //never larger than 4
      stored_config_temp.gsrRange = GSR_AUTORANGE;
      triggerSdCardUpdate = 1;
    }

    //minimum sync broadcast interval is 54 seconds
    if (broadcast_interval < SYNC_INT_C)
    {
      broadcast_interval = SYNC_INT_C;
      triggerSdCardUpdate = 1;
    }
    stored_config_temp.btInterval = broadcast_interval;

    //the button always works for singletouch mode
    //sync always works for singletouch mode
    if (stored_config_temp.singleTouchStart)
    {
      stored_config_temp.userButtonEnable = 1;
      stored_config_temp.sync = 1;
      triggerSdCardUpdate = 1;
    }

    checkSyncCenterName();
    setSyncEstExpLen(stored_config_temp.experimentLengthEstimatedInSec);

    /* Calibration bytes are not copied over from the infomem */

    /* Infomem D - Bytes 0-33 - General settings */
    gConfigBytes *storedConfig = S4Ram_getStoredConfig();
    memcpy(&storedConfig->rawBytes[0], &stored_config_temp.rawBytes[0], NV_NUM_SETTINGS_BYTES);
    /* Infomem D - Bytes 118-122 - Derived channel settings */
    memcpy(&storedConfig->rawBytes[NV_DERIVED_CHANNELS_3],
        &stored_config_temp.rawBytes[NV_DERIVED_CHANNELS_3], 5);
    /* Infomem C - Bytes 128-132 - MPL related settings - no longer used/supported */
    memcpy(&storedConfig->rawBytes[NV_SENSORS3],
        &stored_config_temp.rawBytes[NV_SENSORS3], 5);
    /* Infomem C - Bytes 187-223 - Shimmer name, exp ID, config time, trial ID, num Shimmers, trial config, BT interval, est exp len, max exp len */
    memcpy(&storedConfig->rawBytes[NV_SD_SHIMMER_NAME],
        &stored_config_temp.rawBytes[NV_SD_SHIMMER_NAME], 37);

    /* Infomem B - Bytes 256-381 - Center and Node MAC addresses */
    memcpy(&storedConfig->rawBytes[NV_CENTER],
        &stored_config_temp.rawBytes[NV_CENTER], NV_NUM_BYTES_SYNC_CENTER_NODE_ADDRS);
    //memcpy((uint8_t*) (storedConfig + NV_MAC_ADDRESS + 7), (uint8_t*) (stored_config_temp + NV_MAC_ADDRESS + 7), 153); //25+128

    S4Ram_config2SdHead();
    SetName();

    InfoMem_update();

    /* If the configuration needed to be corrected, update the config file */
    if (triggerSdCardUpdate)
    {
      UpdateSdConfig();
    }
  }
}

void ItoaNo0(uint64_t num, uint8_t *buf, uint8_t max_len)
{ //len = actual len + 1 extra '\0' at the end
  uint8_t idx, i_move;
  memset(buf, 0, max_len);
  if (!num)
    buf[0] = '0';
  for (idx = 0; (idx < max_len - 1) && (num > 0); idx++)
  {
    for (i_move = idx; i_move > 0; i_move--)
      buf[i_move] = buf[i_move - 1];
    buf[0] = '0' + num % 10;
    num /= 10;
  }
}

uint8_t isFileStatusOk(void)
{
  return file_status == FR_OK;
}

uint8_t isSdInfoSyncDelayed(void)
{
  return sdInfoSyncDelayed;
}

void setSdInfoSyncDelayed(uint8_t state)
{
  sdInfoSyncDelayed = state;
}

uint8_t *getConfigTimeTextPtr(void)
{
  return &configTimeText[0];
}

uint8_t *getFileNamePtr(void)
{
  return &fileName[0];
}
