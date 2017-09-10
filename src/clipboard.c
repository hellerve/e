#include "clipboard.h"
#ifdef WIN32
#include <windows.h>

void e_clipboard_copy(char* str) {
  char far* buffer;
  int n;
  HGLOBAL clip;

  n = strlen(str);
  OpenClipboard(NULL);
  EmptyClipboard();
  clip = GlobalAlloc(GMEM_DDESHARE, n+1);

  buffer = (char far*) GlobalLock(clip);
  if (!buffer) return;

  strcpy(buffer, str);

  GlobalUnlock(clip);
  SetClipboardData(CF_TEXT, clip);
  CloseClipboard();
}


char* e_clipboard_paste() {
  char* buffer = NULL;
  char* data = NULL;
  char empty[80] = "";

  int bytes = 0;
  if (OpenClipboard(NULL)) {
    HANDLE hData = GetClipboardData(CF_TEXT);
    char* buffer = (char*) GlobalLock(hData);
    GlobalUnlock(hData);
    CloseClipboard();

    if (!buffer) {
      bytes = strlen(empty);
      data = (char*) malloc(bytes+1);
      strcpy(data, empty);
    } else {
      bytes = strlen(buffer);
      data = (char*) malloc(bytes+1);
      strcpy(data, buffer);
     }
  }
  return data;
}

#elif defined(__APPLE__)

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

#elif __unix__ 
#include <gtk/gtk.h>
GtkClipboard *clipboard;
void e_clipboard_copy(char* str){
	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	gchar *clip_str = str;
	gtk_clipboard_set_text(clipboard, clip_str, -1);
}

char* e_clipboard_paste(){
	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	return (char*)gtk_clipboard_wait_for_text(clipboard);
}

#else

void e_clipboard_copy(char* str) {}

char* e_clipboard_paste() {
  return NULL;
}
#endif
