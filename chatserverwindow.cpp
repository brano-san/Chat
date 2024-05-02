#include "chatserverwindow.h"
#include "ui_chatserverwindow.h"

#include <QtConcurrent>
#include <QMessageBox>
#include <future>

ChatServerWindow::ChatServerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatServerWindow),
    _receiveResultWatcher(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("Чат-Сервер");
    setGeometry(QRect(100, 100, 590, 510));
    ui->clientCountLcd->setDigitCount(2);

    connect(ui->connectButton, &QPushButton::clicked, this,
            &ChatServerWindow::onConnectButtonClicked);

    connect(ui->sendButton, &QPushButton::clicked, this,
            &ChatServerWindow::onSendButtonClicked);

    connect(ui->closeButton, &QPushButton::clicked, this,
            &ChatServerWindow::onCloseConnectionButtonClicked);
}

ChatServerWindow::~ChatServerWindow()
{
    _receiveResultWatcher->cancel();
    _server.closeConnection();
    delete ui;
}

void ChatServerWindow::onClientConnected()
{
    if (!_server.isConnectionOpen())
        return;

    writeToChat("==К серверу подключился клиент!==");
    _clientCount++;
    ui->clientCountLcd->display(_clientCount);

    auto watcher = new QFutureWatcher<void>(this);
    connect(watcher, SIGNAL(finished()), this, SLOT(onClientConnected()));
    watcher->setFuture(QtConcurrent::run(&my_chat::Server::initClient, &_server));

    _receiveResultWatcher = new QFutureWatcher<std::string>(this);
    connect(_receiveResultWatcher, SIGNAL(finished()), this, SLOT(onMessageReceived()));
    _receiveResultWatcher->setFuture(QtConcurrent::run(&my_chat::Server::receiveFromClient, &_server));
}

void ChatServerWindow::onMessageReceived()
{
    if (!_server.isConnectionOpen())
        return onClientDisconnected();

    QString message(_receiveResultWatcher->result().c_str());
    message.replace("\x00", "");

    writeToChat(QString(message) + '\n');

    _receiveResultWatcher = new QFutureWatcher<std::string>(this);
    connect(_receiveResultWatcher, SIGNAL(finished()), this, SLOT(onMessageReceived()));
    _receiveResultWatcher->setFuture(QtConcurrent::run(&my_chat::Server::receiveFromClient, &_server));
}

void ChatServerWindow::onSendButtonClicked()
{
    if (!_server.isConnectionOpen())
    {
        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText("Отправка невозможна, отсутствует подключение!");
        message.exec();
        return;
    }

    if (_clientCount <= 0)
        return;

    if (ui->messageText->toPlainText().isEmpty())
        return;

    _server.sendToClient(
            _name +
            ": " +
            ui->messageText->toPlainText().toStdString()
        );

    writeToChat(
            "Я: " +
            ui->messageText->toPlainText() +
            '\n'
        );

    ui->messageText->setText("");
}

void ChatServerWindow::onConnectButtonClicked()
{
    if (_server.isConnectionOpen())
    {
        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText("Сервер уже включен!");
        message.exec();
        return;
    }

    const std::string ip = "127.0.0.1";
    const unsigned short port = ui->portEdit->text().toUShort();
    _server.setIpAndPort(ip, port);

    try
    {
        _server.openConnection();

        auto watcher = new QFutureWatcher<void>(this);
        connect(watcher, SIGNAL(finished()), this, SLOT(onClientConnected()));
        watcher->setFuture(QtConcurrent::run(&my_chat::Server::initClient, &_server));
    }
    catch (std::exception& e)
    {
        std::string msg { "Возникла ошибка. \n" };
        msg += e.what();
        msg += "\nИсправьте ошибку и попробуйте снова";

        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText(QString(msg.c_str()));
        message.exec();
        return;
    }

    onConnectionOpened();
}

void ChatServerWindow::onCloseConnectionButtonClicked()
{
    closeConnection();
}

void ChatServerWindow::setName()
{
    if (ui->nameEdit->text().isEmpty())
        _name = "Безымянный";
    else
        _name = ui->nameEdit->text().toStdString();
}

void ChatServerWindow::closeConnection()
{
    if (_receiveResultWatcher)
        _receiveResultWatcher->cancel();

    ui->isServerActiveLabel->setText("Сервер выключен");
    _server.closeConnection();
}

void ChatServerWindow::onConnectionOpened()
{
    setName();
    ui->isServerActiveLabel->setText("Сервер включен");
}

void ChatServerWindow::onClientDisconnected()
{
    writeToChat("==От сервера отключился клиент!==");
    _clientCount--;
    ui->clientCountLcd->display(_clientCount);

    if (_clientCount <= 0)
    {
        _receiveResultWatcher->cancel();
        _server.clearClientInfo();

        return;
    }
}

void ChatServerWindow::closeEvent(QCloseEvent *bar)
{
    closeConnection();
    bar->accept();
}

void ChatServerWindow::writeToChat(QString message)
{
    QMutexLocker ml(&_writeToChatMutex);
    ui->chatBrowser->append(message);
}
