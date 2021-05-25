/**
 * Author: James Hwang
 * Copyright (c)  2015  aurender.com
 *
 */

#ifndef DLLMACRO_H
#define DLLMACRO_H

#include <QtCore/qglobal.h>

#ifndef DLLEXPORT
# if defined (DLLEXPORT_PRO)
#  define DLLEXPORT Q_DECL_EXPORT
# else
#  define DLLEXPORT Q_DECL_IMPORT
# endif
#endif

#endif
