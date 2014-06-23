/**************************************************************************************************
  Filename:       _hal_uart_dma.c
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "hal_assert.h"
#include "hal_board.h"
#include "hal_defs.h"
#include "hal_dma.h"
#include "hal_mcu.h"
#include "hal_types.h"
#include "hal_uart.h"
#include "osal.h"

/*********************************************************************
 * MACROS
 */

#define HAL_UART_DMA_NEW_RX_BYTE(IDX)  ((uint8)DMA_PAD == HI_UINT16(dmaCfg.rxBuf[(IDX)]))
#define HAL_UART_DMA_GET_RX_BYTE(IDX)  (*(volatile uint8 *)(dmaCfg.rxBuf+(IDX)))
#define HAL_UART_DMA_CLR_RX_BYTE(IDX)  (dmaCfg.rxBuf[(IDX)] = BUILD_UINT16(0, (DMA_PAD ^ 0xFF)))

/*********************************************************************
 * CONSTANTS
 */

// UxCSR - USART Control and Status Register.
#define CSR_MODE                   0x80
#define CSR_RE                     0x40
#define CSR_SLAVE                  0x20
#define CSR_FE                     0x10
#define CSR_ERR                    0x08
#define CSR_RX_BYTE                0x04
#define CSR_TX_BYTE                0x02
#define CSR_ACTIVE                 0x01

// UxUCR - USART UART Control Register.
#define UCR_FLUSH                  0x80
#define UCR_FLOW                   0x40
#define UCR_D9                     0x20
#define UCR_BIT9                   0x10
#define UCR_PARITY                 0x08
#define UCR_SPB                    0x04
#define UCR_STOP                   0x02
#define UCR_START                  0x01

#define UTX0IE                     0x04
#define UTX1IE                     0x08

#define P2DIR_PRIPO                0xC0

// Incompatible redefinitions result with more than one UART driver sub-module.
#undef PxOUT
#undef PxDIR
#undef PxSEL
#undef UxCSR
#undef UxUCR
#undef UxDBUF
#undef UxBAUD
#undef UxGCR
#undef URXxIE
#undef URXxIF
#undef UTXxIE
#undef UTXxIF
#undef HAL_UART_PERCFG_BIT
#undef HAL_UART_Px_RTS
#undef HAL_UART_Px_CTS
#undef HAL_UART_Px_RX_TX
#undef HAL_UART_Px_RX
#undef PxIFG
#undef PxIF
#undef PxIEN
#undef PICTL_BIT
#undef IENx
#undef IEN_BIT
#if (HAL_UART_DMA == 1)
#define PxOUT                      P0
#define PxIN                       P0
#define PxDIR                      P0DIR
#define PxSEL                      P0SEL
#define UxCSR                      U0CSR
#define UxUCR                      U0UCR
#define UxDBUF                     U0DBUF
#define UxBAUD                     U0BAUD
#define UxGCR                      U0GCR
#define URXxIE                     URX0IE
#define URXxIF                     URX0IF
#define UTXxIE                     UTX0IE
#define UTXxIF                     UTX0IF

#define PxIFG                      P0IFG
#define PxIF                       P0IF
#define PxIEN                      P0IEN
#define PICTL_BIT                  0x01
#define IENx                       IEN1
#define IEN_BIT                    0x20

#else
#define PxOUT                      P1
#define PxIN                       P1
#define PxDIR                      P1DIR
#define PxSEL                      P1SEL
#define UxCSR                      U1CSR
#define UxUCR                      U1UCR
#define UxDBUF                     U1DBUF
#define UxBAUD                     U1BAUD
#define UxGCR                      U1GCR
#define URXxIE                     URX1IE
#define URXxIF                     URX1IF
#define UTXxIE                     UTX1IE
#define UTXxIF                     UTX1IF

#define PxIFG                      P1IFG
#define PxIF                       P1IF
#define PxIEN                      P1IEN
#define PICTL_BIT                  0x04
#define IENx                       IEN2
#define IEN_BIT                    0x10
#endif

#if (HAL_UART_DMA == 1)
#define HAL_UART_PERCFG_BIT        0x01         // USART0 on P0, Alt-1; so clear this bit.
#define HAL_UART_Px_RX_TX          0x0C         // Peripheral I/O Select for Rx/Tx.
#define HAL_UART_Px_RX             0x04         // Peripheral I/O Select for Rx.
#define HAL_UART_Px_RTS            0x20         // Peripheral I/O Select for RTS.
#define HAL_UART_Px_CTS            0x10         // Peripheral I/O Select for CTS.
#else
#define HAL_UART_PERCFG_BIT        0x02         // USART1 on P1, Alt-2; so set this bit.
#define HAL_UART_Px_RTS            0x20         // Peripheral I/O Select for RTS.
#define HAL_UART_Px_CTS            0x10         // Peripheral I/O Select for CTS.
#define HAL_UART_Px_RX_TX          0xC0         // Peripheral I/O Select for Rx/Tx.
#define HAL_UART_Px_RX             0x80         // Peripheral I/O Select for Rx.
#endif

// The timeout tick is at 32-kHz, so multiply msecs by 33.
#define HAL_UART_MSECS_TO_TICKS    33

#if !defined HAL_UART_DMA_RX_MAX
#define HAL_UART_DMA_RX_MAX        128
#endif
#if !defined HAL_UART_DMA_TX_MAX
#define HAL_UART_DMA_TX_MAX        HAL_UART_DMA_RX_MAX
#endif
#if !defined HAL_UART_DMA_HIGH
#define HAL_UART_DMA_HIGH         (HAL_UART_DMA_RX_MAX / 2 - 16)
#endif
#if !defined HAL_UART_DMA_IDLE
#define HAL_UART_DMA_IDLE         (0 * HAL_UART_MSECS_TO_TICKS)
#endif
#if !defined HAL_UART_DMA_FULL
#define HAL_UART_DMA_FULL         (HAL_UART_DMA_RX_MAX - 16)
#endif

#if defined HAL_BOARD_CC2430EB || defined HAL_BOARD_CC2430DB || defined HAL_BOARD_CC2430BB
#define HAL_DMA_U0DBUF             0xDFC1
#define HAL_DMA_U1DBUF             0xDFF9
#else  /* CC2530 */
#define HAL_DMA_U0DBUF             0x70C1
#define HAL_DMA_U1DBUF             0x70F9
#endif

#if (HAL_UART_DMA == 1)
#define DMATRIG_RX                 HAL_DMA_TRIG_URX0
#define DMATRIG_TX                 HAL_DMA_TRIG_UTX0
#define DMA_UDBUF                  HAL_DMA_U0DBUF
#define DMA_PAD                    U0BAUD
#else
#define DMATRIG_RX                 HAL_DMA_TRIG_URX1
#define DMATRIG_TX                 HAL_DMA_TRIG_UTX1
#define DMA_UDBUF                  HAL_DMA_U1DBUF
#define DMA_PAD                    U1BAUD
#endif

// ST-ticks for 1 byte @ 38.4-kB plus 1 tick added for when the txTick is forced from zero to 0xFF.
#define HAL_UART_TX_TICK_MIN       11

/*********************************************************************
 * TYPEDEFS
 */

#if HAL_UART_DMA_RX_MAX <= 256
typedef uint8 rxIdx_t;
#else
typedef uint16 rxIdx_t;
#endif

#if HAL_UART_DMA_TX_MAX <= 256
typedef uint8 txIdx_t;
#else
typedef uint16 txIdx_t;
#endif

typedef struct
{
  uint16 rxBuf[HAL_UART_DMA_RX_MAX];
  rxIdx_t rxHead;
  rxIdx_t rxTail;
  uint8 rxTick;
  uint8 rxShdw;

  uint8 txBuf[2][HAL_UART_DMA_TX_MAX];
  txIdx_t txIdx[2];
  volatile uint8 txSel;
  uint8 txMT;
  uint8 txTick;

  halUARTCBack_t uartCB;
} uartDMACfg_t;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

void HalUARTIsrDMA(void);

/*********************************************************************
 * LOCAL VARIABLES
 */

static uartDMACfg_t dmaCfg;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static rxIdx_t findRxTail(void);

// Invoked by functions in hal_uart.c when this file is included.
static void HalUARTInitDMA(void);
static void HalUARTOpenDMA(halUARTCfg_t *config);
static uint16 HalUARTReadDMA(uint8 *buf, uint16 len);
static uint16 HalUARTWriteDMA(uint8 *buf, uint16 len);
static void HalUARTPollDMA(void);
static uint16 HalUARTRxAvailDMA(void);
static uint8 HalUARTBusyDMA(void);
static void HalUARTSuspendDMA(void);
static void HalUARTResumeDMA(void);
static void HalUARTPollTxTrigger(void);
static void HalUARTArmTxDMA(void);

/*****************************************************************************
 * @fn      findRxTail
 *
 * @brief   Find the rxBuf index where the DMA RX engine is working.
 *
 * @param   None.
 *
 * @return  Index of tail of rxBuf.
 *****************************************************************************/
static rxIdx_t findRxTail(void)
{
  rxIdx_t idx = dmaCfg.rxHead;

  do
  {
    if (!HAL_UART_DMA_NEW_RX_BYTE(idx))
    {
      break;
    }

#if HAL_UART_DMA_RX_MAX == 256
    idx++;
#else
    if (++idx >= HAL_UART_DMA_RX_MAX)
    {
      idx = 0;
    }
#endif
  } while (idx != dmaCfg.rxHead);

  return idx;
}

/******************************************************************************
 * @fn      HalUARTInitDMA
 *
 * @brief   Initialize the UART
 *
 * @param   none
 *
 * @return  none
 *****************************************************************************/
static void HalUARTInitDMA(void)
{
  halDMADesc_t *ch;

  P2DIR &= ~P2DIR_PRIPO;
  P2DIR |= HAL_UART_PRIPO;

#if (HAL_UART_DMA == 1)
  PERCFG &= ~HAL_UART_PERCFG_BIT;    // Set UART0 I/O to Alt. 1 location on P0.
#else
  PERCFG |= HAL_UART_PERCFG_BIT;     // Set UART1 I/O to Alt. 2 location on P1.
#endif
  PxSEL  |= HAL_UART_Px_RX_TX;       // Enable Tx and Rx on P1.
  ADCCFG &= ~HAL_UART_Px_RX_TX;      // Make sure ADC doesnt use this.
  UxCSR = CSR_MODE;                  // Mode is UART Mode.
  UxUCR = UCR_FLUSH;                 // Flush it.

  // Setup Tx by DMA.
  ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_TX );

  // The start address of the destination.
  HAL_DMA_SET_DEST( ch, DMA_UDBUF );

  // Using the length field to determine how many bytes to transfer.
  HAL_DMA_SET_VLEN( ch, HAL_DMA_VLEN_USE_LEN );

  // One byte is transferred each time.
  HAL_DMA_SET_WORD_SIZE( ch, HAL_DMA_WORDSIZE_BYTE );

  // The bytes are transferred 1-by-1 on Tx Complete trigger.
  HAL_DMA_SET_TRIG_MODE( ch, HAL_DMA_TMODE_SINGLE );
  HAL_DMA_SET_TRIG_SRC( ch, DMATRIG_TX );

  // The source address is incremented by 1 byte after each transfer.
  HAL_DMA_SET_SRC_INC( ch, HAL_DMA_SRCINC_1 );

  // The destination address is constant - the Tx Data Buffer.
  HAL_DMA_SET_DST_INC( ch, HAL_DMA_DSTINC_0 );

  // The DMA Tx done is serviced by ISR in order to maintain full thruput.
  HAL_DMA_SET_IRQ( ch, HAL_DMA_IRQMASK_ENABLE );

  // Xfer all 8 bits of a byte xfer.
  HAL_DMA_SET_M8( ch, HAL_DMA_M8_USE_8_BITS );

  // DMA has highest priority for memory access.
  HAL_DMA_SET_PRIORITY( ch, HAL_DMA_PRI_HIGH );

  // Setup Rx by DMA.
  ch = HAL_DMA_GET_DESC1234( HAL_DMA_CH_RX );

  // The start address of the source.
  HAL_DMA_SET_SOURCE( ch, DMA_UDBUF );

  // Using the length field to determine how many bytes to transfer.
  HAL_DMA_SET_VLEN( ch, HAL_DMA_VLEN_USE_LEN );

  /* The trick is to cfg DMA to xfer 2 bytes for every 1 byte of Rx.
   * The byte after the Rx Data Buffer is the Baud Cfg Register,
   * which always has a known value. So init Rx buffer to inverse of that
   * known value. DMA word xfer will flip the bytes, so every valid Rx byte
   * in the Rx buffer will be preceded by a DMA_PAD char equal to the
   * Baud Cfg Register value.
   */
  HAL_DMA_SET_WORD_SIZE( ch, HAL_DMA_WORDSIZE_WORD );

  // The bytes are transferred 1-by-1 on Rx Complete trigger.
  HAL_DMA_SET_TRIG_MODE( ch, HAL_DMA_TMODE_SINGLE_REPEATED );
  HAL_DMA_SET_TRIG_SRC( ch, DMATRIG_RX );

  // The source address is constant - the Rx Data Buffer.
  HAL_DMA_SET_SRC_INC( ch, HAL_DMA_SRCINC_0 );

  // The destination address is incremented by 1 word after each transfer.
  HAL_DMA_SET_DST_INC( ch, HAL_DMA_DSTINC_1 );
  HAL_DMA_SET_DEST( ch, dmaCfg.rxBuf );
  HAL_DMA_SET_LEN( ch, HAL_UART_DMA_RX_MAX );

  // The DMA is to be polled and shall not issue an IRQ upon completion.
  HAL_DMA_SET_IRQ( ch, HAL_DMA_IRQMASK_DISABLE );

  // Xfer all 8 bits of a byte xfer.
  HAL_DMA_SET_M8( ch, HAL_DMA_M8_USE_8_BITS );

  // DMA has highest priority for memory access.
  HAL_DMA_SET_PRIORITY( ch, HAL_DMA_PRI_HIGH );
}

/******************************************************************************
 * @fn      HalUARTOpenDMA
 *
 * @brief   Open a port according tp the configuration specified by parameter.
 *
 * @param   config - contains configuration information
 *
 * @return  none
 *****************************************************************************/
static void HalUARTOpenDMA(halUARTCfg_t *config)
{
  dmaCfg.uartCB = config->callBackFunc;
  // Only supporting subset of baudrate for code size - other is possible.
  HAL_ASSERT((config->baudRate == HAL_UART_BR_9600) ||
             (config->baudRate == HAL_UART_BR_19200) ||
             (config->baudRate == HAL_UART_BR_38400) ||
             (config->baudRate == HAL_UART_BR_57600) ||
             (config->baudRate == HAL_UART_BR_115200));

  if (config->baudRate == HAL_UART_BR_57600 ||
      config->baudRate == HAL_UART_BR_115200)
  {
    UxBAUD = 216;
  }
  else
  {
    UxBAUD = 59;
  }

  switch (config->baudRate)
  {
    case HAL_UART_BR_9600:
      UxGCR = 8;
      break;
    case HAL_UART_BR_19200:
      UxGCR = 9;
      break;
    case HAL_UART_BR_38400:
    case HAL_UART_BR_57600:
      UxGCR = 10;
      break;
    default:
      // HAL_UART_BR_115200
      UxGCR = 11;
      break;
  }

  // 8 bits/char; no parity; 1 stop bit; stop bit hi.
  if (config->flowControl)
  {
    UxUCR = UCR_FLOW | UCR_STOP;
    PxSEL |= HAL_UART_Px_CTS;
    // DMA Rx is always on (self-resetting). So flow must be controlled by the S/W polling the Rx
    // buffer level. Start by allowing flow.
    PxOUT &= ~HAL_UART_Px_RTS;
    PxDIR |=  HAL_UART_Px_RTS;
  }
  else
  {
    UxUCR = UCR_STOP;
  }

  dmaCfg.rxBuf[0] = *(volatile uint8 *)DMA_UDBUF;  // Clear the DMA Rx trigger.
  HAL_DMA_CLEAR_IRQ(HAL_DMA_CH_RX);
  HAL_DMA_ARM_CH(HAL_DMA_CH_RX);
  (void)osal_memset(dmaCfg.rxBuf, (DMA_PAD ^ 0xFF), HAL_UART_DMA_RX_MAX*2);

  UxCSR = (CSR_MODE | CSR_RE);
}

/*****************************************************************************
 * @fn      HalUARTReadDMA
 *
 * @brief   Read a buffer from the UART
 *
 * @param   buf  - valid data buffer at least 'len' bytes in size
 *          len  - max length number of bytes to copy to 'buf'
 *
 * @return  length of buffer that was read
 *****************************************************************************/
static uint16 HalUARTReadDMA(uint8 *buf, uint16 len)
{
  uint16 cnt;

  for (cnt = 0; cnt < len; cnt++)
  {
    if (!HAL_UART_DMA_NEW_RX_BYTE(dmaCfg.rxHead))
    {
      break;
    }
    *buf++ = HAL_UART_DMA_GET_RX_BYTE(dmaCfg.rxHead);
    HAL_UART_DMA_CLR_RX_BYTE(dmaCfg.rxHead);
#if HAL_UART_DMA_RX_MAX == 256
    dmaCfg.rxHead++;
#else
    if (++(dmaCfg.rxHead) >= HAL_UART_DMA_RX_MAX)
    {
      dmaCfg.rxHead = 0;
    }
#endif
  }
  if (UxUCR & UCR_FLOW)
  {
    PxOUT &= ~HAL_UART_Px_RTS;  // Re-enable the flow on any read.
  }

  return cnt;
}

/******************************************************************************
 * @fn      HalUARTWriteDMA
 *
 * @brief   Write a buffer to the UART.
 *
 * @param   buf - pointer to the buffer that will be written, not freed
 *          len - length of
 *
 * @return  length of the buffer that was sent
 *****************************************************************************/
static uint16 HalUARTWriteDMA(uint8 *buf, uint16 len)
{
  txIdx_t txIdx;
  uint8 txSel;
  halIntState_t his;

  HAL_ENTER_CRITICAL_SECTION(his);
  txSel = dmaCfg.txSel;
  txIdx = dmaCfg.txIdx[txSel];
  HAL_EXIT_CRITICAL_SECTION(his);

  // Enforce all or none.
  if ((len + txIdx) > HAL_UART_DMA_TX_MAX)
  {
    return 0;
  }

  (void)osal_memcpy(&(dmaCfg.txBuf[txSel][txIdx]), buf, len);

  HAL_ENTER_CRITICAL_SECTION(his);
  /* If an ongoing DMA Tx finished while this buffer was being *appended*, then another DMA Tx
   * will have already been started on this buffer, but it did not include the bytes just appended.
   * Therefore these bytes have to be re-copied to the start of the new working buffer.
   */
  if (txSel != dmaCfg.txSel)
  {
    HAL_EXIT_CRITICAL_SECTION(his);
    txSel ^= 1;

    (void)osal_memcpy(&(dmaCfg.txBuf[txSel][0]), buf, len);
    HAL_ENTER_CRITICAL_SECTION(his);
    dmaCfg.txIdx[txSel] = len;
  }
  else
  {
    dmaCfg.txIdx[txSel] = txIdx + len;
  }

  // If there is no ongoing DMA Tx, then the channel must be armed here.
  if (dmaCfg.txIdx[(txSel ^ 1)] == 0)
  {
    HAL_EXIT_CRITICAL_SECTION(his);
    HalUARTArmTxDMA();
  }
  else
  {
    HAL_EXIT_CRITICAL_SECTION(his);
  }

  return len;
}

/******************************************************************************
 * @fn      HalUARTPollDMA
 *
 * @brief   Poll a USART module implemented by DMA.
 *
 * @param   none
 *
 * @return  none
 *****************************************************************************/
static void HalUARTPollDMA(void)
{
  uint16 cnt = 0;
  uint8 evt = 0;

  if (HAL_UART_DMA_NEW_RX_BYTE(dmaCfg.rxHead))
  {
    rxIdx_t tail = findRxTail();

    // If the DMA has transferred in more Rx bytes, reset the Rx idle timer.
    if (dmaCfg.rxTail != tail)
    {
      dmaCfg.rxTail = tail;

      // Re-sync the shadow on any 1st byte(s) received.
      if (dmaCfg.rxTick == 0)
      {
        dmaCfg.rxShdw = ST0;
      }
      dmaCfg.rxTick = HAL_UART_DMA_IDLE;
    }
    else if (dmaCfg.rxTick)
    {
      // Use the LSB of the sleep timer (ST0 must be read first anyway).
      uint8 decr = ST0 - dmaCfg.rxShdw;

      if (dmaCfg.rxTick > decr)
      {
        dmaCfg.rxTick -= decr;
        dmaCfg.rxShdw = ST0;
      }
      else
      {
        dmaCfg.rxTick = 0;
      }
    }
    if ((cnt = dmaCfg.rxTail - dmaCfg.rxHead) > HAL_UART_DMA_RX_MAX)
    {
      cnt += HAL_UART_DMA_RX_MAX;
    }
  }
  else
  {
    dmaCfg.rxTick = 0;
  }

  HalUARTPollTxTrigger();

  if (cnt >= HAL_UART_DMA_FULL)
  {
    evt = HAL_UART_RX_FULL;
  }
  else if (cnt >= HAL_UART_DMA_HIGH)
  {
    evt = HAL_UART_RX_ABOUT_FULL;
    if (UxUCR & UCR_FLOW)
    {
      PxOUT |= HAL_UART_Px_RTS;  // Disable Rx flow.
    }
  }
  else if (cnt && !dmaCfg.rxTick)
  {
    evt = HAL_UART_RX_TIMEOUT;
  }

  if (dmaCfg.txMT)
  {
    dmaCfg.txMT = FALSE;
    evt |= HAL_UART_TX_EMPTY;
  }

  if ((evt != 0) && (dmaCfg.uartCB != NULL))
  {
    dmaCfg.uartCB(HAL_UART_DMA-1, evt);
  }
}

/**************************************************************************************************
 * @fn      HalUARTRxAvailDMA()
 *
 * @brief   Calculate Rx Buffer length - the number of bytes in the buffer.
 *
 * @param   none
 *
 * @return  length of current Rx Buffer
 **************************************************************************************************/
static uint16 HalUARTRxAvailDMA(void)
{
  uint16 cnt = 0;

  if (HAL_UART_DMA_NEW_RX_BYTE(dmaCfg.rxHead))
  {
    uint16 idx;

    for (idx = 0; idx < HAL_UART_DMA_RX_MAX; idx++)
    {
      if (HAL_UART_DMA_NEW_RX_BYTE(idx))
      {
        cnt++;
      }
    }
  }

  return cnt;
}

/******************************************************************************
 * @fn      HalUARTBusyDMA
 *
 * @brief   Query the UART hardware & buffers before entering PM mode 1, 2 or 3.
 *
 * @param   None
 *
 * @return  TRUE if the UART H/W is busy or buffers are not empty; FALSE otherwise.
 *****************************************************************************/
static uint8 HalUARTBusyDMA( void )
{
  return !((!(UxCSR & (CSR_ACTIVE | CSR_RX_BYTE))) &&
          (dmaCfg.rxHead == dmaCfg.rxTail) && (dmaCfg.txIdx[0] == 0) && (dmaCfg.txIdx[1] == 0));
}

/******************************************************************************
 * @fn      HalUARTSuspendDMA
 *
 * @brief   Suspend UART hardware before entering PM mode 1, 2 or 3.
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
static void HalUARTSuspendDMA( void )
{
#if defined POWER_SAVING
  UxCSR = CSR_MODE;              // Suspend Rx operations.

  if (UxUCR & UCR_FLOW)
  {
    PxOUT |= HAL_UART_Px_RTS;  // Disable Rx flow.
    PxIFG = (HAL_UART_Px_CTS ^ 0xFF);
    PxIEN |= HAL_UART_Px_CTS;  // Enable the CTS ISR.
  }
  else
  {
    PxIFG = (HAL_UART_Px_RX ^ 0xFF);
    PxIEN |= HAL_UART_Px_RX;   // Enable the Rx ISR.
  }

#if defined HAL_UART_GPIO_ISR
  PxIF  =  0;                  // Clear the next level.
  PICTL |= PICTL_BIT;          // Interrupt on falling edge of input.
  IENx |= IEN_BIT;
#endif
#endif
}

/******************************************************************************
 * @fn      HalUARTResumeDMA
 *
 * @brief   Resume UART hardware after exiting PM mode 1, 2 or 3.
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
static void HalUARTResumeDMA( void )
{
#if defined POWER_SAVING
#if defined HAL_UART_GPIO_ISR
  IENx &= (IEN_BIT ^ 0xFF);
#endif

  if (UxUCR & UCR_FLOW)
  {
    PxIEN &= (HAL_UART_Px_CTS ^ 0xFF);  // Disable the CTS ISR.
    PxOUT &= (HAL_UART_Px_RTS ^ 0xFF);  // Re-enable Rx flow.
  }
  else
  {
    PxIEN &= (HAL_UART_Px_RX ^ 0xFF);   // Enable the Rx ISR.
  }

  UxUCR |= UCR_FLUSH;
  UxCSR = (CSR_MODE | CSR_RE);
#endif
}

/******************************************************************************
 * @fn      HalUARTPollTxTrigger
 *
 * @brief   Ascertain whether a manual trigger is required for the DMA Tx channel.
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
static void HalUARTPollTxTrigger(void)
{
  if ((UxCSR & CSR_TX_BYTE) == 0)  // If no TXBUF to shift register transfer, then TXBUF may be MT.
  {
    // If flow control is enabled, then when the host stalls the next-to-last byte of a
    // txBuf[n] transfer, the last byte will sit in TXBUF and age more than the maximum expected
    // duration of HAL_UART_TX_TICK_MIN and can thereby get clobbered by the first byte of the 
    // txBuf[n^1] buffer.
    if ((dmaCfg.txTick == 0) || ((uint8)(ST0 - dmaCfg.txTick) > HAL_UART_TX_TICK_MIN))
    {
      dmaCfg.txTick = 0;

      if (HAL_DMA_CH_ARMED(HAL_DMA_CH_TX))
      {
        HAL_DMA_MAN_TRIGGER(HAL_DMA_CH_TX);
      }
    }
  }
  else
  {
    UxCSR = (CSR_MODE | CSR_RE);  // Clear the CSR_TX_BYTE flag.
    dmaCfg.txTick = ST0;  // Used to measure the minimum delay for a byte at 38.4-kB.

    if (dmaCfg.txTick == 0)  // Reserve zero to signify that the minimum delay has been met.
    {
      dmaCfg.txTick = 0xFF;
    }
  }
}

/******************************************************************************
 * @fn      HalUARTArmTxDMA
 *
 * @brief   Arm the Tx DMA channel.
 *
 * @param   None
 *
 * @return  None
 *****************************************************************************/
static void HalUARTArmTxDMA(void)
{
  halDMADesc_t *ch = HAL_DMA_GET_DESC1234(HAL_DMA_CH_TX);

  HAL_DMA_SET_SOURCE(ch, dmaCfg.txBuf[dmaCfg.txSel]);
  HAL_DMA_SET_LEN(ch, dmaCfg.txIdx[dmaCfg.txSel]);
  dmaCfg.txSel ^= 1;

  HAL_DMA_ARM_CH(HAL_DMA_CH_TX);
  HalUARTPollTxTrigger();
}

/******************************************************************************
 * @fn      HalUARTIsrDMA
 *
 * @brief   Handle the Tx done DMA ISR.
 *
 * @param   none
 *
 * @return  none
 *****************************************************************************/
void HalUARTIsrDMA(void)
{
  HAL_DMA_CLEAR_IRQ(HAL_DMA_CH_TX);

  // If there is more Tx data ready to go, re-start the DMA immediately on it.
  if (dmaCfg.txIdx[dmaCfg.txSel])
  {
    HalUARTArmTxDMA();
    dmaCfg.txIdx[dmaCfg.txSel] = 0;  // Indicate that the buffer is free now.
  }
  else
  {
    dmaCfg.txIdx[(dmaCfg.txSel ^ 1)] = 0;  // Indicate that the buffer is free now.
  }
}

#if (defined POWER_SAVING && defined HAL_UART_GPIO_ISR)
/***************************************************************************************************
 * @fn      PortX Interrupt Handler
 *
 * @brief   This function is the PortX interrupt service routine.
 *
 * @param   None.
 *
 * @return  None.
 ***************************************************************************************************/
#if (HAL_UART_DMA == 1)
HAL_ISR_FUNCTION(port0Isr, P0INT_VECTOR)
#else
HAL_ISR_FUNCTION(port1Isr, P1INT_VECTOR)
#endif
{
  HAL_ENTER_ISR();

  HalUARTResume();
  if (dmaCfg.uartCB != NULL)
  {
    dmaCfg.uartCB(HAL_UART_DMA-1, HAL_UART_RX_WAKEUP);
  }
  PxIFG = 0;
  PxIF = 0;

  HAL_EXIT_ISR();
}
#endif

/******************************************************************************
******************************************************************************/
