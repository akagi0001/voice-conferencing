#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H

#define AUDIO_INPUT_N   "default"
#define SAMPLE_RATE     44100
#define AUDIO_PACKET_S  1024

#include <pthread.h>
#include <stdbool.h>

#include "../audio_packet.h"

extern pthread_t capture_t;
extern pthread_mutex_t udp_port_lock;
extern bool stop_capture;

// The packet to play
extern AudioPacket audiopacket;

void setup_capture();

void open_capture();

void close_capture();

#endif /* AUDIO_INPUT_H */

