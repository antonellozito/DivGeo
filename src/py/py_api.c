/* Helpers of the Python interface: the layout of the structures Python
   reads and writes (offsets and sizes from the compiler, so that Python
   never assumes a layout), and wrappers for calls that ctypes cannot make
   (variadic functions, macros). */

#include "py_dg.h"

typedef struct _PyField {
  char* name;
  size_t offset,size;
  char type;            /* i int, u unsigned long, d double, p pointer */
} PyField;

#define F(st,field,type) \
  {#st "." #field,offsetof(struct _##st,field), \
    sizeof(((struct _##st*)0)->field),type}

static PyField layout[]={
  F(View,type,'i'), F(View,x,'p'), F(View,xapp,'p'), F(View,app,'p'),
  F(View,width,'i'), F(View,height,'i'),
  F(View,centerX,'d'), F(View,centerY,'d'), F(View,zoomX,'d'),
  F(View,zoomY,'d'), F(View,minX,'d'), F(View,minY,'d'), F(View,maxX,'d'),
  F(View,maxY,'d'), F(View,xyAngle,'d'),
  F(View,xScaleSign,'i'), F(View,yScaleSign,'i'), F(View,showFlags,'u'),
  F(View,toolData,'p'), F(View,activeTool,'p'), F(View,updateFlag,'i'),
  F(View,labels,'p'), F(View,shapes,'p'), F(View,stretchPower,'d'),
  F(View,bEditTopology,'i'),
  F(View,nodeR,'i'), F(View,equilLen,'i'), F(View,minZoomX,'i'),
  F(View,minZoomY,'i'), F(View,normalLen,'i'), F(View,gridPointLen,'i'),
  F(View,srcR,'i'), F(View,arrowWidth,'i'), F(View,arrowLength,'i'),
  F(View,gridCellMax,'i'), F(View,labelOffsetX,'i'),
  F(View,labelOffsetY,'i'), F(View,shpIncr,'i'),
  F(View,meshCellCenterLen,'i'), F(View,meshPointRadius,'i'),
  F(View,gridPointSegEndLength,'i'),

  F(App,type,'i'), F(App,undoStack,'p'), F(App,redoStack,'p'),
  F(App,views,'p'), F(App,highlight,'p'), F(App,mark,'p'),
  F(App,activeAppView,'p'), F(App,errorCode,'i'),
  F(App,cancelToolFlag,'i'), F(App,alt,'i'), F(App,updateLocks,'i'),
  F(App,highlightMode,'i'), F(App,highlightLocks,'i'),
  F(App,minX,'d'), F(App,minY,'d'), F(App,maxX,'d'), F(App,maxY,'d'),
  F(App,xyAngle,'d'), F(App,showFlags,'u'), F(App,outputFlags,'u'),
  F(App,nodes,'p'), F(App,elems,'p'), F(App,equil,'p'),
  F(App,sonnetData,'p'), F(App,mesh,'p'), F(App,fName,'p'),
  F(App,outputMode,'i'),

  F(XApp,type,'i'), F(XApp,x,'p'), F(XApp,views,'p'),
  F(XApp,configFileName,'p'), F(XApp,winToEmbedInto,'u'),
  F(XApp,config,'p'), F(XApp,stringSource,'p'), F(XApp,pargc,'p'),
  F(XApp,argv,'p'), F(XApp,bLoadConfig,'i'),

  F(PyRec,op,'i'), F(PyRec,view,'i'), F(PyRec,i,'i'), F(PyRec,j,'i'),
  F(PyRec,flags,'i'), F(PyRec,obj,'p'), F(PyRec,w,'d'), F(PyRec,s,'d'),
};

/* Field k of the table; returns its size, 0 after the last one */
size_t GetPyLayout(int k,char** name,size_t* offset) {
  if (k<0 || k>=(int)(sizeof(layout)/sizeof(layout[0]))) return 0;
  *name=layout[k].name;
  *offset=layout[k].offset;
  return layout[k].size;
}

char GetPyLayoutType(int k) {
  return layout[k].type;
}

size_t GetPyRecSize(void) {
  return sizeof(struct _PyRec);
}

/* The welcome message of a new view (ViewMsgEx is variadic) */
void PyViewMsgVersion(View w) {
  ViewMsgEx(w,MSG_ABOUT,"$(VERSION)%s",GetVersionStr(DG_VERSION));
}

/* The version of DivGeo, which the resources must carry */
int GetPyVersion(void) {
  return DG_VERSION;
}

char* GetPyVersionStr(int version) {
  return GetVersionStr(version);
}

/* FatalError with a text without % (a leading \r: exit status 1) */
void PyFatalError(char* text) {
  FatalError(text);
}

void PyFatalError2(char* format,char* s1,char* s2) {
  FatalError(format,s1,s2);
}

/* GroupCount is a macro */
int GetPyGroupCount(Group g) {
  return (int)GroupCount(g);
}

/* NotifyView is a macro */
void PyNotifyView(View w,int type,void* obj) {
  NotifyView(w,type,obj);
}
