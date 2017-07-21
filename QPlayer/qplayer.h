#ifndef QPLAYER_H
#define QPLAYER_H
#include <QtCore/QThread>
#include <QtGui/QImage>

class QAudioRender;
struct AVCodecContext;
struct AVFrame;
struct AudioParams;

class QPlayer : public QThread
{
    Q_OBJECT
public:
    explicit QPlayer(QAudioRender &render);
    ~QPlayer() {}

    void setFileName(QString path){ m_fileName = path; }
    void startPlay();

    int audio_decode_frame(AVCodecContext *aCodecCtx, AVFrame *frame, AudioParams &aPara);

protected:
    virtual void run();

signals:
    ///
    /// \brief sig_GetOneFrame Send signal a frame arrived
    ///
    void sig_getOneFrame(QImage);

private:
    QString m_fileName;
    QAudioRender &m_audioRender;
};


#endif // QPLAYER_H
