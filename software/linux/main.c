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
#include "error.h"

/*
 * cleanup - Handles SIGINT (Ctrl+C), ensuring recording is stopped before exit.
 * @note: This is mainly used for debugging purposes. In a real-world application,
 * recording should be stopped gracefully before a shutdown is initiated.
 * @signo: Signal number.
 */
static void cleanup(int signo)
{
    (void)signo; /* Suppress unused parameter warning */

    DEBUG_MESSAGE("Received SIGINT. Stopping recording and cleaning up...\n");
    end_record();
    DEBUG_MESSAGE("Cleanup complete. Exiting.\n");

    exit(0);
}

int main(void)
{
    struct Message msg;
    recording_params_t params = {
        .shutter = 5000,
        .awb = "incandescent",
        .lens_position = 4.0,
        .bitrate = 20000000,
        .resolution = "1920x1080",
        .fps = 30,
        .gain = 1.0,
        .level = "4.2"};

    /* Register SIGINT handler */
    signal(SIGINT, cleanup);

    init_error_system();

    /* Initialize communication */
    comms_init();

    /* Process serial messages */
    while (1)
    {
        if (comms_receive_message(&msg))
        {
            switch (msg.header.message_type)
            {

            case MESSAGE_TYPE_COMMAND:
                switch (msg.body.payload_command.command)
                {
                case COMMAND_RECORD_REQ_START:
                    DEBUG_MESSAGE("Received RECORD START command\n");
                    start_record(params);
                    break;

                case COMMAND_RECORD_REQ_END:
                    DEBUG_MESSAGE("Received RECORD STOP command\n");
                    end_record();
                    break;

                case COMMAND_SHUTDOWN_REQ:
                    DEBUG_MESSAGE("Received SHUTDOWN REQUEST command\n");
                    end_record();
                    comms_send_command(COMMAND_SHUTDOWN_STARTED);
                    break;

                default:
                    WARN("Unknown command: 0x%04x\n", msg.body.payload_command.command);
                    break;
                }
                break;

            case MESSAGE_TYPE_ERROR: /* Log errors received from firmware */
                WARN("[FIRMWARE ERROR] Code %d: %s",
                     msg.body.payload_error.error_code,
                     msg.body.payload_error.error_message);
            break;

            case MESSAGE_TYPE_STATUS:
            {
                const struct StatusBody *s = &msg.body.payload_status;
                DEBUG_MESSAGE("[STATUS] Battery: %.2fV | State: %d | Charging: %s | Error: %d\n",
                       s->battery_voltage,
                       s->state,
                       s->charging ? "Yes" : "No",
                       s->error_code);
                break;
            }

            default:
                DEBUG_MESSAGE("[WARN] Unknown message type: 0x%02x\n", msg.header.message_type);
                break;
            }
        }

        /* Send heartbeat command to indicate system is fully running */
        comms_send_command(COMMAND_HB);

        usleep(50); /* 50ms poll interval */
    }

    return 0;
}
