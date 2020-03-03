﻿#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->comboBox_baut->setCurrentIndex(1);
    ui->comboBox_number->setCurrentIndex(1);
    ui->comboBox_channel->setCurrentIndex(0);
    number_points_show_ = ui->comboBox_number->currentText().toInt();

    this->grabKeyboard();
    //------------------
    range_ = 100;
    offset_ = 0;
    qint16 range_min = -range_;
    qint16 range_max =  range_;

    //------------------
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort serial1;
        serial1.setPort(info);
        if(serial1.open(QIODevice::ReadWrite))
        {
            init_usart_list_.append(info.portName());
            ui->comboBox_name->addItem(info.portName());
            serial1.close();
        }
    }

    //-----------------------------------------------------------------
    serialport_= new QSerialPort();
    connect(serialport_, SIGNAL(readyRead()), this, SLOT(Receieve_Bytes()));

    m_timer_= new QTimer;
    m_timer_->start(100);
    connect(m_timer_, SIGNAL(timeout()), this, SLOT(TimerTimeout()));

    m_timer1_= new QTimer;
    m_timer1_->start(100);
    connect(m_timer1_, SIGNAL(timeout()), this, SLOT(TimerTimeout1()));
    //-----------------------------------------------------------------

    QGroupBox *groupBox = new QGroupBox(QStringLiteral("显示"));
    QGridLayout *layout_grid  = new QGridLayout(groupBox);
    QChartView *chartview = new QChartView(&chart_);
    layout_grid->addWidget(chartview);
    chartview->setRenderHint(QPainter::Antialiasing);
    ui->horizontalLayout->addWidget(groupBox);

    addLines();
    addLines();
    addLines();
    addLines();
    connectMarkers();
    chart_.createDefaultAxes();
    chart_.axisX()->setRange(0,number_points_show_);
    chart_.axisY()->setRange(range_min, range_max);
}

void MainWindow::addLines()
{
    QLineSeries *series = new QLineSeries();
    m_series_.append(series);
    series->setName(QString("Channel" + QString::number(m_series_.count())));

    QList<QPointF> data;
    int offset = chart_.series().count();
    for(qint16 i = 0; i < number_points_show_-4; i++){
        qreal m_y = 100*qSin((offset*60 +i)/57.3);
        data.append(QPointF(i, m_y));
    }
    m_x_ = number_points_show_-4;
    series->append(data);
    for(qint16 i=0; i<data.size(); i++){
        qDebug() << data[i];
    }
    chart_.addSeries(series);
}

void MainWindow::removeSeries()
{
    // Remove last series from chart_
    if (m_series_.count() > 0) {
        QLineSeries *series = m_series_.last();
        chart_.removeSeries(series);
        m_series_.removeLast();
        delete series;
    }
}

void MainWindow::connectMarkers()
{
    foreach (QLegendMarker* marker, chart_.legend()->markers()) {
        // Disconnect possible existing connection to avoid multiple connections
        QObject::disconnect(marker, SIGNAL(clicked()), this, SLOT(handleMarkerClicked()));
        QObject::connect(marker, SIGNAL(clicked()), this, SLOT(handleMarkerClicked()));
    }
}

void MainWindow::disconnectMarkers()
{
    foreach (QLegendMarker* marker, chart_.legend()->markers()) {
        QObject::disconnect(marker, SIGNAL(clicked()), this, SLOT(handleMarkerClicked()));
    }
}

void MainWindow::handleMarkerClicked()
{
    QLegendMarker* marker = qobject_cast<QLegendMarker*> (sender());
    Q_ASSERT(marker);
    switch (marker->type())
    {
        case QLegendMarker::LegendMarkerTypeXY:
        {
        // Toggle visibility of series
        marker->series()->setVisible(!marker->series()->isVisible());

        // Turn legend marker back to visible, since hiding series also hides the marker
        // and we don't want it to happen now.
        marker->setVisible(true);
        // Dim the marker, if series is not visible
        qreal alpha = 1.0;

        if (!marker->series()->isVisible()) {
            alpha = 0.5;
        }

        QColor color;
        QBrush brush = marker->labelBrush();
        color = brush.color();
        color.setAlphaF(alpha);
        brush.setColor(color);
        marker->setLabelBrush(brush);

        brush = marker->brush();
        color = brush.color();
        color.setAlphaF(alpha);
        brush.setColor(color);
        marker->setBrush(brush);

        QPen pen = marker->pen();
        color = pen.color();
        color.setAlphaF(alpha);
        pen.setColor(color);
        marker->setPen(pen);
        break;
        }
    default:
        {
        qDebug() << "Unknown marker type";
        break;
        }
    }
}

void MainWindow::TimerTimeout1()
{
    static quint16 data = 0;
    quint8 send_data[6];
    QByteArray senddata;

    if(ui->pushButton_open->text()== QStringLiteral("关闭串口")){
        data++;
        send_data[0] = 0x55;
        send_data[1] = 0xaa;
        send_data[2] = (quint8)data;
        send_data[3] = (quint8)(data >>8);
        quint16 crc = data_process_.Checksum_u16(&send_data[2], 2);
        send_data[4] = (quint8)crc;
        send_data[5] = (quint8)(crc >>8);

        for(quint8 i=0; i<6; i++){
            senddata.append(send_data[i]);
        }
        serialport_->write(senddata);
    }
}

void MainWindow::TimerTimeout()
{
    QStringList usart_list;
    if(ui->pushButton_open->text()== QStringLiteral("打开串口")){
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
            QSerialPort serial1;
            serial1.setPort(info);
            if(serial1.open(QIODevice::ReadWrite))
            {
                usart_list.append(info.portName());
                serial1.close();
            }
        }
        if(usart_list != init_usart_list_){
            ui->plainTextEdit->appendPlainText(QStringLiteral("串口更新"));
            ui->comboBox_name->clear();
            init_usart_list_.clear();
            for(int i=0; i<usart_list.size(); i++){
                ui->comboBox_name->addItem(usart_list.at(i));
                init_usart_list_.append(usart_list.at(i));
            }
        }
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_pushButton_open_clicked()
{
    if(ui->pushButton_open->text() == QStringLiteral("打开串口")){
        serialport_->setPortName(ui->comboBox_name->currentText());
        serialport_->setBaudRate(ui->comboBox_baut->currentText().toInt());
        serialport_->setDataBits(QSerialPort::Data8);
        serialport_->setParity(QSerialPort::NoParity);
        serialport_->setStopBits(QSerialPort::OneStop);
        serialport_->setFlowControl(QSerialPort::NoFlowControl);
        if(serialport_->open(QIODevice::ReadWrite)){
            ui->comboBox_name->setDisabled(1);
            ui->comboBox_baut->setDisabled(1);
            ui->comboBox_channel->setDisabled(1);
            ui->comboBox_number->setDisabled(1);

            ui->pushButton_open->setText(QStringLiteral("关闭串口"));
            ui->plainTextEdit->appendPlainText(QStringLiteral("串口")+serialport_->portName()+QStringLiteral("已连接"));
        }
        else{
            QMessageBox::critical(this, tr("Error"), serialport_->errorString());
        }
    }
    else{
        if (serialport_->isOpen()){
            serialport_->close();
            ui->plainTextEdit->appendPlainText(QStringLiteral("串口")+serialport_->portName()+QStringLiteral("已关闭"));
        }
        ui->pushButton_open->setText(QStringLiteral("打开串口"));
        ui->comboBox_name->setDisabled(0);
        ui->comboBox_baut->setDisabled(0);
        ui->comboBox_channel->setDisabled(0);
        ui->comboBox_number->setDisabled(0);
    }
}

void MainWindow::Receieve_Bytes(void)
{
    QByteArray temp = serialport_->readAll();
    if(data_process_.data_process(temp)){
        qreal m_y1 = 100*qSin((m_x_)/57.3);
        qreal m_y2 = 100*qSin((m_x_ +60)/57.3);
        qreal m_y3 = 100*qSin((m_x_ +120)/57.3);
        qreal m_y4 = 100*qSin((m_x_ +180)/57.3);
        m_series_.at(0)->append(m_x_, m_y1);
        m_series_.at(1)->append(m_x_, m_y2);
        m_series_.at(2)->append(m_x_, m_y3);
        m_series_.at(3)->append(m_x_, m_y4);
        qreal dwidth= chart_.plotArea().width()/(number_points_show_); //一次滚动多少宽度
        chart_.scroll(dwidth, 0);
        m_x_ += 1.0;
        qDebug() << "rdata" << data_process_.data_receive[0];
    }
}

void MainWindow::on_pushButton_clear_clicked()
{
    ui->plainTextEdit->clear();
}

void MainWindow::on_comboBox_number_currentTextChanged(const QString &arg1)
{
    static bool first_in = true;
    if(first_in){
        first_in = false;
        return;
    }
    number_points_show_ = arg1.toInt();
    qDebug() << number_points_show_;

    for(quint16 i=0; i< m_series_.size(); i++){
        m_series_[i]->clear();
        QList<QPointF> data;
        for(qint16 j = 0; j < number_points_show_-4; j++){
            qreal m_y = 100*qSin((i*60 +j)/57.3);
            data.append(QPointF(j, m_y));
        }
        m_series_[i]->append(data);
    }
    m_x_ = number_points_show_-4;
    chart_.axisX()->setRange(0, number_points_show_);
}

void MainWindow::wheelEvent(QWheelEvent*event){

//    qDebug() << event->delta();
    if(event->delta()>0){
        range_ += event->delta()/12;
        if(range_ >30000){
            range_ =30000;
        }
    }else{
        range_ += event->delta()/12;
        if(range_ <1){
            range_ =1;
        }
    }
    qint16 range_max = range_ + offset_;
    qint16 range_min = -range_ + offset_;
    chart_.axisY()->setRange(range_min, range_max);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
//    qDebug() << "presss" << event->key();
//    if(event->key() == Qt::Key_Up)
//    {
//        offset_ -= 20;
//    }
//    else if(event->key() == Qt::Key_Down)
//    {
//        offset_ += 20;
//    }
//    range_max = range_ + offset_;
//    range_min = -range_ + offset_;
//    chart_.axisY()->setRange(range_min, range_max);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    grabMouse();
    mouse_x_ = event->x();
    mouse_y_ = event->y();
//    qDebug() << event->x() << event->y();

}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    releaseMouse();
//    qDebug() << event->x() << event->y();
    qint32 dx = event->x() - mouse_x_;
    qint32 dy = event->y() - mouse_y_;

    if(abs(dx) <150){
        if(abs(dy) >10){
            offset_ += 2* range_ * dy /chart_.plotArea().height();
            qint16 range_max = range_ + offset_;
            qint16 range_min = -range_ + offset_;
            chart_.axisY()->setRange(range_min, range_max);
        }

    }
}

void MainWindow::on_comboBox_channel_currentIndexChanged(const QString &arg1)
{
    data_process_.set_pack_len(arg1.toInt());
}
