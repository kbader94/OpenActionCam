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

    int serial_read() {
        return Serial.read();
    }
}
