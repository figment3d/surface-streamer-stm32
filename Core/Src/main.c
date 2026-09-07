/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "VL53L1X_api.h"
#include "bmi270_port.h"
#include "lwip/ip4_addr.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
extern struct netif gnetif;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
uint8_t vl53_status = 0;
int8_t bmi270_rslt;
struct bmi2_dev bmi270_dev;
static struct udp_pcb *udp_pcb;
static struct tcp_pcb *tcp_pcb;
static uint8_t tcp_connected = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void TCP_Connect(void);
static void TCP_SendReady(void);

static err_t TCP_ConnectedCallback(
    void *arg,
    struct tcp_pcb *tpcb,
    err_t err
);

static void TCP_ErrorCallback(
    void *arg,
    err_t err
);
/* USER CODE BEGIN PFP */

static err_t TCP_RecvCallback(
    void *arg,
    struct tcp_pcb *tpcb,
    struct pbuf *p,
    err_t err
);

static void UDP_SendReady(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static int I2C_SensorDetected(void)
{
  hi2c1.Instance->ICR =
      I2C_ICR_NACKCF |
      I2C_ICR_STOPCF |
      I2C_ICR_BERRCF |
      I2C_ICR_ARLOCF;

  hi2c1.Instance->CR2 =
      (0x29 << 1) |
      I2C_CR2_AUTOEND |
      I2C_CR2_START;

  HAL_Delay(2);

  uint32_t isr = hi2c1.Instance->ISR;
  int detected = ((isr & I2C_ISR_NACKF) == 0);

  hi2c1.Instance->ICR =
      I2C_ICR_NACKCF |
      I2C_ICR_STOPCF |
      I2C_ICR_BERRCF |
      I2C_ICR_ARLOCF;

  return detected;
}

static int8_t BMI270_EnableAccelGyro(void)
{
    int8_t rslt;
    uint8_t sensors[2] = {
        BMI2_ACCEL,
        BMI2_GYRO
    };

    rslt = bmi2_sensor_enable(
        sensors,
        2,
        &bmi270_dev
    );

    return rslt;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_LWIP_Init();
  /* USER CODE BEGIN 2 */
      
  bmi270_rslt = bmi270_stm32_interface_init(&bmi270_dev);

  if (bmi270_rslt == BMI2_OK)
  {
      bmi270_rslt = bmi270_init(&bmi270_dev);

      if (bmi270_rslt == BMI2_OK)
      {
          bmi270_rslt = BMI270_EnableAccelGyro();
      }
  }

  if (VL53L1X_SensorInit(0x52) == 0)
  {
      if (VL53L1X_StartRanging(0x52) == 0)
      {
          vl53_status = 1;
      }
  }

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
    MX_LWIP_Process();

    static uint32_t lastUdpReadyTick = 0;
    static uint32_t lastTcpReadyTick = 0;

    if (HAL_GetTick() - lastUdpReadyTick >= 1000)
    {
        UDP_SendReady();
        lastUdpReadyTick = HAL_GetTick();
    }

    if (HAL_GetTick() - lastTcpReadyTick >= 1000)
    {
        TCP_SendReady();
        lastTcpReadyTick = HAL_GetTick();
    }

    static uint8_t rxByte;
    static char rxBuf[32];
    static uint32_t rxIndex = 0;

    if (HAL_UART_Receive(
            &hcom_uart[COM1],
            &rxByte,
            1,
            10) == HAL_OK)
    {
      if (rxByte == '\r' || rxByte == '\n')
      {
        if (rxIndex > 0)
        {
          rxBuf[rxIndex] = '\0';

          if (strcmp(rxBuf, "PING") == 0)
          {
            static const char reply[] =
                "SURFACE_STREAMER_READY\r\n";

            HAL_UART_Transmit(
                &hcom_uart[COM1],
                (uint8_t *)reply,
                sizeof(reply) - 1,
                HAL_MAX_DELAY
            );
          }

          else if (strcmp(rxBuf, "I2C_STATUS") == 0)
          {
              if (I2C_SensorDetected())
              {
                  uint16_t distance = 0;
                  uint8_t dataReady = 0;
                  char reply[64];

                  if (vl53_status &&
                      VL53L1X_CheckForDataReady(0x52, &dataReady) == 0 &&
                      dataReady &&
                      VL53L1X_GetDistance(0x52, &distance) == 0)
                  {
                      VL53L1X_ClearInterrupt(0x52);

                      snprintf(
                          reply,
                          sizeof(reply),
                          "I2C_READY %u\r\n",
                          distance
                      );
                  }
                  else
                  {
                      snprintf(
                          reply,
                          sizeof(reply),
                          "I2C_READY\r\n"
                      );
                  }

                  HAL_UART_Transmit(
                      &hcom_uart[COM1],
                      (uint8_t *)reply,
                      strlen(reply),
                      HAL_MAX_DELAY
                  );
              }
              else
              {
                  static const char reply[] =
                      "I2C_NOT_DETECTED\r\n";

                  HAL_UART_Transmit(
                      &hcom_uart[COM1],
                      (uint8_t *)reply,
                      sizeof(reply) - 1,
                      HAL_MAX_DELAY
                  );
              }
          }
  
          else if (strcmp(rxBuf, "SPI_STATUS") == 0)
          {
              char reply[64];

              if (bmi270_rslt == BMI2_OK)
              {
                  snprintf(
                      reply,
                      sizeof(reply),
                      "SPI_CHIP_ID 0x24\r\n"
                  );
              }
              else
              {
                snprintf(
                    reply,
                    sizeof(reply),
                    "SPI_NOT_DETECTED %d CHIP_ID 0x%02X\r\n",
                    bmi270_rslt,
                    bmi270_dev.chip_id
                );              
              }

              HAL_UART_Transmit(
                  &hcom_uart[COM1],
                  (uint8_t *)reply,
                  strlen(reply),
                  HAL_MAX_DELAY
              );
          }
          else if (strcmp(rxBuf, "SPI_DATA") == 0)
          {
              struct bmi2_sens_data sensor_data;
              char reply[128];

              int8_t rslt = bmi2_get_sensor_data(
                  &sensor_data,
                  &bmi270_dev
              );

              if (rslt == BMI2_OK)
              {
                  snprintf(
                      reply,
                      sizeof(reply),
                      "ACC %d %d %d GYR %d %d %d\r\n",
                      sensor_data.acc.x,
                      sensor_data.acc.y,
                      sensor_data.acc.z,
                      sensor_data.gyr.x,
                      sensor_data.gyr.y,
                      sensor_data.gyr.z
                  );
              }
              else
              {
                  snprintf(
                      reply,
                      sizeof(reply),
                      "SPI_DATA_ERROR %d\r\n",
                      rslt
                  );
              }

              HAL_UART_Transmit(
                  &hcom_uart[COM1],
                  (uint8_t *)reply,
                  strlen(reply),
                  HAL_MAX_DELAY
              );
          }
          else if (strcmp(rxBuf, "ETH_STATUS") == 0)
          {
              char msg[64];

              if (gnetif.ip_addr.addr != 0)
              {
                  snprintf(
                      msg,
                      sizeof(msg),
                      "ETH_IP %s %s\r\n",
                      ip4addr_ntoa(netif_ip4_addr(&gnetif)),
                      netif_is_link_up(&gnetif) ? "LINK_UP" : "LINK_DOWN"
                  );
              }
              else
              {
                  snprintf(msg, sizeof(msg), "ETH_NO_IP\r\n");
              }

              HAL_UART_Transmit(
                  &hcom_uart[COM1],
                  (uint8_t *)msg,
                  strlen(msg),
                  HAL_MAX_DELAY
              );
          }          
          rxIndex = 0;
        }
      }
      else if (rxIndex < sizeof(rxBuf) - 1)
      {
        rxBuf[rxIndex++] = (char)rxByte;
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 9;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 1;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLFRACN = 3072;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10707DBC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(I2C_SHUT_GPIO_Port, I2C_SHUT_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin : I2C_SHUT_Pin */
  GPIO_InitStruct.Pin = I2C_SHUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(I2C_SHUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static void UDP_SendReady(void)
{
    ip_addr_t dest_ip;
    struct pbuf *p;
    const char *msg = "STM32_UDP_READY";

    IP4_ADDR(&dest_ip, 192, 168, 10, 1);

    if (udp_pcb == NULL)
    {
        udp_pcb = udp_new();
    }

    if (udp_pcb == NULL)
    {
        return;
    }

    p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg), PBUF_RAM);

    if (p == NULL)
    {
        return;
    }

    memcpy(p->payload, msg, strlen(msg));

    err_t err;

    err = udp_sendto(
        udp_pcb,
        p,
        &dest_ip,
        10000
    );

    char msg2[32];

  snprintf(
      msg2,
      sizeof(msg2),
      "UDP_SEND %d\r\n",
      (int)err
  );

  HAL_UART_Transmit(
      &hcom_uart[COM1],
      (uint8_t *)msg2,
      strlen(msg2),
      HAL_MAX_DELAY
  );

    pbuf_free(p);
}

static err_t TCP_ConnectedCallback(
    void *arg,
    struct tcp_pcb *tpcb,
    err_t err)
{
    if (err == ERR_OK)
    {
        tcp_connected = 1;

        tcp_recv(
            tpcb,
            TCP_RecvCallback
        );

        static const char msg[] =
            "TCP_CONNECTED\r\n";

        HAL_UART_Transmit(
            &hcom_uart[COM1],
            (uint8_t *)msg,
            sizeof(msg) - 1,
            HAL_MAX_DELAY
        );
    }

    return err;
}
static err_t TCP_RecvCallback(
    void *arg,
    struct tcp_pcb *tpcb,
    struct pbuf *p,
    err_t err)
{
    /*
     * p == NULL means the remote side closed
     * the TCP connection normally.
     */
    if (p == NULL)
    {
        tcp_connected = 0;
        tcp_pcb = NULL;

        tcp_close(tpcb);

        static const char msg[] =
            "TCP_CLOSED\r\n";

        HAL_UART_Transmit(
            &hcom_uart[COM1],
            (uint8_t *)msg,
            sizeof(msg) - 1,
            HAL_MAX_DELAY
        );

        return ERR_OK;
    }

    /*
     * We do not currently expect commands from
     * the PC, but acknowledge and discard anything
     * received.
     */
    tcp_recved(
        tpcb,
        p->tot_len
    );

    pbuf_free(p);

    return ERR_OK;
}

static void TCP_ErrorCallback(
    void *arg,
    err_t err)
{
    tcp_pcb = NULL;
    tcp_connected = 0;

    char msg[32];

    snprintf(
        msg,
        sizeof(msg),
        "TCP_ERROR %d\r\n",
        (int)err
    );

    HAL_UART_Transmit(
        &hcom_uart[COM1],
        (uint8_t *)msg,
        strlen(msg),
        HAL_MAX_DELAY
    );
}


static void TCP_Connect(void)
{
    ip_addr_t dest_ip;

    if (tcp_pcb != NULL)
    {
        return;
    }

    tcp_pcb = tcp_new();

    if (tcp_pcb == NULL)
    {
        return;
    }

    tcp_err(
        tcp_pcb,
        TCP_ErrorCallback
    );

    IP4_ADDR(
        &dest_ip,
        192,
        168,
        10,
        1
    );

    err_t err = tcp_connect(
        tcp_pcb,
        &dest_ip,
        10001,
        TCP_ConnectedCallback
    );

    if (err != ERR_OK)
    {
        char msg[32];

        snprintf(
            msg,
            sizeof(msg),
            "TCP_CONNECT %d\r\n",
            (int)err
        );

        HAL_UART_Transmit(
            &hcom_uart[COM1],
            (uint8_t *)msg,
            strlen(msg),
            HAL_MAX_DELAY
        );

        tcp_abort(tcp_pcb);
        tcp_pcb = NULL;
        tcp_connected = 0;
    }
}


static void TCP_SendReady(void)
{
    const char *msg =
        "STM32_TCP_READY";

    if (!tcp_connected ||
        tcp_pcb == NULL)
    {
        TCP_Connect();
        return;
    }

    err_t err = tcp_write(
        tcp_pcb,
        msg,
        strlen(msg),
        TCP_WRITE_FLAG_COPY
    );

    if (err == ERR_OK)
    {
        err = tcp_output(tcp_pcb);
    }

    char msg2[32];

    snprintf(
        msg2,
        sizeof(msg2),
        "TCP_SEND %d\r\n",
        (int)err
    );

    HAL_UART_Transmit(
        &hcom_uart[COM1],
        (uint8_t *)msg2,
        strlen(msg2),
        HAL_MAX_DELAY
    );
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
