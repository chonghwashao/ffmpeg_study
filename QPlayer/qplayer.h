#ifndef QPLAYER_H
#define QPLAYER_H
#include <QtCore/QThread>
#include <QtGui/QImage>

class QPlayer : public QThread
{
    Q_OBJECT
public:
    explicit QPlayer();
    ~QPlayer() {}

    void setFileName(QString path){ m_fileName = path; }
    void startPlay();

protected:
    virtual void run();

signals:
    ///
    /// \brief sig_GetOneFrame Send signal a frame arrived
    ///
    void sig_getOneFrame(QImage);

private:
    QString m_fileName;
};


#endif // QPLAYER_H
