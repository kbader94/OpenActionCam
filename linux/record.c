/*
 * record.c - Handles video recording and transcoding for linux_camera
 */

 #include "record.h"
 #include "error.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/wait.h>
 #include <sys/statvfs.h>
 
 #define OUTPUT_DIR "/home/pi/shared"
 #define RAW_VIDEO OUTPUT_DIR"/video.264"
 #define ENCODED_VIDEO OUTPUT_DIR"/video.mp4"
 #define MIN_FREE_SPACE_MB 500  /* Minimum required free space in MB */
 
 static volatile sig_atomic_t recording;
 static pid_t libcamera_pid;
 
 /*
  * get_available_space - Check available disk space in MB.
  *
  * Returns available space in MB, or -1 on failure.
  */
 static int get_available_space(void)
 {
     struct statvfs stat;
 
     if (statvfs(OUTPUT_DIR, &stat) != 0) {
         ERROR(50, "Failed to check available storage space.");
         return -1;
     }
 
     return (stat.f_bavail * stat.f_frsize) / (1024 * 1024); /* Convert to MB */
 }
 
 /*
  * check_camera_available - Check if a camera is connected.
  *
  * Uses libcamera-vid to list available cameras.
  *
  * Returns 1 if a camera is found, 0 otherwise.
  */
 static int check_camera_available(void)
 {
     FILE *fp;
     char buffer[256];
 
     fp = popen("libcamera-vid --list-cameras 2>&1", "r");
     if (!fp) {
         ERROR(60, "Failed to check camera availability.");
         return 0;
     }
 
     while (fgets(buffer, sizeof(buffer), fp)) {
         if (strstr(buffer, "no cameras available")) {
             pclose(fp);
             return 0;  /* Camera not detected */
         }
     }
 
     pclose(fp);
     return 1;  /* Camera detected */
 }
 
 /*
  * transcode - Transcodes the recorded video to a compressed format.
  */
 static void transcode(void)
 {
     printf("Starting transcoding...\n");
 
     char ffmpeg_cmd[1024];
     snprintf(ffmpeg_cmd, sizeof(ffmpeg_cmd),
              "ffmpeg -y -thread_queue_size 512 -r %d -i \"%s\" "
              "-c:v h264_v4l2m2m -b:v 10M -r %d -fps_mode passthrough "
              "-fflags +genpts -probesize 5000000 -analyzeduration 5000000 "
              "-threads 2 \"%s\"",
              30, RAW_VIDEO, 30, ENCODED_VIDEO);
 
     printf("Executing: %s\n", ffmpeg_cmd);
 
     if (system(ffmpeg_cmd) == 0)
         printf("Transcoding complete! Video saved to: %s\n", ENCODED_VIDEO);
     else
         ERROR(80, "Transcoding process failed.");
 }
 
 /*
  * start_record - Start video recording.
  * @params: Recording parameters.
  */
 void start_record(recording_params_t params)
 {
     int width, height, free_space;
 
     if (recording) {
         WARN("Already recording! Previous session may not have ended properly.");
         return;
     }
 
     if (!check_camera_available()) {
         ERROR(61, "No camera detected.");
         return;
     }
 
     free_space = get_available_space();
     if (free_space < 0)
         return; /* Error checking storage space */
 
     if (free_space < MIN_FREE_SPACE_MB) {
         ERROR(51, "Insufficient storage space! Only %dMB available.", free_space);
         return;
     }
 
     if (sscanf(params.resolution, "%dx%d", &width, &height) != 2) {
         ERROR(52, "Invalid resolution format.");
         return;
     }
 
     system("mkdir -p " OUTPUT_DIR);
 
     char libcamera_cmd[1024];
     snprintf(libcamera_cmd, sizeof(libcamera_cmd),
              "libcamera-vid --framerate %d --width %d --height %d "
              "--bitrate %d --awb %s --gain %.2f --shutter %d --lens-position %.2f -o \"%s\" -t 0 -n",
              params.fps, width, height, params.bitrate,
              params.awb, params.gain, params.shutter, params.lens_position,
              RAW_VIDEO);
 
     printf("Starting recording...\n");
     printf("Command: %s\n", libcamera_cmd);
 
     libcamera_pid = fork();
     if (libcamera_pid == 0) {
         execl("/bin/sh", "sh", "-c", libcamera_cmd, NULL);
         exit(EXIT_FAILURE);
     }
 
     sleep(2);
     int status;
     if (waitpid(libcamera_pid, &status, WNOHANG) > 0) {
         ERROR(70, "Recording failed to start.");
         recording = 0;
         return;
     }
 
     recording = 1;
 }
 
 /*
  * end_record - Stop recording if active, then transcode the video.
  */
 void end_record(void)
 {
     if (!recording) {
         WARN("Tried to stop recording, but no active recording found.");
         return;
     }
 
     printf("Stopping recording...\n");
 
     if (libcamera_pid > 0) {
         kill(libcamera_pid, SIGINT);
         waitpid(libcamera_pid, NULL, 0);
     }
 
     printf("Recording stopped. Flushing data...\n");
     system("sync");
 
     recording = 0;
     transcode();
 }
 