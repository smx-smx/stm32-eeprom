#include "main.h"
#ifndef USE_CHINA
#include "stdio.h"

#ifdef __GNUC__
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

void SystemClock_Config(void);
void Error_Handler(void);

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
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
void Error_Handler(void)
{
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
{}
**/

UART_HandleTypeDef UartHandle;
I2C_HandleTypeDef I2cHandle;

/**
  * @brief UART MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
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

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
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

/*
#define EEPROM_ADDR_SIZE  2
#define EEPROM_PAGE_SIZE 32
static uint8_t i2c_buf_tx[EEPROM_PAGE_SIZE];
static uint8_t i2c_buf_rx[EEPROM_PAGE_SIZE];
*/

static void I2C_Slave_Init() {
  I2cHandle.Instance             = I2C1;                           //Use i2c1 here
  I2cHandle.Mode                 = HAL_I2C_MODE_SLAVE;
  I2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;        //7-bit address mode
  I2cHandle.Init.ClockSpeed      = 1000000;                        //Clock support up to 1M
  I2cHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;        //Turn off dual address mode
  I2cHandle.Init.DutyCycle       = I2C_DUTYCYCLE_16_9;             
  I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;        
  I2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;          
  I2cHandle.Init.OwnAddress1     = 0xA0 >> 0;                           //Device address
  I2cHandle.Init.OwnAddress2     = 0;        
  
  if(HAL_I2C_Init(&I2cHandle) != HAL_OK) { Error_Handler(); }
  
  HAL_I2C_EnableListen_IT(&I2cHandle);
}

void I2C1_ER_IRQHandler(){
  HAL_I2C_ER_IRQHandler(&I2cHandle);
}
void I2C1_EV_IRQHandler()                //Event callback (receive or send)
{
    HAL_I2C_EV_IRQHandler(&I2cHandle);
}



static char DBG_MSG_ADDR = 'a';
static char DBG_MSG_RX = 'r';
static char DBG_MSG_TX = 't';
static char DBG_MSG_LISTEN = 'l';
static char DBG_MSG_END = '.';
#define DBG_MSG_UART(x) HAL_UART_Transmit(&UartHandle, (unsigned char *)&x, 1, -1)

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *I2cHandle){
  // restart
  HAL_I2C_EnableListen_IT(I2cHandle);
}

enum DeviceState {
  STATE_INITIAL,
  STATE_HAVE_ADDRESS
};

static uint8_t offset = 0;
static uint8_t ram[256];
static enum DeviceState state = STATE_INITIAL;


void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t direction, uint16_t addrMatchCode) {
  if(direction == I2C_DIRECTION_TRANSMIT) {
    // master is sending
    /*if( first ) {
      HAL_I2C_Slave_Seq_Receive_IT(hi2c, &offset, 1, I2C_NEXT_FRAME);
    } else {*/
      HAL_I2C_Slave_Seq_Receive_IT(hi2c, &ram[offset], 1, I2C_NEXT_FRAME);
    //}
  } else {
    // master is receiving
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &ram[offset], 1, I2C_NEXT_FRAME);
  }
}


void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c){
  HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &ram[offset], 1, I2C_NEXT_FRAME);
  offset = (offset + 1) % 256;
}


void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c){
  #if ENABLE_WRITE
  /*if(first) {
    first = 0;
  } else {*/
    offset = (offset + 1) % 256;
  //}
  HAL_I2C_Slave_Seq_Receive_IT(hi2c, &ram[offset], 1, I2C_NEXT_FRAME);
  #endif
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c){
  if( HAL_I2C_GetError(hi2c) == HAL_I2C_ERROR_AF) {
    offset--; // transaction terminated by master
  } else {}
}

static void UART_Init(){
  UartHandle.Instance          = USARTx;
  UartHandle.Init.BaudRate     = 115200;
  UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits     = UART_STOPBITS_1;
  UartHandle.Init.Parity       = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode         = UART_MODE_TX_RX;
  UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
  if(HAL_UART_Init(&UartHandle) != HAL_OK) {
    Error_Handler(); 
  }
}

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
  HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 0xFFFF); 
  return ch;
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  SystemClock_Config();

  UART_Init();
  I2C_Slave_Init();
  BSP_LED_Init(LED2);

  printf("ready!\r\n");

  for(int i=0; i<sizeof(ram); i++){
    ram[i] = i % 256;
  }

  while (1)
  {
    #if TEST_DEMO
    BSP_LED_Toggle(LED2);
    HAL_Delay(500);
    printf("UART Printf Example: retarget the C library printf function to the UART\r\n");
    #endif
  }
}
#endif