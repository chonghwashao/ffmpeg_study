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
#include "libswresample/swresample.h"
#include "libavutil/time.h"
}

#include "audiorender.h"

#define CPP_TIME_BASE_Q (AVRational{1, AV_TIME_BASE})

static float newVolume = 1.0f;

static int decoder_decode_frame(AVCodec *d, AVFrame *frame, AVSubtitle *sub);

///现在我们需要做的是让SaveFrame函数能把RGB信息定稿到一个PPM格式的文件中。
///我们将生成一个简单的PPM格式文件，请相信，它是可以工作的。
void SaveFrame(AVFrame *pFrame, int width, int height,int index);

const int QPlayer::MAX_BUFF_SIZE = 128;
QPlayer::QPlayer()
    : m_outSampleRate(44100)
    , m_outChs(2)
{

}

QPlayer::~QPlayer()
{
    m_cond.notify_all();
//    if(m_pDecode->joinable())
//        m_pDecode->join();
    std::unique_lock<std::mutex> lck(m_mtx);
    int queueSize = m_queueData.size();
    for (int i = queueSize - 1; i >= 0; i--)
    {
        PTFRAME f = m_queueData.front();
        m_queueData.pop();
        if (f)
        {
            if (f->data)
                av_free(f->data);
            delete f;
        }
    }
}

void QPlayer::startPlay()
{
    QThread::start();
}

PTFRAME QPlayer::getFrame()
{
    PTFRAME frame = nullptr;
    std::unique_lock<std::mutex> lck(m_mtx);
    ///C++11 features
    m_cond.wait(lck, [this]() {return m_queueData.size() > 0; });
    if (true)
    {
        frame = m_queueData.front();
        m_queueData.pop();
    }
    lck.unlock();
    m_cond.notify_one();
    return frame;
}

void QPlayer::run()
{
    std::string file_path = m_fileName.toLocal8Bit().data();
    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    int ret = -1;
    if (!pFormatCtx)
    {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        return;
    }

    ret = avformat_open_input(&pFormatCtx, file_path.c_str(), NULL, NULL);
    if(ret != 0)
    {
        av_log(NULL, AV_LOG_FATAL, "Open input failed! errCode = %d\n", ret);
        return;
    }

    ///循环查找视频中包含的流信息，直到找到视频类型的流
    ///便将其记录下来 保存到videoStream变量中
    ///这里我们现在只处理视频流  音频流先不管他
    int videoStream(-1), audioStream(-1);
    AVCodecContext *vCodecCtx = avcodec_alloc_context3(NULL);
    AVCodecContext *aCodecCtx = avcodec_alloc_context3(NULL);
    if (!vCodecCtx || !aCodecCtx)
    {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        return/* AVERROR(ENOMEM)*/;
    }

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
    }

    ///如果videoStream为-1 说明没有找到视频流
    if (videoStream == -1) {
        av_log(NULL, AV_LOG_FATAL, "Could not find video stream.\n");
        return;
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

    ///Set audio params
//    AudioParams para;
//    para.channels = aCodecCtx->channels;
//    para.fmt = aCodecCtx->sample_fmt;
//    para.freq = aCodecCtx->sample_rate;
//    para.channel_layout = aCodecCtx->channel_layout;

    int64_t in_channel_layout = av_get_default_channel_layout(aCodecCtx->channels);
    struct SwrContext* au_convert_ctx = swr_alloc();
    int64_t outLayout;
    if (m_outChs == 1)
    {
        outLayout = AV_CH_LAYOUT_MONO;
    }
    else
    {
        outLayout = AV_CH_LAYOUT_STEREO;
    }

    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx,
                                        outLayout, AV_SAMPLE_FMT_S16,
                                        m_outSampleRate, in_channel_layout,
                                        aCodecCtx->sample_fmt, aCodecCtx->sample_rate, 0,
                                        NULL);
    swr_init(au_convert_ctx);

    ///Find video codec
    //AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodec * vCodec = avcodec_find_decoder(vCodecCtx->codec_id);
    if (vCodec == NULL) {
        printf("vCodec not found.\n");
    }

    ///Open codec
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
        static bool isQuit = false;
        if(isQuit)
            break;

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
                sws_scale(img_convert_ctx,
                    (uint8_t const * const *)pFrame->data,
                    pFrame->linesize, 0, vCodecCtx->height, pFrameRGB->data,
                    pFrameRGB->linesize);

                QImage tmpImg((uchar *)out_buffer,vCodecCtx->width,vCodecCtx->height,QImage::Format_RGB888);
                QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
                emit sig_getOneFrame(image);  //发送信号
                break;
            }
            case AVERROR_EOF:
                //printf("the decoder has been fully flushed,and there will be no more output frames.\n");
                break;

            case AVERROR(EAGAIN):
                //printf("Resource temporarily unavailable\n");
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
            switch (errCode)
            {
                case 0:
                {
                    PTFRAME frame = new TFRAME;
                    frame->chs = m_outChs;
                    frame->samplerate = m_outSampleRate;
                    frame->duration = av_rescale_q(packet->duration, pFormatCtx->streams[audioStream]->time_base, CPP_TIME_BASE_Q);
                    frame->pts = av_rescale_q(packet->pts, pFormatCtx->streams[audioStream]->time_base, CPP_TIME_BASE_Q);

                    //resample
                    int outSizeCandidate = m_outSampleRate * 8 *
                        double(frame->duration) / 1000000.0;
                    uint8_t* convertData = (uint8_t*)av_malloc(sizeof(uint8_t)* outSizeCandidate);
                    int out_samples = swr_convert(au_convert_ctx,
                        &convertData, outSizeCandidate,
                        (const uint8_t**)&pFrame->data[0], pFrame->nb_samples);
                    int Audiobuffer_size = av_samples_get_buffer_size(NULL,
                        m_outChs, out_samples, AV_SAMPLE_FMT_S16, 1);
                    frame->data = convertData;
                    frame->size = Audiobuffer_size;
                    std::unique_lock<std::mutex> lck(m_mtx);
                    m_cond.wait(lck, [this](){return m_queueData.size() < MAX_BUFF_SIZE;});
//                    if (m_bStop)
//                    {
//                        av_packet_unref(packet);
//                        break;
//                    }
//                    if (m_bSeeked)
//                    {
//                        m_bSeeked = false;
//                        av_packet_unref(packet);
//                        continue;
//                    }
                    m_queueData.push(frame);
                    lck.unlock();
                    m_cond.notify_one();
                }
                case AVERROR_EOF:
//                    printf("the decoder has been fully flushed,and there will be no more output frames.\n");
                    break;

                case AVERROR(EAGAIN):
//                    printf("Resource temporarily unavailable\n");
                    break;

                case AVERROR(EINVAL):
//                    printf("Invalid argument\n");
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


int QPlayer::decoder_decode_frame(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, AVSubtitle *sub) {
    int ret = AVERROR(EAGAIN);

    for (;;) {
        AVPacket _pkt;

//        if (codecCtx->queue->serial == codecCtx->pkt_serial) {
        do {

            switch (avctx->codec_type) {
                case AVMEDIA_TYPE_VIDEO:
                    ret = avcodec_receive_frame(avctx, frame);
//                    if (ret >= 0) {
//                        if (decoder_reorder_pts == -1) {
//                            frame->pts = frame->best_effort_timestamp;
//                        } else if (!decoder_reorder_pts) {
//                            frame->pts = frame->pkt_dts;
//                        }
//                    }

                    break;
                case AVMEDIA_TYPE_AUDIO:
                    ret = avcodec_receive_frame(avctx, frame);
//                    if (ret >= 0) {
//                        AVRational tb = (AVRational){1, frame->sample_rate};
//                        if (frame->pts != AV_NOPTS_VALUE)
//                            frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(avctx->avctx), tb);
//                        else if (avctx->next_pts != AV_NOPTS_VALUE)
//                            frame->pts = av_rescale_q(avctx->next_pts, avctx->next_pts_tb, tb);
//                        if (frame->pts != AV_NOPTS_VALUE) {
//                            avctx->next_pts = frame->pts + frame->nb_samples;
//                            avctx->next_pts_tb = tb;
//                        }
//                    }
                    break;
            }
            if (ret == AVERROR_EOF) {
                // avctx->finished = avctx->pkt_serial;
                avcodec_flush_buffers(avctx);
                return 0;
            }
            if (ret >= 0)
                return 1;
        } while (ret != AVERROR(EAGAIN));
//        }

//        do {
//            if (avctx->queue->nb_packets == 0)
//                SDL_CondSignal(avctx->empty_queue_cond);
//            if (avctx->packet_pending) {
//                av_packet_move_ref(&pkt, &avctx->pkt);
//                avctx->packet_pending = 0;
//            } else {
//                if (packet_queue_get(avctx->queue, &pkt, 1, &avctx->pkt_serial) < 0)
//                    return -1;
//            }
//        } while (avctx->queue->serial != avctx->pkt_serial);

//        if (pkt.data == flush_pkt.data) {
//            avcodec_flush_buffers(avctx->avctx);
//            avctx->finished = 0;
//            avctx->next_pts = avctx->start_pts;
//            avctx->next_pts_tb = avctx->start_pts_tb;
//        } else {
            if (avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                int got_frame = 0;
                ret = avcodec_decode_subtitle2(avctx, sub, &got_frame, &_pkt);
                if (ret < 0) {
                    ret = AVERROR(EAGAIN);
                } else {
                    if (got_frame && !_pkt.data) {
//                       avctx->packet_pending = 1;
                       av_packet_move_ref(pkt, &_pkt);
                    }
                    ret = got_frame ? 0 : (_pkt.data ? AVERROR(EAGAIN) : AVERROR_EOF);
                }
            } else {
                if (avcodec_send_packet(avctx, &_pkt) == AVERROR(EAGAIN)) {
                    av_log(avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
//                    avctx->packet_pending = 1;
                    av_packet_move_ref(pkt, &_pkt);
                }
            }
            av_packet_unref(&_pkt);
//        }
    }
}
