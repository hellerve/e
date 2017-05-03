#ifdef WIN32
#include <windows.h>

void e_clipboard_copy(char* str) {
  char far *buffer;
  int bytes;
  HGLOBAL clipbuffer;

  bytes = strlen(toclipdata);
  OpenClipboard(NULL);
  EmptyClipboard();
  clipbuffer = GlobalAlloc(GMEM_DDESHARE, bytes+1);

  buffer = (char far*) GlobalLock(clipbuffer);
  if (!buffer) return;

  strcpy(buffer, toclipdata);

  GlobalUnlock(clipbuffer);
  SetClipboardData(CF_TEXT,clipbuffer);
  CloseClipboard();
}


char* e_clipboard_paste(int& bytes) {
  int k;
  char* buffer = NULL;
  char* data = NULL;
  char empty[80] = "";

  bytes = 0;
  if (OpenClipboard(NULL)){
     HANDLE hData = GetClipboardData(CF_TEXT);
     char * buffer = (char*) GlobalLock(hData);
     GlobalUnlock(hData);
     CloseClipboard();
     if (!buffer) {
        bytes = strlen(empty);
        data = (char*) malloc(bytes+1);
        strcpy(data,empty);
        bytes = bytes * -1;
     } else {
        bytes = strlen(buffer);
        data = (char*) malloc(bytes+1);
        strcpy(data,buffer);
     }
  } else {
     k = GetLastError();
     bytes = k < 0 ? k : k * -1;
  }
  return data;
}
#endif
