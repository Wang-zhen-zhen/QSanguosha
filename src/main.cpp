#include <QApplication>

#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <QTime>
#include <cstring>
#include <QDesktopWidget>
#include <QTextCodec>

#include "mainwindow.h"
#include "settings.h"
#include "banpair.h"
#include "server.h"
#include "audio.h"
#include "companion-table.h"

#if defined(WIN32) && defined(VS2010)
#include "breakpad/client/windows/handler/exception_handler.h"

using namespace google_breakpad;

static bool callback(const wchar_t *dump_path, const wchar_t *id,
                     void *context, EXCEPTION_POINTERS *exinfo,
                     MDRawAssertionInfo *assertion,
                     bool succeeded) {
    if (succeeded)
        qWarning("Dump file created in %s, dump guid is %ws\n", dump_path, id);
    else
        qWarning("Dump failed\n");
    return succeeded;
}

int main(int argc, char *argv[]) {
    ExceptionHandler eh(L"./dmp", NULL, callback, NULL,
                        ExceptionHandler::HANDLER_ALL);
#else
int main(int argc, char *argv[]) {
#endif
    if (argc > 1 && strcmp(argv[1], "-server") == 0)
        new QCoreApplication(argc, argv);
    else
        new QApplication(argc, argv);

#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG
    QDir::setCurrent(qApp->applicationDirPath());
#endif
#endif

    //���ǰѳ���Ҫ�õ���Qt���ͳһ���ڳ����Ŀ¼�µ�plugins��Ŀ¼�У�
    //�����Ҫ�Ѳ���Ĳ���·��֪ͨQtϵͳ
    QString pluginPath = qApp->applicationDirPath();
    pluginPath += QString("/plugins");
    qApp->addLibraryPath(pluginPath);

    // initialize random seed for later use
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

    //֧�ִ����ĵ�·�����ļ���
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("GB18030"));

    QTranslator qt_translator, translator;
    qt_translator.load("qt_zh_CN.qm");
    translator.load("sanguosha.qm");

    qApp->installTranslator(&qt_translator);
    qApp->installTranslator(&translator);

    //��Engine�Ĺ��캯���л��ȫ��ָ��Sanguosha���и�ֵ����
    new Engine;

    //��������赺�ϵͳ
    CompanionTable::init();

    Config.init();
    qApp->setFont(Config.AppFont);
    BanPair::loadBanPairs();

    if (qApp->arguments().contains("-server")) {
        Server *server = new Server(qApp);
        printf("Server is starting on port %u\n", Config.ServerPort);

        if (server->listen()) {
            printf("Starting successfully\n");
        }
        else {
            delete server;
            printf("Starting failed!\n");
        }

        return qApp->exec();
    }

    qApp->setStyleSheet(Settings::getQSSFileContent());

    MainWindow main_window;
    Sanguosha->setParent(&main_window);

    //�޸�������ϴ�����󻯴򿪳����´��ٴ�ʱ��
    //���򴰿ڵı���������һ����Խ������Ļ��Χ֮�⡱������
    QDesktopWidget *desk = QApplication::desktop();
    QRect deskAvailRect = desk->availableGeometry();
    QStyle *currentStyle = QApplication::style();
    int titleBarHeight = currentStyle->pixelMetric(QStyle::PM_TitleBarHeight);
    QSize mainWndSize = Config.value("WindowSize", QSize(1366, 706)).toSize();
    QPoint mainWndPos = Config.value("WindowPosition", QPoint(-8, -8)).toPoint();
    if (mainWndSize.height() + titleBarHeight >= deskAvailRect.height()
        && mainWndPos.y() < 0) {
        main_window.showMaximized();
    }
    else {
        main_window.show();
    }

    foreach (const QString &arg, qApp->arguments()) {
        if (arg.startsWith("-connect:")) {
            const_cast<QString &>(arg).remove("-connect:");
            Config.HostAddress = arg;
            Config.setValue("HostAddress", arg);

            main_window.startConnection();
            break;
        }
    }

    return qApp->exec();
}