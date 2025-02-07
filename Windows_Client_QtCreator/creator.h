#ifndef CREATOR_H
#define CREATOR_H

#include <QDialog>
#include <QTcpSocket>
#include <QTimer>

namespace Ui {
class Creator;
}

class Creator : public QDialog
{
    Q_OBJECT

public:
    explicit Creator(QTcpSocket *sock, QWidget *parent = nullptr);
    ~Creator();
    void closeEvent(QCloseEvent *event);

private:
    Ui::Creator *ui;
    QTcpSocket *sock;
    QTimer *timer;
    int time;
    void onTimeout();

    void socketDisconnected();
    void onServerDataReceived();
    void onAns(QString ans);

    void commandHandler(QStringList &command);

signals:
    void windowClosed();
};

#endif // CREATOR_H
