#ifndef _NRF24L01_H
#define _NRF24L01_H

#define CEPIN 10 // pb10
#define CSNPIN 1 // pb1
#define IRQPIN 4 // pa4

 /* Configure NRF24L01 pins: IRQ->PA4 and CSN->PB1  CE->PB10*/
#define IRQ					   (gpio_get(GPIOA_BASE, 1 << IRQPIN))
#define CSN(x)					x ? (gpio_set(GPIOB_BASE, 1 << CSNPIN)) : (gpio_reset(GPIOB_BASE, 1 << CSNPIN));
#define CE(x)					x ? (gpio_set(GPIOB_BASE, 1 << CEPIN)) : (gpio_reset(GPIOB_BASE, 1 << CEPIN));

// SPI(nRF24L01) commands
#define READ_REG_NRF24L01    	0x00 			// Define read command to register
#define WRITE_REG_NRF24L01   	0x20 			// Define write command to register
#define RD_RX_PLOAD_WID       0x60        // Read RX payload width for the top R_RX_PAYLOAD in the RX FIFO.
#define RD_RX_PLOAD 			   0x61 			// Define RX payload register address
#define WR_TX_PLOAD 			   0xA0 			// Define TX payload register address
#define WR_ACK_PLOAD          0xA8        // Write Payload to be transmitted together with ACK packet on PIPE PPP. (PPP valid in the range from 000 to 101).
#define WR_TX_PLOAD_NO_ACK    0xB0        // Used in TX mode. Disables AUTOACK on this specific packet.
#define FLUSH_TX    			   0xE1 			// Define flush TX register command
#define FLUSH_RX    			   0xE2 			// Define flush RX register command
#define REUSE_TX_PL 			   0xE3 			// Define reuse TX payload register command
#define NOP         			   0xFF 			// Define No Operation, might be used to read status register
//***************************************************//
// SPI(nRF24L01) registers(addresses)
#define CONFIG      			0x00				// 'Config' register address
#define	RX_DR			0x40
#define	TX_DS			0x20
#define	MAX_RT		0x10
#define	EN_CRC		0x08
#define	CRC0			0x04
#define	PWR_UP		0x02
#define	PRIM_RX		0x01
#define EN_AA       			0x01           // 'Enable Auto Acknowledgment' register address
#define EN_RXADDR   			0x02           // 'Enabled RX addresses' register address
#define SETUP_AW    			0x03           // 'Setup address width' register address
#define SETUP_RETR  			0x04           // 'Setup Auto. Retrans' register address
#define RF_CH       			0x05           // 'RF channel' register address
#define RF_SETUP    			0x06 				// 'RF setup' register address
#define  RF_DR_HIGH    1 << 3
#define  RF_PWR_18dbm  0 << 1
#define  RF_PWR_12dbm  1 << 1
#define  RF_PWR_6dbm   2 << 1
#define  RF_PWR_0dbm   3 << 1
#define STATUS      			0x07 				// 'Status' register address
#define OBSERVE_TX  			0x08 				// 'Observe TX' register address
#define CD          			0x09 				// 'Carrier Detect' register address
#define RX_ADDR_P0  			0x0A				// 'RX address pipe0' register address
#define RX_ADDR_P1  			0x0B 				// 'RX address pipe1' register address
#define RX_ADDR_P2  			0x0C 				// 'RX address pipe2' register address
#define RX_ADDR_P3  			0x0D 				// 'RX address pipe3' register address
#define RX_ADDR_P4  			0x0E 				// 'RX address pipe4' register address
#define RX_ADDR_P5  			0x0F				// 'RX address pipe5' register address
#define TX_ADDR     			0x10 				// 'TX address' register address
#define RX_PW_P0    			0x11 				// 'RX payload width, pipe0' register address
#define RX_PW_P1    			0x12 				// 'RX payload width, pipe1' register address
#define RX_PW_P2    			0x13 				// 'RX payload width, pipe2' register address
#define RX_PW_P3    			0x14 				// 'RX payload width, pipe3' register address
#define RX_PW_P4    			0x15 				// 'RX payload width, pipe4' register address
#define RX_PW_P5    			0x16 				// 'RX payload width, pipe5' register address
#define FIFO_STATUS 			0x17 			  	// 'FIFO Status Register' register address
#define  RX_EMPTY       1 << 0
#define  RX_FULL        1 << 1
#define  TX_EMPTY       1 << 4
#define  TX_FULL        1 << 5
#define DYNPD   			   0x1C 			  	// dynamic payload length register address
#define FEATURE   			0x1D 			  	// FEATURE register address

#define ADDR_WIDTH   	5
#define PLOAD_WIDTH 	32
#define BCAST (const unsigned char*) "\x1\x2\x3\x5\x5"

unsigned char nrf_write_reg(const unsigned char reg, const unsigned char value);
unsigned char nrf_read_reg(const unsigned char reg);
unsigned char nrf_read_buf(const unsigned char reg, unsigned char *pBuf, const unsigned char bytes);
unsigned char nrf_write_buf(const unsigned char reg, const unsigned char *pBuf, const unsigned char bytes);
void nrf24_init(void);
void nrf_open_rx(const unsigned char *addr);
char nrf_recv(unsigned char * buf, unsigned char buflen);
char nrf_send(const unsigned char* address, const unsigned char * tx_buf, const unsigned char tx_buf_len);

#endif /*_NRF24L01_H*/

