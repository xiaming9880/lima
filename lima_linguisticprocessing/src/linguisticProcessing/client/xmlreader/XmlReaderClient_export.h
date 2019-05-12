//************************************************************************
//! @file       XmlReaderClient_export.h
//! @author     gael.de-chalendar@cea.fr
//! @date       October 2012
//! copyright   Copyright (C) 2012 by CEA LIST (DIASI/LVIC)
//! Project     Juliet
//! @brief  Symbol exporting for Windows DLL XmlReaderClient.dll

#ifndef XMLREADERCLIENT_EXPORT_H
#define XMLREADERCLIENT_EXPORT_H

#ifdef WIN32
#include <cstdint>

#pragma warning( disable : 4512 )

// Avoids compilation errors redefining struc sockaddr in ws2def.h
#define _WINSOCKAPI_

#undef min
#undef max
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#ifdef XMLREADERCLIENT_EXPORTING
  #define XMLREADERCLIENT_EXPORT    __declspec(dllexport)
#else
  #define XMLREADERCLIENT_EXPORT    __declspec(dllimport)
#endif


#else // Not WIN32

#define XMLREADERCLIENT_EXPORT

#endif

#endif // XMLREADERCLIENT_EXPORT_H
