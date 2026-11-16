/* View of the Python interface: every call of the View function table is
   kept as a record for the Python front end. Screen coordinates follow the
   Motif view (x_view.c): rotation, clipping and rectangle corners. */

#include "py_dg.h"

static PyEventProc eventProc=NULL;

/* The records of all views */
static struct _PyRec* recs=NULL;
static size_t nRecs=0,capRecs=0;
static char* recText=NULL;
static size_t nRecText=0,capRecText=0;

static void UnconfigurePyView(View w);
static void AdjustPyViewAspectRatio(View w);
static void SetPyViewMode(View w,int mode);
static void DrawPyViewLine(View w,double x1,double y1,double x2,double y2);
static void DrawPyViewRect(View w,double x1,double y1,double x2,double y2);
static void DrawPyViewCircle(View w,double x,double y,double r);
static void DrawPyViewText(View w,double x,double y,char* text);
static void SetPyViewMsg(View w,char* msg);
static void ClearPyView(View w);
static void FlushPyView(View w);
static void NotifyPyView(View w,int type,void* obj);
static char* GetPyViewStr(View w,int id);

/* Records ************************************************************/

static PyRec AddRec(View w,int op) {
  PyRec r;

  if (nRecs==capRecs) {
    capRecs=capRecs ? 2*capRecs : 4096;
    recs=recs==NULL ? Malloc(capRecs*sizeof(struct _PyRec)) :
      Realloc(recs,capRecs*sizeof(struct _PyRec));
  }
  r=&recs[nRecs++];
  memset(r,0,sizeof(*r));
  r->op=op;
  r->view=w->x->id;
  return r;
}

static void AddRecText(PyRec r,char* s) {
  size_t n=s==NULL ? 0 : strlen(s);

  if (nRecText+n+1>capRecText) {
    capRecText=2*(nRecText+n+1)+4096;
    recText=recText==NULL ? Malloc(capRecText) : Realloc(recText,capRecText);
  }
  r->i=(int)nRecText;
  r->j=s==NULL ? -1 : (int)n;
  if (n) memcpy(recText+nRecText,s,n);
  recText[nRecText+n]=0;
  nRecText+=n+1;
}

/* Not drawn while the App locks updates or before the window exists */
static int Hidden(View w) {
  return (w->app!=NULL && w->app->updateLocks) || !w->x->bRealized;
}

static void Event(View w) {
  if (eventProc!=NULL) eventProc(w);
}

/* Creation ***********************************************************/

View CreatePyView(XApp xap,int id) {
  View w;

  w=CreateAbstractView();

  w->yScaleSign=-1;

  w->UnconfigureProc=UnconfigurePyView;
  w->AdjustAspectRatioProc=AdjustPyViewAspectRatio;
  w->SetModeProc=SetPyViewMode;
  w->DrawLineProc=DrawPyViewLine;
  w->DrawRectProc=DrawPyViewRect;
  w->DrawCircleProc=DrawPyViewCircle;
  w->DrawTextProc=DrawPyViewText;
  w->SetMsgProc=SetPyViewMsg;
  w->ClearProc=ClearPyView;
  w->FlushProc=FlushPyView;
  w->NotifyProc=NotifyPyView;
  w->GetStrProc=GetPyViewStr;

  w->xapp=xap;
  GroupAdd(w->xapp->views,w);

  w->x=Malloc(sizeof(*w->x));
  w->x->id=id;
  w->x->bRealized=0;
  w->x->bUseSquares=0;

  return w;
}

static void UnconfigurePyView(View w) {
  AddRec(w,PYR_UNCONF);
  Event(w);

  GroupDel(w->xapp->views,w);
  w->x=Free(w->x);
}

void SetPyViewRealized(View w,int bRealized) {
  w->x->bRealized=bRealized;
}

void SetPyViewSquares(View w,int bUseSquares) {
  w->x->bUseSquares=bUseSquares;
}

void SetPyEventProc(PyEventProc proc) {
  eventProc=proc;
}

/* Size of the drawing area, as the Motif view reads it */
void SetPyViewSize(View w,int width,int height) {
  w->width=width ? width : 1;
  w->height=height ? height : 1;
}

/* The drawing area was resized: as the resize callback of the Motif view */
void ResizePyView(View w,int width,int height) {
  SetPyViewSize(w,width,height);
  if (w->app!=NULL) Cancel(w->app);
  SetViewFactor(w,w->centerX,w->centerY,w->zoomX,w->zoomY);
  if (w->app!=NULL) UndoMark(w->app);
}

/* The records so far; valid until the next call into the core */
size_t TakePyRecs(struct _PyRec** pRecs,char** pText,size_t* pnText) {
  *pRecs=recs;
  *pText=recText;
  *pnText=nRecText;
  return nRecs;
}

void ResetPyRecs(void) {
  nRecs=0;
  nRecText=0;
}

/* Function table *****************************************************/

static void AdjustPyViewAspectRatio(View w) {
  AddRec(w,PYR_ADJUST);
  w->zoomX=w->zoomY=min(w->zoomX,w->zoomY);
}

static int ValidMode(int mode) {
  switch(mode) {
    case VMX_NODE: case VMX_ELEM: case VMX_EQUIL: case VMX_SURFACE:
    case VMX_TEMPLATE: case VMX_ELEMNORMAL: case VMX_XPOINT: case VMX_GPOINT:
    case VMX_SEPARATOR: case VMX_SOURCE: case VMX_CHORD: case VMX_XPOINTTEST:
    case VMX_XPOINTSEG: case VMX_MESHELEMENT: case VMX_MESHCELLCENTER:
    case VMX12_MESHELEMENT: case VMX2_MESHELEMENT: case VMX_HIGHLIGHTRECT:
    case VM0_NODE: case VM0_ELEM: case VM0_EQUIL: case VM0_HIGHLIGHTRECT:
    case VM0_SURFACE: case VM0_TEMPLATE: case VM0_ELEMNORMAL: case VM0_XPOINT:
    case VM0_ELEMMARK: case VM0_ELEMNUMBER: case VM0_VIEWLABEL:
    case VM0_SONNETDATA: case VM0_GPOINT: case VM0_SEPARATOR: case VM0_SOURCE:
    case VM0_CHORD: case VM0_XPOINTTEST: case VM0_XPOINTSEG:
    case VM0_MESHELEMENT: case VM0_MESHCELLCENTER:
    case VM1_NODE: case VM1_ELEM: case VM1_EQUIL: case VM2_EQUIL:
    case VM3_EQUIL:
    case VM1_HIGHLIGHTRECT: case VM1_SURFACE: case VM1_TEMPLATE:
    case VM1_ELEMNORMAL: case VM1_XPOINT: case VM1_ELEMMARK: case VM1_AXES:
    case VM1_GRID: case VM1_GRIDTEXT: case VM1_ELEMNUMBER: case VM1_VIEWLABEL:
    case VM1_SONNETDATA: case VM1_GPOINT: case VM1_SEPARATOR: case VM1_SOURCE:
    case VM1_CHORD: case VM1_XPOINTTEST: case VM2_XPOINTTEST:
    case VM1_XPOINTSEG:
    case VM1_MESHELEMENT: case VM2_MESHELEMENT: case VM1_MESHCELLCENTER:
    case VM2_MESHCELLCENTER:
      return 1;
  }
  return 0;
}

/* The XOR modes read the highlight mode of the App, as the Motif view */
static int HighlightModeUsed(int mode) {
  switch(mode) {
    case VMX_NODE: case VMX_ELEM: case VMX_EQUIL: case VMX_SURFACE:
    case VMX_TEMPLATE: case VMX_ELEMNORMAL: case VMX_XPOINT: case VMX_GPOINT:
    case VMX_SEPARATOR: case VMX_SOURCE: case VMX_CHORD: case VMX_XPOINTTEST:
    case VMX_XPOINTSEG: case VMX_MESHELEMENT: case VMX_MESHCELLCENTER:
    case VMX12_MESHELEMENT: case VMX_HIGHLIGHTRECT:
      return 1;
  }
  return 0;
}

static void SetPyViewMode(View w,int mode) {
  PyRec r=AddRec(w,PYR_MODE);

  r->i=mode;
  if (Hidden(w)) {
    r->flags|=PYF_HIDDEN;
    return;
  }
  if (!ValidMode(mode))
    FatalError("SetXmViewMode()-mode%d: fatal error 1",mode);
  if (HighlightModeUsed(mode) && w->app->highlightMode) r->flags|=PYF_HLMODE;
}

static void DrawPyViewLine(View w,double x1,double y1,double x2,double y2) {
  PyRec r=AddRec(w,PYR_LINE);

  r->w[0]=x1; r->w[1]=y1; r->w[2]=x2; r->w[3]=y2;
  if (Hidden(w)) {
    r->flags|=PYF_HIDDEN;
    return;
  }

  ScreenRotate(w,1,&x1,&y1);
  ScreenRotate(w,1,&x2,&y2);

  if (x1<w->minX && x2>w->minX)
    y1+=(y2-y1)*(w->minX-x1)/(x2-x1),x1=w->minX; else
  if (x2<w->minX && x1>w->minX)
    y2+=(y1-y2)*(w->minX-x2)/(x1-x2),x2=w->minX;
  if (x1>w->maxX && x2<w->maxX)
    y1+=(y2-y1)*(w->maxX-x1)/(x2-x1),x1=w->maxX; else
  if (x2>w->maxX && x1<w->maxX)
    y2+=(y1-y2)*(w->maxX-x2)/(x1-x2),x2=w->maxX;

  if (y1<w->minY && y2>w->minY)
    x1+=(x2-x1)*(w->minY-y1)/(y2-y1),y1=w->minY; else
  if (y2<w->minY && y1>w->minY)
    x2+=(x1-x2)*(w->minY-y2)/(y1-y2),y2=w->minY;
  if (y1>w->maxY && y2<w->maxY)
    x1+=(x2-x1)*(w->maxY-y1)/(y2-y1),y1=w->maxY; else
  if (y2>w->maxY && y1<w->maxY)
    x2+=(x1-x2)*(w->maxY-y2)/(y1-y2),y2=w->maxY;

  if ((x1<w->minX && x2<w->minX) || (y1<w->minY && y2<w->minY) ||
      (x1>w->maxX && x2>w->maxX) || (y1>w->maxY && y2>w->maxY)) {
    r->flags|=PYF_CLIPPED;
    return;
  }

  r->s[0]=FScreenX(w,x1); r->s[1]=FScreenY(w,y1);
  r->s[2]=FScreenX(w,x2); r->s[3]=FScreenY(w,y2);
}

/* The screen part of a rectangle: corners rotated about the center of the
   view when the view is rotated, an upright rectangle otherwise */
static void ScreenRect(View w,double x1,double y1,double x2,double y2) {
  double x3,y3,x4,y4,ox1,oy1,ox2,oy2;
  PyRec r;

  if (w->xyAngle!=0) {
    ox1=x1;oy1=y1;ox2=x2;oy2=y2;
    x1=ox1; y1=oy1;
    x2=ox2; y2=oy1;
    x3=ox2; y3=oy2;
    x4=ox1; y4=oy2;

    ScreenRotate(w,1,&x1,&y1);
    ScreenRotate(w,1,&x2,&y2);
    ScreenRotate(w,1,&x3,&y3);
    ScreenRotate(w,1,&x4,&y4);

    r=AddRec(w,PYR_SLINE);
    r->s[0]=FScreenX(w,x1); r->s[1]=FScreenY(w,y1);
    r->s[2]=FScreenX(w,x2); r->s[3]=FScreenY(w,y2);
    r=AddRec(w,PYR_SLINE);
    r->s[0]=FScreenX(w,x2); r->s[1]=FScreenY(w,y2);
    r->s[2]=FScreenX(w,x3); r->s[3]=FScreenY(w,y3);
    r=AddRec(w,PYR_SLINE);
    r->s[0]=FScreenX(w,x3); r->s[1]=FScreenY(w,y3);
    r->s[2]=FScreenX(w,x4); r->s[3]=FScreenY(w,y4);
    r=AddRec(w,PYR_SLINE);
    r->s[0]=FScreenX(w,x4); r->s[1]=FScreenY(w,y4);
    r->s[2]=FScreenX(w,x1); r->s[3]=FScreenY(w,y1);
  } else {
    r=AddRec(w,PYR_SRECT);
    r->s[0]=FScreenX(w,x1); r->s[1]=FScreenY(w,y1);
    r->s[2]=FScreenX(w,x2); r->s[3]=FScreenY(w,y2);
  }
}

static void DrawPyViewRect(View w,double x1,double y1,double x2,double y2) {
  PyRec r=AddRec(w,PYR_RECT);

  r->w[0]=x1; r->w[1]=y1; r->w[2]=x2; r->w[3]=y2;
  if (Hidden(w)) {
    r->flags|=PYF_HIDDEN;
    return;
  }
  ScreenRect(w,x1,y1,x2,y2);
}

static void DrawPyViewCircle(View w,double x,double y,double r) {
  PyRec p=AddRec(w,PYR_CIRCLE);
  int wh,h;

  p->w[0]=x; p->w[1]=y; p->w[2]=r;
  ScreenRotate(w,1,&x,&y);

  if (Hidden(w)) {
    p->flags|=PYF_HIDDEN;
    return;
  }
  if (x+r<w->minX || x-r>w->maxX || y+r<w->minY || y-r>w->maxY) {
    p->flags|=PYF_CLIPPED;
    return;
  }

  if (w->x->bUseSquares) {
    ScreenRotate(w,-1,&x,&y);
    ScreenRect(w,x-r,y-r,x+r,y+r);
  } else {
    wh=r*w->zoomX;
    h=r*w->zoomY;
    p->s[0]=FScreenX(w,x);
    p->s[1]=FScreenY(w,y);
    p->s[2]=wh;
    p->s[3]=h;
  }
}

static void DrawPyViewText(View w,double x,double y,char* text) {
  PyRec r=AddRec(w,PYR_TEXT);

  r->w[0]=x; r->w[1]=y;
  AddRecText(r,text);
  if (Hidden(w)) {
    r->flags|=PYF_HIDDEN;
    return;
  }
  ScreenRotate(w,1,&x,&y);
  r->s[0]=FScreenX(w,x);
  r->s[1]=FScreenY(w,y);
}

static void SetPyViewMsg(View w,char* msg) {
  PyRec r=AddRec(w,PYR_MSG);

  AddRecText(r,msg);
  Event(w);
}

static void ClearPyView(View w) {
  PyRec r=AddRec(w,PYR_CLEAR);

  if (Hidden(w)) r->flags|=PYF_HIDDEN;
}

static void FlushPyView(View w) {
  PyRec r=AddRec(w,PYR_FLUSH);

  if (w->app!=NULL && w->app->updateLocks) r->flags|=PYF_HIDDEN;
  Event(w);
}

static void NotifyPyView(View w,int type,void* obj) {
  PyRec r=AddRec(w,PYR_NOTIFY);

  r->i=type;
  r->j=obj==NULL ? -1 : GetObjType(obj);
  r->obj=obj;
  Event(w);
}

static char* GetPyViewStr(View w,int id) {
  return XAppStr(w->xapp,id);
}
