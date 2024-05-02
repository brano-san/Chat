#include "chatclientwindow.h"
#include "ui_chatclientwindow.h"

#include <QtConcurrent>
#include <QMessageBox>
#include <QPushButton>

ChatClientWindow::ChatClientWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatClientWindow),
    _receiveResultWatcher(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("Чат-Клиент");
    setGeometry(QRect(800, 100, 590, 510));

    connect(ui->connectButton, &QPushButton::clicked, this,
            &ChatClientWindow::onConnectButtonClicked);

    connect(ui->sendButton, &QPushButton::clicked, this,
            &ChatClientWindow::onSendButtonClicked);

    connect(ui->disconectButton, &QPushButton::clicked, this,
            &ChatClientWindow::onCloseConnectionButtonClicked);
}

ChatClientWindow::~ChatClientWindow()
{
    _receiveResultWatcher->cancel();
    _client.closeConnection();
    delete ui;
}

void ChatClientWindow::onMessageReceived()
{
    if (!_client.isConnectionOpen())
    {
        writeToChat("==Вы отключились от сервера!==");
        return closeConnection();
    }

    QString message(_receiveResultWatcher->result().c_str());
    message.replace("\x00", "");

    if (message.isEmpty())
        return closeConnection();

    writeToChat(QString(message) + '\n');

    _receiveResultWatcher = new QFutureWatcher<std::string>(this);
    connect(_receiveResultWatcher, SIGNAL(finished()), this, SLOT(onMessageReceived()));
    _receiveResultWatcher->setFuture(QtConcurrent::run(&my_chat::Client::receiveFromServer, &_client));
}

void ChatClientWindow::onSendButtonClicked()
{
    if (!_client.isConnectionOpen())
    {
        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText("Отправка невозможна, отсутстует подключение!");
        message.exec();
        return;
    }

    if (ui->messageText->toPlainText().isEmpty())
        return;

    _client.sendToServer(
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

void ChatClientWindow::onConnectButtonClicked()
{
    if (_client.isConnectionOpen())
    {
        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText("Вы уже подключены к серверу!");
        message.exec();
        return;
    }

    const std::string ip = ui->ipEdit->text().toStdString();
    const unsigned short port = ui->portEdit->text().toUShort();
    _client.setIpAndPort(ip, port);

    try
    {
        _client.openConnection();
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

    writeToChat("==Вы успешно подключились к серверу!==");

    onConnectionOpened();
}

void ChatClientWindow::onCloseConnectionButtonClicked()
{
    if (!_client.isConnectionOpen())
        return;

    closeConnection();
}

void ChatClientWindow::setName()
{
    if (ui->nameEdit->text().isEmpty())
        _name = "Безымянный";
    else
        _name = ui->nameEdit->text().toStdString();
}

void ChatClientWindow::closeConnection()
{
    if (_receiveResultWatcher)
        _receiveResultWatcher->cancel();

    _client.closeConnection();
    ui->connectionLabel->setText("Подключение утеряно");
}

void ChatClientWindow::onConnectionOpened()
{
    setName();

    ui->connectionLabel->setText("Подключение установлено");

    _receiveResultWatcher = new QFutureWatcher<std::string>(this);
    connect(_receiveResultWatcher, SIGNAL(finished()), this, SLOT(onMessageReceived()));
    _receiveResultWatcher->setFuture(QtConcurrent::run(&my_chat::Client::receiveFromServer, &_client));
}

void ChatClientWindow::closeEvent(QCloseEvent *bar)
{
    closeConnection();
    bar->accept();
}

void ChatClientWindow::writeToChat(QString message)
{
    QMutexLocker ml(&_writeToChatMutex);
    ui->chatBrowser->append(message);
}
