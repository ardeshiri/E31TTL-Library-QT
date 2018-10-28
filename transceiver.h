#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTextStream>
#include <QTimer>
#include <QDataStream>
#include <QBuffer>

class Transceiver : public QObject
{
    Q_OBJECT
public:
    explicit Transceiver(QObject *parent = nullptr, QString portname="");
    ~Transceiver();
    void setSerialPort(QString pn);
    void disconnectSerialPort();

    QByteArray getReceiverChannel();
    QByteArray getReceiverAddress();
    QByteArray getModuleChannel();
    QByteArray getModuleAddress();

    QDataStream outputStream;
    QDataStream inputStream;

    bool outputBufferOpen(QIODevice::OpenModeFlag);
    bool inputBufferOpen(QIODevice::OpenModeFlag);
    void outputBufferClose();
    void inputBufferClose();

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
    QSerialPort* transceiverPort;
    QString currentPortName{};

    QByteArray receiverChannel{};
    QByteArray receiverAddress{};
    QByteArray receivedData{};

    char TDHead;
    char TDTail;
    char TPHead;
    char TPBHead;
    char ACKHead;
    char TSTNGHead;
    int settingTKN;
    int currentMode;
    bool pingRQ; //have we sent any request?
    bool auxCurrentState; //aux management

    QTimer* pingRequestTimer;
    int pingTimeoutInterval;

    QByteArray moduleAddress{};
    QByteArray moduleChannel{};
    QByteArray moduleSPED{};
    QByteArray moduleOption{};

    QBuffer* inputBuffer;
    QByteArray inputQBA{};
    QBuffer* outputBuffer;
    QByteArray outputQBA{};
    int_least64_t inputBufferCtr;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

signals:
    void transceiverReceivedLData(QByteArray trd);
    void transceiverReceivedFallingInt();
    void transceiverReceivedRisingInt();
    void transceiverReceivedNV(QByteArray trd);
    void portSet(QSerialPortInfo ps);
    void portConnected();
    void portDisconnected();
    void settingPacketReceived(QByteArray spr);
    //void lengthExceed(QByteArray le);
    void transceiverModeChanged(int tmc);
    void settingUpdated(QByteArray su);
    void unableToWriteAUXH(QByteArray uwa);

    void pingRequestSent();
    void pingRequestRespSent(QByteArray prrs); // aping request hase been received and responded
    void pingRequestRespReceived(int prrr);  // ping request response received in prrr ms
    void pingRequestTimeout(QString prt);  // ping request timeout

    void outputBufferOutput(QByteArray obo);

    void auxChange(bool ac);

    void bufferError(QString);

    void sendingPacket(QByteArray);
    void remainedPacket(QByteArray);

    void wroteBytesOnTransceiver(int ctr);

    void ackReceived();

    void packet41bSent();




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private slots:
    void currentSettinUpdate(QByteArray su);
    void handleTFI();
    void handleTRI();
    void writeRadioDataToOutputBuff(QByteArray rrdob); // a slot to write radio data on output buff if needed
    void writeBufferedDataToRadio(); // a slot to write input buffer data to radio in proper time, should be done at once and not continuesly
    void sendingByteArraySizeCheck(QByteArray sbasc); // to chech that size of sending byte arrays do not exceed 530 bytes
    void transceiverPortOverflow(int bytes);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public slots:
    void readFromTransceiver();                  //reading from divice that leads to one of signals containing proper data
    void writeOnTransceiver(QByteArray wot);     //writing QByteArray on device

    void setReceiverChannel(QByteArray sp);
    void setReceiverAddress(QByteArray sa);

    void viewSavedSettings(); //only mode3
    void viewVersionInformation(); //only mode3
    void resetDevice(); //only mode3

    void setModuleAddress(QByteArray sma);
    void setModuleChannel(QByteArray smc);

    void pingRequest(int pr=0);  // send a ping request
    void pingRequestTimeoutHandler(); //

    void permanentSettingChange(QByteArray psc);
    void temporarySettingChange(QByteArray tsc);

    void readOutputBufferedData();
    void writeDataToInputBuffer(QByteArray wdtb);


};

#endif // TRANSCEIVER_H
