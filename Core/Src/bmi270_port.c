#include "bmi270_port.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

BMI2_INTF_RETURN_TYPE bmi2_spi_read(
    uint8_t reg_addr,
    uint8_t *reg_data,
    uint32_t len,
    void *intf_ptr)
{
    (void)intf_ptr;

    uint8_t tx_buf[64] = { 0 };
    uint8_t rx_buf[64] = { 0 };

    if ((len + 1U) > sizeof(tx_buf))
    {
        return BMI2_E_COM_FAIL;
    }

    tx_buf[0] = reg_addr;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(
            &hspi1,
            tx_buf,
            rx_buf,
            (uint16_t)(len + 1U),
            HAL_MAX_DELAY);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    if (status != HAL_OK)
    {
        return BMI2_E_COM_FAIL;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        reg_data[i] = rx_buf[i + 1U];
    }

    return BMI2_INTF_RET_SUCCESS;
}

BMI2_INTF_RETURN_TYPE bmi2_spi_write(
    uint8_t reg_addr,
    const uint8_t *reg_data,
    uint32_t len,
    void *intf_ptr)
{
    (void)intf_ptr;

    uint8_t tx_buf[64];

    if ((len + 1U) > sizeof(tx_buf))
    {
        return BMI2_E_COM_FAIL;
    }

    tx_buf[0] = reg_addr;

    for (uint32_t i = 0; i < len; i++)
    {
        tx_buf[i + 1U] = reg_data[i];
    }

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(
            &hspi1,
            tx_buf,
            (uint16_t)(len + 1U),
            HAL_MAX_DELAY);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    return (status == HAL_OK)
        ? BMI2_INTF_RET_SUCCESS
        : BMI2_E_COM_FAIL;
}

static void bmi270_dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void bmi2_delay_us(
    uint32_t period,
    void *intf_ptr)
{
    (void)intf_ptr;

    if (period >= 1000)
    {
        HAL_Delay((period + 999) / 1000);
    }
    else
    {
        uint32_t start = DWT->CYCCNT;
        uint32_t ticks = period * (SystemCoreClock / 1000000U);

        while ((DWT->CYCCNT - start) < ticks)
        {
        }
    }
}

int8_t bmi270_stm32_interface_init(struct bmi2_dev *dev)
{
    if (dev == NULL)
    {
        return BMI2_E_NULL_PTR;
    }

    bmi270_dwt_init();

    dev->intf = BMI2_SPI_INTF;
    dev->read = bmi2_spi_read;
    dev->write = bmi2_spi_write;
    dev->delay_us = bmi2_delay_us;
    dev->intf_ptr = &hspi1;
    dev->read_write_len = 32;
    dev->config_file_ptr = NULL;

    return BMI2_OK;
}