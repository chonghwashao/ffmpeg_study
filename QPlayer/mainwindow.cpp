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
    QPlayer *m_thPlayer = new QPlayer;
    connect(m_thPlayer,SIGNAL(sig_getOneFrame(QImage)),ui->widget,SLOT(slot_getOneFrame(QImage)));
}
