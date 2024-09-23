/*
 * shimmer_calibration.c
 *
 *  Created on: Jul 11, 2016
 *      Author: WeiboP
 */

//#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include "msp430.h"

#include "shimmer_definitions.h"
#include "s4_calib.h"
#if USE_FATFS
#include "ff.h"
#endif
#include "hal_Infomem.h"
#include "stm32u5xx.h"

uint8_t shimmerCalib_ram[SHIMMER_CALIB_RAM_MAX], shimmerCalib_macId[5],
    shimmerCalib_ramTemp[SHIMMER_CALIB_RAM_MAX];
uint16_t shimmerCalib_ramLen, shimmerCalib_ramTempLen, shimmerCalib_ramTempMax;

uint8_t ShimmerCalib_findLength(sc_t *sc1)
{
  switch (sc1->id)
  {
  case SC_SENSOR_ANALOG_ACCEL:
    return SC_DATA_LEN_ANALOG_ACCEL;
  case SC_SENSOR_MPU9150_GYRO:
    return SC_DATA_LEN_MPU9250_GYRO;
  case SC_SENSOR_LSM303DLHC_ACCEL:
    return SC_DATA_LEN_LSM303DLHC_ACCEL;
  case SC_SENSOR_LSM303DLHC_MAG:
    return SC_DATA_LEN_LSM303DLHC_MAG;
  case SC_SENSOR_BMP180_PRESSURE:
    return SC_DATA_LEN_BMP180;
  default:
    return 0;
  }
}

void ShimmerCalib_initVer(void)
{
  shimmerCalib_ram[SC_OFFSET_VER_HW_ID_L] = DEVICE_VER & 0xff;
  shimmerCalib_ram[SC_OFFSET_VER_HW_ID_H] = (DEVICE_VER >> 8) & 0xff;
  shimmerCalib_ram[SC_OFFSET_VER_FW_ID_L] = FW_IDENTIFIER & 0xff;
  shimmerCalib_ram[SC_OFFSET_VER_FW_ID_H] = (FW_IDENTIFIER >> 8) & 0xff;
  shimmerCalib_ram[SC_OFFSET_VER_FW_MAJOR_L] = FW_VER_MAJOR & 0xff;
  shimmerCalib_ram[SC_OFFSET_VER_FW_MAJOR_H] = (FW_VER_MAJOR >> 8) & 0xff;
  shimmerCalib_ram[SC_OFFSET_VER_FW_MINOR_L] = FW_VER_MINOR & 0xff;
  shimmerCalib_ram[SC_OFFSET_VER_FW_INTER_L] = FW_VER_REL & 0xff;
}

void ShimmerCalib_defaultAll(void)
{
  ShimmerCalib_default(SC_SENSOR_ANALOG_ACCEL);
  ShimmerCalib_default(SC_SENSOR_MPU9150_GYRO);
  ShimmerCalib_default(SC_SENSOR_LSM303DLHC_ACCEL);
  ShimmerCalib_default(SC_SENSOR_LSM303DLHC_MAG);
}

void ShimmerCalib_init(void)
{
  //uint8_t temp_buf[10];
  //uint8_t temp_ff[10];
  uint16_t length;
  uint8_t temp_btMacAscii[14];
  S4Ram_btMacAsciiGet(temp_btMacAscii);
  memcpy(shimmerCalib_macId, temp_btMacAscii + 8, 4);
  shimmerCalib_macId[4] = 0;
  //InfoMem_initCalib(shimmerCalib_ram);
  //memset(shimmerCalib_ram, 0, SHIMMER_CALIB_RAM_MAX);

  //memset(temp_ff, 0xff, 10);
  InfoMem_readCalib((uint8_t *) &length, 0, 2);
  if ((length == 0) || (length == 0xffff))
  {
    //note: when USE_DEFAULT_SENSOR = 1, length will be 0, i.e. this 'if' will be executed
    shimmerCalib_ramLen = 8;
    shimmerCalib_ram[SC_OFFSET_LENGTH_L] = shimmerCalib_ramLen & 0xff;
    shimmerCalib_ram[SC_OFFSET_LENGTH_H] = (shimmerCalib_ramLen >> 8) & 0xff;
    ShimmerCalib_initVer();
    ShimmerCalib_defaultAll();
    InfoMem_update();
  }
  else
  {
    InfoMem_readCalib(shimmerCalib_ram, 0, SHIMMER_CALIB_RAM_MAX);
    ShimmerCalib_initVer();
  }
}

uint8_t *ShimmerCalib_getRam(void)
{
  return shimmerCalib_ram;
}

void ShimmerCalib_ram2File(void)
{
  char cal_file_name[48]; //buffer[66],
  DIR gdc;
  FIL gfc;
  //sc_t sc1;
  UINT bw;
  FRESULT res;
  uint8_t this_write_size;

  shimmerCalib_ramLen = min(*(uint16_t *) shimmerCalib_ram, SHIMMER_CALIB_RAM_MAX - 2);

  if (shimmerCalib_ramLen > 0)
  {
    strcpy((char *) cal_file_name, "/Calibration");
    if ((res = f_opendir(&gdc, "/Calibration")))
    {
      if ((res = f_opendir(&gdc, "/calibration")))
      {
        if (res == FR_NO_PATH)
        { //we'll have to make /Calibration first
          res = f_mkdir("/Calibration");
        }
      }
      else
      {
        strcpy((char *) cal_file_name, "/calibration");
      }
    }
    strcat(cal_file_name, "/calib_");
    strcat(cal_file_name, (char *) shimmerCalib_macId);

    res = f_open(&gfc, cal_file_name, (FA_WRITE | FA_CREATE_ALWAYS));
    if (res == FR_NO_FILE)
    {
      return;
    }

    while (gfc.fptr < shimmerCalib_ramLen + 2)
    {
      this_write_size = min(shimmerCalib_ramLen + 2 - gfc.fptr, SHIMMER_CALIB_COPY_SIZE);
      f_write(&gfc, shimmerCalib_ram + gfc.fptr, this_write_size, &bw);
    }

    f_close(&gfc);
    HAL_Delay(50); //50ms
  }
}

/*
 * if there wasn't a calibration/calibration file, new file won't be created.
 * ram buffer will use 0 as length, 00000 as content
 */
uint8_t ShimmerCalib_file2Ram()
{
#if USE_FATFS
  char cal_file_name[48];
  DIR gdc;
  FIL gfc;
  FILINFO calibFileInfo;
  UINT bw;
  FRESULT res;
  uint8_t this_read_size;
  uint16_t offset;

  strcpy(cal_file_name, "/Calibration"); //"/Calibration/calibParams"
  if (f_opendir(&gdc, "/Calibration"))
  {
    if (f_opendir(&gdc, "/calibration"))
    {
      //CalibDefault(sensor);
      return 1;
    }
    else
      strcpy(cal_file_name, "/calibration");
  }
  strcat(cal_file_name, "/calib_");
  strcat(cal_file_name, (char *) shimmerCalib_macId);

  res = f_open(&gfc, cal_file_name, (FA_OPEN_EXISTING | FA_READ));
  if (res == FR_NO_FILE)
  {
    return 1;
  }

  f_stat(cal_file_name, &calibFileInfo);

  //if file not successfully open, don't wipe the previous dump RAM
  shimmerCalib_ramLen = 0;
  memset(shimmerCalib_ram, 0, SHIMMER_CALIB_RAM_MAX);

  while (gfc.fptr != calibFileInfo.fsize)
  {
    this_read_size = min(calibFileInfo.fsize - gfc.fptr, SHIMMER_CALIB_COPY_SIZE);
    res = f_read(&gfc, shimmerCalib_ram + offset, this_read_size, &bw);
    offset += this_read_size;
  }
  shimmerCalib_ramLen = min(*(uint16_t *) shimmerCalib_ram, SHIMMER_CALIB_RAM_MAX - 2);
  f_close(&gfc);
  HAL_Delay(50);
#endif

  return 0;
}

uint8_t ShimmerCalib_singleSensorWrite(const sc_t *sc1)
{
  sc_t curr_sc;
  uint16_t cnt = SC_OFFSET_FIRST_SENSOR;
  uint8_t ts[8], sensor_found = 0; //temp_len,

  *(uint64_t *) ts = RTC_get64(); //rwcTimeDiff64 +
  shimmerCalib_ramLen = min(*(uint16_t *) shimmerCalib_ram, SHIMMER_CALIB_RAM_MAX - 2);

  while (cnt + SC_OFFSET_SENSOR_DATA < shimmerCalib_ramLen + 2)
  {
    memcpy((uint8_t *) &curr_sc, shimmerCalib_ram + cnt, SC_OFFSET_SENSOR_TIMESTAMP);
    if ((curr_sc.id == sc1->id) && (curr_sc.range == sc1->range))
    {
      //setup RTC time as calib timestamp
      memcpy(shimmerCalib_ram + cnt + SC_OFFSET_SENSOR_TIMESTAMP, ts, SC_TIMESTAMP_LENGTH);
      memcpy(shimmerCalib_ram + cnt + SC_OFFSET_SENSOR_DATA, sc1->data.raw,
          curr_sc.data_len);
      sensor_found = 1;
      break;
    }
    cnt += SC_OFFSET_SENSOR_DATA + curr_sc.data_len;
  }

  if (!sensor_found)
  {
    memcpy(shimmerCalib_ram + shimmerCalib_ramLen + 2, (uint8_t *) sc1,
        SC_OFFSET_SENSOR_DATA + sc1->data_len);
    shimmerCalib_ramLen += SC_OFFSET_SENSOR_DATA + sc1->data_len;
    *(uint16_t *) shimmerCalib_ram = min(shimmerCalib_ramLen, SHIMMER_CALIB_RAM_MAX - 2);
  }
  return 0;
}

uint8_t ShimmerCalib_singleSensorRead(sc_t *sc1)
{
  sc_t curr_sc;
  uint16_t cnt = SC_OFFSET_FIRST_SENSOR;
  uint8_t sensor_found = 0;

  memset((uint8_t *) sc1->data.raw, 0, sc1->data_len);
  shimmerCalib_ramLen = min(*(uint16_t *) shimmerCalib_ram, SHIMMER_CALIB_RAM_MAX - 2);
  while (cnt < shimmerCalib_ramLen + 2)
  {
    memcpy((uint8_t *) &curr_sc, shimmerCalib_ram + cnt, SC_OFFSET_SENSOR_TIMESTAMP);
    if ((curr_sc.id == sc1->id) && (curr_sc.range == sc1->range))
    {
      memcpy((uint8_t *) sc1, shimmerCalib_ram + cnt,
          SC_OFFSET_SENSOR_DATA + curr_sc.data_len);
      sensor_found = 1;
      break;
    }
    cnt += SC_OFFSET_SENSOR_DATA + curr_sc.data_len;
  }
  if (!sensor_found)
  {
    return 1;
  }
  return 0;
}

void ShimmerCalib_checkRamLen()
{
  shimmerCalib_ramLen = min(*(uint16_t *) shimmerCalib_ram, SHIMMER_CALIB_RAM_MAX - 2);
  memset(shimmerCalib_ram + shimmerCalib_ramLen + 2, 0,
      SHIMMER_CALIB_RAM_MAX - shimmerCalib_ramLen - 2);
}

void ShimmerCalib_ramTempInit()
{
  shimmerCalib_ramTempMax = 0;
  shimmerCalib_ramTempLen = 0;
}

//return 0 : success on this write; 1: successfully finished all;  0xff: fail
//the calib dump write operation starts with offset = 0, ends with shimmerCalib_ramTempMax bytes received.
uint8_t ShimmerCalib_ramWrite(const uint8_t *buf, uint8_t length, uint16_t offset)
{
  if ((length <= 128) && (offset <= (SHIMMER_CALIB_RAM_MAX - 1))
      && (length + offset <= SHIMMER_CALIB_RAM_MAX))
  {
    memcpy(shimmerCalib_ramTemp + offset, buf, length);
  }
  else
  {
    return 0xff;
  }

  //to start a new calib_dump transmission, the sw must use offset = 0 to setup the correct length.
  //offset = 1 is not suggested, but will be considered.
  //starting with offset > 2 is not accepted.
  if (offset < 2)
  {
    shimmerCalib_ramTempMax = *(uint16_t *) shimmerCalib_ramTemp;
    shimmerCalib_ramTempLen = length;
  }
  else
  {
    shimmerCalib_ramTempLen += length;
  }

  if (shimmerCalib_ramTempLen >= shimmerCalib_ramTempMax + 2)
  {
    memcpy(shimmerCalib_ram, shimmerCalib_ramTemp, shimmerCalib_ramTempMax);
    InfoMem_update();
    ShimmerCalib_initVer();
    ShimmerCalib_ramTempInit();
    return 1;
  }

  return 0;
}

//return 0 : success; 1: fail
uint8_t ShimmerCalib_ramRead(uint8_t *buf, uint8_t length, uint16_t offset)
{
  shimmerCalib_ramLen = min(*(uint16_t *) shimmerCalib_ram, SHIMMER_CALIB_RAM_MAX - 2);
  if ((length <= 128) && (offset <= (SHIMMER_CALIB_RAM_MAX - 1))
      && (length + offset <= SHIMMER_CALIB_RAM_MAX))
  {
    memcpy(buf, shimmerCalib_ram + offset, length);
    return 0;
  }
  else
  {
    memset(buf, 0, length);
    return 1;
  }
}

void ShimmerCalib_default(uint8_t sensor)
{
  //int16_t bias, sensitivity, sensitivity_x, sensitivity_y, sensitivity_z;
  //uint8_t bias_byte_one, bias_byte_two, sens_byte_one, sens_byte_two, number_axes = 1;
  //int8_t align_xx, align_xy, align_xz, align_yx, align_yy, align_yz, align_zx, align_zy, align_zz, i = 0;
  //uint16_t address;
  //bool align = FALSE;

  sc_t sc1;
  //uint8_t ts[8];//range, , data_ptr
  uint16_t bias, sensitivity;

  //*(uint64_t*)(sc1.ts) = rwcTimeDiff64 + RTC_get64();
  memset(sc1.ts, 0, 8);

  if (sensor == SC_SENSOR_ANALOG_ACCEL)
  {
    sc1.id = sensor;
    sc1.data_len = SC_DATA_LEN_ANALOG_ACCEL;
    for (sc1.range = 0; sc1.range < SC_SENSOR_RANGE_MAX_ANALOG_ACCEL; sc1.range++)
    {
      bias = 2047;
      sensitivity = 83;
      sc1.data.dd.bias_x = bias;
      sc1.data.dd.bias_y = bias;
      sc1.data.dd.bias_z = bias;
      sc1.data.dd.sens_x = sensitivity;
      sc1.data.dd.sens_y = sensitivity;
      sc1.data.dd.sens_z = sensitivity;
      sc1.data.dd.align_xx = 0;
      sc1.data.dd.align_xy = -100;
      sc1.data.dd.align_xz = 0;
      sc1.data.dd.align_yx = -100;
      sc1.data.dd.align_yy = 0;
      sc1.data.dd.align_yz = 0;
      sc1.data.dd.align_zx = 0;
      sc1.data.dd.align_zy = 0;
      sc1.data.dd.align_zz = -100;
      ShimmerCalib_singleSensorWrite(&sc1);
    }
  }
  else if (sensor == SC_SENSOR_MPU9150_GYRO)
  {
    sc1.id = sensor;
    sc1.data_len = SC_DATA_LEN_MPU9250_GYRO;
    for (sc1.range = 0; sc1.range < SC_SENSOR_RANGE_MAX_MPU9250_GYRO; sc1.range++)
    {
      bias = 0;
      if (sc1.range == SC_SENSOR_RANGE_MPU9250_GYRO_250DPS)
      {
        sensitivity = 13100;
      }
      else if (sc1.range == SC_SENSOR_RANGE_MPU9250_GYRO_500DPS)
      {
        sensitivity = 6550;
      }
      else if (sc1.range == SC_SENSOR_RANGE_MPU9250_GYRO_1000DPS)
      {
        sensitivity = 3280;
      }
      else
      { //(sdHeadText[SDH_GYRO_RANGE] == SC_SENSOR_RANGE_MPU9250_GYRO_2000DPS)
        sensitivity = 1640;
      }
      sc1.data.dd.bias_x = bias;
      sc1.data.dd.bias_y = bias;
      sc1.data.dd.bias_z = bias;
      sc1.data.dd.sens_x = sensitivity;
      sc1.data.dd.sens_y = sensitivity;
      sc1.data.dd.sens_z = sensitivity;
      sc1.data.dd.align_xx = 0;
      sc1.data.dd.align_xy = -100;
      sc1.data.dd.align_xz = 0;
      sc1.data.dd.align_yx = -100;
      sc1.data.dd.align_yy = 0;
      sc1.data.dd.align_yz = 0;
      sc1.data.dd.align_zx = 0;
      sc1.data.dd.align_zy = 0;
      sc1.data.dd.align_zz = -100;
      ShimmerCalib_singleSensorWrite(&sc1);
    }
  }
  else if (sensor == SC_SENSOR_LSM303DLHC_ACCEL)
  {
    sc1.id = sensor;
    sc1.data_len = SC_DATA_LEN_LSM303DLHC_ACCEL;
    for (sc1.range = 0; sc1.range < SC_SENSOR_RANGE_MAX_LSM303DLHC_ACCEL; sc1.range++)
    {
      bias = 0;
      if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_ACCEL_2G)
      {
        sensitivity = 1631;
      }
      else if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_ACCEL_4G)
      {
        sensitivity = 815;
      }
      else if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_ACCEL_8G)
      {
        sensitivity = 408;
      }
      else
      { //(sc1.range == SC_SENSOR_RANGE_LSM303DLHC_ACCEL_16G)
        sensitivity = 135;
      }
      sc1.data.dd.bias_x = bias;
      sc1.data.dd.bias_y = bias;
      sc1.data.dd.bias_z = bias;
      sc1.data.dd.sens_x = sensitivity;
      sc1.data.dd.sens_y = sensitivity;
      sc1.data.dd.sens_z = sensitivity;
      sc1.data.dd.align_xx = -100;
      sc1.data.dd.align_xy = 0;
      sc1.data.dd.align_xz = 0;
      sc1.data.dd.align_yx = 0;
      sc1.data.dd.align_yy = 100;
      sc1.data.dd.align_yz = 0;
      sc1.data.dd.align_zx = 0;
      sc1.data.dd.align_zy = 0;
      sc1.data.dd.align_zz = -100;
      ShimmerCalib_singleSensorWrite(&sc1);
    }
  }
  else if (sensor == SC_SENSOR_LSM303DLHC_MAG)
  {
    sc1.id = sensor;
    sc1.data_len = SC_DATA_LEN_LSM303DLHC_MAG;
    for (sc1.range = 0; sc1.range < SC_SENSOR_RANGE_MAX_LSM303DLHC_MAG; sc1.range++)
    {
      bias = 0;
      if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_MAG_1_3G)
      {
        sc1.data.dd.sens_x = 1100;
        sc1.data.dd.sens_y = 1100;
        sc1.data.dd.sens_z = 980;
      }
      else if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_MAG_1_9G)
      {
        sc1.data.dd.sens_x = 855;
        sc1.data.dd.sens_y = 855;
        sc1.data.dd.sens_z = 760;
      }
      else if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_MAG_2_5G)
      {
        sc1.data.dd.sens_x = 670;
        sc1.data.dd.sens_y = 670;
        sc1.data.dd.sens_z = 600;
      }
      else if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_MAG_4_0G)
      {
        sc1.data.dd.sens_x = 450;
        sc1.data.dd.sens_y = 450;
        sc1.data.dd.sens_z = 400;
      }
      else if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_MAG_4_7G)
      {
        sc1.data.dd.sens_x = 400;
        sc1.data.dd.sens_y = 400;
        sc1.data.dd.sens_z = 355;
      }
      else if (sc1.range == SC_SENSOR_RANGE_LSM303DLHC_MAG_5_6G)
      {
        sc1.data.dd.sens_x = 330;
        sc1.data.dd.sens_y = 330;
        sc1.data.dd.sens_z = 295;
      }
      else
      { //sc1.range == SC_SENSOR_RANGE_LSM303DLHC_MAG_8_1G)
        sc1.data.dd.sens_x = 230;
        sc1.data.dd.sens_y = 230;
        sc1.data.dd.sens_z = 205;
      }
      sc1.data.dd.bias_x = bias;
      sc1.data.dd.bias_y = bias;
      sc1.data.dd.bias_z = bias;
      sc1.data.dd.align_xx = -100;
      sc1.data.dd.align_xy = 0;
      sc1.data.dd.align_xz = 0;
      sc1.data.dd.align_yx = 0;
      sc1.data.dd.align_yy = 100;
      sc1.data.dd.align_yz = 0;
      sc1.data.dd.align_zx = 0;
      sc1.data.dd.align_zy = 0;
      sc1.data.dd.align_zz = -100;
      ShimmerCalib_singleSensorWrite(&sc1);
    }
  }
}

void ShimmerCalib_singleSensorWriteFromInfoMem(uint16_t id, uint8_t range, uint8_t data_len, uint8_t *ptr)
{
  sc_t sc1;
  sc1.id = id;
  sc1.range = range;
  sc1.data_len = data_len;
  *(uint64_t *) (sc1.ts) = RTC_get64();
  memcpy(sc1.data.raw, ptr, sc1.data_len);
  ShimmerCalib_singleSensorWrite(&sc1);
}

void CalibSaveFromInfoMemToCalibDump(uint8_t id)
{
  gConfigBytes *configBytes = S4Ram_getStoredConfig();
  if (id == 0xFF || id == SC_SENSOR_ANALOG_ACCEL)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_ANALOG_ACCEL, SC_SENSOR_RANGE_ANALOG_ACCEL,
        SC_DATA_LEN_ANALOG_ACCEL, &configBytes->lnAccelCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_MPU9150_GYRO)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_MPU9150_GYRO,
        get_config_byte_gyro_range(), SC_DATA_LEN_MPU9250_GYRO,
        &configBytes->gyroCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_LSM303DLHC_MAG)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LSM303DLHC_MAG,
        configBytes->magRange, SC_DATA_LEN_LSM303DLHC_MAG,
        &configBytes->magCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_LSM303DLHC_ACCEL)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LSM303DLHC_ACCEL,
        configBytes->wrAccelRange, SC_DATA_LEN_LSM303DLHC_ACCEL,
        &configBytes->wrAccelCalib.rawBytes[0]);
  }
}

void ShimmerCalibInitFromInfoAll(void)
{
  ShimmerCalibFromInfo(SC_SENSOR_ANALOG_ACCEL, 0);
  ShimmerCalibFromInfo(SC_SENSOR_MPU9150_GYRO, 0);
  ShimmerCalibFromInfo(SC_SENSOR_LSM303DLHC_ACCEL, 0);
  ShimmerCalibFromInfo(SC_SENSOR_LSM303DLHC_MAG, 0);
}

void ShimmerCalibUpdateFromInfoAll(void)
{
  ShimmerCalibFromInfo(SC_SENSOR_ANALOG_ACCEL, 1);
  ShimmerCalibFromInfo(SC_SENSOR_MPU9150_GYRO, 1);
  ShimmerCalibFromInfo(SC_SENSOR_LSM303DLHC_ACCEL, 1);
  ShimmerCalibFromInfo(SC_SENSOR_LSM303DLHC_MAG, 1);
}

void ShimmerCalibFromInfo(uint8_t sensor, uint8_t use_sys_time)
{
  uint8_t info_config, info_valid = 0;
  uint8_t offset;
  int byte_cnt = 0;
  sc_t sc1;

  sc1.id = sensor;
  if (use_sys_time)
  {
    memset(sc1.ts, 0, 8);
  }
  else
  {
    *(uint64_t *) (sc1.ts) = RTC_get64();
  }

  if (sc1.id == SC_SENSOR_ANALOG_ACCEL)
  {
    offset = NV_A_ACCEL_CALIBRATION;
    sc1.range = SC_SENSOR_RANGE_ANALOG_ACCEL;
    sc1.data_len = SC_DATA_LEN_ANALOG_ACCEL;
  }
  else if (sc1.id == SC_SENSOR_MPU9150_GYRO)
  {
    offset = NV_MPU9250_GYRO_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_MPU9250_GYRO;
    InfoMem_readRam(&info_config, NV_CONFIG_SETUP_BYTE2, 1);
    sc1.range = info_config & 0x03;
  }
  else if (sc1.id == SC_SENSOR_LSM303DLHC_ACCEL)
  {
    offset = NV_LSM303DLHC_ACCEL_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_LSM303DLHC_ACCEL;
    InfoMem_readRam(&info_config, NV_CONFIG_SETUP_BYTE0, 1);
    sc1.range = (info_config & 0x0c) >> 2;
  }
  else if (sc1.id == SC_SENSOR_LSM303DLHC_MAG)
  {
    offset = NV_LSM303DLHC_MAG_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_LSM303DLHC_MAG;
    InfoMem_readRam(&info_config, NV_CONFIG_SETUP_BYTE2, 1);
    sc1.range = (info_config & 0xe0) >> 5;
  }
  else
  {
    return;
  }

  InfoMem_readRam(&S4Ram_getStoredConfig()->rawBytes[offset], offset, 21);
  for (byte_cnt = 21; byte_cnt > 0; byte_cnt--)
  {
    if (S4Ram_getStoredConfig()->rawBytes[offset + byte_cnt] != 0xff)
    {
      info_valid = 1;
      break;
    }
  }

  if (info_valid)
  {
    /* if not all 0xff in infomem */
    memcpy(sc1.data.raw, &S4Ram_getStoredConfig()->rawBytes[offset], sc1.data_len);
    ShimmerCalib_singleSensorWrite(&sc1);
  }
}

void ShimmerCalibSyncFromDumpRamAll(void)
{
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_ANALOG_ACCEL);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_MPU9150_GYRO);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LSM303DLHC_MAG);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LSM303DLHC_ACCEL);
}

void ShimmerCalibSyncFromDumpRamSingleSensor(uint8_t sensor)
{
  sc_t sc1;
  uint16_t scs_infomem_offset, scs_sdhead_offset, scs_sdhead_ts;
  sc1.id = sensor;
  sc1.data_len = ShimmerCalib_findLength(&sc1);
  switch (sensor)
  {
  case SC_SENSOR_ANALOG_ACCEL:
    scs_infomem_offset = NV_A_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_A_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_A_ACCEL_CALIB_TS;
    sc1.range = SC_SENSOR_RANGE_ANALOG_ACCEL;
    break;
  case SC_SENSOR_MPU9150_GYRO:
    scs_infomem_offset = NV_MPU9250_GYRO_CALIBRATION;
    scs_sdhead_offset = SDH_MPU9250_GYRO_CALIBRATION;
    scs_sdhead_ts = SDH_MPU9250_GYRO_CALIB_TS;
    sc1.range = get_config_byte_gyro_range();
    break;
  case SC_SENSOR_LSM303DLHC_MAG:
    scs_infomem_offset = NV_LSM303DLHC_MAG_CALIBRATION;
    scs_sdhead_offset = SDH_LSM303DLHC_MAG_CALIBRATION;
    scs_sdhead_ts = SDH_LSM303DLHC_MAG_CALIB_TS;
    sc1.range = S4Ram_getStoredConfig()->magRange;
    break;
  case SC_SENSOR_LSM303DLHC_ACCEL:
    scs_infomem_offset = NV_LSM303DLHC_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_LSM303DLHC_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_LSM303DLHC_ACCEL_CALIB_TS;
    sc1.range = S4Ram_getStoredConfig()->wrAccelRange;
    break;
  default:
    scs_infomem_offset = NV_A_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_A_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_A_ACCEL_CALIB_TS;
    sc1.range = SC_SENSOR_RANGE_ANALOG_ACCEL;
    break;
  }

  ShimmerCalib_singleSensorRead(&sc1);
  memcpy(&S4Ram_getStoredConfig()->rawBytes[scs_infomem_offset], sc1.data.raw, sc1.data_len);
  InfoMem_update();
  memcpy(S4Ram_getSdHeadText() + scs_sdhead_offset, sc1.data.raw, sc1.data_len);
  memcpy(S4Ram_getSdHeadText() + scs_sdhead_ts, sc1.ts, 8);
}
