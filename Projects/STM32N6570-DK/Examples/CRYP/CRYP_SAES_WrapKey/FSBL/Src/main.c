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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BYTES_AES_128 16
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

RNG_HandleTypeDef hrng;

CRYP_HandleTypeDef hcryp;

UART_HandleTypeDef huart1;

XSPI_HandleTypeDef hxspi2;

/* USER CODE BEGIN PV */
/* The size of the plaintext is in words */
#define PLAINTEXT_SIZE    8

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_RNG_Init(void);
static void MX_SAES_CRYP_Init(void);

static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_XSPI2_Init(void);
/* USER CODE BEGIN PFP */
static void XSPI_WriteEnable(XSPI_HandleTypeDef *hxspi);
static void XSPI_AutoPollingMemReady(XSPI_HandleTypeDef *hxspi);
static void XSPI_NOR_OctalDTRModeCfg(XSPI_HandleTypeDef *hxspi);
static void init_XSPI_Cmd(XSPI_RegularCmdTypeDef *sCommand);
/* USER CODE END PFP */

void erase_flash(uint32_t offset, XSPI_RegularCmdTypeDef *sCommand);
void write_flash(uint32_t offset, uint8_t *input, uint32_t bufferSize, XSPI_RegularCmdTypeDef *sCommand);
void read_flash(uint32_t offset, uint8_t *output, uint32_t bufferSize, XSPI_RegularCmdTypeDef *sCommand);

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
  /* STM32N6xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  uint32_t Plaintext[8] = { 0x6BC1BEE2 ,0x2E409F96 ,0xE93D7E11 ,0x7393172A ,
                           0xAE2D8A57 ,0x1E03AC9C ,0x9EB76FAC ,0x45AF8E51};
  uint32_t CiphertextAESECB128[8] = {0xb2239159, 0xa6d024ed, 0x5f265b12, 0xcae73959,
                                      0xb522d004, 0x48878782, 0x7de81ea3, 0x40c0afc};

  uint32_t EncryptedText[PLAINTEXT_SIZE];
  uint32_t DecryptedText[PLAINTEXT_SIZE];

  uint32_t Encryptedkey[4];
  uint32_t Encryptedkey_FLASH[4] = {0};

  XSPI_RegularCmdTypeDef sCommand = {0};
  init_XSPI_Cmd(&sCommand);
  /* USER CODE END 1 */

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* Configure LED2, LED1 */
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED1);

  /* Turn LED2, LED1 off */
  BSP_LED_Off(LED2);
  BSP_LED_Off(LED1);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_RNG_Init();
  MX_SAES_CRYP_Init();

  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_XSPI2_Init();

  XSPI_NOR_OctalDTRModeCfg(&hxspi2);

  // Key reception
  uint32_t offset = 0x07FFF000; // 128 Mo of flash
  read_flash(offset, (uint8_t *)Encryptedkey_FLASH, BYTES_AES_128, &sCommand);

  if (Encryptedkey_FLASH[0] == 0xFFFFFFFF && Encryptedkey_FLASH[1] == 0xFFFFFFFF && Encryptedkey_FLASH[2] == 0xFFFFFFFF && Encryptedkey_FLASH[3] == 0xFFFFFFFF) {
    uint32_t AESKey128_RAM[8] = {0};
  
    char *askKey = "Send your AES-258 key :\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)askKey, strlen(askKey), 0xFFFF);
    if (HAL_UART_Receive(&huart1, (uint8_t*)AESKey128_RAM, BYTES_AES_128, HAL_MAX_DELAY) == HAL_OK) {
      /* USER CODE BEGIN 2 */
      /* User key AESKey128 encryption*/
      if (HAL_CRYPEx_WrapKey(&hcryp, AESKey128_RAM,  Encryptedkey, TIMEOUT_VALUE) != HAL_OK)
      {
        /* Processing Error */
        Error_Handler();
      }

      erase_flash(offset, &sCommand); // Theoretically not need
      write_flash(offset, (uint8_t *)Encryptedkey, BYTES_AES_128, &sCommand);
      read_flash(offset, (uint8_t *)Encryptedkey_FLASH, BYTES_AES_128, &sCommand);

    /* we could not compare results, key is not known and unique for each device */
    } else {
      Error_Handler();
    }
  }

  /* User key AESKey128 decryption*/
  if (HAL_CRYPEx_UnwrapKey(&hcryp, Encryptedkey_FLASH, TIMEOUT_VALUE) != HAL_OK)
  {
    /* Processing Error */
    Error_Handler();
  }
  /* Now key AESKey128 is loaded in SAES_KEYRx registers, immediately usable by
  application for any SAES operation */

  /* Secure AES ECB Encryption */
  if (HAL_CRYP_Encrypt(&hcryp, Plaintext, 8, EncryptedText, TIMEOUT_VALUE) != HAL_OK)
  {
    /* Processing Error */
    Error_Handler();
  }
  /*Compare with expected result, because we know the original encrypted key*/
  if(memcmp(EncryptedText, CiphertextAESECB128, 32) != 0)
  {
    /* Processing Error */
    Error_Handler();
  }

  /*Secure AES ECB Decryption */
  if (HAL_CRYP_Decrypt(&hcryp, EncryptedText, 8, DecryptedText, TIMEOUT_VALUE) != HAL_OK)
  {
    /* Processing Error */
    Error_Handler();
  }
  /*Compare decryption result with Plaintext*/
  if(memcmp(DecryptedText, Plaintext, 32) != 0)
  {
    /* Processing Error */
    Error_Handler();
  }

  /* Turn LED1 on */
  BSP_LED_On(LED1);
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

void erase_flash(uint32_t offset, XSPI_RegularCmdTypeDef *sCommand) {
  /* Enable write operations ------------------------------------------ */
  XSPI_WriteEnable(&hxspi2);

  /* Erasing Sequence ------------------------------------------------- */
  sCommand->OperationType      = HAL_XSPI_OPTYPE_COMMON_CFG;
  sCommand->Instruction        = OCTAL_SECTOR_ERASE_CMD;
  sCommand->Address            = offset;
  sCommand->DataMode           = HAL_XSPI_DATA_NONE;
  sCommand->DQSMode            = HAL_XSPI_DQS_ENABLE;

  if (HAL_XSPI_Command(&hxspi2, sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
}

void write_flash(uint32_t offset, uint8_t *input, uint32_t bufferSize, XSPI_RegularCmdTypeDef *sCommand) {
  /* Configure automatic polling mode to wait for end of erase -------- */
  XSPI_AutoPollingMemReady(&hxspi2);

  /* Enable write operations ------------------------------------------ */
  XSPI_WriteEnable(&hxspi2);

  /* Writing Sequence ------------------------------------------------- */
  sCommand->Instruction = OCTAL_PAGE_PROG_CMD;
  sCommand->DataMode    = HAL_XSPI_DATA_8_LINES;
  sCommand->DataLength  = bufferSize;
  sCommand->Address     = offset;

  if (HAL_XSPI_Command(&hxspi2, sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_XSPI_Transmit(&hxspi2, input, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
}

void read_flash(uint32_t offset, uint8_t *output, uint32_t bufferSize, XSPI_RegularCmdTypeDef *sCommand) {
  /* Configure automatic polling mode to wait for end of erase -------- */
  XSPI_AutoPollingMemReady(&hxspi2);

  /* Cached data is not up to date due to indirect write.
      Force new read by invalidating the corresponding cache lines */
  SCB_InvalidateDCache_by_Addr((void *)(XSPI2_BASE + offset), bufferSize);

  /* Memory-mapped mode configuration ------------------------------- */
  sCommand->OperationType = HAL_XSPI_OPTYPE_WRITE_CFG;
  sCommand->Instruction   = OCTAL_PAGE_PROG_CMD;
  sCommand->DataMode      = HAL_XSPI_DATA_8_LINES;
  sCommand->DataLength    = 1;
  sCommand->DQSMode       = HAL_XSPI_DQS_ENABLE;

  if (HAL_XSPI_Command(&hxspi2, sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  sCommand->OperationType = HAL_XSPI_OPTYPE_READ_CFG;
  sCommand->Instruction   = OCTAL_IO_DTR_READ_CMD;
  sCommand->DummyCycles   = DUMMY_CLOCK_CYCLES_READ;
  sCommand->DQSMode       = HAL_XSPI_DQS_ENABLE;

  if (HAL_XSPI_Command(&hxspi2, sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  XSPI_MemoryMappedTypeDef sMemMappedCfg = {0};

  sMemMappedCfg.NoPrefetchAXI       = HAL_XSPI_AXI_PREFETCH_ENABLE;
  sMemMappedCfg.NoPrefetchData      = HAL_XSPI_AUTOMATIC_PREFETCH_ENABLE;
  sMemMappedCfg.TimeOutActivation   = HAL_XSPI_TIMEOUT_COUNTER_DISABLE;
  sMemMappedCfg.TimeoutPeriodClock  = 0x40;

  if (HAL_XSPI_MemoryMapped(&hxspi2, &sMemMappedCfg) != HAL_OK)
  {
    Error_Handler();
  }

  /* Reading Sequence ------------------------------------------------- */
  __IO uint8_t *mem_addr = (uint8_t *)(XSPI2_BASE + offset);
  for (uint16_t index = 0; index < bufferSize ; index++)
  {
    output[index] = *mem_addr;
    mem_addr++;
  }

  /* Abort xSPI driver to stop the memory-mapped mode ------------ */
  if (HAL_XSPI_Abort(&hxspi2) != HAL_OK)
  {
    Error_Handler();
  }
}
/* USER CODE BEGIN CLK 1 */
/* USER CODE END CLK 1 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the System Power Supply
  */
  if (HAL_PWREx_ConfigSupply(PWR_EXTERNAL_SOURCE_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable HSI */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Get current CPU/System buses clocks configuration and if necessary switch
 to intermediate HSI clock to ensure target clock can be set
  */
  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
  if ((RCC_ClkInitStruct.CPUCLKSource == RCC_CPUCLKSOURCE_IC1) ||
     (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_IC2_IC6_IC11))
  {
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK);
    RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
    {
      /* Initialization Error */
      Error_Handler();
    }
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL1.PLLM = 4;
  RCC_OscInitStruct.PLL1.PLLN = 75;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL1.PLLP1 = 1;
  RCC_OscInitStruct.PLL1.PLLP2 = 1;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_PCLK5
                              |RCC_CLOCKTYPE_PCLK4;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 2;
  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider = 3;
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 4;
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 3;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief SAES Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAES_CRYP_Init(void)
{

  /* USER CODE BEGIN SAES_Init 0 */

  /* USER CODE END SAES_Init 0 */

  /* USER CODE BEGIN SAES_Init 1 */

  /* USER CODE END SAES_Init 1 */
  hcryp.Instance = SAES;
  hcryp.Init.DataType = CRYP_DATATYPE_32B;
  hcryp.Init.KeySize = CRYP_KEYSIZE_128B;
  hcryp.Init.Algorithm = CRYP_AES_ECB;
  hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_WORD;
  hcryp.Init.HeaderWidthUnit = CRYP_HEADERWIDTHUNIT_WORD;
  hcryp.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ALWAYS;
  hcryp.Init.KeyMode = CRYP_KEYMODE_WRAPPED;
  hcryp.Init.KeySelect = CRYP_KEYSEL_HW;
  hcryp.Init.KeyProtection = CRYP_KEYPROT_DISABLE;
  if (HAL_CRYP_Init(&hcryp) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAES_Init 2 */

  /* USER CODE END SAES_Init 2 */

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
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
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
static void MX_GPIO_Init(void) // Description : active le Port E (pour PE6 --> USART1_RX)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/**
  * @brief XSPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_XSPI2_Init(void)
{

  /* USER CODE BEGIN XSPI2_Init 0 */

  /* USER CODE END XSPI2_Init 0 */

  XSPIM_CfgTypeDef sXspiManagerCfg = {0};

  /* USER CODE BEGIN XSPI2_Init 1 */

  /* USER CODE END XSPI2_Init 1 */
  /* XSPI2 parameter configuration*/
  hxspi2.Instance = XSPI2;
  hxspi2.Init.FifoThresholdByte = 4;
  hxspi2.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  hxspi2.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
  hxspi2.Init.MemorySize = HAL_XSPI_SIZE_1GB;
  hxspi2.Init.ChipSelectHighTimeCycle = 1;
  hxspi2.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
  hxspi2.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  hxspi2.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
  hxspi2.Init.ClockPrescaler = 1;
  hxspi2.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  hxspi2.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
  hxspi2.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
  hxspi2.Init.MaxTran = 0;
  hxspi2.Init.Refresh = 0;
  hxspi2.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
  if (HAL_XSPI_Init(&hxspi2) != HAL_OK)
  {
    Error_Handler();
  }
  sXspiManagerCfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  sXspiManagerCfg.IOPort = HAL_XSPIM_IOPORT_2;
  sXspiManagerCfg.Req2AckTime = 1;
  if (HAL_XSPIM_Config(&hxspi2, &sXspiManagerCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN XSPI2_Init 2 */

  /* USER CODE END XSPI2_Init 2 */

}

/* USER CODE BEGIN 4 */

/**
* @brief  This function send a Write Enable and wait it is effective.
* @param  hxspi: XSPI handle
* @retval None
*/
static void XSPI_WriteEnable(XSPI_HandleTypeDef *hxspi)
{
  XSPI_RegularCmdTypeDef  sCommand = {0};
  XSPI_AutoPollingTypeDef sConfig  = {0};

  /* Enable write operations ------------------------------------------ */
  sCommand.OperationType      = HAL_XSPI_OPTYPE_COMMON_CFG;
  sCommand.Instruction        = OCTAL_WRITE_ENABLE_CMD;
  sCommand.InstructionMode    = HAL_XSPI_INSTRUCTION_8_LINES;
  sCommand.InstructionWidth   = HAL_XSPI_INSTRUCTION_16_BITS;
  sCommand.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
  sCommand.AddressMode        = HAL_XSPI_ADDRESS_NONE;
  sCommand.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
  sCommand.DataMode           = HAL_XSPI_DATA_NONE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_XSPI_DQS_DISABLE;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure automatic polling mode to wait for write enabling ---- */
  sCommand.Instruction        = OCTAL_READ_STATUS_REG_CMD;
  sCommand.Address            = 0x0;
  sCommand.AddressMode        = HAL_XSPI_ADDRESS_8_LINES;
  sCommand.AddressWidth       = HAL_XSPI_ADDRESS_32_BITS;
  sCommand.AddressDTRMode     = HAL_XSPI_ADDRESS_DTR_ENABLE;
  sCommand.DataMode           = HAL_XSPI_DATA_8_LINES;
  sCommand.DataDTRMode        = HAL_XSPI_DATA_DTR_ENABLE;
  sCommand.DataLength         = 2;
  sCommand.DummyCycles        = DUMMY_CLOCK_CYCLES_READ_OCTAL;
  sCommand.DQSMode            = HAL_XSPI_DQS_ENABLE;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.MatchMode           = HAL_XSPI_MATCH_MODE_AND;
  sConfig.AutomaticStop       = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
  sConfig.IntervalTime        = AUTO_POLLING_INTERVAL;
  sConfig.MatchMask           = WRITE_ENABLE_MASK_VALUE;
  sConfig.MatchValue          = WRITE_ENABLE_MATCH_VALUE;

  if (HAL_XSPI_AutoPolling(hxspi, &sConfig, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
* @brief  This function read the SR of the memory and wait the EOP.
* @param  hxspi: XSPI handle
* @retval None
*/
static void XSPI_AutoPollingMemReady(XSPI_HandleTypeDef *hxspi)
{
  XSPI_RegularCmdTypeDef  sCommand = {0};
  XSPI_AutoPollingTypeDef sConfig  = {0};

  /* Configure automatic polling mode to wait for memory ready ------ */
  sCommand.OperationType      = HAL_XSPI_OPTYPE_COMMON_CFG;
  sCommand.Instruction        = OCTAL_READ_STATUS_REG_CMD;
  sCommand.InstructionMode    = HAL_XSPI_INSTRUCTION_8_LINES;
  sCommand.InstructionWidth   = HAL_XSPI_INSTRUCTION_16_BITS;
  sCommand.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
  sCommand.Address            = 0x0;
  sCommand.AddressMode        = HAL_XSPI_ADDRESS_8_LINES;
  sCommand.AddressWidth       = HAL_XSPI_ADDRESS_32_BITS;
  sCommand.AddressDTRMode     = HAL_XSPI_ADDRESS_DTR_ENABLE;
  sCommand.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
  sCommand.DataMode           = HAL_XSPI_DATA_8_LINES;
  sCommand.DataDTRMode        = HAL_XSPI_DATA_DTR_ENABLE;
  sCommand.DataLength         = 2;
  sCommand.DummyCycles        = DUMMY_CLOCK_CYCLES_READ_OCTAL;
  sCommand.DQSMode            = HAL_XSPI_DQS_ENABLE;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.MatchMode           = HAL_XSPI_MATCH_MODE_AND;
  sConfig.AutomaticStop       = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
  sConfig.IntervalTime        = AUTO_POLLING_INTERVAL;
  sConfig.MatchMask           = MEMORY_READY_MASK_VALUE;
  sConfig.MatchValue          = MEMORY_READY_MATCH_VALUE;

  if (HAL_XSPI_AutoPolling(hxspi, &sConfig, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
* @brief  This function configure the memory in Octal DTR mode.
* @param  hxspi: XSPI handle
* @retval None
*/
static void XSPI_NOR_OctalDTRModeCfg(XSPI_HandleTypeDef *hxspi)
{
  uint8_t reg = 0;
  XSPI_RegularCmdTypeDef  sCommand = {0};
  XSPI_AutoPollingTypeDef sConfig  = {0};

  sCommand.OperationType      = HAL_XSPI_OPTYPE_COMMON_CFG;
  sCommand.InstructionMode    = HAL_XSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionWidth    = HAL_XSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
  sCommand.AddressDTRMode     = HAL_XSPI_ADDRESS_DTR_DISABLE;
  sCommand.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
  sCommand.DataDTRMode        = HAL_XSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_XSPI_DQS_DISABLE;
  sConfig.MatchMode           = HAL_XSPI_MATCH_MODE_AND;
  sConfig.AutomaticStop       = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
  sConfig.IntervalTime        = 0x10;


  /* Enable write operations */
  sCommand.Instruction = WRITE_ENABLE_CMD;
  sCommand.DataMode    = HAL_XSPI_DATA_NONE;
  sCommand.AddressMode = HAL_XSPI_ADDRESS_NONE;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  /* Reconfigure XSPI to automatic polling mode to wait for write enabling */
  sConfig.MatchMask           = 0x02;
  sConfig.MatchValue          = 0x02;

  sCommand.Instruction        = READ_STATUS_REG_CMD;
  sCommand.DataMode           = HAL_XSPI_DATA_1_LINE;
  sCommand.DataLength         = 1;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_XSPI_AutoPolling(hxspi, &sConfig, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  /* Write Configuration register 2 (with Octal I/O SPI protocol) */
  sCommand.Instruction  = WRITE_CFG_REG_2_CMD;
  sCommand.AddressMode  = HAL_XSPI_ADDRESS_1_LINE;
  sCommand.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;

  sCommand.Address = 0;
  reg = 0x2;


  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_XSPI_Transmit(hxspi, &reg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  sCommand.Instruction    = READ_STATUS_REG_CMD;
  sCommand.DataMode       = HAL_XSPI_DATA_1_LINE;
  sCommand.DataLength     = 1;

  if (HAL_XSPI_Command(hxspi, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_XSPI_AutoPolling(hxspi, &sConfig, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

}

static void init_XSPI_Cmd(XSPI_RegularCmdTypeDef *sCommand) {
  sCommand->InstructionMode    = HAL_XSPI_INSTRUCTION_8_LINES;
  sCommand->InstructionWidth   = HAL_XSPI_INSTRUCTION_16_BITS;
  sCommand->InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
  sCommand->AddressMode        = HAL_XSPI_ADDRESS_8_LINES;
  sCommand->AddressWidth       = HAL_XSPI_ADDRESS_32_BITS;
  sCommand->AddressDTRMode     = HAL_XSPI_ADDRESS_DTR_ENABLE;
  sCommand->AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
  sCommand->DataDTRMode        = HAL_XSPI_DATA_DTR_ENABLE;
  sCommand->DummyCycles        = 0;
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
  /* Turn LED2 on */
  BSP_LED_On(LED2);
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
