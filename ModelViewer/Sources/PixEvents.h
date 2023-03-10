#pragma once

#ifdef _DEBUG
#include "Include/WinPixEventRuntime/pix3.h"
#endif // DEBUG

class ScopedPixEvent;

#ifdef _DEBUG
#define PIX_SCOPED_EVENT(call, cmdList, color, format, ...) PIXScopedEvent(cmdList, color, format, ##__VA_ARGS__); call;
#else
#define PIX_SCOPED_EVENT(call, cmdList, color, format, ...) call;
#endif // DEBUG
