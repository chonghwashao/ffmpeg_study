#include "mainwindow.h"
#include <QApplication>
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
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

///现在我们需要做的是让SaveFrame函数能把RGB信息定稿到一个PPM格式的文件中。
///我们将生成一个简单的PPM格式文件，请相信，它是可以工作的。
void SaveFrame(AVFrame *pFrame, int width, int height,int index);
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //这里简单的输出一个版本号
    std::cout << "Hello FFmpeg!" << std::endl;
    av_register_all();
    avformat_network_init();
    unsigned version = avcodec_version();
    std::cout << "version is:" << version << std::endl;
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    int ret = -1;
    if (!pFormatCtx)
    {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
    }

    char *file_path = "D:/rtsp/Debug/Simpsons.mp4";
    ret = avformat_open_input(&pFormatCtx, file_path, NULL, NULL);
    ///循环查找视频中包含的流信息，直到找到视频类型的流
    ///便将其记录下来 保存到videoStream变量中
    ///这里我们现在只处理视频流  音频流先不管他
    unsigned int videoStream = -1;
	AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
	if (!pCodecCtx)
		return AVERROR(ENOMEM);

	
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
		if (!(ret = avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[i]->codecpar))
			&& pCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO)
		{
            videoStream = i;
            break;
        }
    }

    ///如果videoStream为-1 说明没有找到视频流
    if (videoStream == -1) {
        printf("Didn't find a video stream.");
    }

    ///查找解码器
    //AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	AVCodec * codec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (codec == NULL) {
        printf("Codec not found.\n");
    }

    ///打开解码器
	if (avcodec_open2(pCodecCtx, codec, NULL) < 0) {
        printf("Could not open codec.\n");
    }

    AVFrame *pFrame, *pFrameRGB;
    AVPacket *packet;
    uint8_t *out_buffer;
    int numBytes;
    static struct SwsContext *img_convert_ctx;

    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
               AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height,
               AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
    numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
	
    out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, out_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
    int y_size = pCodecCtx->width * pCodecCtx->height;
    packet = (AVPacket *)malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(packet, y_size); //分配packet的数据
    av_dump_format(pFormatCtx, 0, file_path, 0);

    int index = 0;
    while(1)
    {
        av_log(NULL, AV_LOG_INFO, "read frame %d\n", index);
        if(av_read_frame(pFormatCtx, packet) < 0)
        {
            //av_log(pFormatCtx, AV_LOG_INFO, "read frame error\n");
            break;
        }

        if (packet->stream_index == videoStream)
        {
            //ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,packet);
            avcodec_send_packet(pCodecCtx, packet);
			int errCode = avcodec_receive_frame(pCodecCtx, pFrame);

			switch (errCode) {
			case 0://成功
				printf("got a frame !\n");
				sws_scale(img_convert_ctx,
					(uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
					pFrameRGB->linesize);

				SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, index++); //保存图片
				break;

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
		av_packet_unref(packet);
        if(index >= 50)
            break;
    }

    av_free(out_buffer);
    av_free(pFrameRGB);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    MainWindow w;
    w.show();
    return a.exec();
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
