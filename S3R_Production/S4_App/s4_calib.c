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

#include "s4_calib.h"
#include "shimmer_definitions.h"
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
#if defined(SHIMMER3)
  case SC_SENSOR_ANALOG_ACCEL:
    return SC_DATA_LEN_ANALOG_ACCEL;
  case SC_SENSOR_MPU9X50_ICM20948_GYRO:
    return SC_DATA_LEN_MPU9X50_ICM20948_GYRO;
  case SC_SENSOR_LSM303_ACCEL:
    return SC_DATA_LEN_LSM303_ACCEL;
  case SC_SENSOR_LSM303_MAG:
    return SC_DATA_LEN_LSM303_MAG;
  case SC_SENSOR_BMP180_PRESSURE:
    return SC_DATA_LEN_BMP180;
#elif defined(SHIMMER3R)
  case SC_SENSOR_LSM6DSV_ACCEL:
  case SC_SENSOR_LSM6DSV_GYRO:
  case SC_SENSOR_LIS2DW12_ACCEL:
  case SC_SENSOR_ADXL371_ACCEL:
  case SC_SENSOR_LIS3MDL_MAG:
  case SC_SENSOR_LIS2MDL_MAG:
    return SC_DATA_LEN_STD_IMU_CALIB;
#endif
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
#if defined(SHIMMER3)
  ShimmerCalib_default(SC_SENSOR_ANALOG_ACCEL);
  ShimmerCalib_default(SC_SENSOR_MPU9X50_ICM20948_GYRO);
  ShimmerCalib_default(SC_SENSOR_LSM303_ACCEL);
  ShimmerCalib_default(SC_SENSOR_LSM303_MAG);
#elif defined(SHIMMER3R)
  ShimmerCalib_default(SC_SENSOR_LSM6DSV_ACCEL);
  ShimmerCalib_default(SC_SENSOR_LSM6DSV_GYRO);
  ShimmerCalib_default(SC_SENSOR_LIS2DW12_ACCEL);
  ShimmerCalib_default(SC_SENSOR_ADXL371_ACCEL);
  ShimmerCalib_default(SC_SENSOR_LIS3MDL_MAG);
  ShimmerCalib_default(SC_SENSOR_LIS2MDL_MAG);
#endif
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
    set_file_timestamp(cal_file_name);
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
    {
      strcpy(cal_file_name, "/calibration");
    }
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
  set_file_timestamp(cal_file_name);
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

#if defined(SHIMMER3)
  if (sensor == SC_SENSOR_ANALOG_ACCEL)
  {
    setDefaultKionixCalib(&sc1);
  }
  else if (sensor == SC_SENSOR_MPU9X50_ICM20948_GYRO)
  {
    setDefaultMpu9X50Icm20948GyroCalib(&sc1);
  }
  else if (sensor == SC_SENSOR_LSM303_ACCEL)
  {
    setDefaultLsm303AccelCalib(&sc1);
  }
  else if (sensor == SC_SENSOR_LSM303_MAG)
  {
    setDefaultLsm303MagCalib(&sc1);
  }
#elif defined(SHIMMER3R)
  if (sensor == SC_SENSOR_LSM6DSV_ACCEL)
  {
    setDefaultLsm6dsvAccelCalib(&sc1);
  }
  else if (sensor == SC_SENSOR_LSM6DSV_GYRO)
  {
    setDefaultLsm6dsvGyroCalib(&sc1);
  }
  else if (sensor == SC_SENSOR_LIS2DW12_ACCEL)
  {
    setDefaultLis2dw12AccelCalib(&sc1);
  }
  else if (sensor == SC_SENSOR_ADXL371_ACCEL)
  {
    setDefaultAdxl371AccelCalib(&sc1);
  }
  else if (sensor == SC_SENSOR_LIS3MDL_MAG)
  {
    setDefaultLis3mdlMagCalib(&sc1);
  }
  else if (sensor == SC_SENSOR_LIS2MDL_MAG)
  {
    setDefaultLis2mdlMagCalib(&sc1);
  }
#endif
}

#if defined(SHIMMER3)
void setDefaultKionixCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_ANALOG_ACCEL;
  sc1Ptr->data_len = SC_DATA_LEN_ANALOG_ACCEL;
  for (sc1Ptr->range = 0; sc1Ptr->range < SC_SENSOR_RANGE_MAX_ANALOG_ACCEL;
       sc1Ptr->range++)
  {
    bias = 2047;
    sensitivity = 83;
    sc1Ptr->data.dd.bias_x = bias;
    sc1Ptr->data.dd.bias_y = bias;
    sc1Ptr->data.dd.bias_z = bias;
    sc1Ptr->data.dd.sens_x = sensitivity;
    sc1Ptr->data.dd.sens_y = sensitivity;
    sc1Ptr->data.dd.sens_z = sensitivity;
    sc1Ptr->data.dd.align_xx = 0;
    sc1Ptr->data.dd.align_xy = -100;
    sc1Ptr->data.dd.align_xz = 0;
    sc1Ptr->data.dd.align_yx = -100;
    sc1Ptr->data.dd.align_yy = 0;
    sc1Ptr->data.dd.align_yz = 0;
    sc1Ptr->data.dd.align_zx = 0;
    sc1Ptr->data.dd.align_zy = 0;
    sc1Ptr->data.dd.align_zz = -100;
    ShimmerCalib_singleSensorWrite(sc1Ptr);
  }
}

void setDefaultMpu9X50Icm20948GyroCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_MPU9X50_ICM20948_GYRO;
  sc1Ptr->data_len = SC_DATA_LEN_MPU9X50_ICM20948_GYRO;
  for (sc1Ptr->range = 0;
       sc1Ptr->range < SC_SENSOR_RANGE_MAX_MPU9X50_ICM20948_GYRO; sc1Ptr->range++)
  {
    bias = 0;
    if (sc1Ptr->range == SC_SENSOR_RANGE_MPU9X50_ICM20948_GYRO_250DPS)
    {
      sensitivity = 13100;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_MPU9X50_ICM20948_GYRO_500DPS)
    {
      sensitivity = 6550;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_MPU9X50_ICM20948_GYRO_1000DPS)
    {
      sensitivity = 3280;
    }
    else
    { //(sdHeadText[SDH_GYRO_RANGE] == SC_SENSOR_RANGE_MPU9250_GYRO_2000DPS)
      sensitivity = 1640;
    }
    sc1Ptr->data.dd.bias_x = bias;
    sc1Ptr->data.dd.bias_y = bias;
    sc1Ptr->data.dd.bias_z = bias;
    sc1Ptr->data.dd.sens_x = sensitivity;
    sc1Ptr->data.dd.sens_y = sensitivity;
    sc1Ptr->data.dd.sens_z = sensitivity;
    sc1Ptr->data.dd.align_xx = 0;
    sc1Ptr->data.dd.align_xy = -100;
    sc1Ptr->data.dd.align_xz = 0;
    sc1Ptr->data.dd.align_yx = -100;
    sc1Ptr->data.dd.align_yy = 0;
    sc1Ptr->data.dd.align_yz = 0;
    sc1Ptr->data.dd.align_zx = 0;
    sc1Ptr->data.dd.align_zy = 0;
    sc1Ptr->data.dd.align_zz = -100;
    ShimmerCalib_singleSensorWrite(sc1Ptr);
  }
}

void setDefaultLsm303AccelCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_LSM303_ACCEL;
  sc1Ptr->data_len = SC_DATA_LEN_LSM303_ACCEL;
  for (sc1Ptr->range = 0; sc1Ptr->range < SC_SENSOR_RANGE_MAX_LSM303_ACCEL;
       sc1Ptr->range++)
  {
    bias = 0;
    if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_ACCEL_2G)
    {
      sensitivity = 1631;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_ACCEL_4G)
    {
      sensitivity = 815;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_ACCEL_8G)
    {
      sensitivity = 408;
    }
    else
    { //(sc1Ptr->range == SC_SENSOR_RANGE_LSM303DLHC_ACCEL_16G)
      sensitivity = 135;
    }
    sc1Ptr->data.dd.bias_x = bias;
    sc1Ptr->data.dd.bias_y = bias;
    sc1Ptr->data.dd.bias_z = bias;
    sc1Ptr->data.dd.sens_x = sensitivity;
    sc1Ptr->data.dd.sens_y = sensitivity;
    sc1Ptr->data.dd.sens_z = sensitivity;
    sc1Ptr->data.dd.align_xx = -100;
    sc1Ptr->data.dd.align_xy = 0;
    sc1Ptr->data.dd.align_xz = 0;
    sc1Ptr->data.dd.align_yx = 0;
    sc1Ptr->data.dd.align_yy = 100;
    sc1Ptr->data.dd.align_yz = 0;
    sc1Ptr->data.dd.align_zx = 0;
    sc1Ptr->data.dd.align_zy = 0;
    sc1Ptr->data.dd.align_zz = -100;
    ShimmerCalib_singleSensorWrite(sc1Ptr);
  }
}

void setDefaultLsm303MagCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_LSM303_MAG;
  sc1Ptr->data_len = SC_DATA_LEN_LSM303_MAG;
  for (sc1Ptr->range = 0; sc1Ptr->range < SC_SENSOR_RANGE_MAX_LSM303_MAG; sc1Ptr->range++)
  {
    bias = 0;
    if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_MAG_1_3G)
    {
      sc1Ptr->data.dd.sens_x = 1100;
      sc1Ptr->data.dd.sens_y = 1100;
      sc1Ptr->data.dd.sens_z = 980;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_MAG_1_9G)
    {
      sc1Ptr->data.dd.sens_x = 855;
      sc1Ptr->data.dd.sens_y = 855;
      sc1Ptr->data.dd.sens_z = 760;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_MAG_2_5G)
    {
      sc1Ptr->data.dd.sens_x = 670;
      sc1Ptr->data.dd.sens_y = 670;
      sc1Ptr->data.dd.sens_z = 600;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_MAG_4_0G)
    {
      sc1Ptr->data.dd.sens_x = 450;
      sc1Ptr->data.dd.sens_y = 450;
      sc1Ptr->data.dd.sens_z = 400;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_MAG_4_7G)
    {
      sc1Ptr->data.dd.sens_x = 400;
      sc1Ptr->data.dd.sens_y = 400;
      sc1Ptr->data.dd.sens_z = 355;
    }
    else if (sc1Ptr->range == SC_SENSOR_RANGE_LSM303_MAG_5_6G)
    {
      sc1Ptr->data.dd.sens_x = 330;
      sc1Ptr->data.dd.sens_y = 330;
      sc1Ptr->data.dd.sens_z = 295;
    }
    else
    { //sc1Ptr->range == SC_SENSOR_RANGE_LSM303DLHC_MAG_8_1G)
      sc1Ptr->data.dd.sens_x = 230;
      sc1Ptr->data.dd.sens_y = 230;
      sc1Ptr->data.dd.sens_z = 205;
    }
    sc1Ptr->data.dd.bias_x = bias;
    sc1Ptr->data.dd.bias_y = bias;
    sc1Ptr->data.dd.bias_z = bias;
    sc1Ptr->data.dd.align_xx = -100;
    sc1Ptr->data.dd.align_xy = 0;
    sc1Ptr->data.dd.align_xz = 0;
    sc1Ptr->data.dd.align_yx = 0;
    sc1Ptr->data.dd.align_yy = 100;
    sc1Ptr->data.dd.align_yz = 0;
    sc1Ptr->data.dd.align_zx = 0;
    sc1Ptr->data.dd.align_zy = 0;
    sc1Ptr->data.dd.align_zz = -100;
    ShimmerCalib_singleSensorWrite(sc1Ptr);
  }
}

#elif defined(SHIMMER3R)
void setDefaultLsm6dsvAccelCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_LSM6DSV_ACCEL;
  sc1Ptr->data_len = SC_DATA_LEN_STD_IMU_CALIB;
  for (sc1Ptr->range = 0; sc1Ptr->range < 4; sc1Ptr->range++)
  {
    bias = 0;
    if (sc1Ptr->range == LSM6DSV_2g)
    {
      sensitivity = 1672;
    }
    else if (sc1Ptr->range == LSM6DSV_4g)
    {
      sensitivity = 836;
    }
    else if (sc1Ptr->range == LSM6DSV_8g)
    {
      sensitivity = 418;
    }
    else
    { //(sc1Ptr->range == LSM6DSV_16g)
      sensitivity = 209;
    }
    sc1Ptr->data.dd.bias_x = bias;
    sc1Ptr->data.dd.bias_y = bias;
    sc1Ptr->data.dd.bias_z = bias;
    sc1Ptr->data.dd.sens_x = sensitivity;
    sc1Ptr->data.dd.sens_y = sensitivity;
    sc1Ptr->data.dd.sens_z = sensitivity;
    sc1Ptr->data.dd.align_xx = -100;
    sc1Ptr->data.dd.align_xy = 0;
    sc1Ptr->data.dd.align_xz = 0;
    sc1Ptr->data.dd.align_yx = 0;
    sc1Ptr->data.dd.align_yy = 100;
    sc1Ptr->data.dd.align_yz = 0;
    sc1Ptr->data.dd.align_zx = 0;
    sc1Ptr->data.dd.align_zy = 0;
    sc1Ptr->data.dd.align_zz = -100;
    ShimmerCalib_singleSensorWrite(sc1Ptr);
  }
}

void setDefaultLsm6dsvGyroCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_LSM6DSV_GYRO;
  sc1Ptr->data_len = SC_DATA_LEN_STD_IMU_CALIB;
  for (sc1Ptr->range = 0; sc1Ptr->range < 6; sc1Ptr->range++)
  {
    bias = 0;
    if (sc1Ptr->range == LSM6DSV_125dps)
    {
      sensitivity = 229;
    }
    else if (sc1Ptr->range == LSM6DSV_250dps)
    {
      sensitivity = 114;
    }
    else if (sc1Ptr->range == LSM6DSV_500dps)
    {
      sensitivity = 57;
    }
    else if (sc1Ptr->range == LSM6DSV_1000dps)
    {
      sensitivity = 29;
    }
    else if (sc1Ptr->range == LSM6DSV_2000dps)
    {
      sensitivity = 14;
    }
    else
    { //(sc1Ptr->range == LSM6DSV_4000dps)
      sensitivity = 7;
    }
    sc1Ptr->data.dd.bias_x = bias;
    sc1Ptr->data.dd.bias_y = bias;
    sc1Ptr->data.dd.bias_z = bias;
    sc1Ptr->data.dd.sens_x = sensitivity;
    sc1Ptr->data.dd.sens_y = sensitivity;
    sc1Ptr->data.dd.sens_z = sensitivity;
    sc1Ptr->data.dd.align_xx = -100;
    sc1Ptr->data.dd.align_xy = 0;
    sc1Ptr->data.dd.align_xz = 0;
    sc1Ptr->data.dd.align_yx = 0;
    sc1Ptr->data.dd.align_yy = 100;
    sc1Ptr->data.dd.align_yz = 0;
    sc1Ptr->data.dd.align_zx = 0;
    sc1Ptr->data.dd.align_zy = 0;
    sc1Ptr->data.dd.align_zz = -100;
    ShimmerCalib_singleSensorWrite(sc1Ptr);
  }
}

void setDefaultAdxl371AccelCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_ADXL371_ACCEL;
  sc1Ptr->data_len = SC_DATA_LEN_STD_IMU_CALIB;

  bias = 10; // +1g
  sensitivity = 1; // 100mg/LSB which equates to 1.0197 LSB/m/s^2

  sc1Ptr->range = 0;
  sc1Ptr->data.dd.bias_x = bias;
  sc1Ptr->data.dd.bias_y = bias;
  sc1Ptr->data.dd.bias_z = bias;
  sc1Ptr->data.dd.sens_x = sensitivity;
  sc1Ptr->data.dd.sens_y = sensitivity;
  sc1Ptr->data.dd.sens_z = sensitivity;
  sc1Ptr->data.dd.align_xx = 0;
  sc1Ptr->data.dd.align_xy = 100;
  sc1Ptr->data.dd.align_xz = 0;
  sc1Ptr->data.dd.align_yx = 100;
  sc1Ptr->data.dd.align_yy = 0;
  sc1Ptr->data.dd.align_yz = 0;
  sc1Ptr->data.dd.align_zx = 0;
  sc1Ptr->data.dd.align_zy = 0;
  sc1Ptr->data.dd.align_zz = -100;
  ShimmerCalib_singleSensorWrite(sc1Ptr);
}

void setDefaultLis2dw12AccelCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_LIS2DW12_ACCEL;
  sc1Ptr->data_len = SC_DATA_LEN_STD_IMU_CALIB;
  for (sc1Ptr->range = 0; sc1Ptr->range < 4; sc1Ptr->range++)
  {
    bias = 0;
    if (sc1Ptr->range == LIS2DW12_2g)
    {
      sensitivity = 1671;
    }
    else if (sc1Ptr->range == LIS2DW12_4g)
    {
      sensitivity = 836;
    }
    else if (sc1Ptr->range == LIS2DW12_8g)
    {
      sensitivity = 418;
    }
    else
    { //(sc1Ptr->range == LIS2DW12_16g)
      sensitivity = 209;
    }
    sc1Ptr->data.dd.bias_x = bias;
    sc1Ptr->data.dd.bias_y = bias;
    sc1Ptr->data.dd.bias_z = bias;
    sc1Ptr->data.dd.sens_x = sensitivity;
    sc1Ptr->data.dd.sens_y = sensitivity;
    sc1Ptr->data.dd.sens_z = sensitivity;
    sc1Ptr->data.dd.align_xx = 0;
    sc1Ptr->data.dd.align_xy = -100;
    sc1Ptr->data.dd.align_xz = 0;
    sc1Ptr->data.dd.align_yx = -100;
    sc1Ptr->data.dd.align_yy = 0;
    sc1Ptr->data.dd.align_yz = 0;
    sc1Ptr->data.dd.align_zx = 0;
    sc1Ptr->data.dd.align_zy = 0;
    sc1Ptr->data.dd.align_zz = -100;
    ShimmerCalib_singleSensorWrite(sc1Ptr);
  }
}

void setDefaultLis2mdlMagCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_LIS2MDL_MAG;
  sc1Ptr->data_len = SC_DATA_LEN_STD_IMU_CALIB;

  bias = 0;
  sensitivity = 667;

  sc1Ptr->range = 0;
  sc1Ptr->data.dd.bias_x = bias;
  sc1Ptr->data.dd.bias_y = bias;
  sc1Ptr->data.dd.bias_z = bias;
  sc1Ptr->data.dd.sens_x = sensitivity;
  sc1Ptr->data.dd.sens_y = sensitivity;
  sc1Ptr->data.dd.sens_z = sensitivity;
  sc1Ptr->data.dd.align_xx = -100;
  sc1Ptr->data.dd.align_xy = 0;
  sc1Ptr->data.dd.align_xz = 0;
  sc1Ptr->data.dd.align_yx = 0;
  sc1Ptr->data.dd.align_yy = -100;
  sc1Ptr->data.dd.align_yz = 0;
  sc1Ptr->data.dd.align_zx = 0;
  sc1Ptr->data.dd.align_zy = 0;
  sc1Ptr->data.dd.align_zz = -100;
  ShimmerCalib_singleSensorWrite(sc1Ptr);
}

void setDefaultLis3mdlMagCalib(sc_t *sc1Ptr)
{
  uint16_t bias, sensitivity;
  sc1Ptr->id = SC_SENSOR_LIS3MDL_MAG;
  sc1Ptr->data_len = SC_DATA_LEN_STD_IMU_CALIB;
  for (sc1Ptr->range = 0; sc1Ptr->range < 4; sc1Ptr->range++)
  {
    bias = 0;
    if (sc1Ptr->range == LIS3MDL_4_GAUSS)
    {
      sensitivity = 6842;
    }
    else if (sc1Ptr->range == LIS3MDL_8_GAUSS)
    {
      sensitivity = 3421;
    }
    else if (sc1Ptr->range == LIS3MDL_12_GAUSS)
    {
      sensitivity = 2281;
    }
    else
    { //(sc1Ptr->range == LIS3MDL_16_GAUSS)
      sensitivity = 1711;
    }
    sc1Ptr->data.dd.bias_x = bias;
    sc1Ptr->data.dd.bias_y = bias;
    sc1Ptr->data.dd.bias_z = bias;
    sc1Ptr->data.dd.sens_x = sensitivity;
    sc1Ptr->data.dd.sens_y = sensitivity;
    sc1Ptr->data.dd.sens_z = sensitivity;
    sc1Ptr->data.dd.align_xx = 100;
    sc1Ptr->data.dd.align_xy = 0;
    sc1Ptr->data.dd.align_xz = 0;
    sc1Ptr->data.dd.align_yx = 0;
    sc1Ptr->data.dd.align_yy = -100;
    sc1Ptr->data.dd.align_yz = 0;
    sc1Ptr->data.dd.align_zx = 0;
    sc1Ptr->data.dd.align_zy = 0;
    sc1Ptr->data.dd.align_zz = -100;
    ShimmerCalib_singleSensorWrite(sc1Ptr);
  }
}
#endif

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
#if defined(SHIMMER3)
  if (id == 0xFF || id == SC_SENSOR_ANALOG_ACCEL)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_ANALOG_ACCEL, SC_SENSOR_RANGE_ANALOG_ACCEL,
        SC_DATA_LEN_ANALOG_ACCEL, &configBytes->lnAccelCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_MPU9X50_ICM20948_GYRO)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_MPU9X50_ICM20948_GYRO,
        get_config_byte_gyro_range(), SC_DATA_LEN_MPU9X50_ICM20948_GYRO,
        &configBytes->gyroCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_LSM303_MAG)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LSM303_MAG, configBytes->magRange,
        SC_DATA_LEN_LSM303_MAG, &configBytes->magCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_LSM303_ACCEL)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LSM303_ACCEL,
        configBytes->wrAccelRange, SC_DATA_LEN_LSM303_ACCEL,
        &configBytes->wrAccelCalib.rawBytes[0]);
  }
#elif defined(SHIMMER3R)
  if (id == 0xFF || id == SC_SENSOR_LSM6DSV_ACCEL)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LSM6DSV_ACCEL,
        configBytes->altAccelRange, SC_DATA_LEN_STD_IMU_CALIB,
        &configBytes->lnAccelCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_LSM6DSV_GYRO)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LSM6DSV_GYRO,
        get_config_byte_gyro_range(), SC_DATA_LEN_STD_IMU_CALIB,
        &configBytes->gyroCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_LIS2DW12_ACCEL)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LIS2DW12_ACCEL,
        configBytes->wrAccelRange, SC_DATA_LEN_STD_IMU_CALIB,
        &configBytes->wrAccelCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_ADXL371_ACCEL)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_ADXL371_ACCEL, 0,
        SC_DATA_LEN_STD_IMU_CALIB, &configBytes->altAccelCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_LIS3MDL_MAG)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LIS3MDL_MAG, configBytes->magRange,
        SC_DATA_LEN_STD_IMU_CALIB, &configBytes->magCalib.rawBytes[0]);
  }
  if (id == 0xFF || id == SC_SENSOR_LIS2MDL_MAG)
  {
    ShimmerCalib_singleSensorWriteFromInfoMem(SC_SENSOR_LIS2MDL_MAG, 0,
        SC_DATA_LEN_STD_IMU_CALIB, &configBytes->altMagCalib.rawBytes[0]);
  }
#endif
}

void ShimmerCalibInitFromInfoAll(void)
{
#if defined(SHIMMER3)
  ShimmerCalibFromInfo(SC_SENSOR_ANALOG_ACCEL, 0);
  ShimmerCalibFromInfo(SC_SENSOR_MPU9X50_ICM20948_GYRO, 0);
  ShimmerCalibFromInfo(SC_SENSOR_LSM303_ACCEL, 0);
  ShimmerCalibFromInfo(SC_SENSOR_LSM303_MAG, 0);
#elif defined(SHIMMER3R)
  ShimmerCalibFromInfo(SC_SENSOR_LSM6DSV_ACCEL, 0);
  ShimmerCalibFromInfo(SC_SENSOR_LSM6DSV_GYRO, 0);
  ShimmerCalibFromInfo(SC_SENSOR_LIS2DW12_ACCEL, 0);
  ShimmerCalibFromInfo(SC_SENSOR_ADXL371_ACCEL, 0);
  ShimmerCalibFromInfo(SC_SENSOR_LIS3MDL_MAG, 0);
  ShimmerCalibFromInfo(SC_SENSOR_LIS2MDL_MAG, 0);
#endif
}

void ShimmerCalibUpdateFromInfoAll(void)
{
#if defined(SHIMMER3)
  ShimmerCalibFromInfo(SC_SENSOR_ANALOG_ACCEL, 1);
  ShimmerCalibFromInfo(SC_SENSOR_MPU9X50_ICM20948_GYRO, 1);
  ShimmerCalibFromInfo(SC_SENSOR_LSM303_ACCEL, 1);
  ShimmerCalibFromInfo(SC_SENSOR_LSM303_MAG, 1);
#elif defined(SHIMMER3R)
  ShimmerCalibFromInfo(SC_SENSOR_LSM6DSV_ACCEL, 1);
  ShimmerCalibFromInfo(SC_SENSOR_LSM6DSV_GYRO, 1);
  ShimmerCalibFromInfo(SC_SENSOR_LIS2DW12_ACCEL, 1);
  ShimmerCalibFromInfo(SC_SENSOR_ADXL371_ACCEL, 1);
  ShimmerCalibFromInfo(SC_SENSOR_LIS3MDL_MAG, 1);
  ShimmerCalibFromInfo(SC_SENSOR_LIS2MDL_MAG, 1);
#endif
}

void ShimmerCalibFromInfo(uint8_t sensor, uint8_t use_sys_time)
{
  uint8_t info_config, info_valid = 0;
  uint8_t offset;
  int byte_cnt = 0;
  sc_t sc1;
  gConfigBytes *configBytes = S4Ram_getStoredConfig();

  sc1.id = sensor;
  if (use_sys_time)
  {
    memset(sc1.ts, 0, 8);
  }
  else
  {
    *(uint64_t *) (sc1.ts) = RTC_get64();
  }

#if defined(SHIMMER3)
  if (sc1.id == SC_SENSOR_ANALOG_ACCEL)
  {
    offset = NV_LN_ACCEL_CALIBRATION;
    sc1.range = SC_SENSOR_RANGE_ANALOG_ACCEL;
    sc1.data_len = SC_DATA_LEN_ANALOG_ACCEL;
  }
  else if (sc1.id == SC_SENSOR_MPU9X50_ICM20948_GYRO)
  {
    offset = NV_GYRO_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_MPU9X50_ICM20948_GYRO;
    sc1.range = storedConfig.gyroRangeLsb;
  }
  else if (sc1.id == SC_SENSOR_LSM303_ACCEL)
  {
    offset = NV_WR_ACCEL_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_LSM303_ACCEL;
    sc1.range = configBytes->wrAccelRange;
  }
  else if (sc1.id == SC_SENSOR_LSM303_MAG)
  {
    offset = NV_MAG_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_LSM303_MAG;
    sc1.range = configBytes->magRange;
  }
#elif defined(SHIMMER3R)
  if (sc1.id == SC_SENSOR_LSM6DSV_ACCEL)
  {
    offset = NV_LN_ACCEL_CALIBRATION;
    sc1.range = configBytes->altAccelRange;
    sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
  }
  else if (sc1.id == SC_SENSOR_LSM6DSV_GYRO)
  {
    offset = NV_GYRO_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
    sc1.range = get_config_byte_gyro_range();
  }
  else if (sc1.id == SC_SENSOR_LIS2DW12_ACCEL)
  {
    offset = NV_WR_ACCEL_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
    sc1.range = configBytes->wrAccelRange;
  }
  else if (sc1.id == SC_SENSOR_ADXL371_ACCEL)
  {
    offset = NV_ALT_ACCEL_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
    sc1.range = 0;
  }
  else if (sc1.id == SC_SENSOR_LIS3MDL_MAG)
  {
    offset = NV_MAG_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
    sc1.range = configBytes->magRange;
  }
  else if (sc1.id == SC_SENSOR_LIS2MDL_MAG)
  {
    offset = NV_ALT_MAG_CALIBRATION;
    sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
    sc1.range = 0;
  }
#endif
  else
  {
    return;
  }

  for (byte_cnt = SC_DATA_LEN_STD_IMU_CALIB; byte_cnt > 0; byte_cnt--)
  {
    if (configBytes->rawBytes[offset + byte_cnt] != 0xff)
    {
      info_valid = 1;
      break;
    }
  }

  if (info_valid)
  {
    /* if not all 0xff in infomem */
    memcpy(sc1.data.raw, &configBytes->rawBytes[offset], sc1.data_len);
    ShimmerCalib_singleSensorWrite(&sc1);
  }
}

void ShimmerCalibSyncFromDumpRamAll(void)
{
#if defined(SHIMMER3)
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_ANALOG_ACCEL);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_MPU9X50_ICM20948_GYRO);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LSM303_MAG);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LSM303_ACCEL);
#elif defined(SHIMMER3R)
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LSM6DSV_ACCEL);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LSM6DSV_GYRO);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LIS2DW12_ACCEL);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_ADXL371_ACCEL);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LIS3MDL_MAG);
  ShimmerCalibSyncFromDumpRamSingleSensor(SC_SENSOR_LIS2MDL_MAG);
#endif
}

void ShimmerCalibSyncFromDumpRamSingleSensor(uint8_t sensor)
{
  sc_t sc1;
  uint16_t scs_infomem_offset, scs_sdhead_offset, scs_sdhead_ts;
  sc1.id = sensor;
  sc1.data_len = ShimmerCalib_findLength(&sc1);

  gConfigBytes *configBytes = S4Ram_getStoredConfig();

  switch (sensor)
  {
#if defined(SHIMMER3)
  case SC_SENSOR_ANALOG_ACCEL:
    scs_infomem_offset = NV_LN_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_LN_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_LN_ACCEL_CALIB_TS;
    sc1.range = SC_SENSOR_RANGE_ANALOG_ACCEL;
    break;
  case SC_SENSOR_MPU9X50_ICM20948_GYRO:
    scs_infomem_offset = NV_GYRO_CALIBRATION;
    scs_sdhead_offset = SDH_GYRO_CALIBRATION;
    scs_sdhead_ts = SDH_GYRO_CALIB_TS;
    sc1.range = get_config_byte_gyro_range();
    break;
  case SC_SENSOR_LSM303_MAG:
    scs_infomem_offset = NV_MAG_CALIBRATION;
    scs_sdhead_offset = SDH_MAG_CALIBRATION;
    scs_sdhead_ts = SDH_MAG_CALIB_TS;
    sc1.range = configBytes->magRange;
    break;
  case SC_SENSOR_LSM303_ACCEL:
    scs_infomem_offset = NV_WR_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_WR_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_WR_ACCEL_CALIB_TS;
    sc1.range = configBytes->wrAccelRange;
    break;
  default:
    scs_infomem_offset = NV_LN_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_LN_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_LN_ACCEL_CALIB_TS;
    sc1.range = SC_SENSOR_RANGE_ANALOG_ACCEL;
    break;
#elif defined(SHIMMER3R)
  case SC_SENSOR_LSM6DSV_ACCEL:
    scs_infomem_offset = NV_LN_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_LN_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_LN_ACCEL_CALIB_TS;
    sc1.range = configBytes->altAccelRange;
    break;
  case SC_SENSOR_LSM6DSV_GYRO:
    scs_infomem_offset = NV_GYRO_CALIBRATION;
    scs_sdhead_offset = SDH_GYRO_CALIBRATION;
    scs_sdhead_ts = SDH_GYRO_CALIB_TS;
    sc1.range = get_config_byte_gyro_range();
    break;
  case SC_SENSOR_LIS2DW12_ACCEL:
    scs_infomem_offset = NV_WR_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_WR_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_WR_ACCEL_CALIB_TS;
    sc1.range = configBytes->wrAccelRange;
    break;
  case SC_SENSOR_ADXL371_ACCEL:
    scs_infomem_offset = NV_ALT_ACCEL_CALIBRATION;
    scs_sdhead_offset = SDH_ALT_ACCEL_CALIBRATION;
    scs_sdhead_ts = SDH_ALT_ACCEL_CALIB_TS;
    sc1.range = SC_SENSOR_RANGE_ADXL371_RANGE;
    break;
  case SC_SENSOR_LIS3MDL_MAG:
    scs_infomem_offset = NV_MAG_CALIBRATION;
    scs_sdhead_offset = SDH_MAG_CALIBRATION;
    scs_sdhead_ts = SDH_MAG_CALIB_TS;
    sc1.range = configBytes->magRange;
    break;
  case SC_SENSOR_LIS2MDL_MAG:
    scs_infomem_offset = NV_ALT_MAG_CALIBRATION;
    scs_sdhead_offset = SDH_ALT_MAG_CALIBRATION;
    scs_sdhead_ts = SDH_ALT_MAG_CALIB_TS;
    sc1.range = SC_SENSOR_RANGE_LIS2MDL_RANGE;
    break;
  default:
    break;
#endif
  }

  ShimmerCalib_singleSensorRead(&sc1);
  memcpy(&configBytes->rawBytes[scs_infomem_offset], sc1.data.raw, sc1.data_len);
  InfoMem_update();
  memcpy(S4Ram_getSdHeadText() + scs_sdhead_offset, sc1.data.raw, sc1.data_len);
  memcpy(S4Ram_getSdHeadText() + scs_sdhead_ts, sc1.ts, 8);
}
