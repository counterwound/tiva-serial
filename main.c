/*
 * Assumes pins PD6 and PD7 are jumpered.
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
volatile bool g_bUART2RxFlag = 0;		// UART2 Rx Flag
volatile bool g_bUART3RxFlag = 0;		// UART3 Rx Flag
volatile bool g_bUART4RxFlag = 0;		// UART4 Rx Flag
volatile bool g_bUART5RxFlag = 0;		// UART5 Rx Flag
volatile bool g_bUART7RxFlag = 0;		// UART7 Rx Flag

//*****************************************************************************
// UART message objects that will hold the separate UART messages
//*****************************************************************************

tUARTMsgObject g_sUARTMsgObjectTx;
tUARTMsgObject g_sUARTMsgObject2Rx;
tUARTMsgObject g_sUARTMsgObject3Rx;
tUARTMsgObject g_sUARTMsgObject4Rx;
tUARTMsgObject g_sUARTMsgObject5Rx;
tUARTMsgObject g_sUARTMsgObject7Rx;

//*****************************************************************************
// Message buffers that hold the contents of the messages
//*****************************************************************************

uint8_t g_pui8MsgTx[8];
uint8_t g_pui8Msg2Rx[8];
uint8_t g_pui8Msg3Rx[8];
uint8_t g_pui8Msg4Rx[8];
uint8_t g_pui8Msg5Rx[8];
uint8_t g_pui8Msg7Rx[8];

//*****************************************************************************
// The interrupt handler for the UART2 interrupt
//*****************************************************************************
void DrawScreen(void)
{
	// Clear and reset home screen
	UARTprintf("\033[2J"); // Clear everything

	UARTprintf("\033[1;1f");
	UARTprintf("Counterwound Labs, Inc.");

	UARTprintf("\033[3;10f");	// Set cursor to R3C10
	UARTprintf( "ID");

	UARTprintf("\033[3;20f");	// Set cursor to R3C20
	UARTprintf( "Message");

	UARTprintf("\033[3;47f");	// Set cursor to R3C20
	UARTprintf( "Tx/Rx");

	UARTprintf("\033[3;57f");	// Set cursor to R3C20
	UARTprintf( "Lost");

	UARTprintf("\033[4;1f");	// Set cursor to R4C1
	UARTprintf( "U2 Tx:\r\n");
	UARTprintf( "   Rx:\r\n");

	UARTprintf("\033[6;1f");	// Set cursor to R6C1
	UARTprintf( "U3 Tx:\r\n");
	UARTprintf( "   Rx:\r\n");

	UARTprintf("\033[8;1f");	// Set cursor to R8C1
	UARTprintf( "U4 Tx:\r\n");
	UARTprintf( "   Rx:\r\n");

	UARTprintf("\033[10;1f");	// Set cursor to R10C1
	UARTprintf( "U5 Tx:\r\n");
	UARTprintf( "   Rx:\r\n");

	UARTprintf("\033[12;1f");	// Set cursor to R12C1
	UARTprintf( "U7 Tx:\r\n");
	UARTprintf( "   Rx:\r\n");
}

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
// Generic UART interrupt handler
//*****************************************************************************

void UARTIntHandler(uint32_t numUart)
{
    uint32_t ui32StatusUART;
    uint32_t ui32StatusMsg;

    uint32_t uartBase = 0;
    tUARTMsgObject* uartMsgObject;
    uint8_t* uartMsgBuf;

    switch(numUart)
    {
    case 2:
    	uartBase = UART2_BASE;
    	uartMsgObject = &g_sUARTMsgObject2Rx;
    	uartMsgBuf = g_pui8Msg2Rx;
    	break;
    case 3:
    	uartBase = UART3_BASE;
    	uartMsgObject = &g_sUARTMsgObject3Rx;
    	uartMsgBuf = g_pui8Msg3Rx;
    	break;
    case 4:
    	uartBase = UART4_BASE;
    	uartMsgObject = &g_sUARTMsgObject4Rx;
    	uartMsgBuf = g_pui8Msg4Rx;
    	break;
    case 5:
    	uartBase = UART5_BASE;
    	uartMsgObject = &g_sUARTMsgObject5Rx;
    	uartMsgBuf = g_pui8Msg5Rx;
    	break;
    case 7:
    	uartBase = UART7_BASE;
    	uartMsgObject = &g_sUARTMsgObject7Rx;
    	uartMsgBuf = g_pui8Msg7Rx;
    	break;
    default:
    	// should never be here!
    	uartMsgBuf = 0;
    	// so, crash so it is obvious...
    	*uartMsgBuf = (uint8_t)0xDEAD;
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
    	//UARTMessageGet(uartBase, uartMsgObject);

    	ui32StatusMsg = uartMsgObject->ui32Flags;

    	if ( ui32StatusMsg == MSG_OBJ_NEW_DATA )
    	{
    		// Interrupt triggered with correct CRC
        	g_bUART2RxFlag = 1;
        	ui32TotalRx[numUart]++;
    	}
    	else if ( ui32StatusMsg == MSG_OBJ_DATA_LOST )
		{
    		// Interrupt triggered, bad CRC
    		ui32TotalRx[numUart]++;
    		ui32TotalFx[numUart]++;
		}
    	else if ( ui32StatusMsg == MSG_OBJ_NO_FLAGS )
    	{
			// Interrupt triggered, but incorrect UMSG_START_BYTE
		}
	}
}

//*****************************************************************************
// The interrupt handler for the UART2 interrupt
//*****************************************************************************
void UART2IntHandler(void)
{
	UARTIntHandler(2);
}

//*****************************************************************************
// The interrupt handler for the UART3 interrupt
//*****************************************************************************
void UART3IntHandler(void)
{
	UARTIntHandler(3);
}

//*****************************************************************************
// The interrupt handler for the UART4 interrupt
//*****************************************************************************
void UART4IntHandler(void)
{
	UARTIntHandler(4);
}

//*****************************************************************************
// The interrupt handler for the UART5 interrupt
//*****************************************************************************
void UART5IntHandler(void)
{
	UARTIntHandler(5);
}

//*****************************************************************************
// The interrupt handler for the UART7 interrupt
//*****************************************************************************
void UART7IntHandler(void)
{
	UARTIntHandler(7);
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
// Print message function
//*****************************************************************************
void PrintMessage(tUARTMsgObject *sUARTMsgObject, uint8_t pui8Msg[], uint32_t ui32Row, uint32_t ui32Col, uint32_t ui32Rx, uint32_t ui32Fx)
{
	UARTprintf("\033[%d;%df",ui32Row,ui32Col);

	// Receive data
	UARTprintf("%04x      ", sUARTMsgObject->ui16MsgID);

	unsigned int uIdx;
	for(uIdx = 0; uIdx < sUARTMsgObject->ui32MsgLen; uIdx++)
	{
		UARTprintf( "%02x ", pui8Msg[uIdx] );
	}

	UARTprintf("   %5d    %5d", ui32Rx, ui32Fx);

	UARTprintf("\033[2;1f");	// Set cursor to R2C1
}

//*****************************************************************************
// Main code starts here
//*****************************************************************************
int main(void)
{
	// Set the clocking to run directly from the crystal.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    // Initialize and setup the ports
    PortFunctionInit();
    ConfigureUART();
    ConfigureInterrupts();

    // Initialize a message object to be used for receiving UART messages
	g_sUARTMsgObject2Rx.ui16MsgID = 0;
	g_sUARTMsgObject2Rx.ui32MsgLen = sizeof(g_pui8MsgTx);

    // Initialize a message object to be used for receiving UART messages
	g_sUARTMsgObject3Rx.ui16MsgID = 0;
	g_sUARTMsgObject3Rx.ui32MsgLen = sizeof(g_pui8MsgTx);

    // Initialize a message object to be used for receiving UART messages
	g_sUARTMsgObject4Rx.ui16MsgID = 0;
	g_sUARTMsgObject4Rx.ui32MsgLen = sizeof(g_pui8MsgTx);

    // Initialize a message object to be used for receiving UART messages
	g_sUARTMsgObject5Rx.ui16MsgID = 0;
	g_sUARTMsgObject5Rx.ui32MsgLen = sizeof(g_pui8MsgTx);

    // Initialize a message object to be used for receiving UART messages
	g_sUARTMsgObject7Rx.ui16MsgID = 0;
	g_sUARTMsgObject7Rx.ui32MsgLen = sizeof(g_pui8MsgTx);

	DrawScreen();

	while (1)
	{
		// Generate random message to send
		uint8_t ui8Byte0 = rand();
		uint8_t ui8Byte1 = rand();
		uint8_t ui8Byte2 = rand();
		uint8_t ui8Byte3 = rand();
		uint8_t ui8Byte4 = rand();
		uint8_t ui8Byte5 = rand();
		uint8_t ui8Byte6 = rand();
		uint8_t ui8Byte7 = rand();
		uint16_t ui16ID = rand();

		//*********************************************************************
		// Tx messages
		//*********************************************************************

		// Send UART 2 message
		g_pui8MsgTx[0] = ui8Byte0;
		g_pui8MsgTx[1] = ui8Byte1;
		g_pui8MsgTx[2] = ui8Byte2;
		g_pui8MsgTx[3] = ui8Byte3;
		g_pui8MsgTx[4] = ui8Byte4;
		g_pui8MsgTx[5] = ui8Byte5;
		g_pui8MsgTx[6] = ui8Byte6;
		g_pui8MsgTx[7] = ui8Byte7;

		g_sUARTMsgObjectTx.ui16MsgID = ui16ID;
		g_sUARTMsgObjectTx.ui32MsgLen = sizeof(g_pui8MsgTx);
		g_sUARTMsgObjectTx.pui8MsgData = g_pui8MsgTx;

		UARTMessageSet(UART2_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[2]++;

		PrintMessage(&g_sUARTMsgObjectTx, g_pui8MsgTx, 4, 10, ui32TotalTx[2], 0);

		// Send UART 3 message
		g_pui8MsgTx[0] = ui8Byte0;
		g_pui8MsgTx[1] = ui8Byte1;
		g_pui8MsgTx[2] = ui8Byte2;
		g_pui8MsgTx[3] = ui8Byte3;
		g_pui8MsgTx[4] = ui8Byte4;
		g_pui8MsgTx[5] = ui8Byte5;
		g_pui8MsgTx[6] = ui8Byte6;
		g_pui8MsgTx[7] = ui8Byte7;

		g_sUARTMsgObjectTx.ui16MsgID = ui16ID;
		g_sUARTMsgObjectTx.ui32MsgLen = sizeof(g_pui8MsgTx);
		g_sUARTMsgObjectTx.pui8MsgData = g_pui8MsgTx;

		UARTMessageSet(UART3_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[3]++;

		PrintMessage(&g_sUARTMsgObjectTx, g_pui8MsgTx, 6, 10, ui32TotalTx[3], 0);

		// Send UART 4 message
		g_pui8MsgTx[0] = ui8Byte0;
		g_pui8MsgTx[1] = ui8Byte1;
		g_pui8MsgTx[2] = ui8Byte2;
		g_pui8MsgTx[3] = ui8Byte3;
		g_pui8MsgTx[4] = ui8Byte4;
		g_pui8MsgTx[5] = ui8Byte5;
		g_pui8MsgTx[6] = ui8Byte6;
		g_pui8MsgTx[7] = ui8Byte7;

		g_sUARTMsgObjectTx.ui16MsgID = ui16ID;
		g_sUARTMsgObjectTx.ui32MsgLen = sizeof(g_pui8MsgTx);
		g_sUARTMsgObjectTx.pui8MsgData = g_pui8MsgTx;

		UARTMessageSet(UART4_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[4]++;

		PrintMessage(&g_sUARTMsgObjectTx, g_pui8MsgTx, 8, 10, ui32TotalTx[4], 0);

		// Send UART 5 message
		g_pui8MsgTx[0] = ui8Byte0;
		g_pui8MsgTx[1] = ui8Byte1;
		g_pui8MsgTx[2] = ui8Byte2;
		g_pui8MsgTx[3] = ui8Byte3;
		g_pui8MsgTx[4] = ui8Byte4;
		g_pui8MsgTx[5] = ui8Byte5;
		g_pui8MsgTx[6] = ui8Byte6;
		g_pui8MsgTx[7] = ui8Byte7;

		g_sUARTMsgObjectTx.ui16MsgID = ui16ID;
		g_sUARTMsgObjectTx.ui32MsgLen = sizeof(g_pui8MsgTx);
		g_sUARTMsgObjectTx.pui8MsgData = g_pui8MsgTx;

		UARTMessageSet(UART5_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[5]++;

		PrintMessage(&g_sUARTMsgObjectTx, g_pui8MsgTx, 10, 10, ui32TotalTx[5], 0);

		// Send UART 7 message
		g_pui8MsgTx[0] = ui8Byte0;
		g_pui8MsgTx[1] = ui8Byte1;
		g_pui8MsgTx[2] = ui8Byte2;
		g_pui8MsgTx[3] = ui8Byte3;
		g_pui8MsgTx[4] = ui8Byte4;
		g_pui8MsgTx[5] = ui8Byte5;
		g_pui8MsgTx[6] = ui8Byte6;
		g_pui8MsgTx[7] = ui8Byte7;

		g_sUARTMsgObjectTx.ui16MsgID = ui16ID;
		g_sUARTMsgObjectTx.ui32MsgLen = sizeof(g_pui8MsgTx);
		g_sUARTMsgObjectTx.pui8MsgData = g_pui8MsgTx;

		UARTMessageSet(UART7_BASE, &g_sUARTMsgObjectTx);
		ui32TotalTx[7]++;

		PrintMessage(&g_sUARTMsgObjectTx, g_pui8MsgTx, 12, 10, ui32TotalTx[7], 0);

		//*********************************************************************
		// Rx messages
		//*********************************************************************

		if ( g_bUART2RxFlag )
		{
			g_bUART2RxFlag = 0;
			PrintMessage(&g_sUARTMsgObject2Rx, g_pui8Msg2Rx, 5, 10, ui32TotalRx[2], ui32TotalFx[2]);
		}

		if ( g_bUART3RxFlag )
		{
			g_bUART3RxFlag = 0;
			PrintMessage(&g_sUARTMsgObject3Rx, g_pui8Msg3Rx, 7, 10, ui32TotalRx[3], ui32TotalFx[3]);
		}

		if ( g_bUART4RxFlag )
		{
			g_bUART4RxFlag = 0;
			PrintMessage(&g_sUARTMsgObject4Rx, g_pui8Msg4Rx, 9, 10, ui32TotalRx[4], ui32TotalFx[4]);
		}

		if ( g_bUART5RxFlag )
		{
			g_bUART5RxFlag = 0;
			PrintMessage(&g_sUARTMsgObject5Rx, g_pui8Msg5Rx, 11, 10, ui32TotalRx[5], ui32TotalFx[5]);
		}

		if ( g_bUART7RxFlag )
		{
			g_bUART7RxFlag = 0;
			PrintMessage(&g_sUARTMsgObject7Rx, g_pui8Msg7Rx, 13, 10, ui32TotalRx[7], ui32TotalFx[7]);
		}

		// Slow down the tests
		SysCtlDelay(SysCtlClockGet()/3000);	// Delay 1 ms
	}
}
