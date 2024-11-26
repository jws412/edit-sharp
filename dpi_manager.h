#include <windows.h>
#include "global_data.h"

enum EsError initDpiManager(void);
enum EsError updateAccordingToDpi(HWND hWindow, const RECT *pWindowRect);