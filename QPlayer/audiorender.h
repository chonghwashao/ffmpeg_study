#ifndef AUDIORENDER_H
#define AUDIORENDER_H
#include <al.h>
#include <alc.h>
#include <stdint.h>
#define NUMBUFFERS 4 //音频缓冲个数

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;


class QAudioRender
{
public:
    QAudioRender();

    static ALboolean initOpenAL();
    static ALboolean shutdownOpenAL();
    bool prepare();
    void setParams();
public:
    ALCdevice      *m_pDevice;
    ALuint          uiSource;
    ALuint		    uiBuffers[NUMBUFFERS];
    unsigned long	ulFrequency;
    unsigned long	ulFormat;

};

#endif // AUDIORENDER_H
