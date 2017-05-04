#include "clipboard.h"
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


char* e_clipboard_paste() {
  int k;
  char* buffer = NULL;
  char* data = NULL;
  char empty[80] = "";

  int bytes = 0;
  if (OpenClipboard(NULL)){
     HANDLE hData = GetClipboardData(CF_TEXT);
     char * buffer = (char*) GlobalLock(hData);
     GlobalUnlock(hData);
     CloseClipboard();
     if (!buffer) {
        bytes = strlen(empty);
        data = (char*) malloc(bytes+1);
        strcpy(data,empty);
     } else {
        bytes = strlen(buffer);
        data = (char*) malloc(bytes+1);
        strcpy(data, buffer);
     }
  } else {
     k = GetLastError();
  }
  return data;
}
#endif

#ifdef __APPLE__

void e_clipboard_copy(char* str) {
  const char proto_cmd[] = "echo '%s' | pbcopy";

  char cmd[strlen(str) + strlen(proto_cmd) - 1];
  sprintf(cmd, proto_cmd, str);

  system(cmd);
}

char* e_clipboard_paste() {
  FILE* pb = popen("pbpaste", "r");

  if (!pb) return NULL;

  char* buffer = malloc(1024);
  fgets(buffer, 1024, pb);

  pclose(pb);

  return buffer;
}
#endif
