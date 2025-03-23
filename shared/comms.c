/*
 * comms.c - Portable Arduino and Linux serial communication
 */

 #include "comms.h"
 #include <string.h>

 #if IS_MCU
 static const uint8_t comms_recipient = MESSAGE_RECIPIENT_LINUX;
 #define SERIAL_WRITE(byte) serial_write(byte)
 #define SERIAL_AVAILABLE() serial_available()
 #define SERIAL_READ() serial_read()

 #else /* IS_LINUX */

 #define SERIAL_DEVICE "/dev/ttyS0"  /* Raspberry Pi UART */
 static const uint8_t comms_recipient = MESSAGE_RECIPIENT_FIRMWARE;
 static int serial_fd = -1;  /* Ensure serial_fd is properly declared */
 static int linux_serial_read(void); 
 static int linux_serial_available(void);
 #define SERIAL_WRITE(byte) write(serial_fd, &byte, 1)
 #define SERIAL_AVAILABLE() linux_serial_available()
 #define SERIAL_READ() linux_serial_read()

 #endif
 
 #define BUFFER_SIZE (MAX_PAYLOAD_SIZE + 6)

 static uint8_t rx_buffer[BUFFER_SIZE];
 static uint8_t rx_index = 0;
 static bool receiving = false;
 static uint32_t last_byte_time = 0;

 static int comms_send_message(const struct Message *msg);
 static int comms_serialize_message(const struct Message *msg, uint8_t *out_buf);
 static int comms_deserialize_message(const uint8_t *in_buf, size_t length, struct Message *msg);
 static uint8_t comms_calculate_checksum(const uint8_t *data, uint8_t length);

/* 
 * -------------- Public API ------------------
 */

 /* 
  * @name comms_init
  * @brief Initializes the serial communication for both ATmega and Linux System
  * @note This function sets up the serial port for communication.
  *       For ATmega firmware, it initializes the serial port at 9600 baud rate.
  *       For Linux, it opens the serial device and configures it for 9600 baud rate,
  *       8n1, raw mode, with no flow control.
  *     
  * @return void
  */
 void comms_init(void) {
 #if IS_MCU
     serial_begin(9600);
 #else /* IS_LINUX */
     struct termios options;
     serial_fd = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
     if (serial_fd == -1) {
         perror("Error opening serial port");
         return;
     }
 
     /* Get current options */
     tcgetattr(serial_fd, &options);
 
     /* Set Baud Rate */
     cfsetispeed(&options, B9600);
     cfsetospeed(&options, B9600);
 
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

/*
 * comms_receive_message - Read and decode one complete message from serial.
 * @msg: Pointer to target Message structure.
 *
 * Returns: true if a full valid message was received, false otherwise.
 */
bool comms_receive_message(struct Message *msg)
{
    if (!msg)
        return false;
        
    uint32_t now = GET_TIME_MS();

    if (receiving && (now - last_byte_time > MAX_MESSAGE_TIMEOUT_MS)) {
        receiving = false;
        rx_index = 0;
        /* Set message null */
        memset(msg, 0, sizeof(struct Message));
        return false;  /* Error - Timeout */
    }

    while (SERIAL_AVAILABLE() > 0) {
        int byte = SERIAL_READ();
        if (byte < 0) {
            /* Set message to null */
            memset(msg, 0, sizeof(struct Message));
            return false;
        }

        last_byte_time = now;
        uint8_t b = (uint8_t)byte;

        if (!receiving) {
            if (b == MESSAGE_START) {
                rx_index = 0;
                rx_buffer[rx_index++] = b;
                /* Initialize message to null */
                memset(msg, 0, sizeof(struct Message));
                receiving = true;
            } else {
                rx_index = 0;
                rx_buffer[rx_index++] = b;
                /* Set message to null */
                memset(msg, 0, sizeof(struct Message));
                return false; /* Error - Corrupt start byte */
            }
        } else {
            if (rx_index >= BUFFER_SIZE) {
                receiving = false;
                rx_index = 0;
                /* Set message to null */
                memset(msg, 0, sizeof(struct Message));
                return false; /* Error - Buffer overflow */ 
            }

            rx_buffer[rx_index++] = b;

            if (rx_index >= 6) {
                uint8_t payload_len = rx_buffer[3];
                uint8_t expected_len = 6 + payload_len;

                if (rx_index == expected_len) {
                    if (rx_buffer[expected_len - 1] != MESSAGE_END) {
                        receiving = false;
                        rx_index = 0;
                        /* Set message to null */
                        memset(msg, 0, sizeof(struct Message));
                        return false; /* Error - Corrupt end byte */ 
                    }

                    uint8_t checksum = rx_buffer[expected_len - 2];
                    uint8_t computed = comms_calculate_checksum(&rx_buffer[4], payload_len);
                    if (checksum != computed) {
                        receiving = false;
                        rx_index = 0;
                        /* Set message to null */
                        memset(msg, 0, sizeof(struct Message));
                        return false; /* Error - Bad checksum */
                    }

                    /* Deserialize into Message */
                    return comms_deserialize_message(rx_buffer, expected_len, msg) == 0;
                }
            }
        }
    }

    /* Set message to null */
    memset(msg, 0, sizeof(struct Message));
    return false; /* No Message received*/
}

void comms_send_command(uint16_t command)
{
    struct Message msg;
    msg.header.recipient = comms_recipient;
    msg.header.message_type = MESSAGE_TYPE_COMMAND;
    msg.header.payload_length = sizeof(struct CommandBody);
    msg.body.payload_command.command = command;

    comms_send_message(&msg);
}

void comms_send_error(uint8_t error_code, const char *error_message)
{
    struct Message msg;
    msg.header.recipient = comms_recipient;
    msg.header.message_type = MESSAGE_TYPE_ERROR;

    msg.body.payload_error.error_code = error_code;

    size_t maxlen = sizeof(msg.body.payload_error.error_message);
    if (error_message) {
        strncpy(msg.body.payload_error.error_message, error_message, maxlen - 1);
        msg.body.payload_error.error_message[maxlen - 1] = '\0';
    } else {
        msg.body.payload_error.error_message[0] = '\0';
    }

    msg.header.payload_length = 1 + strlen(msg.body.payload_error.error_message);

    comms_send_message(&msg);
}

void comms_send_status(const struct StatusBody *status)
{
    if (!status) return;

    struct Message msg;
    msg.header.recipient = comms_recipient;
    msg.header.message_type = MESSAGE_TYPE_STATUS;
    msg.header.payload_length = sizeof(struct StatusBody);
    memcpy(&msg.body.payload_status, status, sizeof(struct StatusBody));

    comms_send_message(&msg);
}


/* 
 * -------------- Internal Functions ------------------
 */

/*
 * comms_send_message - Serializes and transmits a full Message to the other platform.
 * @msg: Pointer to a fully populated Message struct.
 *
 * Returns: 0 on success, negative error code on failure.
 */
static int comms_send_message(const struct Message *msg)
{
    if (!msg) {
        return -1;
    }

    /* Construct serialized message buffer */
    uint8_t payload[MAX_PAYLOAD_SIZE + 6];
    int len = comms_serialize_message(msg, payload);
    if (len < 0) {
        return -2; /* Serialization failed */
    }

    for (int i = 0; i < len; ++i) {
        SERIAL_WRITE(payload[i]);
    }

    return 0;
}

 /* 
  * comms_calculate_checksum
  * @param data Pointer to the data array
  * @param length Length of the data array
  * @return Calculated checksum
  */
 static uint8_t comms_calculate_checksum(const uint8_t *data, uint8_t length)
 {
     uint8_t checksum = 0;
     for (uint8_t i = 0; i < length; i++) {
         checksum ^= data[i];  /* Simple XOR checksum */
     }
     return checksum;
 }
 
 /* 
  * comms_serialize_message
  * @param msg Pointer to the message structure
  * @param out_buf Pointer to the output buffer
  * @return < 0 on error, length of serialized message on success
  */
 static int comms_serialize_message(const struct Message *msg, uint8_t *out_buf)
 {
     if (!msg || !out_buf)
         return -1;
 
     out_buf[0] = MESSAGE_START;
     out_buf[1] = msg->header.recipient;
     out_buf[2] = msg->header.message_type;
     out_buf[3] = msg->header.payload_length;
 
     switch (msg->header.message_type) {
     case MESSAGE_TYPE_COMMAND:
         out_buf[4] = msg->body.payload_command.command & 0xFF;
         out_buf[5] = (msg->body.payload_command.command >> 8) & 0xFF;
         break;
 
     case MESSAGE_TYPE_STATUS:
         if (msg->header.payload_length < sizeof(struct StatusBody))
             return -2;
         memcpy(&out_buf[4], &msg->body.payload_status, sizeof(struct StatusBody));
         break;
 
     case MESSAGE_TYPE_ERROR:
         out_buf[4] = msg->body.payload_error.error_code;
         strncpy((char *)&out_buf[5], msg->body.payload_error.error_message, msg->header.payload_length - 1);
         break;
 
     case MESSAGE_TYPE_DATA:
         memcpy(&out_buf[4], msg->body.payload_raw, msg->header.payload_length);
         break;
 
     default:
         return -3;
     }
 
     uint8_t checksum = comms_calculate_checksum(&out_buf[4], msg->header.payload_length);
     out_buf[4 + msg->header.payload_length] = checksum;
     out_buf[5 + msg->header.payload_length] = MESSAGE_END;
 
     return 6 + msg->header.payload_length; // total length
 }

 /* 
  * comms_deserialize_message
  * @param in_buf Pointer to the input buffer
  * @param length Length of the input buffer
  * @param msg Pointer to the message structure to fill
  * @return < 0 on error, 0 on success
  */
 static int comms_deserialize_message(const uint8_t *in_buf, size_t length, struct Message *msg)
 {
     if (!in_buf || !msg || length < 6)
         return -1;
 
     if (in_buf[0] != MESSAGE_START || in_buf[length - 1] != MESSAGE_END)
         return -2;
 
     msg->header.recipient = in_buf[1];
     msg->header.message_type = in_buf[2];
     msg->header.payload_length = in_buf[3];
 
     uint8_t checksum = in_buf[4 + msg->header.payload_length];
     uint8_t computed = comms_calculate_checksum(&in_buf[4], msg->header.payload_length);
     if (checksum != computed)
         return -3;
 
     switch (msg->header.message_type) {
     case MESSAGE_TYPE_COMMAND:
         msg->body.payload_command.command = in_buf[4] | (in_buf[5] << 8);
         break;
 
     case MESSAGE_TYPE_STATUS:
         memcpy(&msg->body.payload_status, &in_buf[4], sizeof(struct StatusBody));
         break;
 
     case MESSAGE_TYPE_ERROR:
         msg->body.payload_error.error_code = in_buf[4];
         strncpy(msg->body.payload_error.error_message, (char *)&in_buf[5], msg->header.payload_length - 1);
         msg->body.payload_error.error_message[msg->header.payload_length - 1] = '\0';
         break;
 
     case MESSAGE_TYPE_DATA:
         memcpy(msg->body.payload_raw, &in_buf[4], msg->header.payload_length);
         break;
 
     default:
         return -4;
     }
 
     return 0;
 }
 
 /* Internal Linux serial functions */
 #if IS_LINUX
 static int linux_serial_available(void) {
     int bytes_available;
 
     if (serial_fd == -1) {
         return 0;  // Prevent reading from an invalid file descriptor
     }
 
     if (ioctl(serial_fd, FIONREAD, &bytes_available) == -1) {
         perror("Error checking available bytes");
         return 0;
     }
 
     return bytes_available;
 }
 
 /* Read a single byte */
 static int linux_serial_read(void) {
     uint8_t byte;
 
     if (serial_fd == -1) {
         return -1;  // Prevent reading from an invalid file descriptor
     }
 
     int result = read(serial_fd, &byte, 1);
     if (result == -1) {
         perror("Error reading from serial");
     }
     /* If the result's length is 1, return the value, otherwise -1 for error */
     return (result == 1) ? byte : -1;
 }
 
 /* Close serial port */
 void comms_close(void) {
     if (serial_fd != -1) {
         close(serial_fd);
         serial_fd = -1;
     }
 }

 #else

 /* Close serial port */
 void comms_close(void) {
    /* Placeholder */
 }

 #endif /* IS_LINUX */
 