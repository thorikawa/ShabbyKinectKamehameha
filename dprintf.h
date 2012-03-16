#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

// Visual Studio �̃f�o�b�O�E�B���h�E�̉����ɑΉ����镶����(�o�C�g���ł͂Ȃ�)
#define DPRINTF_MES_LENGTH 256

// �f�o�b�O�r���h�̂Ƃ������CVC�̏o�͑��Ƀ��b�Z�[�W���o���DMFC��TRACE��ATL��ATLTRACE�Ƃ��Ȃ��D
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