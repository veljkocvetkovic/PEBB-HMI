#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtMqtt/QMqttClient>
#include <QTime>
#include <QTimer>
#include <QtQml/QQmlContext>
#include <QQmlProperty>
#include <QMetaObject>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <fstream>
#include <QFile>
#include <QtMath>


bool connectedToMQTT = false;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // init components
    // add graph into widgets gui
    bool openGLSupported = QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGLRhi;
    if (!openGLSupported) {
        qWarning() << "OpenGL is not set as the graphics backend, so AbstractSeries.useOpenGL will not work.";
        qWarning() << "Set QSG_RHI_BACKEND=opengl environment variable to force the OpenGL backend to be used.";
    }
    this->scopeView = new QQuickView();
    this->dataSource = new DataSource(scopeView);
    scopeView->rootContext()->setContextProperty("dataSource", dataSource);
    scopeView->rootContext()->setContextProperty("openGLSupported", openGLSupported);

    //oscilliscope
    QWidget *scopeContainer = QWidget::createWindowContainer(this->scopeView, this);
    scopeView->setColor(QColor("#2A2C3A")); // I can't make it transparent, so I set it to the same color
    scopeView->setSource(QUrl("qrc:/styles/ScopeView.qml"));
    ui->oscilloscopeLayout->addWidget(scopeContainer);

    //Voltage Control
    this->voltageTumblerView = new QQuickView();
    QWidget *voltageTumblerContainer = QWidget::createWindowContainer(this->voltageTumblerView, this);
    this->voltageTumblerView->setColor(QColor("#2A2C3A")); // I can't make it transparent, so I set it to the same color
    this->voltageTumblerView->setSource(QUrl("qrc:/styles/NumberTumbler.qml"));
    ui->VoltageOutputTumbler->addWidget(voltageTumblerContainer);

    //Current Control
    this->currentTumblerView = new QQuickView();
    QWidget *currentTumblerContainer = QWidget::createWindowContainer(this->currentTumblerView, this);
    this->currentTumblerView->setColor(QColor("#2A2C3A")); // I can't make it transparent, so I set it to the same color
    this->currentTumblerView->setSource(QUrl("qrc:/styles/NumberTumbler.qml"));
    ui->CurrentOutputTumbler->addWidget(currentTumblerContainer);

    ui->MainPages->setCurrentIndex(0);

    //start timer
    QTimer *timer = new QTimer(); // for starting the clock
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    timer->start(1000); // clock update frequency (1 update/s)

//    setOutputCurrent = ui->currentSpinBox->text().toInt();
//    setOutputVoltage = ui->voltageSpinBox->text().toInt();
    updateSetOutputStatus();


    //connect to mqtt
    m_client = new QMqttClient();
    m_client->setKeepAlive(10000);
    m_client->setHostname("137.184.70.171"); // test mde signal
    m_client->setPort(1883);
    m_client->setUsername("mde_test");
    m_client->setPassword("mde_test");

    connect(m_client, &QMqttClient::messageReceived, this, &MainWindow::updateOnMessageReceived);

    connect(m_client, &QMqttClient::disconnected, this, [this](){
        // update the icon here
        connectedToMQTT = false;
        ui->ConnectionSymbol->setStyleSheet("border-image: url(:/pictures/disconnected.png) no-repeat");
        qDebug() << "disconnected\n";
        QTimer::singleShot(1000, this, [this](){this->m_client->connectToHost();});
//        while (true) {
//                if (try to reconnect) {
//                    break;
//                }
//                std::sleep(1); // Wait for 5 seconds before attempting to reconnect
//        }

    });

    connect(m_client, &QMqttClient::connected, this, [this](){
        // update the icon here
        connectedToMQTT = true;
        ui->ConnectionSymbol->setStyleSheet("border-image: url(:/pictures/connected.png) no-repeat");
        qDebug() << "connected\n";
        auto subscription = m_client->subscribe(QString("/pebb/voltage"), 0);
        if (!subscription) {
            qDebug() << QLatin1String("Error:") << QLatin1String("Could not subscribe. Is there a valid connection?");
        }
        subscription = m_client->subscribe(QString("/pebb/current"), 0);
        if (!subscription) {
            qDebug() << QLatin1String("Error:") << QLatin1String("Could not subscribe. Is there a valid connection?");
        }

        // TODO: Ask communication team to send us power status of the PEBB (ON/OFF) when initializing connection
        m_client->subscribe(QString("/pebb/state"), 0);
        m_client->subscribe(QString("/pebb/fault"), 0);
    });

    // UI interaction signals/slots
    connect(ui->SetOutputButton, &QPushButton::clicked, this, &MainWindow::goToSetCurrentVoltagePage);
    connect(ui->CancelButton, &QPushButton::clicked, this, &MainWindow::cancelSetCurrentVoltage);
    connect(ui->OffButton, &QPushButton::clicked, this, &MainWindow::turnOnOff);
    connect(ui->SaveButton, &QPushButton::clicked, this, &MainWindow::saveSetCurrentVoltage);
    connect(ui->StateComboBox, &QComboBox::currentTextChanged, this, &MainWindow::changeState);
    connect(ui->FaultComboBox, &QComboBox::currentTextChanged, this, &MainWindow::changeFault);
    m_client->connectToHost();

    //battery

//    ui->MainPages->setCurrentIndex(0);
//    QTimer *timer2 = new QTimer(); // for starting the clock
//    connect(timer2, &QTimer::timeout, this, &MainWindow::BatteryBar_Update);
//    timer2->start(1000); // clock update frequency (1 update/s)
    //ui->BatteryBar->setValue(deviceInfo->batteryLevel());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::goToSetCurrentVoltagePage()
{
    ui->MainPages->setCurrentIndex(1);
}

void MainWindow::cancelSetCurrentVoltage()
{
    updateSetOutputStatus();
    ui->MainPages->setCurrentIndex(0);
//    ui->currentSpinBox->setValue(setOutputCurrent);
//    ui->voltageSpinBox->setValue(setOutputVoltage);

    QVariant returnedValue;
    QMetaObject::invokeMethod((QObject*) this->voltageTumblerView->rootObject(), "setValue",
        Q_RETURN_ARG(QVariant, returnedValue),
        Q_ARG(QVariant, setOutputVoltage));
    QMetaObject::invokeMethod((QObject*) this->currentTumblerView->rootObject(), "setValue",
        Q_RETURN_ARG(QVariant, returnedValue),
        Q_ARG(QVariant, setOutputCurrent));
}

void MainWindow::saveSetCurrentVoltage()
{
    // set(save) values
    // send mqtt messages
    setOutputCurrent = QQmlProperty::read((QObject*)this->currentTumblerView->rootObject(), "value").toInt();
    setOutputVoltage = QQmlProperty::read((QObject*)this->voltageTumblerView->rootObject(), "value").toInt();
    int setOutputCurrentPeak = qSqrt(2) * setOutputCurrent;
    int setOutputVoltagePeak = qSqrt(2) * setOutputVoltage;
    m_client->publish(QMqttTopicName("/pebb/setVoltage"), QString::number(setOutputVoltagePeak).toUtf8());
    m_client->publish(QMqttTopicName("/pebb/setCurrent"), QString::number(setOutputCurrentPeak).toUtf8());
    updateSetOutputStatus();
    ui->MainPages->setCurrentIndex(0);
}

void MainWindow::setDCCurrentLabel(double currentValue)
{
    ui->DCCurrentLabel->setText(QString("DC Curr.: %1A").arg(currentValue, 0, 'f', 1));
}

void MainWindow::setDCVoltageLabel(double voltageValue)
{
    ui->DCVoltageLabel->setText(QString("DC Volt.: %1V").arg(voltageValue, 0, 'f', 1));
}

void MainWindow::updateTime()
{
    QTime time = QTime::currentTime();
    ui->TimeLabel->setText(time.toString("h:mmAP"));
}

void MainWindow::updateSetOutputStatus()
{
    ui->outputStatusLabel->setText(QString("Set DC Current: %1A\nSet DC Voltage: %2V").arg(QString::number(setOutputCurrent), QString::number(setOutputVoltage)));
}

// maybe we don't need this?
void MainWindow::changeState(const QString &text)
{
    m_client->publish(QMqttTopicName("/pebb/state"), text.toUtf8());
}

void MainWindow::changeFault(const QString &text)
{
    m_client->publish(QMqttTopicName("/pebb/fault"), text.toUtf8());
}

void MainWindow::turnOnOff()
{
    switch(ui->StateComboBox->currentIndex())
    {
        case 0: //ST_OFF
            m_client->publish(QMqttTopicName("/pebb/power"), "on");
            ui->StateComboBox->setItemData(0,QColor(Qt::blue),Qt::BackgroundRole);
            ui->StateComboBox->setStyleSheet("QComboBox#StateComboBox::drop-down {border-width:0px;}QComboBox#StateComboBox::down-arrow {border-width:0px;image: none;} QComboBox#StateComboBox{background-color:green; color:#c2c7d5}");
            break;
        case 1: //ST_ON
            m_client->publish(QMqttTopicName("/pebb/power"), "off");
            ui->StateComboBox->setStyleSheet("QComboBox#StateComboBox::drop-down {border-width:0px;}QComboBox#StateComboBox::down-arrow {border-width:0px;image: none;} QComboBox#StateComboBox{background-color:yellow; color:#808080}");
            break;
        default:
            break;
    }
}

void MainWindow::updateOnMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    if(topic.name().compare("/pebb/voltage") == 0)
    {
        setDCVoltageLabel(message.toDouble());
        this->dataSource->addVoltage(message.toDouble());
//        this->dataSource->addCurrent(message.toDouble());
    }
    else if (topic.name().compare("/pebb/current") ==0)
    {
        setDCCurrentLabel(message.toDouble());
        this->dataSource->addCurrent(message.toDouble());
    }
    else if (topic.name() == "/pebb/state")
    {
        if(message.toStdString().compare("ST_OFF") == 0)
        {
            ui->StateComboBox->setCurrentIndex(0);
        }
        else if (message.toStdString().compare("ST_ON") == 0)
        {
            ui->StateComboBox->setCurrentIndex(1);
        }
    }
    else if (topic.name() == "/pebb/fault")
    {
        if(message.toStdString().compare("NO_FAULT") == 0)
        {
            ui->FaultComboBox->setCurrentIndex(0);
            ui->FaultComboBox->setStyleSheet("QComboBox#FaultComboBox{background-color: green;}");
        }
        else if (message.toStdString().compare("FAULT") == 0)
        {
            ui->FaultComboBox->setCurrentIndex(1);
            ui->FaultComboBox->setStyleSheet("QComboBox#FaultComboBox{background-color: red;}");
        }
    }
}

void MainWindow::changeStateDropDownBGColor(QString backgroundColor)
{
    QString templateSS = "QComboBox#StateComboBox::drop-down {border-width:0px;}QComboBox#StateComboBox::down-arrow {border-width:0px;image: none;} QComboBox#StateComboBox{background-color:%1; color:#c2c7d5}";
    ui->StateComboBox->setStyleSheet(templateSS.arg(backgroundColor));
}

void MainWindow::on_OffButton_clicked()
{

}

void MainWindow::BatteryBar_Update()
{
    QFile bCap("/sys/class/power_supply/BAT0/capacity");

    bCap.open(QIODevice::ReadOnly | QIODevice::Text);
    QString levelString = QString(bCap.readAll());
    int level = levelString.toInt();
    bCap.close();

    if(level > 50){
        ui->BatteryBar->setStyleSheet(QString::fromUtf8("QProgressBar#BatteryBar{color: black;}\n"
            "\n"
            "QProgressBar::chunk {\n"
            "     background-color: #02AA20;\n"
            "     width: 5px;\n"
            " }"));
    }
    else if(level < 50 && level > 20){
        ui->BatteryBar->setStyleSheet(QString::fromUtf8("QProgressBar#BatteryBar{color: black;}\n"
            "\n"
            "QProgressBar::chunk {\n"
            "     background-color: yellow;\n"
            "     width: 5px;\n"
            " }"));
    }
    else if(level < 20){
        ui->BatteryBar->setStyleSheet(QString::fromUtf8("QProgressBar#BatteryBar{color: black;}\n"
            "\n"
            "QProgressBar::chunk {\n"
            "     background-color: red;\n"
            "     width: 5px;\n"
            " }"));
    }

    ui->BatteryBar->setValue(level);
}
