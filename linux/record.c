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
 #include <pthread.h>
 #include <fcntl.h>

 #define OUTPUT_DIR "/home/pi/shared"
 #define RAW_VIDEO OUTPUT_DIR"/video.264"
 #define ENCODED_VIDEO OUTPUT_DIR"/video.mp4"
 #define MIN_FREE_SPACE_MB 500  /* Minimum required free space in MB */
 
 static volatile sig_atomic_t recording;
 static pid_t libcamera_pid;
 static pthread_t stderr_thread;
 static int stderr_fd = -1;

 static void *stderr_monitor_thread(void *arg)
{
    char buffer[256];
    ssize_t n;

    while ((n = read(stderr_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        if (strstr(buffer, "no cameras available")) {
            ERROR(ERR_CAMERA_NOT_FOUND);
            kill(libcamera_pid, SIGINT);
            recording = 0;
            break;
        }

        // You can log this if needed
        // fprintf(stderr, "[libcamera] %s", buffer);
    }

    close(stderr_fd);
    return NULL;
}
 
 
 /*
  * get_available_space - Check available disk space in MB.
  *
  * Returns available space in MB, or -1 on failure.
  */
 static int get_available_space(void)
 {
     struct statvfs stat;
 
     if (statvfs(OUTPUT_DIR, &stat) != 0) {
         ERROR(ERR_INSUFFICIENT_SPACE);
         return -1;
     }
 
     return (stat.f_bavail * stat.f_frsize) / (1024 * 1024); /* Convert to MB */
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
         ERROR(ERR_TRANSCODE_FAILED);
 }
 
 /*
  * start_record - Start video recording.
  * @params: Recording parameters.
  */

  void start_record(recording_params_t params)
  {
      int width, height;
      int pipefd[2];
      int status;
  
      if (recording) {
          WARN("Already recording!");
          return;
      }
  
      if ((get_available_space() < MIN_FREE_SPACE_MB)) {
          ERROR(ERR_INSUFFICIENT_SPACE);
          return;
      }
  
      if (sscanf(params.resolution, "%dx%d", &width, &height) != 2) {
          ERROR(ERR_INVALID_RESOLUTION);
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
  
      if (pipe(pipefd) == -1) {
          ERROR(ERR_PIPE_CREATION_FAILED);
          return;
      }
  
      libcamera_pid = fork();
      if (libcamera_pid == 0) {
          close(pipefd[0]); // Close read end
          dup2(pipefd[1], STDERR_FILENO);
          close(pipefd[1]);
  
          execl("/bin/sh", "sh", "-c", libcamera_cmd, NULL);
          perror("execl");
          exit(EXIT_FAILURE);
      }
  
      // Parent
      close(pipefd[1]);
      stderr_fd = pipefd[0];
  
      // Launch monitor thread
      if (pthread_create(&stderr_thread, NULL, stderr_monitor_thread, NULL) != 0) {
          ERROR(ERR_MONITOR_THREAD_FAILED);
          kill(libcamera_pid, SIGINT);
          waitpid(libcamera_pid, NULL, 0);
          return;
      }
  
      sleep(2); // Let it run briefly
  
      if (waitpid(libcamera_pid, &status, WNOHANG) > 0) {
          ERROR(ERR_RECORD_START_FAILED);
          return;
      }
  
      recording = 1;
      DEBUG_MESSAGE("Recording started successfully.");
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
 
     kill(libcamera_pid, SIGINT);
     waitpid(libcamera_pid, NULL, 0);
     pthread_join(stderr_thread, NULL);
 
     printf("Recording stopped. Flushing data...\n");
     system("sync");
 
     recording = 0;
     transcode();
     
 }
 