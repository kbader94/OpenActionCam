/* 
 * comms.h - Portable Ardino and Linux serial communication
 * 
 */
#include "comms.h"
#include <string.h>

/* Arduino Serial Defintions */
#ifdef ARDUINO
#include <Arduino.h>
#define SERIAL_WRITE(byte) Serial.write(byte)
#define SERIAL_AVAILABLE() Serial.available()
#define SERIAL_READ(byte_ptr) Serial.readBytes(byte_ptr, 1)
#else
/* Linux Serial Definitions */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#define SERIAL_DEVICE "/dev/ttyAMA0"  /* Raspberry Pi UART */
static int serial_fd = -1;  /* Linux serial port file descriptor */

#define SERIAL_WRITE(byte) write(serial_fd, &byte, 1)
#define SERIAL_AVAILABLE() linux_serial_available()
#define SERIAL_READ(byte_ptr) linux_serial_read(byte_ptr)
#endif

/* Universal Serial Definitions */
#define BUFFER_SIZE 64
static uint8_t rx_buffer[BUFFER_SIZE];
static uint8_t rx_index = 0;
static bool receiving = false;
static uint32_t last_byte_time = 0; /* Time of last received byte */
static uint8_t expected_length = 0; /* Expected total message length */

/* Initialize Serial Communication */
void comms_init(void) {
#ifdef ARDUINO
    Serial.begin(ARDUINO_BAUDRATE);
#else
    /* Linux Serial initialization - open serial device and apply options */
    struct termios options;

    serial_fd = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd == -1) {
        perror("Error opening serial port");
        return;
    }

    /* Get current options */
    tcgetattr(serial_fd, &options);

    /* Set Baud Rate */
    cfsetispeed(&options, LINUX_BAUDRATE);
    cfsetospeed(&options, LINUX_BAUDRATE);

    /* Set 8N1 (8-bit, No parity, 1 stop bit) */
    options.c_cflag &= ~PARENB;  /* No parity */
    options.c_cflag &= ~CSTOPB;  /* 1 stop bit */
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;      /* 8-bit characters */

    /* Disable flow control */
    options.c_cflag &= ~CRTSCTS;

    /* Set raw input/output mode */
    options.c_lflag = 0;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    /* Apply settings */
    tcsetattr(serial_fd, TCSANOW, &options);

    /* Set non-blocking mode */
    fcntl(serial_fd, F_SETFL, FNDELAY);
#endif
}

/* Check available bytes (Linux) */
#ifndef ARDUINO
static int linux_serial_available() {
    int bytes_available;
    ioctl(serial_fd, FIONREAD, &bytes_available);
    return bytes_available;
}

/* Read a single byte (Linux) */
static int linux_serial_read(uint8_t *byte_ptr) {
    return read(serial_fd, byte_ptr, 1);
}

/* Close serial port (Linux) */
void comms_close(void) {
    if (serial_fd != -1) {
        close(serial_fd);
        serial_fd = -1;
    }
}
#endif

/* 
 * =======================================================================
 *                          MESSAGE STRUCTURE
 * =======================================================================
 * Each message follows a structured format to ensure reliable parsing:
 *
 *  ┌────────┬────────┬────────┬───────────┬───────────┬───────────┬────────┐
 *  │ Start  │ Recip. │  Type  │ Length(N) │  Payload  │ Checksum  │  End   │
 *  │ (0xAA) │(1 Byte)│(1 Byte)│  (1 Byte) │ (N Bytes) │   (XOR)   │ (0x55) │
 *  ├────────┴────────┴────────┴───────────┴───────────┴───────────┴────────┤

 *
 * - **Start (0xAA)** → Identifies the start of a new message.
 * - **Recipient (1 byte)** → Who the message is intended for.
 * - **Type (1 byte)** → Defines whether it's a **COMMAND** or **DATA** message.
 * - **Length (1 byte)** → Specifies the payload size (0 to `MAX_PAYLOAD_SIZE`).
 * - **Payload (N bytes)** → Actual message data.
 * - **Checksum (1 byte)** → XOR of all payload bytes for integrity checking.
 * - **End (0x55)** → Marks the completion of the message.
 *
 * The function ensures **non-blocking behavior** on Arduino and Linux,
 * handling **partial reads, timeouts, buffer overflow, and corruption**.
 */

bool comms_receive_message(struct Message *msg) {
    static uint8_t byte;                /* Byte storage for incoming data */
    static uint8_t expected_length = 0; /* Expected full message length */
    uint32_t current_time = GET_TIME_MS();  /* Current timestamp for timeout */

    /* ──────────────────────── TIMEOUT CHECK ───────────────────────── */
    if (receiving && (current_time - last_byte_time > MAX_MESSAGE_TIMEOUT_MS)) {
        receiving = false; /* Reset reception state */
        rx_index = 0;
        expected_length = 0;
        return false; /* Error: Message timeout */
    }

    /* ──────────────────────── READ AVAILABLE BYTES ──────────────────────── */
    while (SERIAL_AVAILABLE() > 0) {
        if (SERIAL_READ(&byte) != 1) {
            return false; /* No data read */
        }

        last_byte_time = GET_TIME_MS();  /* Update last received time */

        /* ──────────────────── START BYTE DETECTION ─────────────────── */
        if (!receiving) {
            if (byte == MESSAGE_START) {
                receiving = true; /* Begin receiving */
                rx_index = 0;
                rx_buffer[rx_index++] = byte;
                expected_length = 0;  /* Reset expected length */
            }
        } else {
            /* ─────────────────── BUFFER OVERFLOW PREVENTION ─────────────────── */
            if (rx_index >= BUFFER_SIZE) {
                receiving = false;
                rx_index = 0;
                return false; /* Error: Buffer overflow */
            }

            rx_buffer[rx_index++] = byte;

            /* ──────────────────── PAYLOAD LENGTH EXTRACTION ─────────────────── */
            if (rx_index == 4) {
                expected_length = 6 + rx_buffer[3];  /* HEADER(4) + PAYLOAD + CHECKSUM(1) + END(1) */
                if (expected_length > BUFFER_SIZE) {
                    receiving = false;
                    rx_index = 0;
                    return false; /* Error: Message too large */
                }
            }

            /* ─────────────────── FULL MESSAGE RECEIVED ─────────────────── */
            if (rx_index == expected_length) {
                /* 1. Validate End Marker */
                if (rx_buffer[rx_index - 1] != MESSAGE_END) {
                    receiving = false;
                    rx_index = 0;
                    return false; /* Error: Missing end marker */
                }

                /* 2. Validate Checksum */
                uint8_t checksum = rx_buffer[rx_index - 2];
                uint8_t computed_checksum = comms_calculate_checksum(&rx_buffer[4], rx_buffer[3]);

                if (checksum != computed_checksum) {
                    receiving = false;
                    rx_index = 0;
                    return false; /* Error: Checksum mismatch (Data corrupted) */
                }

                /* ─────────────────── COPY MESSAGE INTO STRUCT ─────────────────── */
                msg->recipient = rx_buffer[1];
                msg->message_type = rx_buffer[2];
                msg->payload_length = rx_buffer[3];
                memcpy(msg->payload, &rx_buffer[4], msg->payload_length);

                /* ─────────────────── RESET BUFFER FOR NEXT MESSAGE ─────────────────── */
                receiving = false;
                rx_index = 0;
                return true; /* Successfully received a valid message */
            }
        }
    }

    return false; /* No complete message received yet */
}

