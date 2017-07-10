#include <stdlib.h>
#include <string.h>

#include "lua_bindings.h"

static void addret(lua_State *L, char* str) {
  const char *r = lua_pushfstring(L, "return %s;", str);
  if (luaL_loadbuffer(L, r, strlen(r), "=stdin") == LUA_OK) lua_remove(L, -2);
  else lua_pop(L, 2);
}

char* e_lua_eval(char* str) {
  char* ret = malloc(100*sizeof(char));
  if (!l) {
    l = luaL_newstate();
    luaL_openlibs(l);
  }
  addret(l, str);

  if (lua_pcall(l, 0, 1, 0)) {
    snprintf(ret, 100, "lua can't execute expression: %s.", lua_tostring(l, -1));
    lua_pop(l, 1);
  } else {
    snprintf(ret, 100, "%s", lua_tostring(l, -1));
    lua_pop(l, lua_gettop(l));
  }

  return ret;
}
