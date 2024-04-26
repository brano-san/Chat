#include "chatserverwindow.h"
#include "chatclientwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ChatServerWindow serverForm;
    serverForm.show();

    ChatClientWindow clientForm;
    clientForm.show();

    return a.exec();
}
