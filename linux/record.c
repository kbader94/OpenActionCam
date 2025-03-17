#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <getopt.h>

#define OUTPUT_DIR "/home/pi/shared"
#define RAW_VIDEO OUTPUT_DIR"/video.264"
#define ENCODED_VIDEO OUTPUT_DIR"/video.mp4"

typedef struct {
    int width;
    int height;
    int fps;
} sensor_support_t;

// Supported video resolutions for Pi Camera Module 3 NoIR
static sensor_support_t sensor_formats[] = {
    {1920, 1080, 30},  // 1080p 30 FPS (Most stable)
    {2304, 1296, 30},  // 1296p 30 FPS (Wider FOV, Cropped)
    {1536,  864, 60},  // 864p 60 FPS (High FPS)
    {1280,  720, 90},  // 720p 90 FPS (Lower Latency)
    { 640,  480, 120}, // VGA 120 FPS (Slow Motion)
    {0, 0, 0}          // End marker
};

volatile sig_atomic_t recording = 1;
pid_t libcamera_pid = -1;

// Default parameters
int shutter = 5000;
char awb[64] = "incandescent";
double lens_position = 4.0;
int bitrate = 20000000;
char resolution[32] = "1920x1080";
int fps = 30;
double gain = 1.0;
char level_str[8] = "4.2";
char encoder[32] = "h264_v4l2m2m";  // Default to HW encoding

// Check if the resolution and FPS are supported by the camera
int is_valid_format(int width, int height, int fps) {
    for (int i = 0; sensor_formats[i].width != 0; i++) {
        if (sensor_formats[i].width == width &&
            sensor_formats[i].height == height &&
            sensor_formats[i].fps == fps) {
            return 1;  // Supported
        }
    }
    return 0;  // Not supported
}

// SIGINT handler to clean up recording and transcode
void cleanup(int signo) {
    (void) signo;
    printf("\nStopping recording...\n");
    if (libcamera_pid > 0) {
        kill(libcamera_pid, SIGINT);
        waitpid(libcamera_pid, NULL, 0);
    }
    printf("Recording stopped.\nFlushing data...\n");
    system("sync");
    printf("Starting transcoding...\n");

    // Parse resolution
    int width, height;
    sscanf(resolution, "%dx%d", &width, &height);

    // Validate format
    if (!is_valid_format(width, height, fps)) {
        printf("Invalid format: %dx%d @ %d FPS is NOT supported! Exiting.\n", width, height, fps);
        exit(1);
    }

    printf("Encoding with HW (%s).\n", encoder);

    // Build ffmpeg command
    char ffmpeg_cmd[1024];
    snprintf(ffmpeg_cmd, sizeof(ffmpeg_cmd),
             "ffmpeg -y -thread_queue_size 512 -r %d -i \"%s\" "
             "-c:v %s -b:v 10M -r %d -fps_mode passthrough -fflags +genpts "
             "-probesize 5000000 -analyzeduration 5000000 -threads 2 \"%s\"",
             fps, RAW_VIDEO, encoder, fps, ENCODED_VIDEO);

    printf("Executing: %s\n", ffmpeg_cmd);
    if(system(ffmpeg_cmd) == 0)
        printf("Transcoding complete! Video saved to: %s\n", ENCODED_VIDEO);
    else
        printf("Error during transcoding.\n");

    exit(0);
}

int main(int argc, char **argv) {
    int opt;
    static struct option long_options[] = {
        {"exposure",    required_argument, 0, 'e'},
        {"awb",         required_argument, 0, 'w'},
        {"focus",       required_argument, 0, 'f'},
        {"bitrate",     required_argument, 0, 'b'},
        {"resolution",  required_argument, 0, 'r'},
        {"fps",         required_argument, 0, 'p'},
        {"gain",        required_argument, 0, 'g'},
        {"level",       required_argument, 0, 'l'},
        {0, 0, 0, 0}
    };

    // Parse command-line arguments
    while ((opt = getopt_long(argc, argv, "e:w:f:b:r:p:g:l:", long_options, NULL)) != -1) {
        switch(opt) {
            case 'e': shutter = atoi(optarg); break;
            case 'w': strncpy(awb, optarg, sizeof(awb)-1); break;
            case 'f': lens_position = atof(optarg); break;
            case 'b': bitrate = atoi(optarg); break;
            case 'r': strncpy(resolution, optarg, sizeof(resolution)-1); break;
            case 'p': fps = atoi(optarg); break;
            case 'g': gain = atof(optarg); break;
            case 'l': strncpy(level_str, optarg, sizeof(level_str)-1); break;
            default:
                fprintf(stderr, "Usage: %s [--exposure microsec] [--awb mode] [--focus lens_position] [--bitrate value] [--resolution WxH] [--fps value] [--gain value] [--level value]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Parse resolution
    int width, height;
    if (sscanf(resolution, "%dx%d", &width, &height) != 2) {
        fprintf(stderr, "Invalid resolution format. Use WxH (e.g., 1920x1080).\n");
        exit(EXIT_FAILURE);
    }

    // Validate format
    if (!is_valid_format(width, height, fps)) {
        fprintf(stderr, "Invalid format: %dx%d @ %d FPS is NOT supported by Pi Camera Module 3.\n", width, height, fps);
        exit(EXIT_FAILURE);
    }

    // Print parameters
    printf("📷 Recording Parameters:\n");
    printf("  Exposure: %d microseconds\n", shutter);
    printf("  AWB: %s\n", awb);
    printf("  Focus: %.2f\n", lens_position);
    printf("  Bitrate: %d\n", bitrate);
    printf("  Resolution: %s\n", resolution);
    printf("  FPS: %d\n", fps);
    printf("  Gain: %.2f\n", gain);
    printf("  Level: %s\n", level_str);

    // Create output directory
    char mkdir_cmd[128];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", OUTPUT_DIR);
    system(mkdir_cmd);

    // Start libcamera-vid
    char libcamera_cmd[1024];
    snprintf(libcamera_cmd, sizeof(libcamera_cmd),
             "libcamera-vid --level %s --framerate %d --width %d --height %d --bitrate %d --profile high --intra 15 --denoise cdn_fast "
             "--awb %s --gain %.2f --shutter %d --autofocus-mode manual --lens-position %.2f -o \"%s\" -t 0 -n",
             level_str, fps, width, height, bitrate, awb, gain, shutter, lens_position, RAW_VIDEO);

    // Handle Ctrl+C
    signal(SIGINT, cleanup);

    // Start recording
    libcamera_pid = fork();
    if (libcamera_pid == 0) {
        execl("/bin/sh", "sh", "-c", libcamera_cmd, NULL);
        exit(EXIT_FAILURE);
    }

    waitpid(libcamera_pid, NULL, 0);
    cleanup(0);
    return 0;
}
