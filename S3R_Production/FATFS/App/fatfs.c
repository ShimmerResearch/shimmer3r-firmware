/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
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
#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */

uint8_t test_cnt;
uint8_t fileName[64], dirName[64], expDirName[32], dataBuf[100], //storedConfig[512], sdHeadText[SD_HEAD_SIZE],
        sdWrBuf[NUM_SDWRBUF][SD_WRITE_BUF_SIZE],//btMacHex[6], btMacAscii[14],
        sdBufInQ, expIdName[MAX_CHARS], shimmerName[MAX_CHARS], configTimeText[UINT32_LEN], //sdBufInQMax,
        dirLen,  sdBufSens = 0, sdBufWr = 0;
uint16_t fileNum, dirCounter, sdWrLen[NUM_SDWRBUF];
uint64_t sdFileCrTs, sdFileSyncTs;
FRESULT file_status;
//char file_name_current[128];
FATFS fatfs;
DIR dir;
FIL dataFile;
FILINFO dataFileInfo;

/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */

void SD_init(void){
   sdBufSens = sdBufWr = 0;
   //fileNum = 0;
}
#define TEST_TEXT_LEN 40
uint8_t SD_test(void){
   FIL test_file;
   char file_name[] = "test.txt";
   char test_text1[TEST_TEXT_LEN] = "This is the 1st line of the test file.\n";
   char test_text2[TEST_TEXT_LEN] = "This is the 2nd line of the test file.\n";
   char test_text3[TEST_TEXT_LEN];
   UINT bw;

   Board_sd2Arm();
   stat.pinPvSd = 1;
   SD_mount(0);
   SD_mount(1);

   stat.badFile += f_open(&test_file, file_name, FA_CREATE_ALWAYS | FA_WRITE);
   stat.badFile += f_write(&test_file, test_text1, TEST_TEXT_LEN-1, &bw);
   stat.badFile += f_write(&test_file, test_text2, TEST_TEXT_LEN-1, &bw);
   stat.badFile += f_close(&test_file);

   memset(test_text3, 0, 40);
   stat.badFile += f_open(&test_file, file_name, FA_OPEN_EXISTING | FA_READ);
   stat.badFile += f_read(&test_file, test_text3, TEST_TEXT_LEN-1, &bw);
   stat.badFile += f_close(&test_file);

   stat.badFile += strcmp(test_text1, test_text3);

   return stat.badFile;
}


void SD_setShimmerName(){
   uint8_t i;
   uint8_t temp_btMacAscii[14];
   for(i=0; (i<MAX_CHARS-1)&&isprint(S4Ram_storedConfigGetByte(NV_SD_SHIMMER_NAME+i)) ; i++);
   if(i){
      //memcpy((char*)shimmerName, (char*)(storedConfig+NV_SD_SHIMMER_NAME), i);
      S4Ram_storedConfigGet(shimmerName, NV_SD_SHIMMER_NAME, i);
      shimmerName[i] = 0;
   }else{
//      strcpy((char*)(storedConfig+NV_SD_SHIMMER_NAME), "idXXXX");
//      memcpy((storedConfig+NV_SD_SHIMMER_NAME)+2, btMacAscii+8, 4);
//      strcpy((char*)shimmerName, (char*)(storedConfig+NV_SD_SHIMMER_NAME));
      S4Ram_storedConfigSet((uint8_t*)"idXXXX", NV_SD_SHIMMER_NAME, 6);
      S4Ram_btMacAsciiGet(temp_btMacAscii);
      S4Ram_storedConfigSet(temp_btMacAscii+8, NV_SD_SHIMMER_NAME+2, 4);
      S4Ram_storedConfigGet(shimmerName, NV_SD_SHIMMER_NAME, 6);
   }
}
void SD_setExpIdName(){
//   uint8_t i;//, len_temp;
//   for(i=0; (i<MAX_CHARS-1)&&(isprint(S4Ram_storedConfigGetByte(NV_SD_EXP_ID_NAME+i))) ; i++);
//   if(i){
//      S4Ram_storedConfigGet(expIdName, NV_SD_EXP_ID_NAME, i);
//      expIdName[i] = 0;
//   }else{
//      S4Ram_storedConfigSet((uint8_t*)"default_exp", NV_SD_EXP_ID_NAME, 12);
//      S4Ram_storedConfigGet(expIdName, NV_SD_EXP_ID_NAME, i);
//   }
   strcpy((char*)expIdName,"DefaultTrial");
}
void SD_setCfgTime(){
   uint32_t cfg_time_temp = 0;
   uint8_t i;
   for(i=0;i<4;i++){
      cfg_time_temp<<=8;
      cfg_time_temp += S4Ram_storedConfigGetByte(NV_SD_CONFIG_TIME+i);
   }
   if(cfg_time_temp){
      S4Calc_itoaNo0((uint64_t)cfg_time_temp, configTimeText, UINT32_LEN);
   }else{
      strcpy((char*)configTimeText,"0");
   }
}

void SD_infomem2Names(){
   SD_setShimmerName();
   SD_setExpIdName();
   SD_setCfgTime();
}

uint8_t SD_setBasedir() {
   FILINFO fno;
   //volatile uint8_t res;
   uint16_t tmp_counter = 0;
   char lfn[_MAX_LFN + 1], * fname, * scout, * dash, dirnum[8];

   SD_infomem2Names();
   //SetName();
#if _USE_LFN
   fno.lfname = lfn;
   fno.lfsize = sizeof(lfn);
#endif

   file_status = f_opendir(&dir, "/data");
   if(file_status){
      if(file_status == FR_NO_PATH)   // we'll have to make /data first
         file_status = f_mkdir("/data");
      if(file_status)  // in every case, we're toast
         return 0;//FAIL;

      // try one more time
      file_status = f_opendir(&dir, "/data");
      if(file_status)
         return 0;//FAIL;
   }
   file_status = f_closedir(&dir);

   strcpy((char*)expDirName, "data/");
   strcat((char*)expDirName, (char*)expIdName);
   strcat((char*)expDirName, "_");
   strcat((char*)expDirName, (char*)configTimeText);

   file_status = f_opendir(&dir, (char*)expDirName);
   if(file_status){
      if(file_status == FR_NO_PATH)      // we'll have to make the experiment folder first
         file_status = f_mkdir((char*)expDirName);
      if(file_status)         // in every case, we're toast
         return 0;//FAIL;

      // try one more time
      file_status = f_opendir(&dir, (char*)expDirName);
      if(file_status)
         return 0;//FAIL;
   }

   dirCounter = 0;   // this might be the first log for this shimmer

   // file name format
   // shimmername    as defined in sdlog.cfg
   // -              separator
   // 000
   // we want to create a new directory with a sequential run number each power-up/reset for each shimmer
   while(f_readdir(&dir, &fno) == FR_OK){
      if(*fno.fname == 0)
         break;
      else if(fno.fattrib & AM_DIR){
#if _USE_LFN
         fname = (*fno.lfname) ? fno.lfname : fno.fname;
#else
         fname = (*lfn) ? lfn : fno.fname;
#endif

         if(!strncmp(fname, (char*)shimmerName, strlen(fname)-4)){ // -4 because of the -000 etc.
            scout = strchr(fname, '-');
            if(scout){      // if not, something is seriously wrong!
               scout++;
//               while(dash = strchr(scout, '-'))  // In case the shimmer name contains '-'
//                  scout = dash + 1;
               dash = strchr(scout, '-');
               while(dash){  // In case the shimmer name contains '-'
                  scout = dash + 1;
                  dash = strchr(scout, '-');
               }
               strcpy(dirnum, scout);
               tmp_counter = atoi(dirnum);
               if(tmp_counter >= dirCounter){
                  dirCounter = tmp_counter;
                  dirCounter++;                   // start with next in numerical sequence
               }
            }
            else
               return 0;//FAIL;
         }
      }
   }
   file_status = f_closedir(&dir);

   // at this point, we have the id string and the counter, so we can make a directory name
   return 1;//SUCCESS;
}

uint8_t SD_makeBasedir() {
   memset(dirName,0,64);

   char dir_counter_text[4];
   S4Calc_itoaWith0((uint64_t)dirCounter, (uint8_t*)dir_counter_text,4);

   strcpy((char*)dirName, (char*)expDirName);
   strcat((char*)dirName, "/");
   strcat((char*)dirName, (char*)shimmerName);
   strcat((char*)dirName, "-");
   strcat((char*)dirName, dir_counter_text);

   file_status = f_mkdir((char*)dirName);
   if(file_status){
      //FindError(fileBad, dirName);
      return 0;//FAIL;
   }

   memset(fileName,0,64);
   strcpy((char*)fileName,(char*)dirName);
   dirLen = strlen((char*)dirName);
   //strcat((char*)fileName,"/000");
   fileNum = 0;
   //sprintf((char*)fileName, "/%03d", fileNum++);

   return 1;//SUCCESS;
}

void SD_makeFileName(char* name_buf) {
   //strcpy(file_name, "this_is_a_very_long_name_for_the_dataFile");
   uint8_t temp_str[5];
   sprintf((char*)temp_str, "/%03d", fileNum++);

   strcpy((char*)name_buf,(char*)dirName);
   strcat((char*)name_buf,(char*)temp_str);
}

void SD_fileInit(void) {
#if USE_SD
   char file_name[128];
   //char file_name[128];
   UINT bw;
   uint8_t temp_sdHeadText[SD_HEAD_SIZE];

   //SD_config2SdHead();
   //Board_sdPowerCycle();
   S4Ram_config2SdHead();

   sensing.isSdOperating = 1;
//   SD_mount(0);
//   SD_mount(1);
   SD_setBasedir();
   SD_makeBasedir();
   SD_makeFileName(file_name);

   S4Ram_sdHeadTextGet(temp_sdHeadText, 0, SD_HEAD_SIZE);

   file_status = f_open(&dataFile, file_name, FA_CREATE_ALWAYS | FA_WRITE);
   f_stat(file_name, &dataFileInfo);
   sdFileSyncTs = sdFileCrTs = RTC_get64();
#if USE_8BYTES_INIT_TS
   *(uint64_t*)(temp_sdHeadText+SDH_MY_LOCALTIME_0TH) = sdFileSyncTs;
#else
   temp_sdHeadText[SDH_MY_LOCALTIME_5TH] = (sdFileSyncTs >> 32)&0xff;
   *(uint32_t*)(temp_sdHeadText+SDH_MY_LOCALTIME) = (uint32_t)(sdFileSyncTs & 0xffffffff);
#endif
   file_status = f_write(&dataFile, temp_sdHeadText, SD_HEAD_SIZE, &bw);
   sensing.isSdOperating = 0;
   sensing.isFileCreated = 1;
   memset(sdWrLen, 0, NUM_SDWRBUF*sizeof(sdWrLen[0]));
   sdBufInQ = sdBufSens = sdBufWr = 0;
#endif //USE_SD
}

void SD_close(void){
#if USE_SD
   f_sync(&dataFile);
   f_close(&dataFile);
   file_status = FR_OK;
   //Board_sdPower(0);
   stat.badFile = 0;
   sensing.isFileCreated = 0;
#endif //USE_SD
}

void SD_writeToBuff(uint8_t *buf, uint16_t len) {
#if USE_SD
   //uint8_t *sensing_buf;
   //uint16_t *sensing_buf_len;
   if ((NUM_SDWRBUF == sdBufInQ)||
      (0 == sensing.isFileCreated)){
      __NOP();
      return;
   }
   //sdWrBuf[NUM_SDWRBUF][SD_WRITE_BUF_SIZE], sdBufSens, sdBufWr, sdBufInQ;

   memcpy(sdWrBuf[sdBufSens] + sdWrLen[sdBufSens], buf, len);
   sdWrLen[sdBufSens] += len;
   if (sdWrLen[sdBufSens] + len > SD_WRITE_BUF_SIZE) {
      S4_Task_set(TASK_SDWRITE);
      sdBufInQ++;
      sdBufSens++;
      if (sdBufSens >= NUM_SDWRBUF) {
         sdBufSens = 0;
      }
   }
#endif //USE_SD
}

void SD_writeToCard(void) {
   //__disable_irq();
   UINT bw;
   uint8_t *writing_buf;
   uint16_t *writing_buf_len;


   writing_buf = sdWrBuf[sdBufWr];
   writing_buf_len = sdWrLen + sdBufWr;

   __NOP();

   if((0 == *writing_buf_len) ||
      (0 == sdBufInQ)) {
      return;
   }

   sensing.inSdWr = 1;
   //HAL_Delay(5);

   sensing.isSdOperating = 1;

   file_status = f_lseek (&dataFile, dataFileInfo.fsize);
   assert_param(file_status == FR_OK);
   file_status = f_write(&dataFile, writing_buf, *writing_buf_len, &bw);
   assert_param(file_status == FR_OK);

   __NOP();
   __NOP();

   if ((sensing.latestTs - sdFileCrTs) > 32768 * 180) { //(&& (test_cnt < 15))
      //sdFileCrTs = sensing.latestTs;
      sdFileSyncTs = sdFileCrTs = RTC_get64();
      char file_name[256];
      file_status = f_sync(&dataFile);
      assert_param(file_status == FR_OK);
      file_status = f_close(&dataFile);
      assert_param(file_status == FR_OK);
      SD_makeFileName(file_name);
      file_status = f_open(&dataFile, file_name, FA_CREATE_ALWAYS | FA_WRITE);
      assert_param(file_status == FR_OK);
//    S4_RTC_GetDateTime(&hrtc, &rtc_data);
//    sdFileSyncTs = sdFileCrTs = rtc_data.ticks;
      //memcpy(sdHeadText, (uint8_t*)&time_diff, 4);
      uint8_t temp_sdHeadText[SD_HEAD_SIZE];
      S4Ram_sdHeadTextGet(temp_sdHeadText, 0, SD_HEAD_SIZE);
#if USE_8BYTES_INIT_TS
      *(uint64_t*)(temp_sdHeadText+SDH_MY_LOCALTIME_0TH) = sdFileSyncTs;
#else
      temp_sdHeadText[SDH_MY_LOCALTIME_5TH] = (sdFileSyncTs >> 32)&0xff;
      *(uint32_t*)(temp_sdHeadText+SDH_MY_LOCALTIME) = (uint32_t)(sdFileSyncTs & 0xffffffff);
#endif

      file_status = f_write(&dataFile, temp_sdHeadText, SD_HEAD_SIZE, &bw);
      assert_param(file_status == FR_OK);
   } else if (sensing.latestTs - sdFileSyncTs > 32768 * 10) {
      file_status = f_sync(&dataFile);
      assert_param(file_status == FR_OK);
      sdFileSyncTs = RTC_get64();
   }
//   else if (test_cnt >= 15) {
//      f_close(&dataFile);
//   }

   sensing.isSdOperating = 0;

   *writing_buf_len = 0;
   //sdBufWr++;
   if (++sdBufWr >= NUM_SDWRBUF) {
      sdBufWr = 0;
   }
   //sdBufInQ--;
   if(--sdBufInQ){
      S4_Task_set(TASK_SDWRITE);
   }

   if(file_status != 0){
      stat.badFile = 1;
   }else{
      stat.badFile = 0;
   }

   sensing.inSdWr = 0;
   sensing.inSdWrCnt = 0;
   //__enable_irq();
}

void SD_mount(uint8_t val){
   if(1 == val){
      f_mount(&fatfs, (TCHAR const*)SDPath, 0);
   }else{
      f_mount(0, (TCHAR const*)SDPath, 0);
   }
}
void SD_insertedCheck() {
   if (HAL_GPIO_ReadPin(SD_DETECT_N_GPIO_Port, SD_DETECT_N_Pin) == GPIO_PIN_RESET) { //inserted
      stat.isSdInserted = 1;
   } else {
      stat.isSdInserted = 0;
   }
}

/* USER CODE END Application */
