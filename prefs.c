#include <stdio.h>
#include "common.h"
#include "prefs.h"

struct prefs prefs;

void prefs_default()
{
  prefs.tosimage = xstrdup("tos.img");
  prefs.stateimage = NULL;
  prefs.diskimage = NULL;
  prefs.diskimage2 = NULL;
  prefs.cartimage = NULL;
  prefs.hdimage = NULL;
}

void prefs_set(char *key, char *value)
{
  if(!strncasecmp("tosimage", key, 8)) {
    if(prefs.tosimage != NULL) free(prefs.tosimage);
    if(value != NULL)
      prefs.tosimage = xstrdup(value);
    else
      prefs.tosimage = NULL;
  } else if(!strncasecmp("stateimage", key, 10)) {
    if(prefs.stateimage != NULL) free(prefs.stateimage);
    if(value != NULL)
      prefs.stateimage = xstrdup(value);
    else
      prefs.stateimage = NULL;
  } else if(!strncasecmp("hdimage", key, 10)) {
    if(prefs.hdimage != NULL) free(prefs.hdimage);
    if(value != NULL)
      prefs.hdimage = xstrdup(value);
    else
      prefs.hdimage = NULL;
  } else if(!strncasecmp("diskimage2", key, 10)) {
    if(prefs.diskimage2 != NULL) free(prefs.diskimage2);
    if(value != NULL)
      prefs.diskimage2 = xstrdup(value);
    else
      prefs.diskimage2 = NULL;
  } else if(!strncasecmp("diskimage", key, 9)) {
    if(prefs.diskimage != NULL) free(prefs.diskimage);
    if(value != NULL)
      prefs.diskimage = xstrdup(value);
    else
      prefs.diskimage = NULL;
  } else if(!strncasecmp("cartimage", key, 9)) {
    if(prefs.cartimage != NULL) free(prefs.cartimage);
    if(value != NULL)
      prefs.cartimage = xstrdup(value);
    else
      prefs.cartimage = NULL;
  }
}

void prefs_init()
{
  prefs_default();
}

