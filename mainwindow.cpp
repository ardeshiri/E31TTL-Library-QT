#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    transceiver = new Transceiver{this,"COM15"};
    transceiver2 = new Transceiver{this,"COM6"};


    connect(transceiver,SIGNAL(transceiverReceivedLData(QByteArray)),this,SLOT(writeTr1DataOnScreen(QByteArray)));
   // connect(transceiver,SIGNAL(transceiverReceivedRisingInt()),this,SLOT(interruptReceived()));
  //  connect(transceiver,SIGNAL(transceiverReceivedFallingInt()),this,SLOT(interruptReceived()));
    connect(transceiver,SIGNAL(transceiverReceivedNV(QByteArray)),this,SLOT(writeTr1DataOnScreen(QByteArray)));
    connect(transceiver,SIGNAL(pingRequestRespSent(QByteArray)),this,SLOT(writeTr1DataOnScreen(QByteArray)));
    connect(transceiver,SIGNAL(auxChange(bool)),this,SLOT(auxstate(bool)));


    connect(transceiver2,SIGNAL(transceiverReceivedLData(QByteArray)),this,SLOT(writeTr2DataOnScreen(QByteArray)));
   // connect(transceiver2,SIGNAL(transceiverReceivedRisingInt()),this,SLOT(interruptReceived2()));
   // connect(transceiver2,SIGNAL(transceiverReceivedFallingInt()),this,SLOT(interruptReceived2()));
  //  connect(transceiver2,SIGNAL(transceiverReceivedNV(QByteArray)),this,SLOT(writeTr2DataOnScreen(QByteArray)));
    connect(transceiver2,SIGNAL(transceiverModeChanged(int)),this,SLOT(cmodeSet2(int)));
    connect(transceiver2,SIGNAL(pingRequestRespReceived(int)),this,SLOT(cmodeSet2(int)));
    connect(transceiver2,SIGNAL(pingRequestTimeout(QString)),ui->textEdit_4,SLOT(append(QString)));

    connect(transceiver2,SIGNAL(outputBufferOutput(QByteArray)),this,SLOT(writeTr2DataOnScreen(QByteArray)));


    connect(this,SIGNAL(writeScrToTr1(QByteArray)),transceiver,SLOT(sendingByteArraySizeCheck(QByteArray)));
    connect(this,SIGNAL(writeScrToTr2(QByteArray)),transceiver2,SLOT(sendingByteArraySizeCheck(QByteArray)));

    //connect(transceiver2,SIGNAL(settingPacketReceived(QByteArray)),this,SLOT(writeTr2DataOnScreen(QByteArray)));

    connect(transceiver,SIGNAL(wroteBytesOnTransceiver(int)),ui->lcdNumber,SLOT(display(int)));

    setwin = new Setting{this};

}



MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::writeTr1DataOnScreen(QByteArray qb)
{
    ui->textEdit_2->append(QString::fromStdString(qb.toStdString()));
}



void MainWindow::writeTr2DataOnScreen(QByteArray wt)
{
    ui->textEdit_4->append(QString::fromStdString(wt.toStdString()));
}


void MainWindow::on_pushButton_clicked()
{
    transceiver->setReceiverChannel(QByteArray::fromHex(ui->t1channel->text().toLatin1()));
    transceiver->setReceiverAddress(QByteArray::fromHex(ui->t1address->text().toLatin1()));
    emit writeScrToTr1(QByteArray::fromStdString(ui->textEdit->toPlainText().toStdString()));
}



void MainWindow::on_pushButton_2_clicked()
{
    transceiver2->setReceiverChannel(QByteArray::fromHex(ui->t2channel->text().toLatin1()));
    transceiver2->setReceiverAddress(QByteArray::fromHex(ui->t2address->text().toLatin1()));
    ui->textEdit_4->append(QByteArray::fromHex(ui->t2channel->text().toLatin1()).toHex());
    emit writeScrToTr2(QByteArray::fromStdString(ui->textEdit_3->toPlainText().toStdString()));
}



void MainWindow::interruptReceived()
{
    ui->textEdit_2->append("Interupt!");
}



void MainWindow::interruptReceived2()
{
    ui->textEdit_4->append("Interupt!");
}


void MainWindow::on_pushButton_3_clicked()
{
    setwin->show();///////////////
}

void MainWindow::cmodeSet2(int cms)
{
    ui->cmode2->setText(QString::number(cms));
}

void MainWindow::on_pingbutton2_clicked()
{
    transceiver2->setReceiverChannel(QByteArray::fromHex(ui->t2channel->text().toLatin1()));
    transceiver2->setReceiverAddress(QByteArray::fromHex(ui->t2address->text().toLatin1()));
    transceiver2->pingRequest(20);
}

void MainWindow::on_pushButton_4_clicked()
{
    transceiver->writeDataToInputBuffer(QByteArray::fromStdString(ui->textEdit->toPlainText().toStdString()));
}

void MainWindow::on_pushButton_5_clicked()
{
    transceiver2->readOutputBufferedData();
}



void MainWindow::auxstate(bool as)
{
    if(as == true)
        ui->textEdit_2->append("aux high!");
    if(as == false)
        ui->textEdit_2->append("aux low!");
}

void MainWindow::on_textEdit_textChanged()
{
    ui->lineEdit->setText((QString::number(ui->textEdit->toPlainText().length())));
}
