#ifndef RECORD_H
#define RECORD_H

#include <stdbool.h>

typedef struct {
    int shutter;
    char awb[64];
    double lens_position;
    int bitrate;
    char resolution[32];
    int fps;
    double gain;
    char level[8];
    char encoder[32];
} recording_params_t;

void start_record(recording_params_t params);
bool is_recording();
void end_record();

#endif /* RECORD_H */
