#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , sock(nullptr)
    , gameWindow(nullptr)
{
    playerCount = 0;
    ui->setupUi(this);

    ui->nickLabel->setText(" ");
    ui->codeLabel->setText(" ");

    ui->mainMenu->setEnabled(false);
    ui->lobby->hide();
    ui->creatingAddQuiz->hide();
    ui->creatingQuiz->hide();

    ui->startBtn->setEnabled(false);

    connectToTCP();

    connect(ui->connectingInfo, &QPushButton::clicked, this, &MainWindow::connectToTCP);

    connect(ui->createBtn, &QPushButton::clicked, this, &MainWindow::onCreateBtn);
    connect(ui->joinBtn, &QPushButton::clicked, this, &MainWindow::onJoinBtn);

    connect(ui->cancelBtn, &QPushButton::clicked, this, &MainWindow::onCancelBtn);
    connect(ui->nextBtn, &QPushButton::clicked, this, &MainWindow::onNextBtn);
    connect(ui->cancelAddBtn, &QPushButton::clicked, this, &MainWindow::onCancelBtn);
    connect(ui->nextAddBtn, &QPushButton::clicked, this, &MainWindow::onNextAddBtn);
    connect(ui->endBtn, &QPushButton::clicked, this, &MainWindow::onEndBtn);
    connect(ui->copyBtn, &QPushButton::clicked, this, &MainWindow::onCopyBtn);
    connect(ui->startBtn, &QPushButton::clicked, this, &MainWindow::onStartBtn);
    connect(ui->exitBtn, &QPushButton::clicked, this, &MainWindow::onExitBtn);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectToTCP(){
    ui->connectingInfo->setText("Connecting...");
    ui->connectingInfo->setStyleSheet("QPushButton { color: auto; font-weight: auto; }");
    ui->connectingInfo->setEnabled(false);

    if(sock)
        delete sock;
    sock = new QTcpSocket(this);
    connTimeoutTimer = new QTimer(this);
    connTimeoutTimer->setSingleShot(true);
    connect(connTimeoutTimer, &QTimer::timeout, [&]{
        sock->abort();
        sock->deleteLater();
        sock = nullptr;
        connTimeoutTimer->deleteLater();
        connTimeoutTimer=nullptr;
        ui->connectingInfo->setText("Timeout: Try again");
        ui->connectingInfo->setEnabled(true);
        ui->connectingInfo->setStyleSheet("QPushButton { color: red; font-weight: bold; }");
    });


    connect(sock, &QTcpSocket::connected, this, &MainWindow::socketConnected);
    connect(sock, &QTcpSocket::disconnected, this, &MainWindow::socketDisconnected);
    connect(sock, &QTcpSocket::readyRead, this, &MainWindow::onServerDataReceived);

    QString ip = ui->ip->text();
    sock->connectToHost(ip, 8080);
    connTimeoutTimer->start(2000);
}

void MainWindow::socketConnected(){
    connTimeoutTimer->stop();
    connTimeoutTimer->deleteLater();
    connTimeoutTimer=nullptr;

    ui->connectingInfo->setText("connected");
    ui->connectingInfo->setStyleSheet("QPushButton { color: green; font-weight: auto; }");
    ui->mainMenu->setEnabled(true);
}

void MainWindow::socketDisconnected(){
    ui->connectingInfo->setText("disconnected: try again");
    ui->connectingInfo->setEnabled(true);
    ui->connectingInfo->setStyleSheet("QPushButton { color: red; font-weight: bold; }");
    ui->mainMenu->setEnabled(false);

    ui->lobby->hide();
    ui->creatingAddQuiz->hide();
    ui->creatingQuiz->hide();
    ui->mainMenu->show();
}

void MainWindow::onServerDataReceived(){
    QByteArray incomingData = sock->readAll();
    qDebug() << incomingData;

    while (incomingData.contains('\n')) {
        int newlineIndex = incomingData.indexOf('\n');
        QByteArray line = incomingData.left(newlineIndex);
        incomingData.remove(0, newlineIndex + 1);

        QString strLine = QString::fromUtf8(line);
        QStringList parts = strLine.split('\t');

        // for (const QString &part : parts) {
        //     qDebug() << part;
        // }

        commandHandler(parts);
    }
}

bool MainWindow::nickAsk(){
    ui->nickLabel->setText(" ");

    QString nick = ui->nickEdit->text();
    if (nick.isEmpty()) {
        ui->nickEdit->setStyleSheet("QLineEdit { background-color: red; }");
        return false;
    }
    else{
        ui->nickEdit->setStyleSheet("QLineEdit { background-color: auto; }");
    }
    QString query = "nick\t" + nick + '\n';
    sock->write(query.toUtf8());
    return true;
}

bool MainWindow::codeAsk(){
    ui->codeLabel->setText(" ");

    QString code = ui->codeEdit->text();
    if (code.isEmpty()) {
        ui->codeEdit->setStyleSheet("QLineEdit { background-color: red; }");
        return false;
    }
    else{
        ui->codeEdit->setStyleSheet("QLineEdit { background-color: auto; }");
    }
    QString query = "code\t" + code + '\n';
    sock->write(query.toUtf8());
    return true;
}

void MainWindow::commandHandler(QStringList &command){
    //informacja zwrotna o poprawności nicku: ()
    if(command[0] == "nick"){
        if(command.size() != 2){
            nickAsk();
            return;
        }
        if(command[1] == '1'){
            emit nickGood();
        }
        else{
            disconnect(this, nickGood, nullptr, nullptr);
            ui->nickEdit->setStyleSheet("QLineEdit { background-color: red; }");
            ui->nickLabel->setText("nick already exist on server");
        }
        return;
    }
    //informacja zwrotna o poprawności kodu:
    if(command[0] == "code"){
        if(command.size() != 2){
            codeAsk();
            return;
        }
        if(command[1] == '1'){
            emit codeGood();
        }
        else{
            disconnect(this, codeGood, nullptr, nullptr);
            ui->codeEdit->setStyleSheet("QLineEdit { background-color: red; }");
            ui->codeLabel->setText("wrond code");
        }
        return;
    }
    //informacja o kodzie gry
    if(command[0] == "gameCode"){
        if(command.size() != 2){
            return;
        }
        ui->code->setText(command[1]);
        return;
    }
    //informcja o łączących się graczach
    if(command[0] == "player"){
        if(command.size() != 2){
            return;
        }
        playerCount++;
        ui->nickBrowser->append(command[1]);
        ui->startBtn->setEnabled(true);
        return;
    }
    //informacja o rozłączonym graczu
    if(command[0] == "playerDisconnected"){
        if(command.size() != 2){
            return;
        }
        playerCount--;
        qDebug() << playerCount;
        if(playerCount == 0){
            ui->startBtn->setEnabled(false);
        }
        QString disconnectedNick = command[1];
        qDebug() << disconnectedNick;
        QString currentText = ui->nickBrowser->toPlainText();
        QStringList lines = currentText.split('\n');
        lines.removeAll(disconnectedNick);
        ui->nickBrowser->setPlainText(lines.join('\n'));
        return;
    }
}

void MainWindow::onCreateBtn(){
    if(!nickAsk()) return;
    disconnect(this, nickGood, nullptr, nullptr);
    connect(this, MainWindow::nickGood, this, [&]{
        disconnect(this, nickGood, nullptr, nullptr);
        ui->creatingQuiz->show();
        ui->mainMenu->hide();
    });
}
void MainWindow::onJoinBtn(){
    if(!nickAsk()) return;
    disconnect(this, nickGood, nullptr, nullptr);
    connect(this, MainWindow::nickGood, this, [&]{
        disconnect(this, nickGood, nullptr, nullptr);
        if(!codeAsk()) return;
            connect(this, MainWindow::codeGood, this, [&]{
                disconnect(this, codeGood, nullptr, nullptr);
                disconnect(sock, &QTcpSocket::readyRead, nullptr, nullptr);
                QThread::msleep(100);
                QString code = ui->codeEdit->text();
                gameWindow = new Game(sock, code);
                gameWindow->show();
                this->hide();
                connect(gameWindow, &Game::windowClosed, this, [&]{
                    this->show();
                    // QThread::msleep(100);
                    connect(sock, &QTcpSocket::readyRead, this, &MainWindow::onServerDataReceived);
            });
        });
    });
}

void MainWindow::onCancelBtn(){
    QString query = "cancel\n";
    sock->write(query.toUtf8());

    ui->creatingQuiz->hide();
    ui->creatingAddQuiz->hide();
    ui->mainMenu->show();
}

void MainWindow::onNextBtn(){
    QString name = ui->nameEdit->text();
    if (name.isEmpty()) {
        ui->nameEdit->setStyleSheet("QLineEdit { background-color: red; }");
        return;
    }
    else{
        ui->nameEdit->setStyleSheet("QLineEdit { background-color: auto; }");
    }

    int aTime = ui->answerTime->value();
    int rTime = ui->reducedTime->value();

    QString query = "create\t" + name + '\t' + QString::number(aTime) + '\t' + QString::number(rTime) + '\n';
    sock->write(query.toUtf8());

    ui->creatingQuiz->hide();
    ui->creatingAddQuiz->show();
}

void MainWindow::onNextAddBtn(){
    bool emptyFlag = false;
    QString question = ui->questionEdit->text();
    if (question.isEmpty()) {
        ui->questionEdit->setStyleSheet("QLineEdit { background-color: red; }");
        emptyFlag = true;
    }
    else{
        ui->questionEdit->setStyleSheet("QLineEdit { background-color: auto; }");
    }

    QString ansA = ui->ansAedit->text();
    if (ansA.isEmpty()) {
        ui->ansAedit->setStyleSheet("QLineEdit { background-color: red; }");
        emptyFlag = true;
    }
    else{
        ui->ansAedit->setStyleSheet("QLineEdit { background-color: auto; }");
    }
    QString ansB = ui->ansBedit->text();
    if (ansB.isEmpty()) {
        ui->ansBedit->setStyleSheet("QLineEdit { background-color: red; }");
        emptyFlag = true;
    }
    else{
        ui->ansBedit->setStyleSheet("QLineEdit { background-color: auto; }");
    }
    QString ansC = ui->ansCedit->text();
    if (ansC.isEmpty()) {
        ui->ansCedit->setStyleSheet("QLineEdit { background-color: red; }");
        emptyFlag = true;
    }
    else{
        ui->ansCedit->setStyleSheet("QLineEdit { background-color: auto; }");
    }
    QString ansD = ui->ansDedit->text();
    if (ansD.isEmpty()) {
        ui->ansDedit->setStyleSheet("QLineEdit { background-color: red; }");
        emptyFlag = true;
    }
    else{
        ui->ansDedit->setStyleSheet("QLineEdit { background-color: auto; }");
    }

    if(emptyFlag) return;

    QString query = "add\t" + question + '\t' + ansA + '\t' + ansB + '\t' + ansC + '\t' + ansD + '\t' + ui->answerBox->currentText() + '\n';
    sock->write(query.toUtf8());

    ui->ansAedit->setText("");
    ui->ansBedit->setText("");
    ui->ansCedit->setText("");
    ui->ansDedit->setText("");
    ui->questionEdit->setText("");
    ui->endBtn->setEnabled(true);
}

void MainWindow::onEndBtn(){
    QString query = "gameCode\n";
    sock->write(query.toUtf8());
    ui->nickBrowser->clear();
    ui->creatingAddQuiz->hide();
    ui->lobby->show();
}

void MainWindow::onCopyBtn(){
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->code->text());
}

void MainWindow::onStartBtn(){
    disconnect(sock, &QTcpSocket::readyRead, nullptr, nullptr);
    QThread::msleep(100);
    creatorWindow = new Creator(sock);
    creatorWindow->show();
    QString query = "start\n";
    sock->write(query.toUtf8());
    this->hide();
    connect(creatorWindow, &Creator::windowClosed, this, [&]{
        ui->nickBrowser->clear();
        ui->startBtn->setEnabled(false);
        this->show();
        QThread::msleep(100);
        connect(sock, &QTcpSocket::readyRead, this, &MainWindow::onServerDataReceived);
        onEndBtn();
    });
}
void MainWindow::onExitBtn(){
    QString query = "exit\n";
    sock->write(query.toUtf8());
    ui->creatingQuiz->hide();
    ui->creatingAddQuiz->hide();
    ui->lobby->hide();
    ui->mainMenu->show();
}
