#ifndef GAME_H
#define GAME_H

#include <QDialog>
#include <QTcpSocket>
#include <QClipboard>
#include <QTimer>

namespace Ui {
class Game;
}

class Game : public QDialog
{
    Q_OBJECT

public:
    explicit Game(QTcpSocket *sock, QString code, QWidget *parent = nullptr);
    ~Game();
    void closeEvent(QCloseEvent *event);

private:
    Ui::Game *ui;
    QTcpSocket *sock;
    QTimer *timer;
    QString code;
    int time;
    bool inProgress;
    bool host;

    void socketDisconnected();
    void onServerDataReceived();
    void onCopyBtn();
    void onTimeout();
    void onAns(QString ans);

    void commandHandler(QStringList &command);
signals:
    void windowClosed();
};

#endif // GAME_H
