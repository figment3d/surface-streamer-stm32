/**
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "vl53l1_platform.h"
#include "main.h"

/*
 * I2C1 is created and initialized in main.c.
 * The VL53L1X ULD passes the device address in STM32 HAL's
 * 8-bit address form (normally 0x52 for the default 7-bit address 0x29).
 */
extern I2C_HandleTypeDef hi2c1;


int8_t VL53L1_WriteMulti(
    uint16_t dev,
    uint16_t index,
    uint8_t *pdata,
    uint32_t count)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(
        &hi2c1,
        dev,
        index,
        I2C_MEMADD_SIZE_16BIT,
        pdata,
        (uint16_t)count,
        HAL_MAX_DELAY);

    return (status == HAL_OK) ? 0 : -1;
}


int8_t VL53L1_ReadMulti(
    uint16_t dev,
    uint16_t index,
    uint8_t *pdata,
    uint32_t count)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(
        &hi2c1,
        dev,
        index,
        I2C_MEMADD_SIZE_16BIT,
        pdata,
        (uint16_t)count,
        HAL_MAX_DELAY);

    return (status == HAL_OK) ? 0 : -1;
}


int8_t VL53L1_WrByte(
    uint16_t dev,
    uint16_t index,
    uint8_t data)
{
    return VL53L1_WriteMulti(dev, index, &data, 1);
}


int8_t VL53L1_WrWord(
    uint16_t dev,
    uint16_t index,
    uint16_t data)
{
    uint8_t buffer[2];

    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data & 0xFF);

    return VL53L1_WriteMulti(dev, index, buffer, 2);
}


int8_t VL53L1_WrDWord(
    uint16_t dev,
    uint16_t index,
    uint32_t data)
{
    uint8_t buffer[4];

    buffer[0] = (uint8_t)(data >> 24);
    buffer[1] = (uint8_t)(data >> 16);
    buffer[2] = (uint8_t)(data >> 8);
    buffer[3] = (uint8_t)(data & 0xFF);

    return VL53L1_WriteMulti(dev, index, buffer, 4);
}


int8_t VL53L1_RdByte(
    uint16_t dev,
    uint16_t index,
    uint8_t *data)
{
    return VL53L1_ReadMulti(dev, index, data, 1);
}


int8_t VL53L1_RdWord(
    uint16_t dev,
    uint16_t index,
    uint16_t *data)
{
    uint8_t buffer[2];
    int8_t status;

    status = VL53L1_ReadMulti(dev, index, buffer, 2);

    if (status == 0)
    {
        *data =
            ((uint16_t)buffer[0] << 8) |
            (uint16_t)buffer[1];
    }

    return status;
}


int8_t VL53L1_RdDWord(
    uint16_t dev,
    uint16_t index,
    uint32_t *data)
{
    uint8_t buffer[4];
    int8_t status;

    status = VL53L1_ReadMulti(dev, index, buffer, 4);

    if (status == 0)
    {
        *data =
            ((uint32_t)buffer[0] << 24) |
            ((uint32_t)buffer[1] << 16) |
            ((uint32_t)buffer[2] << 8) |
            (uint32_t)buffer[3];
    }

    return status;
}


int8_t VL53L1_WaitMs(
    uint16_t dev,
    int32_t wait_ms)
{
    (void)dev;

    if (wait_ms > 0)
    {
        HAL_Delay((uint32_t)wait_ms);
    }

    return 0;
}