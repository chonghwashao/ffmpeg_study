#include "playwidget.h"
#include "ui_playwidget.h"
#include <QtGui/QPainter>

PlayWidget::PlayWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayWidget)
{
    ui->setupUi(this);
}

PlayWidget::~PlayWidget()
{
    delete ui;
}

void PlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, this->width(), this->height()); //先画成黑色

    if (mImage.size().width() <= 0) return;

    ///将图像按比例缩放成和窗口一样大小
    QImage img = mImage.scaled(this->size(), Qt::IgnoreAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;

    painter.drawImage(QPoint(x,y),img); //画出图像
}

void PlayWidget::slot_getOneFrame(QImage img)
{
    mImage = img;
    update();
}
