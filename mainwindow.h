#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <transceiver.h>
#include "setting.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Transceiver* transceiver;
    Transceiver* transceiver2;
    Setting* setwin;

signals:
    void writeScrToTr1(QByteArray wst);
    void writeScrToTr2(QByteArray wst);

private slots:
    void writeTr1DataOnScreen(QByteArray wt);
    void writeTr2DataOnScreen(QByteArray wt);
    void interruptReceived();
    void interruptReceived2();
    void cmodeSet2(int);

    void auxstate(bool);


    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pingbutton2_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_textEdit_textChanged();
};

#endif // MAINWINDOW_H
