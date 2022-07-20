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

            ui->statusbar->showMessage("open error!");

        }
        ui->BtnComport->setText("Close");
        ui->statusbar->showMessage("Open ok");
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
        comport.write(packet);
        WaitMs(500);
        QByteArray reply= comport.readAll();
        QByteArray expected= QByteArray::fromHex("0103633004");
        qDebug()<<"<<----------- "<<reply.toHex();
        if(reply!=expected)
        {
            qDebug()<<"<<-----------error------------------ "<<expected.toHex();
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
    if(!comport.isOpen())
    {
        ui->statusbar->showMessage("port open error");
        return;
    }
     comport.readAll();
    comport.write(CreateEraseCommand());
    WaitMs(500);
    QByteArray reply= comport.readAll();
    if(reply!=QByteArray::fromHex("0102422004"))
    { ui->statusbar->showMessage("Erase chip error!");
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
    ProgramFlash(fileName);

}
//========================================================================================
