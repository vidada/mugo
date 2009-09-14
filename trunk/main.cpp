#include <QtGui/QApplication>
#ifdef Q_WS_WIN
#include <QWindowsVistaStyle>
#include <QPlastiqueStyle>
#endif
#include <QDebug>
#include <QSettings>
#include <QTranslator>
#include <QLibraryInfo>
#include <QTextCodec>
#include <QDir>
#include "appdef.h"
#include "mainwindow.h"


QString getTranslationPath(){
    QString appPath = qApp->applicationDirPath();
    QStringList pathList;

#ifdef Q_WS_WIN
    pathList << appPath + "/translations";
#elif defined(Q_WS_X11)
    pathList << appPath + "/translations"
             << "/usr/share/" APPNAME "/translations"
             << "/usr/local/share/" APPNAME "/translations";
#elif defined(Q_WS_MAC)
    pathList << QFileInfo(appPath + "/../Resources/translations/").absolutePath()
             << "./translations";
#endif
    QStringList::iterator iter = pathList.begin();
    while (iter != pathList.end()){
        QDir dir(*iter);
        if (dir.exists())
            return *iter;
        ++iter;
    }

    return "./";
}

int main(int argc, char *argv[])
{
// is QFileOpenEvent received on macx??
    QApplication a(argc, argv);
    a.setOrganizationName(AUTHOR);
    a.setApplicationName(APPNAME);
    a.setApplicationVersion(VERSION);
#ifdef Q_WS_WIN
//    a.setStyle(new QWindowsVistaStyle);
    a.setStyle(new QPlastiqueStyle);
#endif

    // Load translation
    QSettings settings;
    QString locale = settings.value("language").toString();
    if (locale.isEmpty())
        locale = QLocale::system().name();

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    QTranslator myappTranslator;
    myappTranslator.load("mugo." + locale, getTranslationPath());
    a.installTranslator(&myappTranslator);

    QTextCodec::setCodecForCStrings( QTextCodec::codecForLocale() );
    QTextCodec::setCodecForTr( QTextCodec::codecForLocale() );

    MainWindow w;
    w.show();
    return a.exec();
}
