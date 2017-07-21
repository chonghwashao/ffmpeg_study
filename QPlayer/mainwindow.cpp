#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qplayer.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initialize();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initialize()
{
    m_audioRender.initOpenAL();
    m_audioRender.prepare();
    QPlayer *m_thPlayer = new QPlayer(m_audioRender);
    m_thPlayer->setFileName("D:/rtsp/Debug/Simpsons.mp4");
    connect(m_thPlayer,SIGNAL(sig_getOneFrame(QImage)),ui->widget,SLOT(slot_getOneFrame(QImage)));


    m_thPlayer->startPlay();
}
