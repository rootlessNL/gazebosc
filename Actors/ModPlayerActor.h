#ifndef MODPLAYERACTOR_H
#define MODPLAYERACTOR_H

#include "libsphactor.hpp"
#include "SDL_audio.h"

extern "C" {
#include "hxcmod.h"
}

#define SAMPLERATE 48000
#define NBSTEREO16BITSAMPLES 16384

struct ModPlayerActor : Sphactor
{
    ModPlayerActor() {}

    zmsg_t *
    handleInit(sphactor_event_t *ev);

    zmsg_t *
    handleAPI(sphactor_event_t *ev);

    zmsg_t *
    handleTimer(sphactor_event_t *ev);

    zmsg_t *
    handleStop(sphactor_event_t *ev);

    int queueAudio();
    void mixAudio(Uint8 *stream, int len);
    zmsg_t *getPatternEventMsg();

    uint32_t *buffer_dat;
    modcontext modctx;
    unsigned char * modfile;
    tracker_buffer_state trackbuf_state1;
    SDL_AudioDeviceID audiodev = -1;
    bool playing = false;
};

#endif // MODPLAYERACTOR_H
