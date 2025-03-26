#include <Arduino.h>

extern "C" {
    void serial_begin(unsigned long baudrate) {
        Serial.begin(baudrate);
    }

    void serial_write(uint8_t byte) {
        Serial.write(byte);
    }

    int serial_available() {
        return Serial.available();
    }

    /* Read a single byte from the serial port
     * @return (int) the byte read, or -1 if no byte is available
     */
    int serial_read() {
        uint8_t byte = 0;
        if (Serial.available()) {
            Serial.readBytes(&byte, 1);
            return byte;
        }
        return -1;  /* No byte available */ 
    }

}
