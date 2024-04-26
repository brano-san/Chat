#include "chatserverwindow.h"
#include "qmessagebox.h"
#include "ui_chatserverwindow.h"
#include "Server.h"

#include <QtConcurrent>
#include <future>

ChatServerWindow::ChatServerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatServerWindow),
    _receiveResultWatcher(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("Чат-Сервер");
    this->setGeometry(QRect(100, 100, 590, 510));
    this->ui->clientCountLcd->setDigitCount(2);

    connect(this->ui->connectButton, &QPushButton::released, this,
            &ChatServerWindow::onConnectButtonReleased);

    connect(this->ui->sendButton, &QPushButton::released, this,
            &ChatServerWindow::onSendButtonReleased);

    connect(this->ui->closeButton, &QPushButton::released, this,
            &ChatServerWindow::onCloseConnectionButtonReleased);

    this->ui->portEdit->setText("9192");
}

ChatServerWindow::~ChatServerWindow()
{
    delete ui;
}

void ChatServerWindow::onClientConnected()
{
    if (!this->_server.isConnectionOpen())
        return;

    this->writeToChat("==К серверу подключился клиент!==");
    this->_clientCount++;
    this->ui->clientCountLcd->display(_clientCount);

    auto wathcer = new QFutureWatcher<void>(this);
    connect(wathcer, SIGNAL(finished()), this, SLOT(onClientConnected()));
    wathcer->setFuture(QtConcurrent::run(&my_chat::Server::initClient, &_server));

    this->_receiveResultWatcher = new QFutureWatcher<std::string>(this);
    connect(_receiveResultWatcher, SIGNAL(finished()), this, SLOT(onMessageReceived()));
    _receiveResultWatcher->setFuture(QtConcurrent::run(&my_chat::Server::receiveFromClient, &_server));
}

void ChatServerWindow::onMessageReceived()
{
    if (!this->_server.isConnectionOpen())
        return this->onClientDisconnected();

    QString message(_receiveResultWatcher->result().c_str());
    message.replace("\x00", "");

    this->writeToChat(QString(message) + '\n');

    this->_receiveResultWatcher = new QFutureWatcher<std::string>(this);
    connect(_receiveResultWatcher, SIGNAL(finished()), this, SLOT(onMessageReceived()));
    _receiveResultWatcher->setFuture(QtConcurrent::run(&my_chat::Server::receiveFromClient, &_server));
}

void ChatServerWindow::onSendButtonReleased()
{
    if (!this->_server.isConnectionOpen())
    {
        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText("Отправка невозможна, отсутствует подключение!");
        message.exec();
        return;
    }

    if (this->_clientCount <= 0)
        return;

    if (this->ui->messageText->toPlainText().isEmpty())
        return;

    this->_server.sendToClient(
            this->_name +
            ": " +
            this->ui->messageText->toPlainText().toStdString()
        );

    this->writeToChat(
            "Я: " +
            this->ui->messageText->toPlainText() +
            '\n'
        );

    this->ui->messageText->setText("");
}

void ChatServerWindow::onConnectButtonReleased()
{
    if (this->_server.isConnectionOpen())
    {
        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText("Сервер уже включен!");
        message.exec();
        return;
    }

    const std::string ip = "127.0.0.1";
    const unsigned short port = ui->portEdit->text().toUShort();
    this->_server.setIpAndPort(ip, port);

    try
    {
        this->_server.openConnection();

        auto wathcer = new QFutureWatcher<void>(this);
        connect(wathcer, SIGNAL(finished()), this, SLOT(onClientConnected()));
        wathcer->setFuture(QtConcurrent::run(&my_chat::Server::initClient, &_server));
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

void ChatServerWindow::onCloseConnectionButtonReleased()
{
    this->closeConnection();
}

void ChatServerWindow::setName()
{
    if (this->ui->nameEdit->text().isEmpty())
        _name = "Безымянный";
    else
        _name = this->ui->nameEdit->text().toStdString();
}

void ChatServerWindow::closeConnection()
{
    if (_receiveResultWatcher)
        this->_receiveResultWatcher->cancel();

    this->ui->isServerActiveLabel->setText("Сервер выключен");
    this->_server.closeConnection();
}

void ChatServerWindow::onConnectionOpened()
{
    this->setName();
    this->ui->isServerActiveLabel->setText("Сервер включен");
}

void ChatServerWindow::onClientDisconnected()
{
    this->writeToChat("==От сервера отключился клиент!==");
    this->_clientCount--;
    this->ui->clientCountLcd->display(_clientCount);

    if (_clientCount <= 0)
    {
        this->_receiveResultWatcher->cancel();
        this->_server.clearClientInfo();

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
    QMutexLocker ml(&this->_writeToChatMutex);
    this->ui->chatBrowser->append(message);
}
