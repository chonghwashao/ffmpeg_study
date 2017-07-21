#include "audiorender.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "aldlist.h"

void ALFWprintf( const char* x, ... )
{
    va_list args;
    va_start( args, x );
    vprintf( x, args );
    va_end( args );
}

QAudioRender::QAudioRender() : m_pDevice(NULL), ulFrequency(0), ulFormat(0)
{
    memset((void *)uiBuffers, 0, sizeof(uiBuffers));
}

ALboolean QAudioRender::initOpenAL()
{
    ALDeviceList *pDeviceList = NULL;
    ALCcontext *pContext = NULL;
    ALCdevice *pDevice = NULL;
    ALint i;
    ALboolean bReturn = AL_FALSE;

    pDeviceList = new ALDeviceList();
    if ((pDeviceList) && (pDeviceList->GetNumDevices()))
    {
        //ALFWprintf("\nSelect OpenAL Device:\n");
        for (i = 0; i < pDeviceList->GetNumDevices(); i++)
            ALFWprintf("%d. %s%s\n", i + 1, pDeviceList->GetDeviceName(i), i == pDeviceList->GetDefaultDevice() ? "(DEFAULT)" : "");


        ///默认使用第一个设备输出
        pDevice = alcOpenDevice(pDeviceList->GetDeviceName(0));
        if (pDevice)
        {
            pContext = alcCreateContext(pDevice, NULL);
            if (pContext)
            {
                //ALFWprintf("\nOpened %s Device\n", alcGetString(pDevice, ALC_DEVICE_SPECIFIER));
                alcMakeContextCurrent(pContext);
                bReturn = AL_TRUE;
            }
            else
            {
                alcCloseDevice(pDevice);
            }
        }

        delete pDeviceList;
    }

    return bReturn;
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

bool QAudioRender::prepare()
{
    // Generate some AL Buffers for streaming
    alGenBuffers( NUMBUFFERS, uiBuffers );

    // Generate a Source to playback the Buffers
    alGenSources( 1, &uiSource );
    return true;
}

void QAudioRender::setParams()
{

}
