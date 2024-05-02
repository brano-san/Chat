#ifndef CHATCLIENTWINDOW_H
#define CHATCLIENTWINDOW_H

#include <QCloseEvent>
#include <QWidget>

#include "Client.h"
#include "qfuturewatcher.h"

namespace Ui {
class ChatClientWindow;
}

class ChatClientWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatClientWindow(QWidget *parent = nullptr);

    ~ChatClientWindow();

private slots:
    void onMessageReceived();

    void onSendButtonClicked();

    void onConnectButtonClicked();

    void onCloseConnectionButtonClicked();

private:
    void setName();

    void closeConnection();

    void onConnectionOpened();

    void closeEvent(QCloseEvent *bar);

    void writeToChat(QString message);

private:
    QMutex _writeToChatMutex;

    std::string _name;

    Ui::ChatClientWindow *ui;

    my_chat::Client _client;

    QFutureWatcher<std::string> *_receiveResultWatcher;
};

#endif // CHATCLIENTWINDOW_H
