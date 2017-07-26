#ifndef QPLAYER_H
#define QPLAYER_H
#include <QtCore/QThread>
#include <QtGui/QImage>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "audiorender.h"

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVSubtitle;
struct AudioParams;

class QPlayer : public QThread
{
    Q_OBJECT
public:
    explicit QPlayer();
    ~QPlayer();

    void setFileName(QString path){ m_fileName = path; }
    void startPlay();

    PTFRAME getFrame();

protected:
    virtual void run();
    bool isFull() { return m_queueData.size() >= MAX_BUFF_SIZE; }

    int decoder_decode_frame(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, AVSubtitle *sub);

signals:
    ///
    /// \brief sig_GetOneFrame Send signal a frame arrived
    ///
    void sig_getOneFrame(QImage);

private:
    QString m_fileName;

    std::mutex m_mtx;
    std::condition_variable m_cond;
    std::queue<PTFRAME> m_queueData;
    int m_outSampleRate;
    int m_outChs;
    std::atomic_bool m_isShutdown;
    static const int MAX_BUFF_SIZE;

};


#endif // QPLAYER_H
