#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QClipboard>
#include <QThread>

#include "game.h"
#include "creator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QTcpSocket *sock;
    QTimer *connTimeoutTimer;

    Game *gameWindow;
    Creator *creatorWindow;

    int playerCount;

    void connectToTCP(); // funkcja łącząca clienta do servera
    void socketConnected(); // funkcja obsługująca połączenie się
    void socketDisconnected(); // funckja obsługująca odłączenie się
    void onServerDataReceived(); // funckja obsługująca otrzymywanie paczek


    void commandHandler(QStringList &command);
    bool nickAsk();
    bool codeAsk();

    void onCreateBtn();
    void onJoinBtn();

    void onCancelBtn();
    void onNextBtn();
    void onNextAddBtn();
    void onEndBtn();
    void onCopyBtn();
    void onStartBtn();
    void onExitBtn();

signals:
    void nickGood();
    void codeGood();
};
#endif // MAINWINDOW_H
