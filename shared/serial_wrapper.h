#ifndef SERIAL_WRAPPER_H
#define SERIAL_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

void serial_begin(unsigned long baudrate);
void serial_write(uint8_t byte);
int serial_available(void);
int serial_read(void);

#ifdef __cplusplus
}
#endif

#endif /* SERIAL_WRAPPER_H */
