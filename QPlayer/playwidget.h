#ifndef PLAYWIDGET_H
#define PLAYWIDGET_H

#include <QWidget>
#include <QtGui/QImage>

namespace Ui {
class PlayWidget;
}

class PlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlayWidget(QWidget *parent = 0);
    ~PlayWidget();

public slots:
    void slot_getOneFrame(QImage img);

private:
    Ui::PlayWidget *ui;
    QImage mImage;

protected:
    void paintEvent(QPaintEvent * event);
};

#endif // PLAYWIDGET_H
