#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QtSerialPort>
#include <QDebug>
#include <QByteArray>
#include <QFileDialog>
#include <QFile>
#define SOH 0x01
#define EOT 0x04
#define DLE 0x10
#define PROGRAM_FLASH 0x03
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QSerialPort comport;
    QFileDialog fileDialog;
    QString fileName;
     const unsigned short crc_table[16] =
    {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    QString ParsVersion(QByteArray data);
    QByteArray CreateEraseCommand();
    unsigned short CalculateCrc(QByteArray data);
    unsigned short CalculateCrc(char *data, unsigned int len);
    QList<QByteArray> LoadHexFile(QString HexFilePath);
    unsigned short ConvertAsciiToHex();
    QList<QByteArray> GetHexFromContent(QList<QByteArray> data);
    void ProgramFlash(QString fileName);
    QByteArray CreateFlashPacket(int start, int end, QList<QByteArray> hexContent);
    void WaitMs(int ms);
    void WaitMsNofeedback(int ms);
public slots:
    void ComportReadyRead();

private slots:
    void on_BtnComport_clicked();

    void on_BtnGetVersion_clicked();

    void on_BtnLoadHex_clicked();

    void on_BtnErase_clicked();

    void on_BtnFlash_clicked();

    void on_BtnGotoBoot_clicked();

    void on_BootGoToBootFromFlash_clicked();


    void on_BtnGetCodeVersion_clicked();

    void on_BtnRefreshPort_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
