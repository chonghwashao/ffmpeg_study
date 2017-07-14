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

protected:
    virtual void run();

signals:
    ///
    /// \brief sig_GetOneFrame Send signal a frame arrived
    ///
    void sig_GetOneFrame(QImage);
};



#endif // QPLAYER_H
