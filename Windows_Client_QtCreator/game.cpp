#include "game.h"
#include "ui_game.h"

Game::Game(QTcpSocket *sock, QString code, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Game)
    , sock(sock)
    , timer(nullptr)
    , code(code)
{
    inProgress = false;
    host = true;
    ui->setupUi(this);
    connect(sock, &QTcpSocket::disconnected, this, &Game::socketDisconnected);
    connect(sock, &QTcpSocket::readyRead, this, &Game::onServerDataReceived);
    connect(ui->copyBtn, &QPushButton::clicked, this, &Game::onCopyBtn);
    connect(ui->exit, &QPushButton::clicked, this, [=]() {this->close();});
    connect(ui->exit_2, &QPushButton::clicked, this, [=]() {this->close();});
    connect(ui->exit_3, &QPushButton::clicked, this, [=]() {this->close();});

    connect(ui->ansA, &QPushButton::clicked, this, [=]() { onAns("A"); ui->ansA->setStyleSheet("QPushButton { border: 2px solid yellow; }");});
    connect(ui->ansB, &QPushButton::clicked, this, [=]() { onAns("B"); ui->ansB->setStyleSheet("QPushButton { border: 2px solid yellow; }");});
    connect(ui->ansC, &QPushButton::clicked, this, [=]() { onAns("C"); ui->ansC->setStyleSheet("QPushButton { border: 2px solid yellow; }");});
    connect(ui->ansD, &QPushButton::clicked, this, [=]() { onAns("D"); ui->ansD->setStyleSheet("QPushButton { border: 2px solid yellow; }");});

    QString query = "gameLobby\t" + code + "\n";
    sock->write(query.toUtf8());

    ui->game->hide();
    ui->scores->hide();
    ui->game->setEnabled(false);
}

Game::~Game()
{
    delete ui;
}

void Game::closeEvent(QCloseEvent *event) {
    QString query = "exit\n";
    sock->write(query.toUtf8());
    disconnect(timer, &QTimer::timeout, nullptr, nullptr);
    disconnect(sock, &QTcpSocket::readyRead, nullptr, nullptr);
    emit windowClosed();
    QDialog::closeEvent(event);
}

void Game::socketDisconnected(){
    this->close();
}

void Game::onServerDataReceived(){
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

void Game::commandHandler(QStringList &command){
    //informacja o kodzie gry
    if(command[0] == "gameLobby"){
        if(command.size() != 4){
            return;
        }
        ui->lobby->setTitle(command[1]);
        ui->nick->setText(command[2]);
        ui->code->setText(command[3]);
        return;
    }
    if(command[0] == "noHost"){
        if(!inProgress){
            this->close();
        }
        return;
    }
    if(command[0] == "player"){
        if(command.size() != 2){
            return;
        }
        ui->nickBrowser->append(command[1]);
        return;
    }
    if(command[0] == "playerDisconnected"){
        if(command.size() != 2){
            return;
        }
        ui->lobby->show();
        ui->game->hide();
        ui->scores->hide();
        QString disconnectedNick = command[1];
        qDebug() << disconnectedNick;
        QString currentText = ui->nickBrowser->toPlainText();
        QStringList lines = currentText.split('\n');
        lines.removeAll(disconnectedNick);
        ui->nickBrowser->setPlainText(lines.join('\n'));
        return;
    }
    if(command[0] == "start"){
        inProgress = true;
        ui->scores->hide();
        ui->lobby->hide();
        ui->game->show();
        return;
    }
    if(command[0] == "question"){
        if(command.size() != 7){
            return;
        }
        ui->game->setEnabled(true);
        ui->lobby->hide();
        ui->scores->hide();
        ui->game->show();
        ui->otherPoints->clear();
        ui->points->setText("");

        ui->ansA->setStyleSheet("");
        ui->ansB->setStyleSheet("");
        ui->ansC->setStyleSheet("");
        ui->ansD->setStyleSheet("");

        ui->question->setText(command[1]);
        ui->ansA->setText(command[2]);
        ui->ansB->setText(command[3]);
        ui->ansC->setText(command[4]);
        ui->ansD->setText(command[5]);

        ui->timer->setMaximum(command[6].toInt());
        time = command[6].toInt();
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Game::onTimeout);
        timer->start(1000);
        ui->timer->setValue(time);

        return;
    }
    if(command[0] == "reducedTime"){
        if(command.size() != 2){
            return;
        }
        ui->timer->setValue(command[1].toInt());
        time = command[1].toInt();
        return;
    }
    if(command[0] == "proper"){
        if(command.size() != 2){
            return;
        }

        if(command[1] == 'A'){
            ui->ansA->setStyleSheet("QPushButton { border: 2px solid green; }");
        }
        if(command[1] == 'B'){
            ui->ansB->setStyleSheet("QPushButton { border: 2px solid green; }");
        }
        if(command[1] == 'C'){
            ui->ansC->setStyleSheet("QPushButton { border: 2px solid green; }");
        }
        if(command[1] == 'D'){
            ui->ansD->setStyleSheet("QPushButton { border: 2px solid green; }");
        }

        return;
    }
    if(command[0] == "score"){
        if(command.size() == 2){
            QString query = "Your current score: " + command[1];
            ui->points->setText(query);
        }
        else if(command.size() == 3){
            QString query = command[1] + ": " + command[2];
            ui->otherPoints->append(query);
        }

        // Uruchom timer po wysłaniu wyniku
        QTimer::singleShot(3000, this, [this]() {
            QString nextQuery = "next\n";
            sock->write(nextQuery.toUtf8());
        });

        return;
    }
    //leader <nick> <punkty>
    if(command[0] == "leader"){
        inProgress = false;
        if(command.size() == 3){
            QString query = "Winner is \"" + command[1] + "\" with " + command[2] + " points";
            ui->leader->setText(query);
        }
        if(!host){
            this->close();
        }

        return;
    }

}

void Game::onCopyBtn(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->code->text());
}

void Game::onTimeout(){
    time -= 1;
    if(time == 0){
        ui->game->setEnabled(false);
        QString query = "endTime\n";
        sock->write(query.toUtf8());
    }
    if(time >= 0){
        ui->timer->setValue(time);
    }
    if(time == -3){
        disconnect(timer, &QTimer::timeout, nullptr, nullptr);
        timer->stop();
        timer->deleteLater();
        timer = nullptr;
        QString query = "scores\n";
        sock->write(query.toUtf8());

        ui->scores->show();
        ui->game->hide();
    }
}


void Game::onAns(QString ans){
    ui->game->setEnabled(false);
    QString query = "answer\t" + ans + "\n";
    sock->write(query.toUtf8());
}
