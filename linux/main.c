/*
 * main.c - Entry point for linux_camera
 * Handles communication with ATmega and manages recording lifecycle accordingly.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <string.h>
 #include <signal.h>
 #include "comms.h"
 #include "record.h"
 
 /*
  * cleanup - Handles SIGINT (Ctrl+C), ensuring recording is stopped before exit.
  * @note: This is mainly used for debugging purposes. In a real-world application,
  * recording should be stopped gracefully before a shutdown is initiated. 
  * @signo: Signal number.
  */
 static void cleanup(int signo)
 {
     (void) signo; /* Suppress unused parameter warning */
     
     printf("\nReceived SIGINT. Stopping recording and cleaning up...\n");
     end_record();
     printf("Cleanup complete. Exiting.\n");
 
     exit(0);
 }
 
 int main(void)
 {
     /* Register SIGINT handler */
     signal(SIGINT, cleanup);
 
     /* Initialize communication */
     comms_init();
     
     /* Send BOOT command to indicate system is fully running */ 
     comms_send_command(COMMAND_BOOT);
 
     struct Message msg;
     recording_params_t params = {
         .shutter = 5000,
         .awb = "incandescent",
         .lens_position = 4.0,
         .bitrate = 20000000,
         .resolution = "1920x1080",
         .fps = 30,
         .gain = 1.0,
         .level = "4.2"
     };
 
     while (1) {
        if (comms_receive_message(&msg)) {
            if (msg.header.message_type == MESSAGE_TYPE_COMMAND) {
                switch (msg.body.payload_command.command) {
                    case COMMAND_RECORD_REQ_START:
                        printf("Received RECORD START command\n");
                        start_record(params);
                        break;

                    case COMMAND_RECORD_REQ_END:
                        printf("Received RECORD STOP command\n");
                        end_record();
                        break;

                    case COMMAND_SHUTDOWN_REQ:
                        printf("Received SHUTDOWN REQUEST command\n");
                        end_record();
                        comms_send_command(COMMAND_SHUTDOWN_STARTED);
                        break;

                    default:
                        printf("Unknown command: 0x%04x\n", msg.body.payload_command.command);
                        break;
                }
            } else if (msg.header.message_type == MESSAGE_TYPE_ERROR) {
                printf("[FIRMWARE ERROR] Code %d: %s\n",
                    msg.body.payload_error.error_code,
                    msg.body.payload_error.error_message);
            }
        }

        usleep(5000);  // Polling interval
    }
 
     return 0;
 }
 