#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QString>
#include <QTimer>
#include <QList>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool SetPort(QString port=QString("ttyUSB0"));
    int OpenPort();

    void InitialPort();
    void InitialTimer();




private slots:
    void ConnectionCheck();
    void GetLogData();
    void RefreshFile();
    void RetryConnect();


    void on_TimeStamp_toggled(bool checked);

    void on_Stop_toggled(bool checked);

private:
    Ui::MainWindow *ui;

    //default maxline and characters
    int _maxLine=15000;
    int _cleanBrowserChar=100;
    int _wortPerLine=50;

    //default timer interval
    int _refreshTime=500;
    int _connectionCheckInterval=1000;
    int _retryInterval=500;

    QString path;
    QString port;
    QString fileName;
    QMutex *_readMutex;
    QMutex *_writeMutex;

    QTimer *_connectionCheck;
    QTimer *_refreshFile;
    QTimer *_retryConnect;

    QList<QSerialPortInfo> _portList;
    QSerialPort* _logPort;

    QList<QString *> _recData;
    QString *_allRecData;
};
#endif // MAINWINDOW_H
