#include "creator.h"
#include "ui_creator.h"

Creator::Creator(QTcpSocket *sock, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Creator)
    , sock(sock)
    , timer(nullptr)
{
    ui->setupUi(this);

    connect(sock, &QTcpSocket::disconnected, this, &Creator::socketDisconnected);
    connect(sock, &QTcpSocket::readyRead, this, &Creator::onServerDataReceived);
    connect(ui->exit, &QPushButton::clicked, this, [=]() {this->close();});
    connect(ui->exit_, &QPushButton::clicked, this, [=]() {this->close();});

    ui->data->show();
    ui->leaderboard->hide();
}

Creator::~Creator()
{
    delete ui;
}

void Creator::closeEvent(QCloseEvent *event) {
    QString query = "noHost\n";
    sock->write(query.toUtf8());
    disconnect(timer, &QTimer::timeout, nullptr, nullptr);
    disconnect(sock, &QTcpSocket::readyRead, nullptr, nullptr);
    emit windowClosed();
    QDialog::closeEvent(event);
}

void Creator::socketDisconnected(){
    this->close();
}

void Creator::onServerDataReceived(){
    QByteArray incomingData = sock->readAll();
    qDebug() << incomingData;

    while (incomingData.contains('\n')) {
        int newlineIndex = incomingData.indexOf('\n');
        QByteArray line = incomingData.left(newlineIndex);
        incomingData.remove(0, newlineIndex + 1);

        QString strLine = QString::fromUtf8(line);
        QStringList parts = strLine.split('\t');

        commandHandler(parts);
    }
}

void Creator::commandHandler(QStringList &command){
    //question <pytanie> <poprawna odpowiedź> <czas>
    if(command[0] == "question"){
        if(command.size() != 4){
            return;
        }
        ui->question->setText(command[1]);

        ui->aLabel->setStyleSheet("");
        ui->bLabel->setStyleSheet("");
        ui->cLabel->setStyleSheet("");
        ui->dLabel->setStyleSheet("");

        ui->ansA->setMaximum(0);
        ui->ansA->setValue(0);
        ui->ansB->setMaximum(0);
        ui->ansB->setValue(0);
        ui->ansC->setMaximum(0);
        ui->ansC->setValue(0);
        ui->ansD->setMaximum(0);
        ui->ansD->setValue(0);
        ui->ansN->setMaximum(0);
        ui->ansN->setValue(0);

        if(command[2] == "A"){
            ui->aLabel->setStyleSheet("QLabel { background-color: green; }");
        }
        else if(command[2] == "B"){
            ui->bLabel->setStyleSheet("QLabel { background-color: green; }");
        }
        else if(command[2] == "C"){
            ui->cLabel->setStyleSheet("QLabel { background-color: green; }");
        }
        else if(command[2] == "D"){
            ui->dLabel->setStyleSheet("QLabel { background-color: green; }");
        }

        ui->timer->setMaximum(command[3].toInt());
        time = command[3].toInt();
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Creator::onTimeout);
        timer->start(1000);
        ui->timer->setValue(time);
        return;
    }
    //reducedTime <time>
    if(command[0] == "reducedTime"){
        if(command.size() != 2){
            return;
        }
        ui->timer->setValue(command[1].toInt());
        time = command[1].toInt();
        return;
    }
    //answer <odpowidz> <nick>
    if(command[0] == "answer"){
        if(command.size() != 3){
            return;
        }
        onAns(command[1]);
    }
    //leader <nick> <punkty>
    if(command[0] == "leader"){
        if(command.size() == 3){
            ui->leaderBrowser->clear();
            ui->data->hide();
            ui->leaderboard->show();

            QString query = "Winner is \"" + command[1] + "\" with " + command[2] + " points";
            ui->leader->setText(query);
            query = "scores\n";
            sock->write(query.toUtf8());
        }
        return;
    }
    if(command[0] == "score"){
        if(command.size() == 3){
            QString query = command[1] + ": " + command[2];
            ui->leaderBrowser->append(query);
            return;
        }
        return;
    }
}

void Creator::onTimeout(){
    time -= 1;
    if(time == 0){
        disconnect(timer, &QTimer::timeout, nullptr, nullptr);
        timer->stop();
        timer->deleteLater();
        timer = nullptr;
    }
    if(time >= 0){
        ui->timer->setValue(time);
    }
}

void Creator::onAns(QString ans){
    ui->ansA->setMaximum(ui->ansA->maximum() + 1);
    ui->ansB->setMaximum(ui->ansB->maximum() + 1);
    ui->ansC->setMaximum(ui->ansC->maximum() + 1);
    ui->ansD->setMaximum(ui->ansD->maximum() + 1);
    ui->ansN->setMaximum(ui->ansN->maximum() + 1);

    if(ans == "A"){
        ui->ansA->setValue(ui->ansA->value() + 1);
    }
    if(ans == "B"){
        ui->ansB->setValue(ui->ansB->value() + 1);
    }
    if(ans == "C"){
        ui->ansC->setValue(ui->ansC->value() + 1);
    }
    if(ans == "D"){
        ui->ansD->setValue(ui->ansD->value() + 1);
    }
    if(ans == "none"){
        ui->ansN->setValue(ui->ansN->value() + 1);
    }
}
