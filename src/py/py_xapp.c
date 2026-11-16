/* XApp of the Python interface: configuration given by Python, and the
   string source. The message table is the one of the Motif front end
   (x_xapp.c, read without X through RESOURCE_IDS_ONLY); the values are
   resolved by Python from the same resources. */

#include "py_dg.h"

#define RESOURCE_IDS_ONLY
#include "x_xapp.c"
#undef RESOURCE_IDS_ONLY

static char* pyConfigFileName=NULL;
static int pyLoadConfig=1;
static unsigned long pyWinToEmbedInto=None;
static char** msgValues=NULL;
static StringSource strs=NULL;
static PyConfigureProc configureProc=NULL;

/* Options and resources read by Python before CreateXApp; an empty
   configFileName: the default name, from argv[0] */
void SetPyXAppConfig(char* configFileName,int bLoadConfig,
    unsigned long winToEmbedInto) {
  if (pyConfigFileName!=NULL) pyConfigFileName=Free(pyConfigFileName);
  pyConfigFileName=MallocString(configFileName==NULL ? "" : configFileName);
  pyLoadConfig=bLoadConfig;
  pyWinToEmbedInto=winToEmbedInto;
}

/* Called at the end of ConfigureXApp, where the Motif front end shows the
   usage, checks the resource version and loads the help file */
void SetPyConfigureProc(PyConfigureProc proc) {
  configureProc=proc;
}

/* The help file name of the Motif front end: argv[0] with the extension
   of help files */
char* GetPyHelpFileName(XApp xap) {
  static char helpFileName[DG_FNAME_LEN];

  strcpy(helpFileName,xap->argv[0]);
  strcpy(GetFileExt(helpFileName),XAppStr(xap,FSTR_HELPEXT));
  return helpFileName;
}

int GetPyMsgCount(void) {
  int k;

  for (k=0;viewMsgs[k].p!=NULL;k++);
  return k;
}

int GetPyMsgId(int k) {
  return viewMsgs[k].id;
}

char* GetPyMsgName(int k) {
  return viewMsgs[k].p;
}

/* Value of entry k from the resources (NULL: not found) */
void SetPyMsgValue(int k,char* value) {
  if (msgValues==NULL) {
    msgValues=Malloc(GetPyMsgCount()*sizeof(char*));
    memset(msgValues,0,GetPyMsgCount()*sizeof(char*));
  }
  if (msgValues[k]!=NULL) msgValues[k]=Free(msgValues[k]);
  if (value!=NULL) msgValues[k]=MallocString(value);
}

/* As the Motif string source: literal names start with \r, a name without
   resource gives the name itself */
static char* PyXAppGetStringProc(StringSource strS,int id) {
  char* resourceName;
  int k;

  resourceName=TranslateId(viewMsgs,id);
  if (resourceName[0]=='\r') return resourceName+1;

  for (k=0;viewMsgs[k].p!=NULL;k++) if (viewMsgs[k].id==id) break;
  if (msgValues==NULL || msgValues[k]==NULL) return resourceName;
  return msgValues[k];
}

void ConfigureXApp(XApp xap) {
  xap->x=Malloc(sizeof(*xap->x));

  strs=Malloc(sizeof(*strs));
  strs->type=T_STRINGSOURCE;
  strs->GetStringProc=PyXAppGetStringProc;
  strs->userData=xap;
  xap->stringSource=strs;

  xap->bLoadConfig=pyLoadConfig;
  if (pyConfigFileName!=NULL && *pyConfigFileName)
    xap->configFileName=MallocString(pyConfigFileName);
  else {
    xap->configFileName=Malloc(DG_FNAME_LEN);
    strcpy(xap->configFileName,xap->argv[0]);
    strcpy(GetFileExt(xap->configFileName),XAppStr(xap,FSTR_CFGEXT));
  }
  xap->winToEmbedInto=pyWinToEmbedInto;

  if (configureProc!=NULL) configureProc(xap);
}

void UnconfigureXApp(XApp xap) {
  strs=Free(strs);
  xap->configFileName=Free(xap->configFileName);
  xap->winToEmbedInto=None;
  xap->stringSource=NULL;
  xap->x=Free(xap->x);
}
