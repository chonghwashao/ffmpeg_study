#include "qplayer.h"
#include <QtCore/QDebug>

///由于我们建立的是C++的工程
///编译的时候使用的C++的编译器编译
///而FFMPEG是C的库
///因此这里需要加上extern "C"
///否则会提示各种未定义
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/pixfmt.h"
#include "libavutil/imgutils.h"
}

#include "audiorender.h"

static float newVolume = 1.0f;

static int decoder_decode_frame(AVCodec *d, AVFrame *frame, AVSubtitle *sub);

///现在我们需要做的是让SaveFrame函数能把RGB信息定稿到一个PPM格式的文件中。
///我们将生成一个简单的PPM格式文件，请相信，它是可以工作的。
void SaveFrame(AVFrame *pFrame, int width, int height,int index);


QPlayer::QPlayer(QAudioRender &render) : m_audioRender(render)
{

}

void QPlayer::startPlay()
{
    QThread::start();
}

void QPlayer::run()
{
    //char *file_path = m_fileName.toLocal8Bit().data();
    char *file_path = "D:/rtsp/Debug/Simpsons.mp4";
    //这里简单的输出一个版本号
//    std::cout << "Hello FFmpeg!" << std::endl;
    av_register_all();
//    avformat_network_init();
//    unsigned version = avcodec_version();
//    std::cout << "version is:" << version << std::endl;

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    int ret = -1;
    if (!pFormatCtx)
    {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
    }

    ret = avformat_open_input(&pFormatCtx, file_path, NULL, NULL);
    ///循环查找视频中包含的流信息，直到找到视频类型的流
    ///便将其记录下来 保存到videoStream变量中
    ///这里我们现在只处理视频流  音频流先不管他
    int videoStream(-1), audioStream(-1);
    AVCodecContext *vCodecCtx = avcodec_alloc_context3(NULL);
    AVCodecContext *aCodecCtx = avcodec_alloc_context3(NULL);
    if (!vCodecCtx || !aCodecCtx)
        return/* AVERROR(ENOMEM)*/;

    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        ///或者使用接口avcodec_parameters_to_context填充到context中使用
        AVMediaType codec_type = pFormatCtx->streams[i]->codecpar->codec_type;
        if(codec_type== AVMEDIA_TYPE_VIDEO && videoStream < 0)
        {
            videoStream = i;
            ret = avcodec_parameters_to_context(vCodecCtx, pFormatCtx->streams[i]->codecpar);
        }
        else if(codec_type == AVMEDIA_TYPE_AUDIO && audioStream < 0)
        {
            audioStream = i;
            ret = avcodec_parameters_to_context(aCodecCtx, pFormatCtx->streams[i]->codecpar);
        }
        else if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {

        }
//        if (!(ret = avcodec_parameters_to_context(pTempCtx, pFormatCtx->streams[i]->codecpar)))
//        {
//            if(pTempCtx->codec_type == AVMEDIA_TYPE_VIDEO && videoStream == -1)
//            {
//                *pCodecCtx = *pTempCtx;
//                videoStream = i;
//                break;
//            }
//            else if(pCodecCtx->codec_type == AVMEDIA_TYPE_AUDIO && audioStream == -1)
//                audioStream = i;
//            else if(pCodecCtx->codec_type == AVMEDIA_TYPE_SUBTITLE)
//                ;
//        }
    }

    ///如果videoStream为-1 说明没有找到视频流
    if (videoStream == -1) {
        printf("Didn't find a video stream.");
    }

    ///查找解码器
    AVCodec * aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if(aCodec == NULL)
    {
        printf("aCodec not found.\n");
    }

    if (avcodec_open2(aCodecCtx, aCodec, NULL) < 0) {
        printf("Could not open codec.\n");
    }

    ///设置音频参数
    AudioParams para;
    para.channels = aCodecCtx->channels;
    para.fmt = aCodecCtx->sample_fmt;
    para.freq = aCodecCtx->sample_rate;
    para.channel_layout = aCodecCtx->channel_layout;

    ///查找视频解码器
    //AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodec * vCodec = avcodec_find_decoder(vCodecCtx->codec_id);
    if (vCodec == NULL) {
        printf("vCodec not found.\n");
    }

    ///打开解码器
    if (avcodec_open2(vCodecCtx, vCodec, NULL) < 0) {
        printf("Could not open codec.\n");
    }

    AVFrame *pFrame, *pFrameRGB;
    AVPacket *packet;
    uint8_t *out_buffer;
    int numBytes;
    static struct SwsContext *img_convert_ctx;

    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    img_convert_ctx = sws_getContext(vCodecCtx->width, vCodecCtx->height,
               AV_PIX_FMT_YUV420P, vCodecCtx->width, vCodecCtx->height,
               AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, vCodecCtx->width, vCodecCtx->height, 1);

    out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, out_buffer, AV_PIX_FMT_RGB24, vCodecCtx->width, vCodecCtx->height, 1);
    int y_size = vCodecCtx->width * vCodecCtx->height;
    packet = (AVPacket *)malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(packet, y_size); //分配packet的数据
    //av_dump_format(pFormatCtx, 0, file_path, 0);

    while(1)
    {
        if(av_read_frame(pFormatCtx, packet) < 0)
        {
            //av_log(pFormatCtx, AV_LOG_INFO, "read frame error\n");
            break;
        }

        if (packet->stream_index == videoStream)
        {
            avcodec_send_packet(vCodecCtx, packet);
            int errCode = avcodec_receive_frame(vCodecCtx, pFrame);

            switch (errCode) {
            case 0://成功
            {
                //printf("got a frame !\n");
                sws_scale(img_convert_ctx,
                    (uint8_t const * const *)pFrame->data,
                    pFrame->linesize, 0, vCodecCtx->height, pFrameRGB->data,
                    pFrameRGB->linesize);

                //SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, index++); //保存图片
                //把这个RGB数据 用QImage加载
                QImage tmpImg((uchar *)out_buffer,vCodecCtx->width,vCodecCtx->height,QImage::Format_RGB888);
                QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
                emit sig_getOneFrame(image);  //发送信号
                break;
            }
            case AVERROR_EOF:
                printf("the decoder has been fully flushed,and there will be no more output frames.\n");
                break;

            case AVERROR(EAGAIN):
                printf("Resource temporarily unavailable\n");
                break;

            case AVERROR(EINVAL):
                printf("Invalid argument\n");
                break;
            default:
                break;
            }
        }
        else if(packet->stream_index == audioStream)
        {
            avcodec_send_packet(aCodecCtx, packet);
            int errCode = avcodec_receive_frame(aCodecCtx, pFrame);
            switch (errCode) {
            case 0://成功
            {
                audio_decode_frame(aCodecCtx, pFrame, para);
                break;
            }
            case AVERROR_EOF:
                printf("the decoder has been fully flushed,and there will be no more output frames.\n");
                break;

            case AVERROR(EAGAIN):
                printf("Resource temporarily unavailable\n");
                break;

            case AVERROR(EINVAL):
                printf("Invalid argument\n");
                break;
            default:
                break;
            }
        }
        msleep(5);
        av_packet_unref(packet);

    }

    av_free(out_buffer);
    av_free(pFrameRGB);
    avcodec_close(vCodecCtx);
    avcodec_close(aCodecCtx);
    avformat_close_input(&pFormatCtx);
}

void SaveFrame(AVFrame *pFrame, int width, int height,int index)
{

  FILE *pFile;
  char szFilename[32];
  int  y;

  // Open file
  sprintf(szFilename, "frame%d.ppm", index);
  pFile=fopen(szFilename, "wb");

  if(pFile==NULL)
    return;

  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // Write pixel data
  for(y=0; y<height; y++)
  {
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  }

  // Close file
  fclose(pFile);
}


//static int QPlayer::decoder_decode_frame(AVCodec *d, AVFrame *frame, AVSubtitle *sub)
//{
//    Q_UNUSED(sub);
//    return 0;
//}

int QPlayer::audio_decode_frame(AVCodecContext *aCodecCtx, AVFrame *frame, AudioParams &aPara)
{
    Q_UNUSED(aCodecCtx);
    int data_size, resampled_data_size;
    int64_t dec_channel_layout;
    av_unused double audio_clock0;
    int wanted_nb_samples;
    //Frame *af = frame;

    data_size = av_samples_get_buffer_size(NULL, frame->channels,
                                             frame->nb_samples,
                                             (AVSampleFormat)frame->format, 1);

    dec_channel_layout =
      (frame->channel_layout && frame->channels == av_get_channel_layout_nb_channels(frame->channel_layout)) ?
      frame->channel_layout : av_get_default_channel_layout(frame->channels);
    //wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);
    uint8_t *audio_buf = frame->data[0];
    resampled_data_size = data_size;

    ALint iBuffersProcessed, iTotalBuffersProcessed, iQueuedBuffers;
    //static bool bCached = false;
    static int iBufferded = 0;
    ALint iState;
    alGetSourcei(m_audioRender.uiSource, AL_SOURCE_STATE, &iState);

    if(iBufferded < 4)
    {
        alBufferData(m_audioRender.uiBuffers[iBufferded], aPara.fmt, audio_buf, resampled_data_size, aPara.freq);
        printf("@1errCode = %d\n", alGetError());
        alSourceQueueBuffers(m_audioRender.uiSource, 1, &m_audioRender.uiBuffers[iBufferded]);
        printf("@2errCode = %d\n", alGetError());
        iBufferded++;
    }

    if(iBufferded >= 4)
    {
        // Start playing source
        alSourcef(m_audioRender.uiSource, AL_GAIN, newVolume);
        alSourcePlay(m_audioRender.uiSource);
        printf("@3errCode = %d\n", alGetError());
    }

    return 0;
}
