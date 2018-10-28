#include "transceiver.h"

Transceiver::Transceiver(QObject *parent,QString portname) : QObject(parent),currentPortName(portname)
{

    transceiverPort = new QSerialPort{this};
    settingTKN = 0; //to count received data bytes
    TDHead = (char)0xef;     //head of a data
    TPHead = (char)0xee;     //head of a ping request
    TPBHead = (char)0xed;    //head of a ping response
    TDTail = (char)0xf1;     //tail for any packet except controles
    ACKHead = (char)0xaa;    //ack massage head
    TSTNGHead = (char)0xc0;  //setting head
    currentMode = 5;         //invalid mode (not sure wich mode we are in)

    auxCurrentState = false; //aux low

    moduleAddress.append((char)0xff); //H
    moduleAddress.append((char)0xff); //L
    moduleChannel.append((char)0x50); //temporary shoud get from setting

    receiverAddress.append((char)0xff); //H
    receiverAddress.append((char)0xff); //L
    receiverChannel.append((char)0x50); //temporary shoud get from main window


    pingRQ = false; // no request
    pingTimeoutInterval = 2000; // milliseconds to wait for response of ping
    pingRequestTimer = new QTimer{this};
    pingRequestTimer -> setInterval(pingTimeoutInterval);
    pingRequestTimer -> setSingleShot(true);


    inputBuffer = new QBuffer{&inputQBA,this};  // to use inputBuffer we shoud write on it
    //inputBuffer->open(QIODevice::ReadWrite);
    inputStream.setDevice(inputBuffer);

    outputBuffer = new QBuffer{&outputQBA,this};
    //outputBuffer->open(QIODevice::ReadWrite);
    outputStream.setDevice(outputBuffer);


    if(!(portname==""))
    {
        setSerialPort(portname);
    }

    connect(transceiverPort,SIGNAL(readyRead()),this,SLOT(readFromTransceiver()));

    connect(this,SIGNAL(sendingPacket(QByteArray)),this,SLOT(writeOnTransceiver(QByteArray)));

    //connect(this,SIGNAL(lengthExceed(QByteArray)),this,SLOT(writeOnTransceiver(QByteArray)));
    connect(pingRequestTimer,SIGNAL(timeout()),this,SLOT(pingRequestTimeoutHandler()));
    connect(this,SIGNAL(settingPacketReceived(QByteArray)),this,SLOT(currentSettinUpdate(QByteArray)));

    connect(this,SIGNAL(transceiverReceivedFallingInt()),this,SLOT(handleTFI()));
    connect(this,SIGNAL(transceiverReceivedRisingInt()),this,SLOT(handleTRI()));

    connect(this,SIGNAL(transceiverReceivedLData(QByteArray)),this,SLOT(writeRadioDataToOutputBuff(QByteArray)));  //********


    QTimer::singleShot(2000,this,SLOT(pingRequest()));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Transceiver::~Transceiver()
{
    disconnectSerialPort();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::setSerialPort(QString pn)
{
    if(pn=="")
    {
        return;
    }

    QSerialPortInfo PI(pn);

    if(PI.isNull())
    {
        return;
    }


    if(PI.isBusy())
    {
        return;
    }
    else
    {
        currentPortName = pn;
        transceiverPort -> setPort(PI);
        transceiverPort->setBaudRate(QSerialPort::Baud9600);
        transceiverPort->setDataBits(QSerialPort::Data8);
        transceiverPort->setParity(QSerialPort::NoParity);
        transceiverPort->setStopBits(QSerialPort::OneStop);
        transceiverPort->setFlowControl(QSerialPort::NoFlowControl);
        emit portSet(PI);
        if(transceiverPort->open(QIODevice::ReadWrite))
        {
            emit portConnected();
            return;
        }
        else
        {
            return;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::setReceiverChannel(QByteArray sp)
{
   if (sp.length()<1)
       return;
   receiverChannel.clear();
   receiverChannel.append(sp.left(1)); // use QByteArray::fromHex(sp.toLatin1()) to convert STR to QBA
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::setReceiverAddress(QByteArray sa)
{

    if (sa.length()<2)
        return;
    receiverAddress.clear();
    receiverAddress.append(sa.left(2)); // use QByteArray::fromHex(sp.toLatin1()) to convert STR to QBA
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QByteArray Transceiver::getReceiverChannel()
{
    return receiverChannel;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QByteArray Transceiver::getReceiverAddress()
{
    return receiverAddress;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QByteArray Transceiver::getModuleChannel()
{
    return moduleChannel;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QByteArray Transceiver::getModuleAddress()
{
    return moduleAddress;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::disconnectSerialPort()
{
    if(transceiverPort->isOpen())
    {

        receiverChannel.clear();
        receiverAddress.clear();
        transceiverPort->clear();
        transceiverPort->clearError();
        transceiverPort->close();

        emit portDisconnected();
        currentPortName = "";
    }
    else
    {
       return;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::readFromTransceiver()
{
    while (transceiverPort->bytesAvailable()) {
        char tch{};
        transceiverPort->getChar(&tch);

        if( receivedData.length()==0 && tch==TSTNGHead ) //setting
        {
        settingTKN++;
        receivedData.append(tch);
        }

        if(settingTKN != 0)
        {
        settingTKN++;
        receivedData.append(tch);
        if(settingTKN == 6 )
        {
            emit settingPacketReceived(receivedData);
            receivedData.clear();
            settingTKN = 0;
        }
        continue;
        }

        if(tch != (char)0xf1)
        {
        receivedData.append(tch);
        }
        else
        {
            if(receivedData.at(0)== TDHead)  //data received
            {
            emit transceiverReceivedLData(receivedData.right((receivedData.length())-1));
            receivedData.clear();
            return;
            }

            if(receivedData.at(0) == TPHead)  //ping request received
            {
                if(transceiverPort->isWritable())
                {
                 QByteArray packet{};
                 packet.append((receivedData.right((receivedData.length())-1)).left(2)); //address of the node that has reqested pin
                 packet.append((receivedData.right((receivedData.length())-1)).mid(2,1));//channel of the node that has reqested pin
                 packet.append(TPBHead);
                 packet.append(receivedData.mid(4,(receivedData.length()-4))); //body
                 packet.append(TDTail);
                 transceiverPort->write(packet);
                 emit pingRequestRespSent(packet);
                }
            receivedData.clear();
            return;
            }

            if(receivedData.at(0)== TPBHead && pingRQ == true) //ping response received
            {
            int elapsedTime = 2000 - pingRequestTimer->remainingTime();
            pingRequestTimer -> stop();
            emit pingRequestRespReceived(elapsedTime);
            pingRQ = false;
            receivedData.clear();
            return;
            }

          if(receivedData.at(0)== ACKHead) //ack
          {
              emit ackReceived();
              receivedData.clear();
              return;
          }

          if(receivedData.at(0)== (char)0xf0) //interrupt from our transceiver
          {

          if(receivedData.at(1)== (char)0x77)
          emit transceiverReceivedFallingInt();

          if(receivedData.at(1)== (char)0x88)
          emit transceiverReceivedRisingInt();

          if(receivedData.at(1)== (char)0xaa)
          {
              currentMode = 0;
              emit transceiverModeChanged(0);
          }
          if(receivedData.at(1)== (char)0xbb)
          {
              currentMode = 1;
              emit transceiverModeChanged(1);
          }
          if(receivedData.at(1)== (char)0xcc)
          {
              currentMode = 2;
              emit transceiverModeChanged(2);
          }
          if(receivedData.at(1)== (char)0xdd)
          {
              currentMode = 3;
              emit transceiverModeChanged(3);
          }        
          receivedData.clear();
          return;
          }
          emit transceiverReceivedNV(receivedData);
          receivedData.clear();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::writeOnTransceiver(QByteArray wot)
{
    if(transceiverPort->isWritable())
  {
   wot = wot.left(530);
   int ctr = 0;

    while (wot.length()>0)
    {
        QByteArray packet{};
        packet.append(receiverAddress);
        packet.append(receiverChannel);
        packet.append(TDHead);
        packet.append(wot.left(41));
        packet.append(TDTail);
        transceiverPort->write(packet);
        emit packet41bSent();
        ctr += packet.size();

        if(wot.length()<42)
            wot.clear();
        else
            wot = wot.right((wot.length()-41)>0?(wot.length()-41):wot.length());
    }
    emit wroteBytesOnTransceiver(ctr);
  }

     /*   if(transceiverPort->isWritable())
     {
      QByteArray packet{};
      packet.append(receiverAddress);
      packet.append(receiverChannel);
      packet.append(TDHead);
      if(wot.length()>41)
      {
          transceiverPort->write(packet.append((wot.left(41))).append(TDTail));
          emit lengthExceed(wot.right((wot.length())-41));
          return;
      }
      packet.append(wot);
      packet.append(TDTail);
      transceiverPort->write(packet);
     }  */
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::viewSavedSettings()
{
    if(transceiverPort->isWritable())
   {
    QByteArray packet{};
    packet.append((char)0xC1);
    packet.append((char)0xC1);
    packet.append((char)0xC1);
    transceiverPort->write(packet);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::viewVersionInformation()
{
    if(transceiverPort->isWritable())
   {
    QByteArray packet{};
    packet.append((char)0xC3);
    packet.append((char)0xC3);
    packet.append((char)0xC3);
    transceiverPort->write(packet);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::resetDevice()
{
    if(transceiverPort->isWritable())
   {
    QByteArray packet{};
    packet.append((char)0xC4);
    packet.append((char)0xC4);
    packet.append((char)0xC4);
    transceiverPort->write(packet);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::permanentSettingChange(QByteArray psc)
{
    if(psc.length() == 5)
    {
        QByteArray packet{};
        packet.append((char)0xC0);
        packet.append(psc);
        transceiverPort->write(packet);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::temporarySettingChange(QByteArray tsc)
{
    if(tsc.length() == 5)
    {
        QByteArray packet{};
        packet.append((char)0xC2);
        packet.append(tsc);
        transceiverPort->write(packet);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::pingRequest(int pr)
{
    if(transceiverPort->isWritable())
    {
     pingRQ = true;
     if(pr>38)
         pr=38;
     if(pr<1)
         pr=0;

     QByteArray body{};
     body.resize(pr);

     for(int i = 0; i<pr; ++i)
         body[i]=(char)0xff;

     QByteArray packet{};
     packet.append(receiverAddress);
     packet.append(receiverChannel);
     packet.append(TPHead);
     packet.append(moduleAddress);
     packet.append(moduleChannel);
     packet.append(body);
     packet.append(TDTail);
     transceiverPort->write(packet);
     pingRequestTimer -> start();
     emit pingRequestSent();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::pingRequestTimeoutHandler()
{
    QString to{QString::number(pingTimeoutInterval)};
    to += " ms interval reached. no response.";
    emit pingRequestTimeout(to);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::setModuleAddress(QByteArray sma)
{
    if(sma.length() == 2)
    {
    moduleAddress.clear();
    moduleAddress.append(sma);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::setModuleChannel(QByteArray smc)
{
    if(smc.length() == 1)
    {
    moduleChannel.clear();
    moduleChannel.append(smc);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::currentSettinUpdate(QByteArray csu)
{
    moduleAddress.clear();
    moduleAddress.append(csu.mid(1,2));

    moduleSPED.clear();
    moduleSPED.append(csu.mid(3,1));

    moduleChannel.clear();
    moduleChannel.append(csu.mid(4,1));

    moduleOption.clear();
    moduleOption.append(csu.mid(5,1));

    emit settingUpdated(csu);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::handleTFI()
{
    if(auxCurrentState == true)
    {
    auxCurrentState = false;
    emit auxChange(false);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::handleTRI()
{
    if(auxCurrentState == false)
    {
    auxCurrentState = true;
    emit auxChange(true);
    }

    if(!inputQBA.isEmpty())
     writeBufferedDataToRadio();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::readOutputBufferedData()
{
    if(outputBuffer->open(QIODevice::ReadOnly))
    {
    outputBuffer->seek(0);
    if(outputBuffer->bytesAvailable())
        {
            emit outputBufferOutput(outputBuffer->readAll());
            outputQBA.clear();
        }
    }
    else {
        emit bufferError("Error while openning output buffer to read");
    }

    if(outputBuffer->isOpen())
        outputBuffer->close();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::writeDataToInputBuffer(QByteArray wdtb)
{
    if(inputBuffer->open(QIODevice::WriteOnly|QIODevice::Append))
    {
    if(inputBuffer->isWritable())
        inputBuffer->write(wdtb);
    else
        emit bufferError("Error while writing to input buffer");
    }
    else {
        emit bufferError("Error while openning input buffer to write");
    }
    if(inputBuffer->isOpen())
        inputBuffer->close();

    if(auxCurrentState == true)
    {
        if(!inputQBA.isEmpty())
            writeBufferedDataToRadio();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::writeRadioDataToOutputBuff(QByteArray rrdob)
{
    if(outputBuffer->open(QIODevice::WriteOnly|QIODevice::Append))
    {
    if(outputBuffer->isWritable())
        outputBuffer->write(rrdob);
    else
        emit bufferError("Error while writing to output buffer");
    }
    else {
        emit bufferError("Error while openning output buffer to write");
    }

    if(outputBuffer->isOpen())
        outputBuffer->close();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::writeBufferedDataToRadio()
{
    if(inputBuffer->open(QIODevice::ReadOnly))
    {
    inputBuffer->seek(0);

    if(inputBuffer->bytesAvailable())
        {
            writeOnTransceiver(inputBuffer->read(530));
            inputQBA = inputQBA.right((inputQBA.length()>530)?(inputQBA.length()-530):0);
        }
    }
    else {
        emit bufferError("Error while openning input buffer to read");
    }

    if(inputBuffer->isOpen())
        inputBuffer->close();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Transceiver::outputBufferOpen(QIODevice::OpenModeFlag ob)
{
    return outputBuffer->open(ob);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Transceiver::inputBufferOpen(QIODevice::OpenModeFlag ib)
{
    return inputBuffer->open(ib);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::sendingByteArraySizeCheck(QByteArray sbasc)
{
    if(auxCurrentState == true)
    {
        if(sbasc.length()>530)
        {
            emit sendingPacket(sbasc.left(530));
            auxCurrentState = false;
            emit auxChange(false);
            emit remainedPacket(sbasc.right((sbasc.length())-530));
            return;
        }
        emit sendingPacket(sbasc);
    }
    else
    {
        emit remainedPacket(sbasc);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Transceiver::transceiverPortOverflow(int bytes)
{
    if(bytes>594)
        if(auxCurrentState == true)
        {
            auxCurrentState = false;
            emit auxChange(false);
        }
}
