/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BL_MAJOR 0
#define BL_MINOR 1
#define APP_ADDRESS 0x08004000U
#define FLASH_USER_START_ADDR   0x0800FC00
#define RX_BUFFER_SIZE 256 // Chunk boyutu

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

uint8_t BL_Version[2] = {BL_MAJOR, BL_MINOR};
typedef void (*pFunction)(void);
volatile uint8_t RxBuffer[RX_BUFFER_SIZE];
volatile uint8_t RxCpltFlag = 0; // Alım tamamlanınca 1 olacak bayrak

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

static int ota_download_and_flash(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

void Flash_Write(uint32_t address, uint32_t data);
uint32_t Flash_Read(uint32_t address);

static void jump_to_app(void) {
    uint32_t app_stack = *(volatile uint32_t*)APP_ADDRESS;
    uint32_t app_reset_handler = *(volatile uint32_t*)(APP_ADDRESS + 4);
    pFunction app_entry = (pFunction)app_reset_handler;

    __disable_irq();

    // Periferalleri ve RCC’yi resetle
    HAL_RCC_DeInit();
    HAL_DeInit();

    // SysTick kapat
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    // MSP ayarla (stack pointer)
    __set_MSP(app_stack);

    // Vector Table’ı yeni uygulamaya yönlendir
    SCB->VTOR = APP_ADDRESS;

    __enable_irq();

    // Reset handler'a atla
    app_entry();

    // Buraya asla düşmemeli
    while (1);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  printf("Press button to enter OTA mode (10s timeout)...\n");
  uint32_t timeout = HAL_GetTick() + 10000;
  GPIO_PinState button_state;
  do {
      button_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3);
      if (button_state == GPIO_PIN_RESET || HAL_GetTick() > timeout) break;
  } while (1);

  if (button_state == GPIO_PIN_RESET) {
      // Enter UART update mode
      printf("Starting firmware download via UART...\n");
      printf("READY\n");
      if (ota_download_and_flash() != 0) {  // Implement this function (see below)
          printf("Update failed! Halting.\n");
          while (1);
      }
      printf("Update done! Resetting...\n");
      HAL_NVIC_SystemReset();
  } else {
      jump_to_app();  // Normal boot
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


static int ota_download_and_flash(void)
{
    //uint8_t buffer[RX_BUFFER_SIZE];
    uint32_t fw_size = 0;
    uint32_t address = APP_ADDRESS;
    uint8_t header[8];

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = APP_ADDRESS,
        .NbPages = 48
    };
    uint32_t erase_error;
    if (HAL_FLASHEx_Erase(&erase, &erase_error) != HAL_OK) {
        printf("Flash silme hatasi! Kod: %lu\n", erase_error);
        HAL_FLASH_Lock();
        return -1;
    }
    printf("Flash silme basarili.\n");

    // Header alımını kontrol et
    HAL_StatusTypeDef header_status = HAL_UART_Receive(&huart1, header, 8, 5000);
    if (header_status != HAL_OK) {
        if (header_status == HAL_TIMEOUT) {
            printf("Header alma hatasi! Zaman asimi!\n");
        } else {
            printf("Header alma hatasi! Durum kodu: %d\n", header_status);
        }
        HAL_FLASH_Lock();
        return -1;
    }
    printf("OK\n");

    fw_size = *(uint32_t*)header;

    // Veri alma döngüsünü kontrol et
    while (fw_size > 0) {
        //uint32_t chunk_size = (fw_size > RX_BUFFER_SIZE) ? RX_BUFFER_SIZE : fw_size;
        //uint32_t bytes_received = 0;

        // YENİ ALIM DÖNGÜSÜ: Veriyi 4 baytlık küçük parçalar halinde al
        //uint32_t current_chunk_address = 0;
        //uint32_t remaining_bytes = chunk_size;
        //printf("recieve öncesi test mesajı\n");
        printf("SEND_CHUNK\n");

        //veri alma döngüsü
        while (fw_size > 0)
        {
            uint32_t chunk_size = (fw_size > RX_BUFFER_SIZE) ? RX_BUFFER_SIZE : fw_size;

            printf("SEND_CHUNK\n"); // Python'a gönderme izni

            // 1. Kesme tabanlı alımı başlat
            RxCpltFlag = 0; // Bayrağı sıfırla
            HAL_UART_Receive_IT(&huart1, (uint8_t*)RxBuffer, chunk_size);

            // 2. Alım tamamlanana kadar bekle
            uint32_t start_time = HAL_GetTick();
            while(RxCpltFlag == 0)
            {
                if((HAL_GetTick() - start_time) > 5000) // 5 saniye zaman aşımı
                {
                    // Kesme tabanlı alımı durdur
                    HAL_UART_AbortReceive(&huart1);
                    printf("HATA: KESME ZAMAN ASIMI!\n");
                    HAL_FLASH_Lock();
                    return -1;
                }
            }

            // 3. Flash'a yaz (Buffer artık RxBuffer olacak)
            for (uint32_t i = 0; i < chunk_size; i += 4) {
                uint32_t word;
                memcpy(&word, (uint8_t *)(RxBuffer + i), 4); // RxBuffer'dan oku
                if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, word) != HAL_OK) {
                    printf("Flash yazma hatasi!\n");
                    HAL_FLASH_Lock();
                    return -1;
                }
                address += 4;
            }

            printf("OK\n"); // Chunk onayı
            fw_size -= chunk_size;
        }

    HAL_FLASH_Lock();
    printf("SUCCESS\n");
    printf("Firmware basariyla yazildi!\n");
    return 0;
}
    return 0;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == USART1)
  {
    RxCpltFlag = 1; // Başarıyla 256 bayt aldım, bayrağı yükselt.
  }
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
