#include "calculator.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 从资源文件加载样式表，保证程序部署后界面风格一致
    QFile styleFile(QStringLiteral(":/style.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));

    Calculator window;
    window.show();

    return app.exec();
}
