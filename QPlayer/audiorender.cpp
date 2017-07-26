#include "audiorender.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern "C"
{
#include "libavutil/avutil.h"
}
#include "aldlist.h"
#include "qplayer.h"
#define SERVICE_UPDATE_PERIOD (20)

void ALFWprintf( const char* x, ... )
{
    va_list args;
    va_start( args, x );
    vprintf( x, args );
    va_end( args );
}

QAudioRender::QAudioRender(QPlayer & decoder)
    : m_pDevice(NULL)
    , ulFrequency(0)
    , ulFormat(0)
    , m_decoder(decoder)
{
    memset((void *)m_buffers, 0, sizeof(m_buffers));
}

ALboolean QAudioRender::initOpenAL()
{
    ALCdevice* pDevice;
    ALCcontext* pContext;

    pDevice = alcOpenDevice(NULL);
    pContext = alcCreateContext(pDevice, NULL);
    alcMakeContextCurrent(pContext);

    if (alcGetError(pDevice) != ALC_NO_ERROR)
        return AL_FALSE;

    return 0;
}

ALboolean QAudioRender::shutdownOpenAL()
{
    ALCcontext *pContext;
    ALCdevice *pDevice;

    pContext = alcGetCurrentContext();
    pDevice = alcGetContextsDevice(pContext);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(pContext);
    alcCloseDevice(pDevice);

    return AL_TRUE;
}

void QAudioRender::start()
{
//    if (!m_bStop)
//    {
//        Stop();
//    }
//    m_bStop = false;
    alGenSources(1, &m_source);
    if (alGetError() != AL_NO_ERROR)
    {
        printf("Error generating audio source.");
        return;
    }
    ALfloat SourcePos[] = { 0.0, 0.0, 0.0 };
    ALfloat SourceVel[] = { 0.0, 0.0, 0.0 };
    ALfloat ListenerPos[] = { 0.0, 0, 0 };
    ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
    // first 3 elements are "at", second 3 are "up"
    ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };
    alSourcef(m_source, AL_PITCH, 1.0);
    alSourcef(m_source, AL_GAIN, 1.0);
    alSourcefv(m_source, AL_POSITION, SourcePos);
    alSourcefv(m_source, AL_VELOCITY, SourceVel);
    alSourcef(m_source, AL_REFERENCE_DISTANCE, 50.0f);
    alSourcei(m_source, AL_LOOPING, AL_FALSE);
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    alListener3f(AL_POSITION, 0, 0, 0);

//    m_decoder = std::move(std::make_unique<AudioDecoder>());
//    m_decoder.reset(new AudioDecoder()); //for ios
//    m_decoder->OpenFile(path);
//    m_decoder->StartDecode();
    alGenBuffers(NUMBUFFERS, m_buffers);
    //m_ptPlaying = std::move(std::make_unique<std::thread>(&ALEngine::SoundPlayingThread, this));
//    m_ptPlaying.reset(new std::thread(&ALEngine::SoundPlayingThread, this));    //for ios

    QThread::start();
}

void QAudioRender::setParams()
{

}

void QAudioRender::run()
{
    //get frame
    for (int i = 0; i < NUMBUFFERS; i++)
    {
        soundCallback(m_buffers[i]);
    }
    play();

    //
    while (true)
    {
//        if (m_bStop)
//            break;
        //std::this_thread::sleep_for(std::chrono::milliseconds(SERVICE_UPDATE_PERIOD));
        msleep(SERVICE_UPDATE_PERIOD);
        ALint processed = 0;
        alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
        //printf("the processed is:%d\n", processed);
        while (processed > 0)
        {
            ALuint bufferID = 0;
            alSourceUnqueueBuffers(m_source, 1, &bufferID);
            soundCallback(bufferID);
            processed--;
        }
        play();
    }
}

int QAudioRender::soundCallback(ALuint &bufferID)
{
    PTFRAME frame = m_decoder.getFrame();
    if (frame == nullptr)
        return -1;
    ALenum fmt;
    if (frame->chs == 1)
    {
        fmt = AL_FORMAT_MONO16;
    }
    else
    {
        fmt = AL_FORMAT_STEREO16;
    }
    alBufferData(bufferID, fmt, frame->data, frame->size, frame->samplerate);
    alSourceQueueBuffers(m_source, 1, &bufferID);
    if (frame)
    {
        av_free(frame->data);
        delete frame;
    }
    return 0;
}

int QAudioRender::play()
{
    int state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state == AL_STOPPED || state == AL_INITIAL)
    {
        alSourcePlay(m_source);
    }

    return 0;
}

int QAudioRender::pausePlay()
{
    int state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state == AL_PAUSED)
    {
        alSourcePlay(m_source);
    }

    return 0;
}

int QAudioRender::pause()
{
    alSourcePause(m_source);
    return 0;
}

int QAudioRender::stop()
{
//    if(m_bStop)
//        return 0;
//    m_bStop = true;
    alSourceStop(m_source);
    alSourcei(m_source, AL_BUFFER, 0);
//    m_decoder.StopDecode();//要先把decoder stop,否则可能hang住 播放线程
//    if(m_ptPlaying->joinable())
//        m_ptPlaying->join();
    //
    alDeleteBuffers(NUMBUFFERS, m_buffers);
    alDeleteSources(1, &m_source);


    return 0;
}
