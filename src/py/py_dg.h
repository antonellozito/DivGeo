/* DivGeo with the Python interface: the View and XApp implementation in C
   that the Python front end (lib/python/divgeo) loads with ctypes, plus
   helpers for calls that ctypes cannot make (variadic functions, macros).

   Every call of the View function table is kept as a record, in one list
   for all views in the order of the calls. Python takes the records, draws
   them and handles messages and notifications. Screen coordinates are
   computed here exactly as the Motif view computes them. */

#ifndef _py_dg_h
#define _py_dg_h

#define NO_DUMMIES
#include "dg.h"

/* Record operations: the letters of the View functions */
#define PYR_MODE    'M'
#define PYR_LINE    'L'
#define PYR_RECT    'R'
#define PYR_CIRCLE  'C'
#define PYR_TEXT    'T'
#define PYR_CLEAR   'K'
#define PYR_FLUSH   'F'
#define PYR_ADJUST  'A'
#define PYR_MSG     'S'
#define PYR_NOTIFY  'N'
#define PYR_UNCONF  'U'
#define PYR_SLINE   'l'     /* screen line of a rotated rectangle */
#define PYR_SRECT   'r'     /* screen rectangle of a circle drawn as square */

/* Record flags */
#define PYF_HIDDEN  0x0001  /* not drawn: update locks or no window yet */
#define PYF_CLIPPED 0x0002  /* not drawn: outside the view */
#define PYF_HLMODE  0x0004  /* highlight mode of the App at SetMode */

typedef struct _PyRec {
  int op;
  int view;         /* number of the view on the Python side */
  int i;            /* mode; notification type; offset of a text */
  int j;            /* object type of a notification; length of a text */
  int flags;
  void* obj;        /* object of a notification */
  double w[4];      /* arguments as the core passed them */
  double s[4];      /* screen coordinates (pixels, before rounding) */
}* PyRec;

struct _ViewExt {
  int id;           /* number of the view on the Python side */
  int bRealized;    /* the window exists: primitives are drawn */
  int bUseSquares;  /* circles drawn as squares (resource useSquares) */
};

struct _XAppExt {
  int dummy;
};

typedef void (*PyEventProc)(View w);
typedef void (*PyConfigureProc)(XApp xap);

/* py_view.c */
View CreatePyView(XApp xap,int id);
void SetPyViewRealized(View w,int bRealized);
void SetPyViewSquares(View w,int bUseSquares);
void SetPyViewSize(View w,int width,int height);
void ResizePyView(View w,int width,int height);
void SetPyEventProc(PyEventProc proc);
size_t TakePyRecs(struct _PyRec** recs,char** text,size_t* nText);
void ResetPyRecs(void);

/* py_xapp.c */
void SetPyXAppConfig(char* configFileName,int bLoadConfig,
    unsigned long winToEmbedInto);
int GetPyMsgCount(void);
int GetPyMsgId(int k);
char* GetPyMsgName(int k);
void SetPyMsgValue(int k,char* value);
void SetPyConfigureProc(PyConfigureProc proc);
char* GetPyHelpFileName(XApp xap);

/* py_api.c */
size_t GetPyLayout(int k,char** name,size_t* offset);
char GetPyLayoutType(int k);
size_t GetPyRecSize(void);
void PyViewMsgVersion(View w);
int GetPyVersion(void);
char* GetPyVersionStr(int version);
void PyFatalError(char* text);
void PyFatalError2(char* format,char* s1,char* s2);
void PyNotifyView(View w,int type,void* obj);
int GetPyGroupCount(Group g);

#endif
