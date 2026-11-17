/* Variables and setup dialogs of the Python interface: the core part of
   the callbacks of the Motif front end (x_vars.c, x_setup.c), in the same
   order of core calls. Python reads the dialog fields and shows the
   boxes. */

#include "py_dg.h"

#ifndef True
#define True 1
#define False 0
#endif

/* Pointers of the members of a group, at most n; returns the number of
   members */
int PyGroupList(Group g,void** items,int n) {
  void* p;
  Index ix;
  int i=0;

  for (p=Group1st(g,&ix);p!=NULL;p=Next(&ix)) {
    if (i<n) items[i]=p;
    i++;
  }
  return i;
}

/* Variables dialog (x_vars.c) ****************************************/

/* The kind of a value of GetVarEx: 1 no selection, 2 different values,
   0 a value */
int PyVarValueKind(void* p) {
  return p==VV_NOSEL ? 1 : p==VV_DIFF ? 2 : 0;
}

/* GetVarValueDescr */
char* PyVarValueDescr(View w,void* value,VarDef vd) {
  static char s[DG_FNAME_LEN];
  Group g;
  char* ps;

  if (vd->varType & VTM_HASGROUP) {
    g=value;
    if (g==NULL || !GroupCount(g))
      return GetStr(w,STR_NOELEMS);
    if (vd->varType & VTF_HASELEMS)
      sprintf(s,"%u %s",(unsigned)GroupCount(g),GetStr(w,STR_ELEM));
    else if (vd->varType & VTF_HASCHORDS)
      sprintf(s,"%u %s",(unsigned)GroupCount(g),GetStr(w,STR_CHORD));
    else if (vd->varType & VTF_HAS_MESH_OBJECTS)
      sprintf(s,"%u %s",(unsigned)GroupCount(g),
      GetStr(w,vd->varType==VT_MESH_CELLS? STR_MESH_CELL : STR_MESH_ELEMENT));
    else assert(0);
    if (GroupCount(g)>1) strcat(s,"s");
    return s;
  } else {
    ps=value;
    if (ps==VV_NOSEL) ps=GetStr(w,STR_NOSEL);
    else if (ps==VV_DIFF) ps=GetStr(w,STR_DIFF);

    return ps;
  }
}

/* AcceptVarsDlgField with the text of the field (NULL for a group) */
int PyAcceptVarsField(View w,void* obj,VarDef vd,char* text) {
  void* val,* errObj=NULL;
  int r;

  if (GetVarExLocks(w->app,obj,vd)) return ERR_USEDBY;

  if (vd->varType & VTM_HASGROUP) {
    val=CopyGroup(w->app->mark,NULL);
  } else
    val=text;

  r=SetVarEx(w->app,obj,vd,val);
  if (r) {
    CheckValue(w->app,val,vd->varType,&errObj);
    if (errObj!=NULL) {
      LabelObject(w,errObj,GetStr(w,STR_ERRLABEL),True);
      UndoMark(w->app);
    }
  }

  if (vd->varType & VTM_HASGROUP)
    val=FreeGroup(val);

  return r;
}

/* CbVarsEditCompare_p after the scope check: -2 a type without a
   comparison, -1 not a number, else the number of objects matching (then
   marked) */
int PyVarsCompare(View w,void* obj,VarDef vd,int mode,char* s) {
  int i1=0,i2,comp;
  double f1=0,f2;
  char* s1;
  Group g;
  Index ix;
  void* p;
  int n;

  switch (vd->varType) {
    case VT_INT:
    case VT_FLOAT:
    case VT_TEXT:
    case VT_FILENAME:
      break;
    default:
      return -2;
  }

  switch (vd->varType) {
    case VT_INT:
      if (sscanf(s,"%d",&i1)!=1) return -1;
      break;
    case VT_FLOAT:
      if (sscanf(s,SCANFLT,&f1)!=1) return -1;
      break;
  }

  g=GetVarOriginGroup(w->app,obj,vd,0);

  for (p=Group1st(g,&ix);p!=NULL;p=Next(&ix)) {
    s1=GetVar(p,vd,obj);

    switch(vd->varType) {
      case VT_INT:
        if (sscanf(s1,"%d",&i2)!=1) comp=-2;
        else if (i1==i2) comp=0;
        else comp=(i1<i2)? 1 : -1;
        break;
      case VT_FLOAT:
        if (sscanf(s1,SCANFLT,&f2)!=1) comp=-2;
        else if (f1==f2) comp=0;
        else comp=(f1<f2)? 1 : -1;
        break;
      case VT_TEXT:
      case VT_FILENAME:
        comp=strcmp(s1,s);
        break;
      default:
        assert(0);
    }

    switch (mode) {
      case 1:
        if (comp!=0) GroupDel(g,p);
        break;
      case 2:
        if (comp==0) GroupDel(g,p);
        break;
      case 3:
        if (comp!=1) GroupDel(g,p);
        break;
      case 4:
        if (comp!=1 && comp!=0) GroupDel(g,p);
        break;
      case 5:
        if (comp!=-1) GroupDel(g,p);
        break;
      case 6:
        if (comp!=-1 && comp!=0) GroupDel(g,p);
        break;
      default:
        assert(0);
    }
  }

  MarkGroup(w->app,g,10);
  UndoMark(w->app);

  n=GroupCount(g);
  g=FreeGroup(g);
  return n;
}

/* CbVarsEditDisplay_p after the scope check */
void PyVarsDisplay(View w,void* obj,VarDef vd) {
  char* s;
  Group g;
  Index ix;
  void* p;

  g=GetVarOriginGroup(w->app,obj,vd,0);

  RemoveAllViewLabels(w);

  for (p=Group1st(g,&ix);p!=NULL;p=Next(&ix)) {
    s=GetVar(p,vd,obj);
    LabelObject(w,p,PyVarValueDescr(w,s,vd),False);
  }

  g=FreeGroup(g);

  UndoMark(w->app);
}

/* ResetInvalidVarsDlg: the descriptions of the invalid variables; returns
   CheckAllVars (the OK button sensitive) */
int PyInvalidVarsItems(View w,PyItemProc proc) {
  VarDef vd;
  VarSet vs;
  int i,r;
  Group g;
  Index ix;

  r=CheckAllVars(w->app,&g);

  for (i=0,vd=Group1st(g,&ix);vd!=NULL;i++,vd=Next(&ix)) {
    vs=Next(&ix);
    assert(vs!=NULL);
    proc(vd->descr,++i);
  }

  FreeGroup(g);
  return r;
}

/* CbInvalidVarsWhere up to the Variables dialog: the variable set to edit,
   NULL if none; *pReset when the list must be made again */
VarSet PyInvalidVarsWhere(View w,int pos,int* pReset) {
  Group g;
  VarDef vd=NULL;
  VarSet vs=NULL;
  int err=0;

  *pReset=False;
  CheckAllVars(w->app,&g);
  if (pos>=1 && pos<=(int)(GroupCount(g)/2)) {
    vd=GroupAt(g,(pos-1)*2);
    vs=GroupAt(g,(pos-1)*2+1);
  }
  FreeGroup(g);
  if (vd==NULL) return NULL;

  if (!CheckVar(w->app,&g,vd,vs,&err)) {
    FreeGroup(g);
    *pReset=True;
    return NULL;
  }

  if (vd->varType & VTM_HASGROUP) {
    MarkGroup(w->app,GetVar(vs,vd,NULL),10);
    if (!IsEmptyGroup(g)) LabelObject(w,Group1st(g,NULL),
        GetStr(w,STR_ERRLABEL),True);
  } else MarkGroup(w->app,g,10);

  FreeGroup(g);

  if (err!=0) ViewMsgEx(w,err,NULL);

  return vs;
}

/* Save configuration (x_setup.c) *************************************/

/* The test of CbSaveConfigDlgOk: the model has objects of its own */
int PyAppHasGeometry(App a) {
  return a->equil!=NULL || a->template!=NULL ||
      !IsEmptyGroup(a->nodes) || !IsEmptyGroup(a->elems) ||
      !IsEmptyGroup(a->surfacesEx) || !IsEmptyGroup(a->gridPointsEx) ||
      !IsEmptyGroup(a->separators);
}

/* OpenSaveConfigDlg: the file of the configuration (s: DG_FNAME_LEN) */
void PyConfigFileName(View w,char* s) {
  if (w->xapp->config==NULL) {
    strcpy(s,w->xapp->argv[0]);
    strcpy(GetFileExt(s),GetStr(w,FSTR_CFGEXT));
  } else strcpy(s,w->xapp->config->fName);
}

/* CbSaveConfigDlgOk: the model saved as configuration and loaded again;
   the error code */
int PySaveConfig(View w,char* s) {
  App a=w->app;
  XApp xap=w->xapp;
  char* msg;
  int err;

  err=SaveApp(a,s,DGFM_CONFIG);
  if (err) return err;

  if (xap->config!=NULL) xap->config=FreeApp(xap->config);

  xap->config=LoadApp(xap,s,&msg,&err);
  if (msg!=NULL) Free(msg);
  return err;
}

static Group OtherApps(View w) {
  Group g;
  View w1;
  Index ix;

  g=CreateGroup();
  for (w1=Group1st(w->xapp->views,&ix);w1!=NULL;w1=Next(&ix))
    if (w1->app!=NULL && w1->app!=w->app && !InGroup(g,w1->app))
      GroupAdd(g,w1->app);
  return g;
}

/* CbSaveConfigDlgOk: other models open */
int PyOtherApps(View w) {
  Group g;
  int n;

  g=OtherApps(w);
  n=GroupCount(g);
  g=FreeGroup(g);
  return n;
}

/* CbSaveConfigDlgOk: the other models updated to the configuration */
void PyUpdateOtherApps(View w) {
  Group g;
  App a;
  Index ix;

  g=OtherApps(w);
  for (a=Group1st(g,&ix);a!=NULL;a=Next(&ix)) {
    if (IsEmptyGroup(a->views)) continue;
    UpdateLoadedApp(a,w->xapp->config);
    UndoMark(a);
  }
  g=FreeGroup(g);
}
