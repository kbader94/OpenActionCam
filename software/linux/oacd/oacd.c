// oacd.c - Minimal watchdog-aware daemon
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <linux/watchdog.h>
#include <systemd/sd-daemon.h>

#define WATCHDOG_DEV "/dev/watchdog1"
#define PING_INTERVAL_SEC 5  /* Seconds between pings to systemd AND firmware */

int main(void)
{
    fprintf(stderr, "OACT watchdog daemon starting...\n");

    int fd = open(WATCHDOG_DEV, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open watchdog device");
        return 1;
    }
    
    if (sd_notify(0, "READY=1") <=0 ) {
        perror("Failed to notify systemd of startup");
        return 1;
    };
    fprintf(stderr, "OACT watchdog daemon started...\n");

    while (1) {
        /* Ping systemd */
        if (sd_notify(0, "WATCHDOG=1") <= 0) {
            fprintf(stderr, "Warning: failed to notify systemd\n");
        }

        /* Ping firmware watchdog */
        if (ioctl(fd, WDIOC_KEEPALIVE, 0) < 0) {
            fprintf(stderr, "Warning: failed to ping watchdog: %s\n", strerror(errno));
        }

        sleep(PING_INTERVAL_SEC);
    }

    close(fd);  /* should not reach */ 
    return 0;
}
