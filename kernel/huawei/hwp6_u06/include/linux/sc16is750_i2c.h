#ifndef __LINUX_I2C__SC16IS750_H
#define __LINUX_I2C__SC16IS750_H

#include <mach/gpio.h> 

/* The data transmission (Tx/Rx) is done using two separate buffers. The size of the buffer limits
   the size of the data that can be transmitted or received by the buffer in one time. Avoid bigger
   size of the buffer due to possible limitation of the memory
*/

#define MAX_RECEIVED 4200//can't exceed the max irda frame bytes length
#define MAX_SENT 4200//can't exceed the max irda frame bytes length


/* The FIFO size for sc16is750 */
#define MAX_FIFO 64

/* use READ_RHR as cmd with ioctl before trying to read out the received data buffer otherwise
   the memory location(reg) of SC16IS750 is read out according to the previous addr send by write()*/
#define READ_RHR 0x0505

/* use READ_RX_SIZE as cmd with ioctl and then issue read() in order to get the size of received data
    in the buffer saved by the driver */
#define READ_RX_SIZE 0x0506

/* use WRITE_THR as cmd with ioctl and then issue write() in order to write to the Tx FIFO.*/
#define WRITE_THR 0x0507

/* IOCTL command to set the baud rate for UART communication */
#define SET_BAUD 0x0508

/* IOCTL commands to set the hardware flow control for UART */
#define NO_HW_CONTROL 0x0550 
#define	HW_CONTROL 0x0551

/* IOCTL commands to set the parity in UART frame */
#define NO_PARITY 0x0750
#define ODD_PARITY 0x0751
#define EVEN_PARITY 0x0752
#define FORCED_ONE_PARITY 0x0753
#define FORCED_ZERO_PARITY 0x0754

/* IOCTL commands to set the UART word length and the size of stop bits */
#define WL5_1SB 0x0850
#define WL5_1_5SB 0x0851
#define WL6_1SB 0x0852
#define WL6_2SB 0x0853
#define WL7_1SB 0x0854
#define WL7_2SB 0x0855
#define WL8_1SB 0x0856
#define WL8_2SB 0x0857


/* Default TLR and TCR values. */
#define DEFAULT_TCR 0x4E
#define DEFAULT_TLR 0xAB


/* During the tranmission, data is sent to Tx FIFO in chunks of MAX_TX_FIFO. After every chunk is send out via UART,
the device get THR empty interrupt and it write next chunk to the Tx FIFO. So MAX_TX_FIFO can not be greater than MAX_FIFO (64 bytes)*/
#define MAX_TX_FIFO 64


/* Major and Minor numbers can be changed if required. since the device nodes are not created dynamically,
   you need to mknod first before the driver is used. Currently set to the values as defined below */
#define I2C_MAJOR_NUM 155
#define I2C_MINOR_NUM 0

/* Register address for sc16is750 */
#define RHR 0x00 << 3
#define THR 0x00 << 3
#define IER 0x01 << 3
#define FCR 0x02 << 3
#define IIR 0x02 << 3
#define LCR 0x03 << 3
#define MCR 0x04 << 3
#define LSR 0x05 << 3
#define MSR 0x06 << 3
#define SPR 0x07 << 3
#define TCR 0x06 << 3
#define TLR 0x07 << 3
#define TXLVL 0x08 << 3
#define RXLVL 0x09 << 3
#define IODir 0x0A << 3
#define IOState 0x0B << 3
#define IOIntEna 0x0C << 3
#define IOControl 0x0E << 3
#define EFCR 0x0F << 3
#define DLL 0x00 << 3
#define DLH 0x01 << 3
#define EFR 0x02 << 3
#define XON1 0x04 << 3
#define XON2 0x05 << 3
#define XOFF1 0x06 << 3
#define XOFF2 0x07 << 3

/* Set IRQ_GPIO_022*/
#define IRDA_SHDN GPIO_2_4
#define IRDA_RESET GPIO_2_5
#define IRDA_IRQ     GPIO_2_6
#define IRDA_2V85EN  GPIO_18_6

//#define IRDA_VBUS "irda"

static struct irda_device_platorm_data{
	int en_gpio;
	int (*gpio_config)(void *gpio_data, bool configure);
	void *gpio_data;
};


/* Get the bit value of x for n [max_size, 0] */
#define GET_BIT(x,n) ((x >> n) & 0x01)

/* Set the bit value of x for bit n */
#define SET_BIT(n) (0x01 << n)

/* Clear the bit n (write '0') of x */
#define CLEAR_BIT(n) ~SET_BIT(n)

#endif

