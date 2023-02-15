#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QList<QSerialPortInfo> list=   QSerialPortInfo::availablePorts();
    for (int i=0;i<list.length();i++)
    {
        ui->CmbPortName->addItem(list[i].portName());
    }
    comport.setPortName("COM9");
    comport.setBaudRate(115200);
    ui->CmbBoardType->addItem("main");
    ui->CmbBoardType->addItem("VALV");
    ui->CmbBoardType->addItem("FEED");
    ui->CmbBoardType->addItem("RLAL");

    for (int i=1;i<255;i++)
    {
        ui->CmbBoardID->addItem(QString::number(i));
    }

    //    connect(&comport,SIGNAL(readyRead()),this,SLOT(ComportReadyRead()));

}




MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ComportReadyRead()
{
    QByteArray response=comport.readAll();
    qDebug()<<response.toHex();
    //ParsVersion(response);
}


void MainWindow::on_BtnComport_clicked()
{
    ui->statusbar->showMessage("");
    if(ui->BtnComport->text()=="Open"){
        comport.setPortName(ui->CmbPortName->currentText());
        comport.setBaudRate(115200);
        if(!comport.open(QSerialPort::ReadWrite))
        {

            ui->statusbar->showMessage("Port Open Error!");

        }
        ui->BtnComport->setText("Close");
        ui->statusbar->showMessage("Port Opened Successfully!");
    }
    else
    {
        ui->BtnComport->setText("Open");
        comport.close();

    }


}

unsigned short MainWindow::ConvertAsciiToHex()
{
    void *VdAscii,  *VdHexRec;
    char temp[5] = {'0','x',NULL, NULL, NULL};
    unsigned int i = 0;
    char *Ascii;
    char *HexRec;

    Ascii = (char *)VdAscii;
    HexRec = (char *)VdHexRec;

    while(1)
    {
        temp[2] = Ascii[i++];
        temp[3] = Ascii[i++];
        if((temp[2] == NULL) || (temp[3] == NULL))
        {
            // Not a valid ASCII. Stop conversion and break.
            i -= 2;
            break;
        }
        else
        {
            // Convert ASCII to hex.
            sscanf(temp, "%x", HexRec);
            HexRec++;
        }
    }

    return (i/2); // i/2: Because, an representing Hex in ASCII takes 2 bytes.
}

QList<QByteArray> MainWindow::LoadHexFile(   QString HexFilePath )
{
    int iRet;
    QList<QByteArray> result;
    //	char HexRec[255];
    QByteArray HexRec;

    // Open file
    QFile HexFilePtr(HexFilePath);

    if(!QFile::exists(HexFilePath))
    {
        qDebug()<<"Failed to open hex file.";
        return result;
    }
    else
    {
        if(!HexFilePtr.open(QFile::ReadOnly))
        {
            qDebug()<<"cant open file";
            return result;
        }
        qDebug()<<"reading file";

        while (true)
        {
            HexRec=HexFilePtr.readLine().trimmed();
            if(HexRec!="")
                result.append(HexRec);
            if(HexRec.length()<1)break;

        }

    }

    return result;
}
QByteArray MainWindow::LoadHexFileValve(   QString HexFilePath )
{
    int iRet;
    QByteArray result;
    //	char HexRec[255];
    QByteArray HexRec;

    // Open file
    QFile HexFilePtr(HexFilePath);

    if(!QFile::exists(HexFilePath))
    {
        qDebug()<<"Failed to open hex file.";
        return result;
    }
    else
    {
        if(!HexFilePtr.open(QFile::ReadOnly))
        {
            qDebug()<<"cant open file";
            return result;
        }
        qDebug()<<"reading file";

        while (true)
        {
            HexRec=HexFilePtr.readLine().trimmed();
            if(HexRec!="")

                if(HexRec.length()<20){
                    HexRec.append("ff");
                    HexRec.append("3f");
                    HexRec.append("ff");
                    HexRec.append("3f");
                }
            // qDebug()<<

            result.append(HexRec.mid(9,HexRec.length()-(9+2)));//ff 3f ff 3f

            if(HexRec.length()<1)break;

        }

    }
    qDebug()<<"===========";
    qDebug()<<"hex:"<<result;
    // qDebug()<<"===========";
    return result;
}

QByteArray MainWindow::CreateEraseCommand()
{
    QByteArray result,buffer;
    buffer.append(0x02);//cmd erase
    uint16_t crc=CalculateCrc(buffer);
    buffer.append(crc&0xff);
    buffer.append((crc>>8)&0xff);
    result.append(SOH);
    for(int i = 0; i < buffer.length(); i++)
    {
        if(((buffer[i]&0xff) == EOT) || ((buffer[i]&0xff) == SOH)
                || ((buffer[i]&0xff) == DLE))
        {
            //			TxPacket[TxPacketLen++] = DLE;
            result.append(DLE);
        }
        result.append(buffer[i]);

    }
    result.append(EOT);
    return result;
}
//========================================================================================

void MainWindow::on_BtnGetVersion_clicked()
{
    if(!comport.isOpen())
    {
        ui->statusbar->showMessage("port is closed");
        return;
    }
    comport.readAll();

    QByteArray buffer;
    buffer.append(0x01);
    buffer.append(0x10);
    buffer.append(0x01);
    buffer.append(0x21);
    buffer.append(0x10);
    buffer.append(0x10);
    buffer.append(0x04);
    comport.write(buffer);//Bootloader Firmware Version: 32.32
    WaitMs(400);
    QByteArray reply= comport.readAll();
    qDebug()<<reply<<" hex"<<reply.toHex();
    ui->statusbar->showMessage(ParsVersion(reply));


}
//========================================================================================
QString MainWindow::ParsVersion(QByteArray data)
{
    if(data.length()<7)return "Get version error";
    if((data[0]&0xff)!=SOH)return "Get version error";
    if((data[data.length()-1]&0xff) !=EOT)return "";

    uint8_t major=data[3]&0xff;
    uint8_t minor=data[4]&0xff;
    QString version= "version="+QString::number( major)+"."+QString::number( minor);
    return version;
}
//========================================================================================
QList<QByteArray> MainWindow::GetHexFromContent(QList<QByteArray> content)
{
    QList<QByteArray> result;
    for (int i=0;i<content.length();i++)
    {
        QByteArray data=QByteArray::fromHex(content[i].mid(1,content[i].length()-1));
        result.append(data);
    }
    return result;
}
//========================================================================================
unsigned short MainWindow::CalculateCrc(char *data, unsigned int len)
{
    unsigned int i;
    unsigned short crc = 0;

    while(len--)
    {
        i = (crc >> 12) ^ (*data >> 4);
        crc = crc_table[i & 0x0F] ^ (crc << 4);
        i = (crc >> 12) ^ (*data >> 0);
        crc = crc_table[i & 0x0F] ^ (crc << 4);
        data++;
    }

    return (crc & 0xFFFF);
}
//========================================================================================
unsigned short MainWindow::CalculateCrc(QByteArray data)
{

    unsigned short crc = 0;
    unsigned int i;

    for(int index=0;index<data.length();index++)
    {
        i = (crc >> 12) ^ (data[index] >> 4);
        crc = crc_table[i & 0x0F] ^ (crc << 4);
        i = (crc >> 12) ^ (data[index] >> 0);
        crc = crc_table[i & 0x0F] ^ (crc << 4);

    }

    return (crc & 0xFFFF);
}
//========================================================================================
QByteArray MainWindow::CreateFlashPacket(int start,int end, QList<QByteArray> hexContent)
{
    QByteArray buffer;

    buffer.append(PROGRAM_FLASH);
    for(int i=start;i<end;i++)
    {
        buffer.append(hexContent[i]);

    }
    uint16_t crc=CalculateCrc((char*)buffer.toStdString().c_str(),buffer.length());
    buffer.append(crc&0xff);
    buffer.append((crc>>8)&0xff);
    QByteArray packet;
    packet.append(SOH);
    for(int i=0;i<buffer.length();i++)
    {
        if((buffer[i]&0xff)==EOT ||(buffer[i]&0xff)==DLE || (buffer[i]&0xff)==SOH )
        {
            packet.append(DLE);
        }
        packet.append(buffer[i]);

    }
    packet.append(EOT);
    return packet;
}
//========================================================================================
QByteArray MainWindow::CreateFlashPacketValve(int start,int end,uint32_t flashAddress, QByteArray hexContent)
{
    QByteArray buffer;

    buffer.append(0x55);
    buffer.append(0x02);
    buffer.append(0x10);
    buffer.append((char)0x00);
    buffer.append(0x55);
    buffer.append(0xaa);


    buffer.append(flashAddress&0xff);
    buffer.append((flashAddress>>8)&0xff);
    buffer.append((flashAddress>>16)&0xff);
    buffer.append((flashAddress>>24)&0xff);

    for(int i=start;i<end;i++)
    {
        buffer.append(hexContent[i]);

    }
    //    uint16_t crc=CalculateCrc((char*)buffer.toStdString().c_str(),buffer.length());
    //    buffer.append(crc&0xff);
    //    buffer.append((crc>>8)&0xff);
    //    QByteArray packet;
    //    packet.append(SOH);
    //    for(int i=0;i<buffer.length();i++)
    //    {
    //        if((buffer[i]&0xff)==EOT ||(buffer[i]&0xff)==DLE || (buffer[i]&0xff)==SOH )
    //        {
    //            packet.append(DLE);
    //        }
    //        packet.append(buffer[i]);

    //    }
    //    packet.append(EOT);
    return buffer;
}
//========================================================================================
void MainWindow::WaitMsNofeedback(int ms)
{
    QEventLoop q;
    QTimer tT;
    tT.setSingleShot(true);
    connect(&tT, SIGNAL(timeout()), &q, SLOT(quit()));
    //    connect(&comport, SIGNAL(readyRead()), &q, SLOT(quit()));
    tT.start(ms); // 5s timeout
    q.exec();
    if(tT.isActive()){
        // download complete
        tT.stop();
    } else {

    }
}
//========================================================================================
void MainWindow::WaitMs(int ms)
{
    QEventLoop q;
    QTimer tT;
    tT.setSingleShot(true);
    connect(&tT, SIGNAL(timeout()), &q, SLOT(quit()));
    connect(&comport, SIGNAL(readyRead()), &q, SLOT(quit()));
    tT.start(ms); // 5s timeout
    q.exec();
    if(tT.isActive()){
        // download complete
        tT.stop();
    } else {

    }
}
//========================================================================================
void MainWindow::ProgramFlash(QString fileName)
{

    ui->PgrFlash->setValue(0);
    if(!comport.isOpen())
    {
        ui->statusbar->showMessage("port is closed");
        return;
    }
    comport.readAll();
    QList<QByteArray> stringContent= LoadHexFile(fileName);
    QList<QByteArray> hexContent =GetHexFromContent(stringContent);


    for (int i=0;i<hexContent.length();i+=11)
    {
        int progress=i*100/hexContent.length();
        qDebug()<<"completed:"<< progress<<"%";
        ui->PgrFlash->setValue(progress);
        QByteArray  packet=CreateFlashPacket(i,fmin(hexContent.length(),i+11),hexContent);

        qDebug()<<"----------->> "<<packet.toHex();
        comport.readAll();
        comport.write(packet);
        comport.flush();
        WaitMs(1000);
        WaitMsNofeedback(20);
        QByteArray reply= comport.readAll();
        QByteArray expected= QByteArray::fromHex("0103633004");
        if(progress==99 || progress==100) expected= reply;
        qDebug()<<"<<----------- "<<reply.toHex();
        if(reply!=expected)
        {
            qDebug()<<"<<-----------error------------------ "<<expected.toHex();
            ui->statusbar->showMessage("Flash Error!");
            return ;
        }

    }
    ui->PgrFlash->setValue(100);
}
//========================================================================================
void MainWindow::ProgramFlashValve(QString fileName)
{

    ui->PgrFlash->setValue(0);
    if(!comport.isOpen())
    {
        ui->statusbar->showMessage("port is closed");
        return;
    }
    comport.readAll();
    QByteArray stringContent= LoadHexFileValve(fileName);
    QByteArray hexContent =QByteArray::fromHex(stringContent);
    //GetHexFromContent(stringContent);


    for (int i=0;i<hexContent.length();i+=16)
    {
        int progress=i*100/hexContent.length();
        qDebug()<<"completed:"<< progress<<"%";
        ui->PgrFlash->setValue(progress);
        int add=2048+i/2;//fmin(hexContent.length(),i+11)
        QByteArray  packet=CreateFlashPacketValve(i,fmin(hexContent.length(),i+16),add,hexContent);

        qDebug()<<"----------->> "<<packet.toHex();
        comport.readAll();
        comport.write(packet);
        comport.flush();
        WaitMs(1000);
        WaitMsNofeedback(20);
        QByteArray reply= comport.readAll();

        QString rx="5502100055aa";


        if((add&0xff)<16)rx+="0";
        rx+=QString::number(add&0xff,16);
        add>>=8;
         if((add&0xff)<16)rx+="0";
        rx+=QString::number(add&0xff,16);
        add>>=8;
        if((add&0xff)<16)rx+="0";
        rx+=QString::number(add&0xff,16);
        add>>=8;
        if((add&0xff)<16)rx+="0";
        rx+=QString::number(add&0xff,16);

        rx+="01";

       // qDebug()<<"<<-----rx------ "<<rx;

        QByteArray expected= QByteArray::fromHex(rx.toLatin1());
        if(progress==99 || progress==100) expected= reply;
        qDebug()<<"<<----------- "<<reply.toHex();
        if(reply!=expected)
        {
            qDebug()<<"<<-----------error------------------ "<<expected.toHex();
            ui->statusbar->showMessage("Flash Error!");
            return ;
        }

    }
    ui->PgrFlash->setValue(100);
}
//========================================================================================
void MainWindow::on_BtnLoadHex_clicked()
{
    ui->statusbar->showMessage("");
    fileName = QFileDialog::getOpenFileName(this, tr("Open hex file"),
                                            "F://nojan//Sorter//remote programming//RemoteProgrammingCodes//Main_V11_PCBV5_010309_Takshoot_WithTimingFeedback_Final//991016_Config_V000//firmware//CB_V000.X//dist//default//production",
                                            tr("intel hex files (*.hex)"));


}
//========================================================================================
void MainWindow::on_BtnErase_clicked()
{
    ui->statusbar->showMessage("");
    QString type[4]={"0102422004","5503800355AA00040000","5503800355AA00040000","5503800355AA00040000"};
    QString correctReplay[4]={"0102422004","5503800355AA0004000001","5503800355AA0004000001","5503800355AA0004000001"};
    if(!comport.isOpen())
    {
        ui->statusbar->showMessage("port open error");
        return;
    }
    comport.readAll();
    if(ui->CmbBoardType->currentIndex()==0)comport.write(CreateEraseCommand());
    else {
        qDebug()<<"-->"<<type[ui->CmbBoardType->currentIndex()];
        comport.write(QByteArray::fromHex(type[ui->CmbBoardType->currentIndex()].toLatin1()));
    }
    WaitMs(500);
    QByteArray reply= comport.readAll();
    qDebug()<<"replay="<<reply.toHex();

    if(reply!=QByteArray::fromHex(correctReplay[ui->CmbBoardType->currentIndex()].toLatin1()))
    {
        ui->statusbar->showMessage("Erase chip error!");
        return;
    }
    ui->statusbar->showMessage("Erase Ok");


}
//========================================================================================
void MainWindow::on_BtnFlash_clicked()
{

    if(fileName=="")
    {
        ui->statusbar->showMessage("please select hex file");
        return;
    }
    if(ui->CmbBoardType->currentIndex()==0)
        ProgramFlash(fileName);
    else
        ProgramFlashValve(fileName);

}
//========================================================================================

void MainWindow::on_BtnGotoBoot_clicked()
{
    ui->statusbar->showMessage("");
    if(!comport.isOpen())
    {
        ui->statusbar->showMessage("port open error");
        comport.close();
        ui->BtnComport->setText("Open");
    }
    comport.setPortName(ui->CmbPortName->currentText());
    comport.setBaudRate(115200);
    comport.open(QSerialPort::ReadWrite);
    comport.readAll();
    QString text="run"+ui->CmbBoardType->currentText()+ui->CmbBoardID->currentText();
    qDebug()<<text;
    comport.write(text.toLatin1());


    WaitMs(500);
    ui->statusbar->showMessage(comport.readAll());

}

void MainWindow::on_BootGoToBootFromFlash_clicked()
{
    ui->statusbar->showMessage("");
    if(!comport.isOpen())
    {
        comport.close();
        ui->BtnComport->setText("Open");

    }
    comport.setPortName(ui->CmbPortName->currentText());
    comport.setBaudRate(115200);
    comport.open(QSerialPort::ReadWrite);
    comport.readAll();
    QString text="btl"+ui->CmbBoardType->currentText()+ui->CmbBoardID->currentText();
    qDebug()<<text;
    comport.write(text.toLatin1());
    WaitMs(500);
    QByteArray answer= comport.readAll();
    qDebug()<<"ans="<<answer;
    ui->statusbar->showMessage("baud 1000000 : "+answer);
    comport.close();

    if(answer.contains(text.toLatin1()))
    {
        return;
    }
    comport.setBaudRate(115200);
    comport.open(QSerialPort::ReadWrite);
    comport.readAll();

    comport.write(text.toLatin1());
    WaitMs(500);
    answer= comport.readAll();
    ui->statusbar->showMessage("baud 115200 : "+answer);
    comport.close();

}



void MainWindow::on_BtnGetCodeVersion_clicked()
{
    ui->statusbar->showMessage("");
    QString type[4]={"1E","00","00","07"};
    if(!comport.isOpen())
    {
        ui->statusbar->showMessage("port open error");
        return;
    }
    comport.setBaudRate(115200);
    comport.readAll();
    QByteArray ba;//=FE0A00000000000000000000000000000000000000000000F4
    //    0x01	0x04	0x00	0x1E	0x00	0x01
    uint8_t id= ui->CmbBoardType->currentIndex()*10+ui->CmbBoardID->currentText().toInt();
    if(ui->CmbBoardType->currentText()=="RLAL")id=100;
    QString command=QString::number(id,16);
    command+="0400";

    //    command+="1E";//main
    //    command+="00";//valv feeder
    //    command+="07";//relay
    command+=type[ui->CmbBoardType->currentIndex()];

    command+="0001";
    ba=QByteArray::fromHex(command.toLatin1());
    uint16_t crc=CalculateCrc(ba);
    ba.append((crc>>8)&0xff);
    ba.append(crc&0xff);
    qDebug()<<"--->"<<ba.toHex();
    comport.write(ba);

    WaitMs(500);
    QByteArray reply= comport.readAll();
    qDebug()<<"<---"<< reply.toHex();
    int version=reply[4]&0xff;
    version<<=8;
    version+=reply[5]&0xff;

    ui->statusbar->showMessage(QString::number(version));
}

void MainWindow::on_BtnRefreshPort_clicked()
{
    QList<QSerialPortInfo> list=   QSerialPortInfo::availablePorts();
    ui->CmbPortName->clear();
    for (int i=0;i<list.length();i++)
    {
        ui->CmbPortName->addItem(list[i].portName());
    }
}
