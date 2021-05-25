/**
 * Author: James Hwang
 * Copyright (c)  2015  aurender.com
 *
 */


#ifndef ARIRANG_LOGGER_H
#define ARIRANG_LOGGER_H

#include <QDebug>
#include <QFile>

#include "DllMacro.h"

#define LOGDEBUG 1
#define LOGINFO 2
#define LOGEXTRA 5
#define LOGVERBOSE 8
#define LOGTHIRDPARTY 9
#define LOGSQL 10

namespace Logger
{
    class DLLEXPORT TLog : public QDebug
    {
    public:
        TLog( unsigned int debugLevel = 0 );
        virtual ~TLog();

    private:
        QString m_msg;
        unsigned int m_debugLevel;
    };

    class DLLEXPORT TDebug : public TLog
    {
    public:
        TDebug( unsigned int debugLevel = LOGDEBUG ) : TLog( debugLevel )
        {
        }
    };

    class DLLEXPORT TSqlLog : public TLog
    {
    public:
        TSqlLog() : TLog( LOGSQL )
        {
        }
    };

    DLLEXPORT void ArirangLogHandler( QtMsgType type, const char* msg );
    DLLEXPORT void setupLogfile( QFile& f );
}

#define tLog Logger::TLog
#define tDebug Logger::TDebug
#define tSqlLog Logger::TSqlLog
DLLEXPORT void tLogNotifyShutdown();

// Macro for messages that severely hurt performance but are helpful
// in some cases for better debugging.
#ifdef ARIRANG_FINEGRAINED_MESSAGES
    #define FINEGRAINED_MSG(a) tDebug( LOGVERBOSE ) << a ;
#else
    #define FINEGRAINED_MSG(a)
#endif

#endif // ARIRANG_LOGGER_H
