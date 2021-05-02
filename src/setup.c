
#include "main.h"

void SystemClock_Config(void);
void Error_Handler(void);


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */


/**
  * @brief UART MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
	GPIO_InitTypeDef  GPIO_InitStruct;

	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO TX/RX clock */
	USARTx_TX_GPIO_CLK_ENABLE();
	USARTx_RX_GPIO_CLK_ENABLE();
	/* Enable USART2 clock */
	USARTx_CLK_ENABLE();

	/*##-2- Configure peripheral GPIO ##########################################*/
	/* UART TX GPIO pin configuration  */
	GPIO_InitStruct.Pin       = USARTx_TX_PIN;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
	GPIO_InitStruct.Alternate = USARTx_TX_AF;
	HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);

	/* UART RX GPIO pin configuration  */
	GPIO_InitStruct.Pin = USARTx_RX_PIN;
	GPIO_InitStruct.Alternate = USARTx_RX_AF;
	HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStruct);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle) {
    GPIO_InitTypeDef GPIO_InitStruct; 
     __HAL_RCC_GPIOB_CLK_ENABLE() ;
    /**I2C1 GPIO Configuration    
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA 
    */
    // init pin 8 on bank B
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;            //Open drain output (i2c requires this mode)
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

void I2C_Slave_Init(I2C_HandleTypeDef *i2ch) {
  i2ch->Instance             = I2C1;                           //Use i2c1 here
  i2ch->Mode                 = HAL_I2C_MODE_SLAVE;
  i2ch->Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;        //7-bit address mode
  i2ch->Init.ClockSpeed      = 1000000;                        //Clock support up to 1M
  i2ch->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;        //Turn off dual address mode
  i2ch->Init.DutyCycle       = I2C_DUTYCYCLE_16_9;             
  i2ch->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;        
  i2ch->Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;          
  i2ch->Init.OwnAddress1     = 0xA0 >> 0;                           //Device address
  i2ch->Init.OwnAddress2     = 0;        
  
  if(HAL_I2C_Init(i2ch) != HAL_OK) { Error_Handler(); }
  
  // start first transaction
  HAL_I2C_EnableListen_IT(i2ch);
}

void UART_Init(UART_HandleTypeDef *uart){
  uart->Instance          = USARTx;
  uart->Init.BaudRate     = 115200;
  uart->Init.WordLength   = UART_WORDLENGTH_8B;
  uart->Init.StopBits     = UART_STOPBITS_1;
  uart->Init.Parity       = UART_PARITY_NONE;
  uart->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  uart->Init.Mode         = UART_MODE_TX_RX;
  uart->Init.OverSampling = UART_OVERSAMPLING_16;
  if(HAL_UART_Init(uart) != HAL_OK) {
    Error_Handler(); 
  }
}