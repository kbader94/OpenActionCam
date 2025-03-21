/*
 * comms.c - Portable Arduino and Linux serial communication
 */

 #include "comms.h"
 #include <string.h>
 
 #if IS_MCU

 #include "serial_wrapper.h"
 #define SERIAL_WRITE(byte) serial_write(byte)
 #define SERIAL_AVAILABLE() serial_available()
 #define SERIAL_READ() serial_read()

 #else /* IS_LINUX */

 #include <stdio.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <errno.h>
 #include <termios.h>
 #include <sys/ioctl.h>
 #define SERIAL_DEVICE "/dev/ttyAMA0"  /* Raspberry Pi UART */
 static int serial_fd = -1;  /* Ensure serial_fd is properly declared */
 #define SERIAL_WRITE(byte) write(serial_fd, &byte, 1)
 #define SERIAL_AVAILABLE() linux_serial_available()
 #define SERIAL_READ() linux_serial_read()

 #endif
 
 #define BUFFER_SIZE 64
 static uint8_t rx_buffer[BUFFER_SIZE];
 static uint8_t rx_index = 0;
 static bool receiving = false;
 static uint32_t last_byte_time = 0; /* Time of last received byte */

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
 
 /* Linux serial functions */
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
 
     return (result == 1) ? byte : -1;
 }
 
 /* Close serial port */
 void comms_close(void) {
     if (serial_fd != -1) {
         close(serial_fd);
         serial_fd = -1;
     }
 }
 #endif /* IS_LINUX */
 
 /* 
  * Calculate Checksum for data
  * @param data Pointer to the data array
  * @param length Length of the data array
  * @return Calculated checksum
  * @brief Calculate Checksum
  * @note Uses simple XOR checksum
  */
 uint8_t comms_calculate_checksum(uint8_t *data, uint8_t length) {
     uint8_t checksum = 0;
     for (uint8_t i = 0; i < length; i++) {
         checksum ^= data[i];  /* Simple XOR checksum */
     }
     return checksum;
 }
 
 /* Send a complete message */
 void comms_send_message(uint8_t recipient, uint8_t type, uint8_t *payload, uint8_t length) {
     if (length > MAX_PAYLOAD_SIZE) {
         length = MAX_PAYLOAD_SIZE;
     }
 
     uint8_t frame[MAX_PAYLOAD_SIZE + 6];
 
     frame[0] = MESSAGE_START;
     frame[1] = recipient;
     frame[2] = type;
     frame[3] = length;
     memcpy(&frame[4], payload, length);
     frame[4 + length] = comms_calculate_checksum(payload, length);
     frame[5 + length] = MESSAGE_END;
 
     for (uint8_t i = 0; i < length + 6; i++) {
         SERIAL_WRITE(frame[i]);
     }
 }
 
 /* Receive a message */
 bool comms_receive_message(struct Message *msg) {
     static uint8_t byte;
     static uint8_t expected_length = 0;
     uint32_t current_time = GET_TIME_MS();
     
     /* initialize message */
     msg->message_type = 0; 
     msg->payload_length = 0;
 
     /* Timeout Check */
     if (receiving && (current_time - last_byte_time > MAX_MESSAGE_TIMEOUT_MS)) {
         receiving = false;
         rx_index = 0;
         expected_length = 0;
         return false;
     }
 
     /* Read Available Bytes */
     while (SERIAL_AVAILABLE() > 0) {
         int byte_value = SERIAL_READ();
         if (byte_value == -1) {
             return false;
         }
         byte = (uint8_t) byte_value;
 
         last_byte_time = GET_TIME_MS();
 
         /* Start Byte Detection */
         if (!receiving) {
             if (byte == MESSAGE_START) {
                 receiving = true;
                 rx_index = 0;
                 rx_buffer[rx_index++] = byte;
                 expected_length = 0;
             }
         } else {
             if (rx_index >= BUFFER_SIZE) {
                 receiving = false;
                 rx_index = 0;
                 return false;
             }
 
             rx_buffer[rx_index++] = byte;
 
             if (rx_index == 4) {
                 expected_length = 6 + rx_buffer[3];
                 if (expected_length > BUFFER_SIZE) {
                     receiving = false;
                     rx_index = 0;
                     return false;
                 }
             }
 
             if (rx_index == expected_length) {
                 if (rx_buffer[rx_index - 1] != MESSAGE_END) {
                     receiving = false;
                     rx_index = 0;
                     return false;
                 }
 
                 uint8_t checksum = rx_buffer[rx_index - 2];
                 uint8_t computed_checksum = comms_calculate_checksum(&rx_buffer[4], rx_buffer[3]);
 
                 if (checksum != computed_checksum) {
                     receiving = false;
                     rx_index = 0;
                     return false;
                 }
 
                 msg->recipient = rx_buffer[1];
                 msg->message_type = rx_buffer[2];
                 msg->payload_length = rx_buffer[3];
                 memcpy(msg->payload, &rx_buffer[4], msg->payload_length);
 
                 receiving = false;
                 rx_index = 0;
                 return true;
             }
         }
     }
     return false;
 }
 
 /* Sends a command to the other platform.*/
 void comms_send_command(uint16_t command) {
    #if IS_MCU
    uint8_t recipient = MESSAGE_RECIPIENT_LINUX;
    #else
    uint8_t recipient = MESSAGE_RECIPIENT_FIRMWARE;
    #endif
    comms_send_message(recipient, MESSAGE_TYPE_COMMAND, (uint8_t *)&command, sizeof(command));
 }