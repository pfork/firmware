#ifndef cmd_h
#define cmd_h
void stream_rnd ( void );
void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep);
#endif
