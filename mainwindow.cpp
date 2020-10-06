#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _readMutex=new QMutex();
    _writeMutex=new QMutex();

    //String init
    path=QDir(QCoreApplication::applicationDirPath()).absolutePath()+"/ControllerLOG";
    port=QString("ttyUSB0");
    fileName=QString("ControllerLOG.txt");

    _allRecData=new QString;
    _originalData=new QString(GetOriginDataFromFile());

    //when the number of characters exceed _maxLine*_wortPerLine, delete _wortPerLine characters at the beginning
    _maxLine=10000;
    _cleanBrowserChar=500;
    _wortPerLine=50;


    //Setting timer interval (msec)
    _refreshTime=500;
    _retryInterval=1000;
    _connectionCheckInterval=1000;
\
    InitialPort();
    InitialTimer();

    ui->TimeStamp->setCheckable(true);
    ui->TimeStamp->setChecked(true);

    ui->Stop->setCheckable(true);
    ui->Stop->setChecked(false);

    //set portname and open serial port
    qDebug()<<"Port open status: "
           <<SetPort(port);
    int openport=OpenPort();
    qDebug()<<"Return code opening "<<port<<" : "
           <<openport;

    if(!openport==1){ui->Stop->setChecked(true);}

    //_refreshFile->start();

}

MainWindow::~MainWindow()
{
    if(_connectionCheck->isActive()){_connectionCheck->stop();}
    if(_refreshFile->isActive()){_refreshFile->stop();}
    if(_retryConnect->isActive()){_refreshFile->stop();}

    delete _readMutex;
    delete _writeMutex;

    delete _allRecData;
    delete _originalData;

    CleanList();

    delete ui;
}

void MainWindow::CleanList()
{
    for(auto &a:_recData)
    {
        if(!(a==nullptr)){delete a;}
    }
    for(auto &a:_timeStamp)
    {
        if(!(a==nullptr)){delete a;}
    }
    for(auto &a:_outputData)
    {
        if(!(a==nullptr)){delete a;}
    }
}

void MainWindow::InitialPort()
{
    _logPort= new QSerialPort(this);
    _logPort->setBaudRate(QSerialPort::Baud115200);
    _logPort->setDataBits(QSerialPort::Data8);
    _logPort->setParity(QSerialPort::NoParity);
    _logPort->setStopBits(QSerialPort::OneStop);
    _logPort->setFlowControl(QSerialPort::NoFlowControl);

    connect(_logPort,&QSerialPort::readyRead,this,&MainWindow::GetLogData,Qt::QueuedConnection);
}
void MainWindow::InitialTimer()
{
    _connectionCheck=new QTimer(this);
    _connectionCheck->setInterval(_connectionCheckInterval);
    connect(_connectionCheck,&QTimer::timeout,this,&MainWindow::ConnectionCheck);

    _refreshFile=new QTimer(this);
    _refreshFile->setInterval(_refreshTime);
    connect(_refreshFile,&QTimer::timeout,this,&MainWindow::RefreshFile);

    _retryConnect=new QTimer(this);
    _retryConnect->setInterval(_retryInterval);
    connect(_retryConnect,&QTimer::timeout,this,&MainWindow::RetryConnect);
}

int MainWindow::OpenPort()
{
    if(_logPort->open(QIODevice::ReadWrite))
    {
        _connectionCheck->start();
        _refreshFile->start();
        return 1;
    }
    else
    {
        return 0;
        //start retry timer when booting connetion failed
        if(!_retryConnect->isActive()){_retryConnect->start();}
    }
}

bool MainWindow::SetPort(QString port)
{
    QSerialPortInfo onePortInfo;
    _portList.clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        onePortInfo = info;
        _portList.append(info);
        //qDebug()<<"Available Port is"<<onePortInfo.portName();
    }
    if(!_portList.isEmpty())
    {
        for(auto &a:_portList)
        {
            if(a.portName()==port)
            {
                _logPort->setPort(a);
            }
        }
        //qDebug()<<_logPort->portName();
        if(_logPort->portName()==port)
        {
            return true;
        }
        else
        {
            //qDebug()<<"Failed to find "<<port;
            return false;
        }
    }
    else
    {
        //qDebug("Failed to find any ports!");
        return false;
    }
}

void MainWindow::ConnectionCheck()
{
    QList<QSerialPortInfo> currentPortList;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        currentPortList.append(info);
    }
    if(!currentPortList.isEmpty())
    {
        for(auto &a:currentPortList)
        {
            if(a.portName()=="ttyUSB0")
            {
                return;
            }
        }
        qDebug("Connection failed!");
        ui->textBrowser->append(QString("Connection failed!"));
        _logPort->close();

        _refreshFile->stop();
        _connectionCheck->stop();
        _retryConnect->start();
    }
    else
    {
        QString hint="Failed to find any ports! Connection failed";
        qDebug()<<hint;
        ui->textBrowser->append(hint);
        _logPort->close();
        _refreshFile->stop();
        _connectionCheck->stop();
        _retryConnect->start();
    }
}

void MainWindow::RetryConnect()
{
    if(SetPort(port))
    {
        int status=OpenPort();
        switch(status)
        {
        case 0:
            ui->Stop->setChecked(true);
            return;
        case 1:
            //stop the retry timer if serial port is connected
            _retryConnect->stop();
            break;
        }
    }
    else
    {
        ui->Stop->setChecked(true);
    }
    //qDebug("Try to connect");
}

void MainWindow::GetLogData()
{
    int msecDay=QTime::currentTime().msecsSinceStartOfDay();
    QTime msecNow=QTime::fromMSecsSinceStartOfDay(msecDay);
    QString now=msecNow.toString("hh:mm:ss.zzz");
    QString timestamp="["+now+"]  ";


    QMutexLocker PullingLocker(_readMutex);
    QString recv = _logPort->readAll();
    bool isTimeStamp = ui->TimeStamp->isChecked();
    QString outputData = timestamp+recv;


    if(_recData.size()>_maxLine)
    {
        delete _recData.first();
        delete _timeStamp.first();
        delete _outputData.first();

        if(isTimeStamp){ui->textBrowser->append(outputData);}
        else{ui->textBrowser->append(recv);}

        _recData.append(new QString(recv));
        _timeStamp.append(new QString(timestamp));
        _outputData.append(new QString(timestamp+recv));
    }
    else
    {
        if(isTimeStamp){ui->textBrowser->append(outputData);}
        else{ui->textBrowser->append(recv);}

        _recData.append(new QString(recv));
        _timeStamp.append(new QString(timestamp));
        _outputData.append(new QString(timestamp+recv));
    }

    QString textInBrowser=ui->textBrowser->toPlainText();
    if(textInBrowser.size()>_maxLine*_wortPerLine)
    {
        textInBrowser.remove(0,_cleanBrowserChar);
        ui->textBrowser->setText(textInBrowser);
    }
}

void MainWindow::RefreshFile()
{
    QMutexLocker writeLocker(_writeMutex);
    _allRecData->clear();
    _allRecData->append(*_originalData);

    QDir dir;
    if(!dir.exists(path))
    {
        dir.mkpath(path);
    }
    if(!dir.exists(path))
    {
        qDebug("Failed to add path /ControllerLOG");
        return;
    }
    //QString fileNameWithTime=QDate::currentDate().toString();
    //fileNameWithTime+=".txt";

    QFile file(path+"/"+fileName);
    if(!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        qDebug("failed to open");
        return;
    }

    //add new rec data to the _recData
    bool isTimeStamp = ui->TimeStamp->isChecked();
    if(isTimeStamp)
    {
        for(auto &a:_outputData)
        {
            _allRecData->append(*a);
        }
    }
    else
    {
        for(auto &a:_recData)
        {
            _allRecData->append(*a);
        }

    }
    //Test capacity
    //_allRecData->append(QString("Test222222wqoiuweqwio2321wqoieuwqieu239wqeoiu29\n"));

    //Delete _wortPerLine in the beginning when exceeding _maxLine*_wortPerLine
    if(_allRecData->size()>_maxLine*_wortPerLine)
    {
        _allRecData->remove(0,_wortPerLine);
    }
    file.resize(0);
    QTextStream out(&file);
    out<<*_allRecData;
    file.close();

    writeLocker.unlock();
}

QString MainWindow::GetOriginDataFromFile()
{
    QMutexLocker writeLocker(_writeMutex);
    QString data;

    QDir dir;
    if(!dir.exists(path))
    {
        dir.mkpath(path);
    }
    if(!dir.exists(path))
    {
        qDebug("Failed to add path /ControllerLOG");
        return QString("");
    }
    //QString fileNameWithTime=QDate::currentDate().toString();
    //fileNameWithTime+=".txt";

    QFile file(path+"/"+fileName);
    if(!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        qDebug("failed to open");
        return QString("");
    }
    QTextStream in(&file);
    data.append(in.readAll());
    file.close();
    return data;
}


void MainWindow::on_TimeStamp_toggled(bool checked)
{
    if(checked)
    {
        qDebug()<<"Enable Timestamp";
        RefreshBrowser(checked);
    }
    else
    {
        qDebug()<<"Disable Timestamp";
        RefreshBrowser(checked);
    }
}
void MainWindow::RefreshBrowser(bool TimstampChecked)
{
    ui->textBrowser->clear();
    ui->textBrowser->append(*_originalData);
    if(_outputData.size()<=0 || _recData.size()<=0){return;}

    if(TimstampChecked)
    {
        for(auto a:_outputData)
        {
            ui->textBrowser->append(*a);
        }
    }
    else
    {
        for(auto a:_recData)
        {
            ui->textBrowser->append(*a);
        }
    }

    QString textInBrowser=ui->textBrowser->toPlainText();
    if(textInBrowser.size()>_maxLine*_wortPerLine)
    {
        textInBrowser.remove(0,_cleanBrowserChar);
        ui->textBrowser->setText(textInBrowser);
    }

}

void MainWindow::on_Stop_toggled(bool checked)
{
    if(checked)
    {
        ui->Stop->setText("Start");
        if(_logPort->isOpen()){_logPort->close();}
        if(_refreshFile->isActive()){_refreshFile->stop();}
        if(_connectionCheck->isActive()){_connectionCheck->stop();}
        if(_retryConnect->isActive()){_retryConnect->stop();}
    }
    else
    {
       ui->Stop->setText("Stop");
       RetryConnect();
       _retryConnect->start();
    }

}
