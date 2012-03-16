#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

// Visual Studio のデバッグウィンドウの横幅に対応する文字数(バイト数ではない)
#define DPRINTF_MES_LENGTH 256

// デバッグビルドのときだけ，VCの出力窓にメッセージを出す．MFCのTRACEやATLのATLTRACEとおなじ．
#ifdef _DEBUG
inline void dprintf_real( const _TCHAR * fmt, ... )
{
  _TCHAR buf[DPRINTF_MES_LENGTH];
  va_list ap;
  va_start(ap, fmt);
  _vsntprintf(buf, DPRINTF_MES_LENGTH, fmt, ap);
  va_end(ap);
  OutputDebugString(buf);
}
#  define dprintf dprintf_real
#else
#  define dprintf __noop
#endif