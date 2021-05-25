/**
 * Author: James Hwang
 * Copyright (c)  2015  aurender.com
 *
 */


#include "Logger.h"

#include <iostream>
#include <fstream>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QTime>
#include <QVariant>

// #include "utils/ArirangUtils.h"

#define LOGFILE_SIZE 1024 * 256

#define RELEASE_LEVEL_THRESHOLD 0
#define DEBUG_LEVEL_THRESHOLD LOGEXTRA
#define LOG_SQL_QUERIES 1

using namespace std;

ofstream logStream;
static int s_threshold = -1;
QMutex s_mutex;
bool shutdownInProgress = false;


namespace Logger
{

static void
log( const char *msg, unsigned int debugLevel, bool toDisk = true )
{
    if ( s_threshold < 0 )
    {
        if ( qApp->arguments().contains( "--verbose" ) )
        {
            s_threshold = LOGTHIRDPARTY;
        }
        else
        {
#ifdef QT_NO_DEBUG
            s_threshold = RELEASE_LEVEL_THRESHOLD;
#else
            s_threshold = DEBUG_LEVEL_THRESHOLD;
#endif
        }
    }

    if ( debugLevel > LOGTHIRDPARTY )
        toDisk = false;

#ifdef LOG_SQL_QUERIES
    if ( debugLevel == LOGSQL )
        toDisk = true;
#endif

    if ( toDisk || (int)debugLevel <= s_threshold )
    {
        QMutexLocker lock( &s_mutex );

#ifdef LOG_SQL_QUERIES
        if ( debugLevel == LOGSQL )
            logStream << "JSQLQUERY: ";
#endif

        // logStream << QDate::currentDate().month()  << "."
        //           << QDate::currentDate().day()    << "-"
        //           << QTime::currentTime().hour()   << ":"
        //           << QTime::currentTime().minute() << ":"
        //           << QTime::currentTime().second()
        //           << "[" << QString::number( debugLevel ).toUtf8().data() << "]: "
        //           << msg << endl;
        logStream << QDateTime::currentDateTime().toString("hh:mm:ss-MM.dd").toUtf8().data()
                  << "[" << QString::number( debugLevel ).toUtf8().data() << "]: "
                  << msg << endl;

        logStream.flush();
    }

    if ( debugLevel <= LOGEXTRA || (int)debugLevel <= s_threshold )
    {
        QMutexLocker lock( &s_mutex );

        wcout << QTime::currentTime().toString("hh:mm:ss").toUtf8().data()
              << "[" << QString::number( debugLevel ).toStdWString().c_str() << "]: "
              << msg << endl;

        wcout.flush();
    }
}


void
ArirangLogHandler( QtMsgType type, const QMessageLogContext& context, const QString& msg )
{
    static QMutex s_mutex;

    QByteArray ba = msg.toUtf8();
    const char* message = ba.constData();

    QMutexLocker locker( &s_mutex );
    switch( type )
    {
        case QtDebugMsg:
            log( message, LOGTHIRDPARTY );
            break;

        case QtCriticalMsg:
            log( message, 0 );
            break;

        case QtWarningMsg:
            log( message, 0 );
            break;

        case QtFatalMsg:
            log( message, 0 );
            break;
    }
}



void
setupLogfile( QFile& f )
{
    if ( QFileInfo( f ).size() > LOGFILE_SIZE )
    {
        QByteArray lc;
        {
            f.open( QIODevice::ReadOnly | QIODevice::Text );
            f.seek( f.size() - ( LOGFILE_SIZE - ( LOGFILE_SIZE / 4 ) ) );
            lc = f.readAll();
            f.close();
        }

        f.remove();

        {
            f.open( QIODevice::WriteOnly | QIODevice::Text );
            f.write( lc );
            f.close();
        }
    }

#ifdef OFSTREAM_CAN_OPEN_WCHAR_FILE_NAMES
    // this is not supported in upstream libstdc++ as shipped with GCC
    // GCC needs the patch from https://gcc.gnu.org/ml/libstdc++/2011-06/msg00066.html applied
    // we could create a CMake check like the one for taglib, but I don't care right now :P
    logStream.open( f.fileName().toStdWString().c_str() );
#else
    logStream.open( f.fileName().toStdString().c_str() );
#endif

    qInstallMessageHandler( ArirangLogHandler );
}

}

using namespace Logger;

TLog::TLog( unsigned int debugLevel )
    : QDebug( &m_msg )
    , m_debugLevel( debugLevel )
{
}


TLog::~TLog()
{
    log( m_msg.toUtf8().data(), m_debugLevel );
}


void
tLogNotifyShutdown()
{
    QMutexLocker locker( &s_mutex );
    shutdownInProgress = true;
}
