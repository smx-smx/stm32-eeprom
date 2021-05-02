#include "main.h"
#include "stdio.h"

UART_HandleTypeDef UartHandle;
I2C_HandleTypeDef I2cHandle;

void I2C_Slave_Init(I2C_HandleTypeDef *i2ch);
void UART_Init(UART_HandleTypeDef *uart);

typedef struct logEntry {
  uint8_t completed : 1;
  uint8_t is_write : 1;
  uint8_t count : 7; // (2^7)-1 max size, more than enough
  uint16_t address
} logEntry_t;

#define LOG_MAXSIZE 32
static logEntry_t logBuffer[LOG_MAXSIZE];

static int logIndex = 0;
static logEntry_t *lastLog = &logBuffer[0];

#define LOGENTRY_NEXT_INDEX() ((logIndex + 1) & (LOG_MAXSIZE - 1))
#define LOGENTRY_NEXT() ( &logBuffer[LOGENTRY_NEXT_INDEX()] )

void I2C1_ER_IRQHandler(){
  HAL_I2C_ER_IRQHandler(&I2cHandle);
}
/** Event callback (receive or send) **/
void I2C1_EV_IRQHandler(){
    HAL_I2C_EV_IRQHandler(&I2cHandle);
}

#ifdef __GNUC__
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
  HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 0xFFFF); 
  return ch;
}

static char DBG_MSG_ADDR = 'a';
static char DBG_MSG_DATA = 'd';
static char DBG_MSG_RX = 'r';
static char DBG_MSG_TX = 't';
static char DBG_MSG_LISTEN = 'l';
static char DBG_MSG_END = '.';
#define DBG_MSG_UART(x) HAL_UART_Transmit(&UartHandle, (unsigned char *)&x, 1, -1)

enum DeviceState {
  // initial state
  STATE_INITIAL = 0,
  // receiving the first byte of word addr
  STATE_RECEIVING_ADDRESS,
  // after the 2nd byte of word addr
  STATE_HAVE_ADDRESS
};

/** eeprom data **/
#define EEPROM_SIZE (32 * 1024)
#define EEPROM_OFFSET(x) ((x) & (sizeof(ram) - 1))

static uint8_t ram[EEPROM_SIZE];
static uint16_t word_addr = 0;
static enum DeviceState state = STATE_INITIAL;
static uint8_t word_addr_byte = 0;

// end of I2C transaction (STOP)
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *I2cHandle){
  // restart
  if(lastLog->count > 0){
    lastLog->completed = 1;
    lastLog = LOGENTRY_NEXT();
  }

  state = STATE_INITIAL;
  HAL_I2C_EnableListen_IT(I2cHandle);
}


void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t direction, uint16_t addrMatchCode) {
  if(direction == I2C_DIRECTION_TRANSMIT) {
    // master is sending, start first receive
    // if the master is writing, it always writes the address first
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, &word_addr_byte, 1, I2C_NEXT_FRAME);
  } else {
    // master is receiving, start first transmit
    word_addr = EEPROM_OFFSET(word_addr);
    lastLog->address = word_addr;
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &ram[word_addr], 1, I2C_NEXT_FRAME);
  }
}


void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c){
  // we just sent something to the master

  // offer the next eeprom byte (the master will NACK if it doesn't want it)
  word_addr = EEPROM_OFFSET(word_addr + 1);
  HAL_I2C_Slave_Seq_Transmit_IT(hi2c, &ram[word_addr], 1, I2C_NEXT_FRAME);

  // log data writes
  lastLog->count++;
}


void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c){
  // we just received something from the master
  if(state == STATE_INITIAL){ // received byte0 of addr
    // [DE] AD
    // overwrite previous word_addr
    word_addr = word_addr_byte << 8;
    state = STATE_RECEIVING_ADDRESS;

    // start to receive next addr byte
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, &word_addr_byte, 1, I2C_NEXT_FRAME);
  } else {
    if(state == STATE_RECEIVING_ADDRESS){ // received byte1 of addr
      // DE [AD]
      word_addr |= word_addr_byte;
      state = STATE_HAVE_ADDRESS;

      lastLog->address = word_addr;
    }
    // handle next (or first) data RX
    lastLog->count++;
    lastLog->is_write = 1;
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, &ram[word_addr], 1, I2C_NEXT_FRAME);
    word_addr = EEPROM_OFFSET(word_addr + 1);
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c){
  
  if( HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_AF) {
    return;
  }
  // if master NACK'd, the last write didn't go through
  word_addr--; // transaction terminated by master
  if(lastLog->count > 0){
    lastLog->count--;
    /**
     * in the event of a failed RX
     * if we have 0 bytes the transmission ended without
     * any data being written, convert this to a read
     **/
    if(lastLog->count == 0){
      lastLog->is_write = 0;
    }
  }
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

  UART_Init(&UartHandle);
  I2C_Slave_Init(&I2cHandle);
  
  BSP_LED_Init(LED2);
  BSP_LED_Off(LED2);

  printf("ready!\r\n");

  for(int i=0; i<sizeof(ram); i++){
    ram[i] = i % 256;
  }

  while (1)
  {
    //HAL_Delay(400);

    for(int i=0; i<LOG_MAXSIZE; i++){
      logEntry_t *ent =  &logBuffer[i];
      if(ent->count == 0 || !ent->completed){
        continue;
      }
      printf("%c 0x%x %u\r\n", ((ent->is_write) ? 'w' : 'r'), ent->address, ent->count);
      ent->count = 0;
      ent->completed = 0;
    }

    /*if(word_addr != 0){
      printf("%u,%u\r\n", word_addr, word_addr_byte);
    }*/
    #if TEST_DEMO
    BSP_LED_Toggle(LED2);
    HAL_Delay(500);
    printf("UART Printf Example: retarget the C library printf function to the UART\r\n");
    #endif
  }
}