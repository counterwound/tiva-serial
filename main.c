/*
 * Assumes pins Tx and Rx pins are jumpered
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/random.h"
#include "uart-message/umsg.h"
#include "message-buffer/buffer.h"
#include "pinmux.h"

#define SCREEN_BAUD 	115200
#define UART_BAUD 		115200

//*****************************************************************************
// Global Variables
//*****************************************************************************
uint32_t ui32TotalTx[8] = {0,0,0,0,0,0,0,0}; // Total number of messages Rx and failed
uint32_t ui32TotalRx[8] = {0,0,0,0,0,0,0,0}; // Total number of messages Rx successfully
uint32_t ui32TotalFx[8] = {0,0,0,0,0,0,0,0}; // Total number of messages Rx and failed

//*****************************************************************************
// Flags
//*****************************************************************************

//*****************************************************************************
// UART message objects that will hold the separate UART messages
//*****************************************************************************
tUARTMsgObject g_sUARTMsgObjectTx;
tUARTMsgObject g_sUARTMsgObjectRx;

//*****************************************************************************
// Staging area wich holds the contents of the messages
//*****************************************************************************
uint8_t g_pui8UARTDataTx[8];
uint8_t g_pui8UARTDataRx[8];

//*****************************************************************************
// message-buffer containers for UART messages
//*****************************************************************************
#define UART_MESSAGE_BUFFER_DEPTH 32
tBufObject g_sUARTbuffer;
tMsgObject g_sUARTmessage[UART_MESSAGE_BUFFER_DEPTH];

//*****************************************************************************
// Configure Interrupts
//*****************************************************************************
void ConfigureUART(void)
{
	// Use the internal 16MHz oscillator as the UART clock source.
	UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

	// Initialize the UART0 using uartstdio
    UARTStdioConfig(0, SCREEN_BAUD, 16000000);

    // Initialize UART2
	UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), UART_BAUD,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	UARTFIFOEnable(UART2_BASE);
	UARTFIFOLevelSet(UART2_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);

	// Initialize UART3
	UARTConfigSetExpClk(UART3_BASE, SysCtlClockGet(), UART_BAUD,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	UARTFIFOEnable(UART3_BASE);
	UARTFIFOLevelSet(UART3_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);

    // Initialize UART4
	UARTConfigSetExpClk(UART4_BASE, SysCtlClockGet(), UART_BAUD,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	UARTFIFOEnable(UART4_BASE);
	UARTFIFOLevelSet(UART4_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);

    // Initialize UART5
	UARTConfigSetExpClk(UART5_BASE, SysCtlClockGet(), UART_BAUD,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	UARTFIFOEnable(UART5_BASE);
	UARTFIFOLevelSet(UART5_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);

    // Initialize UART7
	UARTConfigSetExpClk(UART7_BASE, SysCtlClockGet(), UART_BAUD,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	UARTFIFOEnable(UART7_BASE);
	UARTFIFOLevelSet(UART7_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);
}

//*****************************************************************************
// Parse and direct message to UART buffer
//*****************************************************************************
void handleUartMessage(tUARTMsgObject* uartMsgObject, size_t numUart)
{
	tMsgObject tmpMsgObject;
	uint32_t extendedID = (uint32_t) (uartMsgObject->ui16MsgID) + (0x10000 * numUart);

	// push incoming message into a tMsgObject so it can be added to the buffer
	populateMsgObject(&tmpMsgObject, extendedID, uartMsgObject->pui8MsgData, uartMsgObject->ui32MsgLen);
	pushMsgToBuf(&g_sUARTbuffer, tmpMsgObject);

	UARTprintf("\r\nTx UART: %d ID: %04x Data:",numUart,uartMsgObject->ui16MsgID );
	uint64_t i;
	for(i = 0; i < uartMsgObject->ui32MsgLen; i++)
	{
		UARTprintf(" %02x", uartMsgObject->pui8MsgData[i] );
	}
}

//*****************************************************************************
// Process the information in the UART Buffer
//*****************************************************************************
void processUARTBuffer(tBufObject* uartBufObject)
{
	uint32_t ui32Status;
	tMsgObject tmpMsgObject;

	while ( !isBufEmpty(uartBufObject) )
	{
		ui32Status = popMsgFromBuf(uartBufObject, &tmpMsgObject);

		if ( ui32Status == BUF_OBJ_NO_FLAGS )
		{
			size_t numUart = (tmpMsgObject.ui32MsgID & 0xFF0000) >> 16;
			uint16_t msgID = (uint16_t) (tmpMsgObject.ui32MsgID & 0xFFFF);
			UARTprintf("\r\nRx UART: %d ID: %04x Data:",numUart, msgID );
			uint64_t i;
			for(i = 0; i < 8; i++)
			{
				UARTprintf(" %02x", tmpMsgObject.pui8MsgData[i] );
			}
		}

		if ( (ui32Status & BUF_OBJ_EMPTY_POP) == BUF_OBJ_EMPTY_POP )
		{
			UARTprintf("\r\nPop Error, Empty Buffer" );
		}

		if ( (ui32Status & BUF_OBJ_DATA_LOST) == BUF_OBJ_DATA_LOST )
		{
			UARTprintf("\r\nPush Error, Data Lost" );
		}
	}
}


//*****************************************************************************
// Generic UART interrupt handler
//*****************************************************************************
void UARTIntHandler(uint32_t uartBase)
{
	uint32_t ui32StatusUART;

    tUARTMsgObject* uartMsgObject = &g_sUARTMsgObjectRx;
    uint8_t* uartMsgBuf = g_pui8UARTDataRx;
    size_t numUart;

    switch(uartBase)
    {
    case UART2_BASE:
    	numUart = 2;
    	break;
    case UART3_BASE:
    	numUart = 3;
    	break;
    case UART4_BASE:
    	numUart = 4;
    	break;
    case UART5_BASE:
    	numUart = 5;
    	break;
    case UART7_BASE:
    	numUart = 7;
    	break;
    default:
    	break;
    }

    // Get the interrrupt status.
    ui32StatusUART = UARTIntStatus(uartBase, true);

    // Clear the asserted interrupts.
    UARTIntClear(uartBase, ui32StatusUART);

    // Interrupt for when data is Rx
    // Tied to UARTFIFOEnable()
    if(ui32StatusUART == UART_INT_RX)
	{
        // A buffer for storing the received data must be provided,
        // so set the buffer pointer within the message object.
    	uartMsgObject->pui8MsgData = uartMsgBuf;

        // Receive a message
    	UARTMessageGet(uartBase, uartMsgObject);

    	switch(uartMsgObject->ui32Flags)
    	{
    	case MSG_OBJ_NEW_DATA:
    		// Interrupt triggered with correct CRC
    		handleUartMessage(uartMsgObject,numUart);
    		ui32TotalRx[numUart]++;
    		break;
    	case MSG_OBJ_DATA_LOST:
    		// Interrupt triggered, bad CRC
    		ui32TotalRx[numUart]++;
    		ui32TotalFx[numUart]++;
    		break;
    	case MSG_OBJ_NO_FLAGS:
    		// Interrupt triggered, but incorrect UMSG_START_BYTE
    	default:
    		break;
    	}
	}
}

//*****************************************************************************
// The interrupt handler for the UART interrupts
//*****************************************************************************
void UART2IntHandler(void)
{
	UARTIntHandler((uint32_t)UART2_BASE);
}

void UART3IntHandler(void)
{
	UARTIntHandler((uint32_t)UART3_BASE);
}

void UART4IntHandler(void)
{
	UARTIntHandler((uint32_t)UART4_BASE);
}

void UART5IntHandler(void)
{
	UARTIntHandler((uint32_t)UART5_BASE);
}

void UART7IntHandler(void)
{
	UARTIntHandler((uint32_t)UART7_BASE);
}

//*****************************************************************************
// Configure the buffers
//*****************************************************************************
void ConfigureBuffers(void)
{
	initMsgBuffer(&g_sUARTbuffer, g_sUARTmessage, UART_MESSAGE_BUFFER_DEPTH);
}

//*****************************************************************************
// Configure the interrupts
//*****************************************************************************
void ConfigureInterrupts(void)
{
	// Enable processor interrupts.
	IntMasterEnable();

	IntEnable(INT_UART2);
	UARTIntEnable(UART2_BASE, UART_INT_RX);

	IntEnable(INT_UART3);
	UARTIntEnable(UART3_BASE, UART_INT_RX);

	IntEnable(INT_UART4);
	UARTIntEnable(UART4_BASE, UART_INT_RX);

	IntEnable(INT_UART5);
	UARTIntEnable(UART5_BASE, UART_INT_RX);

	IntEnable(INT_UART7);
	UARTIntEnable(UART7_BASE, UART_INT_RX);
}

//*****************************************************************************
// Main code starts here
//*****************************************************************************
int main(void){
	// Set the clocking to run directly from the crystal.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    // Initialize and setup the ports
    PortFunctionInit();
    ConfigureUART();
    ConfigureBuffers();
    ConfigureInterrupts();

    // Initialize a message object to be used for receiving UART messages
	g_sUARTMsgObjectRx.ui16MsgID = 0;
	g_sUARTMsgObjectRx.ui32MsgLen = sizeof(g_pui8UARTDataTx);

	while (1)
	{
		// Generate random message to send
		uint16_t ui16ID = rand();
		g_pui8UARTDataTx[0] = rand();
		g_pui8UARTDataTx[1] = rand();
		g_pui8UARTDataTx[2] = rand();
		g_pui8UARTDataTx[3] = rand();
		g_pui8UARTDataTx[4] = rand();
		g_pui8UARTDataTx[5] = rand();
		g_pui8UARTDataTx[6] = rand();
		g_pui8UARTDataTx[7] = rand();

		g_sUARTMsgObjectTx.ui16MsgID = ui16ID;
		g_sUARTMsgObjectTx.ui32MsgLen = sizeof(g_pui8UARTDataTx);
		g_sUARTMsgObjectTx.pui8MsgData = g_pui8UARTDataTx;

		//*********************************************************************
		// Tx messages
		//*********************************************************************

		// Send UART 2 message
		UARTMessageSet(UART2_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[2]++;

		// Send UART 3 message
		UARTMessageSet(UART3_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[3]++;

		// Send UART 4 message
		UARTMessageSet(UART4_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[4]++;

		// Send UART 5 message
		UARTMessageSet(UART5_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[5]++;

		// Send UART 7 message
		UARTMessageSet(UART7_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[7]++;

		// Slow down the tests
		SysCtlDelay(SysCtlClockGet()/30);	// Delay 100 ms

		processUARTBuffer(&g_sUARTbuffer);
	}
}
