#ifndef AUDIORENDER_H
#define AUDIORENDER_H
#include <al.h>
#include <alc.h>
#include <stdint.h>
#include <QtCore/QThread>
#define NUMBUFFERS 4 //音频缓冲个数
class QPlayer;
typedef struct _tFrame
{
    void* data;
    int size;
    int chs;
    int samplerate;
    uint64_t pts;
    uint64_t duration;
}TFRAME, *PTFRAME;


typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;


class QAudioRender : public QThread
{
public:
    explicit QAudioRender(QPlayer & decoder);

    static ALboolean initOpenAL();
    static ALboolean shutdownOpenAL();
    void start();
    void setParams();

protected:
    void run();
    int soundCallback(ALuint& bufferID);

    int play();
    int pausePlay();
    int pause();
    int stop();
public:
    ALCdevice      *m_pDevice;
    ALuint          m_source;
    ALuint		    m_buffers[NUMBUFFERS];
    unsigned long	ulFrequency;
    unsigned long	ulFormat;
    QPlayer & m_decoder;

};

#endif // AUDIORENDER_H
