#include <stdio.h>
#include "common.h"
#include "prefs.h"

struct prefs prefs;

void prefs_default()
{
  prefs.tosimage = strdup("tos.img");
  prefs.stateimage = NULL;
  prefs.diskimage = NULL;
}

void prefs_set(char *key, char *value)
{
  if(!strncasecmp("tosimage", key, 8)) {
    if(prefs.tosimage != NULL) free(prefs.tosimage);
    if(value != NULL)
      prefs.tosimage = strdup(value);
    else
      prefs.tosimage = NULL;
  } else if(!strncasecmp("stateimage", key, 10)) {
    if(prefs.stateimage != NULL) free(prefs.stateimage);
    if(value != NULL)
      prefs.stateimage = strdup(value);
    else
      prefs.stateimage = NULL;
  } else if(!strncasecmp("diskimage", key, 9)) {
    if(prefs.diskimage != NULL) free(prefs.diskimage);
    if(value != NULL)
      prefs.diskimage = strdup(value);
    else
      prefs.diskimage = NULL;
  }
}

void prefs_init()
{
  prefs_default();
}

