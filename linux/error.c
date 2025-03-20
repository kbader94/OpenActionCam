#include "error.h"
#include <string.h>
#include <stdlib.h>

/* Global Error State */
static uint8_t current_error = 0;

/* Initialize Error System */
void init_error_system() {
    /* Open syslog for system-wide logging */
    openlog("linux_camera", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Error system initialized.");
}

/* Store and log an error */
void throw_error(uint8_t code, const char* message) {
    current_error = code;

    /* Write to log file */
    FILE* log_file = fopen("/var/log/linux_camera.log", "a");
    if (log_file) {
        fprintf(log_file, "[ERROR %d] %s\n", code, message);
        fclose(log_file);
    } else {
        syslog(LOG_ERR, "Failed to open log file.");
    }
}

/* Reset error */
void reset_error() {
    current_error = 0;
}

/* Get the current error */
uint8_t get_current_error() {
    return current_error;
}
