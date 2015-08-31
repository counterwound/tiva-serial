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

#define SCREEN_BAUD 	115200
#define UART_BAUD 		115200

//*****************************************************************************
// Flags
//*****************************************************************************
volatile bool g_bUART2RxFlag = 0;		// UART2 Rx Idle Flag

//*****************************************************************************
// UART message objects that will hold the separate UART messages
//*****************************************************************************

tUARTMsgObject g_sUARTMsgObjectTx;
tUARTMsgObject g_sUARTMsgObjectRx;

//*****************************************************************************
// Message buffers that hold the contents of the messages
//*****************************************************************************

uint8_t g_pui8MsgTx[8];
uint8_t g_pui8MsgRx[8];

//*****************************************************************************
// The interrupt handler for the UART2 interrupt
//*****************************************************************************
void DrawScreen(void)
{
	// Clear and reset home screen
	UARTprintf("\033[2J"); // Clear everything

	UARTprintf("\033[1;1f");
	UARTprintf("Counterwound Labs, Inc.");
	UARTprintf("\033[2;1f");
	UARTprintf("--------------------\r\n");
}

//*****************************************************************************
// The interrupt handler for the UART2 interrupt
//*****************************************************************************
void UART2IntHandler(void)
{
    uint32_t ui32Status;

    // Get the interrrupt status.
    ui32Status = UARTIntStatus(UART2_BASE, true);

    // Clear the asserted interrupts.
    UARTIntClear(UART2_BASE, ui32Status);

    // Interrupt for when data is Rx
    // Tied to UARTFIFOEnable()
    if(ui32Status == UART_INT_RX)
	{
    	g_bUART2RxFlag = 1;
	}
}

//*****************************************************************************
// Main code starts here
//*****************************************************************************
int main(void)
{
	// Enable Peripheral Clocks
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    // Enable pin PA0 for UART0 U0RX
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0);

    // Enable pin PA1 for UART0 U0TX
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1);

    // Enable pin PD6 for UART2 U2RX
    //
    GPIOPinConfigure(GPIO_PD6_U2RX);
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6);

    // Enable pin PD7 for UART2 U2TX
    // First open the lock and select the bits we want to modify in the GPIO commit register.
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0x80;

    // Now modify the configuration of the pins that we unlocked.
    GPIOPinConfigure(GPIO_PD7_U2TX);
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_7);

    // Set the clocking to run directly from the crystal.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);
	// Use the internal 16MHz oscillator as the UART clock source.
	UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

	// Initialize  UART0 using uartstdio
    UARTStdioConfig(0, SCREEN_BAUD, 16000000);

    // Initialize UART2
	UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), UART_BAUD,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFOEnable(UART2_BASE);
    UARTFIFOLevelSet(UART2_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);

	// Enable processor interrupts
	IntMasterEnable();

	IntEnable(INT_UART2);
	UARTIntEnable(UART2_BASE, UART_INT_RX);

    // Initialize a message object to be used for receiving UART messages
	g_sUARTMsgObjectRx.ui16MsgID = 0;
	g_sUARTMsgObjectRx.ui32MsgLen = sizeof(g_pui8MsgTx);

	DrawScreen();

	while (1)
	{
		if ( g_bUART2RxFlag )
		{
			g_bUART2RxFlag = 0;

		    // A buffer for storing the received data must be provided,
		    // so set the buffer pointer within the message object.
		    g_sUARTMsgObjectRx.pui8MsgData = g_pui8MsgRx;

		    // Receive a message
			UARTMessageGet(UART2_BASE, &g_sUARTMsgObjectRx);

			// Receive data
			unsigned int uIdx;

			UARTprintf("Rx ID: %04x, Msg: ", g_sUARTMsgObjectRx.ui16MsgID);

			for(uIdx = 0; uIdx < g_sUARTMsgObjectRx.ui32MsgLen; uIdx++)
			{
				UARTprintf( "%02x ", g_pui8MsgRx[uIdx] );
			}
			UARTprintf("\r\n");
		}

//		if ( !UARTBusy(UART2_BASE) )
//		{
			g_pui8MsgTx[0] = rand();
			g_pui8MsgTx[1] = rand();
			g_pui8MsgTx[2] = rand();
			g_pui8MsgTx[3] = rand();
			g_pui8MsgTx[4] = rand();
			g_pui8MsgTx[5] = rand();
			g_pui8MsgTx[6] = rand();
			g_pui8MsgTx[7] = rand();

			// Send message
			g_sUARTMsgObjectTx.ui16MsgID = rand();
			g_sUARTMsgObjectTx.ui32MsgLen = sizeof(g_pui8MsgTx);
			g_sUARTMsgObjectTx.pui8MsgData = g_pui8MsgTx;

			UARTMessageSet(UART2_BASE, &g_sUARTMsgObjectTx);

			// Transmitted data
			unsigned int uIdx;

			UARTprintf("Tx ID: %04x, Msg: ", g_sUARTMsgObjectTx.ui16MsgID);
			for(uIdx = 0; uIdx < g_sUARTMsgObjectTx.ui32MsgLen; uIdx++)
			{
				UARTprintf( "%02x ", g_pui8MsgTx[uIdx] );
			}
			UARTprintf("\r\n");
//		}

		// Slow down the tests
		SysCtlDelay(SysCtlClockGet()/3);	// Delay 1 second
	}
}
