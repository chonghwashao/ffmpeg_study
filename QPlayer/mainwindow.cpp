#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qplayer.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_thPlayer(),
    m_audioRender(m_thPlayer)
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
    m_audioRender.start();

    m_thPlayer.setFileName("D:/Samples/Simpsons.mp4");
    connect(&m_thPlayer,SIGNAL(sig_getOneFrame(QImage)),ui->widget,SLOT(slot_getOneFrame(QImage)));

    m_thPlayer.startDecode();
}
