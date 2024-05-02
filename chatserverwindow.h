#ifndef CHATSERVERWINDOW_H
#define CHATSERVERWINDOW_H

#include <QCloseEvent>
#include <QWidget>

#include "Server.h"
#include "qfuturewatcher.h"

namespace Ui {
class ChatServerWindow;
}

class ChatServerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatServerWindow(QWidget *parent = nullptr);

    ~ChatServerWindow();

private slots:
    void onClientConnected();

    void onMessageReceived();

    void onSendButtonClicked();

    void onConnectButtonClicked();

    void onCloseConnectionButtonClicked();

private:
    void setName();

    void closeConnection();

    void onConnectionOpened();

    void onClientDisconnected();

    void closeEvent(QCloseEvent *bar);

    void writeToChat(QString message);

private:
    QMutex _writeToChatMutex;

    std::string _name;

    int _clientCount = 0;

    my_chat::Server _server;

    Ui::ChatServerWindow *ui;

    QFutureWatcher<std::string> *_receiveResultWatcher;
};

#endif // CHATSERVERWINDOW_H
