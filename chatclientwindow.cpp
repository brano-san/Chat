#include "chatclientwindow.h"
#include "qmessagebox.h"
#include "qtconcurrentrun.h"
#include "ui_chatclientwindow.h"

ChatClientWindow::ChatClientWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatClientWindow),
    _receiveResultWatcher(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("Чат-Клиент");
    this->setGeometry(QRect(800, 100, 590, 510));

    connect(this->ui->connectButton, &QPushButton::released, this,
            &ChatClientWindow::onConnectButtonReleased);

    connect(this->ui->sendButton, &QPushButton::released, this,
            &ChatClientWindow::onSendButtonReleased);

    connect(this->ui->disconectButton, &QPushButton::released, this,
            &ChatClientWindow::onCloseConnectionButtonReleased);

    this->ui->ipEdit->setText("127.0.0.1");
    this->ui->portEdit->setText("9192");
}

ChatClientWindow::~ChatClientWindow()
{
    this->_receiveResultWatcher->cancel();
    this->_client.closeConnection();
    delete ui;
}

void ChatClientWindow::onMessageReceived()
{
    if (!this->_client.isConnectionOpen())
    {
        this->writeToChat("==Вы отключились от сервера!==");
        return this->closeConnection();
    }

    QString message(_receiveResultWatcher->result().c_str());
    message.replace("\x00", "");

    if (message.isEmpty())
        return this->closeConnection();

    this->writeToChat(QString(message) + '\n');

    this->_receiveResultWatcher = new QFutureWatcher<std::string>(this);
    connect(_receiveResultWatcher, SIGNAL(finished()), this, SLOT(onMessageReceived()));
    _receiveResultWatcher->setFuture(QtConcurrent::run(&my_chat::Client::receiveFromServer, &_client));
}

void ChatClientWindow::onSendButtonReleased()
{
    if (!_client.isConnectionOpen())
    {
        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText("Отправка невозможна, отсутстует подключение!");
        message.exec();
        return;
    }

    if (this->ui->messageText->toPlainText().isEmpty())
        return;

    this->_client.sendToServer(
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

void ChatClientWindow::onConnectButtonReleased()
{
    if (this->_client.isConnectionOpen())
    {
        QMessageBox message;
        message.setWindowTitle("Ошибка");
        message.setText("Вы уже подключены к серверу!");
        message.exec();
        return;
    }

    const std::string ip = ui->ipEdit->text().toStdString();
    const unsigned short port = ui->portEdit->text().toUShort();
    this->_client.setIpAndPort(ip, port);

    try
    {
        this->_client.openConnection();
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

    this->writeToChat("==Вы успешно подключились к серверу!==");

    this->onConnectionOpened();
}

void ChatClientWindow::onCloseConnectionButtonReleased()
{
    if (!this->_client.isConnectionOpen())
        return;

    this->closeConnection();
}

void ChatClientWindow::setName()
{
    if (this->ui->nameEdit->text().isEmpty())
        _name = "Безымянный";
    else
        _name = this->ui->nameEdit->text().toStdString();
}

void ChatClientWindow::closeConnection()
{
    if (_receiveResultWatcher)
        this->_receiveResultWatcher->cancel();

    this->_client.closeConnection();
    this->ui->connectionLabel->setText("Подключение утеряно");
}

void ChatClientWindow::onConnectionOpened()
{
    this->setName();

    this->ui->connectionLabel->setText("Подключение установлено");

    this->_receiveResultWatcher = new QFutureWatcher<std::string>(this);
    connect(_receiveResultWatcher, SIGNAL(finished()), this, SLOT(onMessageReceived()));
    _receiveResultWatcher->setFuture(QtConcurrent::run(&my_chat::Client::receiveFromServer, &_client));
}

void ChatClientWindow::closeEvent(QCloseEvent *bar)
{
    this->closeConnection();
    bar->accept();
}

void ChatClientWindow::writeToChat(QString message)
{
    QMutexLocker ml(&this->_writeToChatMutex);
    this->ui->chatBrowser->append(message);
}
