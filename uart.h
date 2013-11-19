#ifndef uart_h
#define uart_h
int uart_init ( void );
void uart_putc ( unsigned int x );
void hexstring ( unsigned int, unsigned int );
void uart_string ( const char* );
#endif
