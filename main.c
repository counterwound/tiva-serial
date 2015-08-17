#include <stdint.h>
#include <stdbool.h>
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

#define UART_BAUD 115200

//*****************************************************************************
// Flags
//*****************************************************************************
volatile bool g_bUART2RxFlag = 0;		// UART2 Rx Idle Flag

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
	UARTprintf("--------------------");
}

//*****************************************************************************
// Generate data for a random message
//*****************************************************************************
uint64_t RandomMessage(void)
{
	// �FIXME? Not so random but good enough for now
	uint64_t ui64Message = 0x0807060504030201;

	return ui64Message;
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
   		// Loop while there are characters in the receive FIFO.
		while(UARTCharsAvail(UART2_BASE));
		{
			UARTCharGetNonBlocking(UART2_BASE);
		}
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
    UARTStdioConfig(0, UART_BAUD, 16000000);

    // Initialize UART2
	UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), UART_BAUD,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFOEnable(UART2_BASE);
    UARTFIFOLevelSet(UART2_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);


	// Enable processor interrupts.
	IntMasterEnable();

	IntEnable(INT_UART2);
	UARTIntEnable(UART2_BASE, UART_INT_RX);
//	UARTTxIntModeSet(UART2_BASE, UART_TXINT_MODE_EOT);

	DrawScreen();

	while (1)
	{
		if ( g_bUART2RxFlag )
		{
			g_bUART2RxFlag = 0;

			// Code here
		}

		if ( !UARTBusy(UART2_BASE) )
		{
			UARTprintf("\r\n%08x",RandomMessage());
		}

		SysCtlDelay(SysCtlClockGet()/3);	// Delay 1 second
	}
}
