/* Menu commands and enable rules of the Python interface: the core part of
   the callbacks of the Motif front end (x_menu.c, x_notify.c, x_view.c),
   in the same order of core calls, without the widgets. Error and
   question boxes are shown by Python (SetPyBoxProc). */

#include "py_dg.h"

/* The values of Xlib, used by the copied code */
#ifndef True
#define True 1
#define False 0
#endif

static PyBoxProc boxProc=NULL;

void SetPyBoxProc(PyBoxProc proc) {
  boxProc=proc;
}

static void ErrorBoxPy(View w,char* text) {
  if (boxProc!=NULL) boxProc(w,PYBOX_ERROR,text);
}

/* Edit menu (x_menu.c) ***********************************************/

void PyCbUndo(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);

  Undo(w->app);
}

void PyCbRedo(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);

  Redo(w->app);
}

void PyCbRedoAll(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);

  while (!IsEmptyGroup(w->app->redoStack)) Redo(w->app);
}

void PyCbMarkAll(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);

  MarkGroup(w->app,w->app->elems,1);
  SetViewFlags(w,w->showFlags | SHW_ELEMS);
  UndoMark(w->app);
}

void PyCbMarkAllChords(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);

  MarkGroup(w->app,w->app->chords,1);
  SetViewFlags(w,w->showFlags | SHW_CHORDS);
  UndoMark(w->app);
}

void PyCbUnmarkAll(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);

  MarkGroup(w->app,w->app->mark,0);
  SetViewFlags(w,w->showFlags | SHW_ELEMS);
  UndoMark(w->app);
}

/* Delete of objects of one type; objType as the userData of the Motif
   menu entries */
void PyCbCmDelObjects(View w,int objType) {
  void* p;
  Index ix;

  if (w->app==NULL) return;
  SetActiveView(w);

  switch(objType) {
    case T_MARKING:
      for (p=AppMark1st(w->app,&ix);p!=NULL;p=Next(&ix)) {
        switch (GetObjType(p)) {
          case T_ELEM:
          case T_SOURCE:
          case T_CHORD:
            if (IsLocked(p)) {
              Cancel(w->app);
              LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
              SetViewMsg(w,WhyLocked(w,p));
              return;
            }
            DelObject(w->app,p);
            break;
          default:
            Cancel(w->app);
            LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
            SetViewMsg(w,GetStr(w,ERR_DEL_MARKED_OBJECT_TYPE));
            return;

        }
      }
      UndoMark(w->app);
      ViewMsgEx(w,MSG_MARKED_REMOVED,NULL);
      break;
    case T_NODE:
      for (p=AppNode1st(w->app,&ix);p!=NULL;p=Next(&ix)) {
        if (!IsEmptyGroup(((Node)p)->elems)) continue;
        if (IsLocked(p)) {
          Cancel(w->app);
          LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
          SetViewMsg(w,WhyLocked(w,p));
          return;
        }
        DelObject(w->app,p);
      }
      UndoMark(w->app);
      ViewMsgEx(w,MSG_EMPTYNODESREMOVED,NULL);
      break;
    case T_ELEM:
      for (p=AppElem1st(w->app,&ix);p!=NULL;p=Next(&ix)) {
        if (IsLocked(p)) {
          Cancel(w->app);
          LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
          SetViewMsg(w,WhyLocked(w,p));
          return;
        }
        DelObject(w->app,p);
      }
      UndoMark(w->app);
      ViewMsgEx(w,MSG_ELEMSREMOVED,NULL);
      break;
    case T_SOURCE:
      for (p=AppSource1st(w->app,&ix);p!=NULL;p=Next(&ix)) {
        if (IsLocked(p)) {
          Cancel(w->app);
          LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
          SetViewMsg(w,WhyLocked(w,p));
          return;
        }
        DelObject(w->app,p);
      }
      UndoMark(w->app);
      ViewMsgEx(w,MSG_SOURCESREMOVED,NULL);
      break;
    case T_CHORD:
      for (p=AppChord1st(w->app,&ix);p!=NULL;p=Next(&ix)) {
        if (IsLocked(p)) {
          Cancel(w->app);
          LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
          SetViewMsg(w,WhyLocked(w,p));
          return;
        }
        DelObject(w->app,p);
      }
      UndoMark(w->app);
      ViewMsgEx(w,MSG_CHORDSREMOVED,NULL);
      break;
    case T_SURFACEEX:
      for (p=AppSurfaceEx1st(w->app,&ix);p!=NULL;p=Next(&ix)) {
        if (IsLocked(p)) {
          Cancel(w->app);
          LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
          SetViewMsg(w,WhyLocked(w,p));
          return;
        }
        DelObject(w->app,p);
      }
      UndoMark(w->app);
      ViewMsgEx(w,MSG_SURFACESREMOVED,NULL);
      break;
    case T_DEL_VSURFACE:
      for (p=AppSurfaceEx1st(w->app,&ix);p!=NULL;p=Next(&ix)) {
        if (!SurfaceExVirtual((SurfaceEx)p)) continue;
        if (IsLocked(p)) {
          Cancel(w->app);
          LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
          SetViewMsg(w,WhyLocked(w,p));
          return;
        }
        DelObject(w->app,p);
      }
      UndoMark(w->app);
      ViewMsgEx(w,MSG_SURFACESREMOVED,NULL);
      break;
    case T_GRIDPOINTEX:
      for (p=AppGridPointEx1st(w->app,&ix);p!=NULL;p=Next(&ix)) {
        if (IsLocked(p)) {
          Cancel(w->app);
          LabelObject(w,p,GetStr(w,STR_ERRLABEL),True);
          SetViewMsg(w,WhyLocked(w,p));
          return;
        }
        DelObject(w->app,p);
      }
      UndoMark(w->app);
      ViewMsgEx(w,MSG_GPOINTSREMOVED,NULL);
      break;
    case T_SEPARATOR:
      RemoveAllSeparators(w->app);
      UndoMark(w->app);
      ViewMsgEx(w,MSG_SEPARATORSREMOVED,NULL);
      break;
    case T_EQUIL:
      if (IsLocked(w->app->equil))
        SetViewMsg(w,WhyLocked(w,w->app->equil));
      else {
        DelObject(w->app,w->app->equil);
        UndoMark(w->app);
        ViewMsgEx(w,MSG_EQUILREMOVED,NULL);
      }
      break;
    case T_TEMPLATE:
      if (IsLocked(w->app->template))
        SetViewMsg(w,WhyLocked(w,w->app->template));
      else {
        DelObject(w->app,w->app->template);
        UndoMark(w->app);
        ViewMsgEx(w,MSG_TEMPLATEREMOVED,NULL);
      }
      break;
    case T_SONNET:
      if (IsLocked(w->app->sonnetData))
        SetViewMsg(w,WhyLocked(w,w->app->sonnetData));
      else {
        DelObject(w->app,w->app->sonnetData);
        UndoMark(w->app);
        ViewMsgEx(w,MSG_SONNETREMOVED,NULL);
      }
      break;
    case T_MESH:
      if (w->app->mesh==NULL) break;
      if (IsLocked(w->app->mesh))
        SetViewMsg(w,WhyLocked(w,w->app->mesh));
      else {
        DelObject(w->app,w->app->mesh);
        UndoMark(w->app);
        ViewMsgEx(w,MSG_SONNETREMOVED,NULL);
      }
      break;
    default:
      assert(0);
  }
}

void PyCbCmRemoveSeparators(View w) {

  if (w->app==NULL) return;
  SetActiveView(w);

  RemoveAllSeparators(w->app);
  UndoMark(w->app);
}

/* View menu (x_menu.c) ***********************************************/

void PyCbViewRefresh(View w) {
  SetActiveView(w);
  ClearView(w);
  RepaintView(w);
}

void PyCbZoomIn(View w) {
  SetActiveView(w);
  if (w->app==NULL) return;
  SetViewFactor(w,w->centerX,w->centerY,w->zoomX*2,w->zoomY*2);
  UndoMark(w->app);
}

void PyCbZoomOut(View w) {
  SetActiveView(w);
  if (w->app==NULL) return;
  SetViewFactor(w,w->centerX,w->centerY,w->zoomX/2,w->zoomY/2);
  UndoMark(w->app);
}

void PyCbShowPicture(View w) {
  SetActiveView(w);
  if (w->app==NULL) return;
  ShowPicture(w);
  UndoMark(w->app);
}

void PyCbShowSelection(View w) {
  double minX,minY,maxX,maxY;

  if (w->app==NULL) return;
  SetActiveView(w);

  if (IsEmptyGroup(w->app->mark)) {
    ErrorBoxPy(w,GetStr(w,ERR_NOSELECTION));
    return;
  }
  minX=0;maxX=-1;
  CalcGroupExtents(w->app->mark,&minX,&minY,&maxX,&maxY);
  if (minX>=maxX || minY>=maxY)
    SetViewFactor(w,minX,minY,w->zoomX,w->zoomY);
  else SetViewRect(w,
    minX-(maxX-minX)*w->shpIncr/100,
    minY-(maxY-minY)*w->shpIncr/100,
    maxX+(maxX-minX)*w->shpIncr/100,
    maxY+(maxY-minY)*w->shpIncr/100);

  UndoMark(w->app);
}

void PyCbIncAngle(View w) {
  ValidatePtr(w,"CbIncAngle");
  SetActiveView(w);

  SetViewAngle(w,w->xyAngle+M_PI/180);
  UndoMark(w->app);
}

void PyCbDecAngle(View w) {
  ValidatePtr(w,"CbDecAngle");
  SetActiveView(w);

  SetViewAngle(w,w->xyAngle-M_PI/180);
  UndoMark(w->app);
}

void PyCbResetAngle(View w) {
  ValidatePtr(w,"CbDecAngle");
  SetActiveView(w);

  SetViewAngle(w,0);
  UndoMark(w->app);
}

void PyCbStretchX(View w) {
  ValidatePtr(w,"CbStretchX");
  SetActiveView(w);

  SetViewRect(w,(w->minX+w->centerX)/2,w->minY,(w->maxX+w->centerX)/2,w->maxY);
  UndoMark(w->app);
}

void PyCbStretchY(View w) {
  ValidatePtr(w,"CbStretchY");
  SetActiveView(w);

  SetViewRect(w,w->minX,(w->minY+w->centerY)/2,w->maxX,(w->maxY+w->centerY)/2);
  UndoMark(w->app);
}

void PyCbShrinkX(View w) {
  ValidatePtr(w,"CbShrinkX");
  SetActiveView(w);

  SetViewRect(w,2*w->minX-w->centerX,w->minY,2*w->maxX-w->centerX,w->maxY);
  UndoMark(w->app);
}

void PyCbShrinkY(View w) {
  ValidatePtr(w,"CbShrinkY");
  SetActiveView(w);

  SetViewRect(w,w->minX,2*w->minY-w->centerY,w->maxX,2*w->maxY-w->centerY);
  UndoMark(w->app);
}

void PyCbResetAspectRatio(View w) {
  double zoom,x1,x2,y1,y2;

  ValidatePtr(w,"CbResetAspectRatio");
  SetActiveView(w);

  zoom=min(w->zoomX,w->zoomY);
  x1=w->centerX-w->width/zoom/2;
  x2=w->centerX+w->width/zoom/2;
  y1=w->centerY-w->height/zoom/2;
  y2=w->centerY+w->height/zoom/2;

  SetViewRect(w,x1,y1,x2,y2);
  UndoMark(w->app);
}

void PyCmPrevZoom(View w) {
  double minX,minY,maxX,maxY,xyAngle;
  int i;

  if (w->app==NULL) return;
  SetActiveView(w);

  i=GetPrevViewInfo(w->app,&minX,&minY,&maxX,&maxY,&xyAngle,False);
  if (i) {
    Cancel(w->app);
  } else {
    SetView(w,minX,minY,maxX,maxY,xyAngle);
    MarkPrevViewDone(w->app);
    UndoMark(w->app);
  }
}

void PyCmRemoveLabels(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);

  RemoveAllViewLabels(w);
  UndoMark(w->app);
}

/* The show flags of the Display and Mode menus (CbChangeFlags with the
   flags ProcessXViewShowFlags read from the toggles) */
void PyCbChangeFlags(View w,unsigned long flags) {
  SetActiveView(w);
  SetViewFlags(w,flags);
  if (w->app!=NULL) UndoMark(w->app);
}

/* Commands menu (x_menu.c) *******************************************/

void PyCbCmAppendTemplate(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);
  if (w->app->template==NULL) {
    ErrorBoxPy(w,GetStr(w,ERR_NOTEMPLATE));
    return;
  }

  LockAppUpdate(w->app,1);
  AppendTemplate(w->app);
  SetViewFlags(w,w->showFlags | SHW_ELEMS | SHW_IRRNODES | SHW_NORMALS);
  LockAppUpdate(w->app,-1);
  SetViewMsg(w,GetStr(w,0));
  UndoMark(w->app);
}

void PyCmConvertElemsToChords(View w) {
  int i;
  void* pErr;
  Group g;

  if (w->app==NULL) return;
  SetActiveView(w);

  g=CopyGroup(w->app->mark,NULL);
  RestrictGroupToType(g,T_ELEM);

  if (IsEmptyGroup(g)) {
    FreeGroup(g);
    ErrorBoxPy(w,GetStr(w,ERR_NO_MARKED_ELEMS));
    return;
  }

  i=ConvertElemsToChords(w->app,g,&pErr);
  FreeGroup(g);

  if (i) {
    Cancel(w->app);
    if (pErr!=NULL) LabelObject(w,pErr,GetStr(w,STR_ERRLABEL),True);
    UndoMark(w->app);
    ErrorBoxPy(w,i==ERR_LOCKED? WhyLocked(w,pErr) : GetStr(w,i));
  } else UndoMark(w->app);
}

void PyCmConvertChordsToElems(View w) {
  int i;
  void* pErr;
  Group g;

  if (w->app==NULL) return;
  SetActiveView(w);

  g=CopyGroup(w->app->mark,NULL);
  RestrictGroupToType(g,T_CHORD);

  if (IsEmptyGroup(g)) {
    FreeGroup(g);
    ErrorBoxPy(w,GetStr(w,ERR_NO_MARKED_CHORDS));
    return;
  }

  i=ConvertChordsToElems(w->app,g,&pErr);
  FreeGroup(g);

  if (i) {
    Cancel(w->app);
    if (pErr!=NULL) LabelObject(w,pErr,GetStr(w,STR_ERRLABEL),True);
    UndoMark(w->app);
    ErrorBoxPy(w,i==ERR_LOCKED? WhyLocked(w,pErr) : GetStr(w,i));
  } else UndoMark(w->app);
}

void PyCbCmRenumber(View w) {
  if (w->app==NULL) return;
  SetActiveView(w);

  LockAppUpdate(w->app,1);
  RenumberElems(w->app);
  LockAppUpdate(w->app,-1);
  UndoMark(w->app);
}

void PyCbCmExtChords(View w) {
  void* obj;
  int nc=0;
  Index ix;

  if (w->app==NULL) return;
  for (obj=Group1st(w->app->mark,&ix);obj!=NULL;obj=Next(&ix))
    if (GetObjType(obj)==T_CHORD) {
      nc++;
      ExtendChord(w->app,(Chord)obj);
    }
  if (nc) UndoMark(w->app);
  else SetViewMsg(w,GetStr(w,MSG_NOMARKEDCHORDS));
}

/* 1: the invalid variables dialog is to be shown */
int PyCbCmShowInvalidVars(View w) {
  if (!CheckAllVars(w->app,NULL)) {
    ViewMsgEx(w,MSG_ALLVARSVALID,NULL);
    return 0;
  }
  ViewMsgEx(w,MSG_SOMEVARSINVALID,NULL);
  return 1;
}

void PyCmRebuildCarreInfo(View w) {
  int r;

  SetActiveView(w);

  r=RebuildCarreSurfaces(w->app,-1);
  if (r) {
    Cancel(w->app);
    ErrorBoxPy(w,GetStr(w,r));
    return;
  }

  r=RebuildCarreGridPoints(w->app,-1);
  if (r) {
    Cancel(w->app);
    ErrorBoxPy(w,GetStr(w,r));
    return;
  }

  UndoMark(w->app);
}

/* Variables menu (x_menu.c); the edit dialog is opened by Python */
VarSet PyCbAddVarSet(View w,VarSetDef vsd) {
  VarSet vs;

  if (w->app==NULL) return NULL;
  SetActiveView(w);

  vs=AddVarSet(w->app,vsd);

  UndoMark(w->app);
  return vs;
}

void PyCbDelVarSet(View w,VarSet vs) {
  if (w->app==NULL) return;
  SetActiveView(w);

  DelVarSet(w->app,vs);
  UndoMark(w->app);
}

/* Entries of the Variables menu (UpdateXViewVarsMenu): proc is called for
   each entry in the order Motif creates them. Returns -1 without a model,
   else 1 if the Add cascade is sensitive plus 2 if the Remove one is. */
static void VarsMenuItem(View w,VarSetDef vsd,int menu,PyVarsMenuProc proc) {
  int i;
  VarSet vs;
  Index ix;
  char buf[256],* s;

  i=GroupCount(vsd->varSets);
  if (i>1) {
    proc(menu,PYVM_SUBMENU,vsd,vsd->descr);
    for (vs=Group1st(vsd->varSets,&ix);vs!=NULL;vs=Next(&ix)) {
      s=GetIndexVarValue(vs);
      if (s==NULL) {
        sprintf(buf,"#%u",(unsigned)GroupIndex(vsd->varSets,vs)+1);
        s=buf;
      }
      proc(menu,PYVM_SUBITEM,vs,s);
    }
    proc(menu,PYVM_SUBEND,vsd,NULL);
  } else {
    vs=Group1st(vsd->varSets,NULL);
    proc(menu,PYVM_ITEM,vs,GetObjDescription(w,vs));
  }
}

int PyVarsMenu(View w,PyVarsMenuProc proc) {
  VarSetDef vsd;
  Index ix;
  int i,j,r=0;

  if (w->app==NULL) return -1;

  for (j=0,vsd=AppVarSetDef1st(w->app,&ix);vsd!=NULL;vsd=Next(&ix)) {
    i=GroupCount(vsd->varSets);
    if (i<vsd->maxVarSets) {
      j++;
      proc(PYVM_ADD,PYVM_ITEM,vsd,GetObjDescription(w,vsd));
    }
  }
  if (j) r|=1;

  for (j=0,vsd=AppVarSetDef1st(w->app,&ix);vsd!=NULL;vsd=Next(&ix)) {
    i=GroupCount(vsd->varSets);
    if (!i) continue;

    VarsMenuItem(w,vsd,PYVM_EDIT,proc);
    if (i>vsd->minVarSets) {
      j++;
      VarsMenuItem(w,vsd,PYVM_DELETE,proc);
    }
  }
  if (j) r|=2;
  return r;
}

/* Enable rules (x_notify.c, x_view.c): -1 without a model (no change),
   else the sensitivity */
int PyDwUndoButton(View w,int d) {
  if (w->app==NULL) return -1;

  switch (d) {
    case 0:
      return !!GroupCount(w->app->undoStack);
    case 1:
      return !!GroupCount(w->app->redoStack);
    case 2:
      return !GetPrevViewInfo(w->app,NULL,NULL,NULL,NULL,NULL,True);
    default:
      assert(0);
  }
  return -1;
}

int PyDwIfExists(View w,int d) {
  Node n;
  SurfaceEx sx;
  Index ix;

  if (w->app==NULL) return -1;

  switch(d) {
    case T_MARKING:
      return !IsEmptyGroup(w->app->mark);
    case T_NODE:
      return !IsEmptyGroup(w->app->nodes);
    case T_ELEM:
      return !IsEmptyGroup(w->app->elems);
    case T_SURFACEEX:
      return !IsEmptyGroup(w->app->surfacesEx);
    case T_DEL_VSURFACE:
      for (sx=AppSurfaceEx1st(w->app,&ix);sx!=NULL;sx=Next(&ix))
        if (SurfaceExVirtual(sx)) return True;
      return False;
    case T_GRIDPOINTEX:
      return !IsEmptyGroup(w->app->gridPointsEx);
    case T_SEPARATOR:
      return !IsEmptyGroup(w->app->separators);
    case T_SOURCE:
      return !IsEmptyGroup(w->app->sources);
    case T_CHORD:
      return !IsEmptyGroup(w->app->chords);
    case T_EQUIL:
      return w->app->equil!=NULL;
    case T_TEMPLATE:
      return w->app->template!=NULL;
    case T_SONNET:
      return w->app->sonnetData!=NULL;
    case T_XPOINTTEST:
      return !IsEmptyGroup(w->app->xpointTests);
    case T_XPOINTSEG:
      return !IsEmptyGroup(w->app->xPointSegs);
    case T_GRIDPOINTSEG:
      return !IsEmptyGroup(w->app->gridPointSegs);
    case T_SURFACEZONE:
      return !IsEmptyGroup(w->app->surfaceZones);
    case T_MESH:
      return w->app->mesh!=NULL;
    case T_EMPTYNODE:
      for (n=AppNode1st(w->app,&ix);n!=NULL;n=Next(&ix))
        if (IsEmptyNode(n)) return True;
      return False;
    default:
      assert(0);
  }
  return -1;
}

int PyDwIfNotExists(View w,int d) {
  int r=PyDwIfExists(w,d);

  return r<0 ? r : !r;
}

int PyDwShowFlags(View w,int d) {
  if (w->app==NULL) return -1;

  switch (d) {
    case 0:
      return !!(w->showFlags & SHW_STRETCH);
    case 1:
      return !(w->showFlags & SHW_TOPVIEW);
    default:
      assert(0);
  }
  return -1;
}

int PyDwRebuildCarreInfo(View w) {
  assert(w->type==T_VIEW);

  return w->app->outputMode==OUTPUTMODE_CARRE &&
      (GroupCount(w->app->surfacesEx) || GroupCount(w->app->gridPointsEx));
}

/* Messages (macros and variadic functions of the core) ****************/

void PyViewMsg(View w,char* msg) {
  SetViewMsg(w,msg);
}

void PyViewMsgId(View w,int id) {
  ViewMsgEx(w,id,NULL);
}

char* PyGetStr(View w,int id) {
  return GetStr(w,id);
}

/* Tests of NotifyXmView, CloseXmView (x_view.c, x_fileio.c) */
int PyVarFromVarSetDef(Var v) {
  return GetObjType(v->origin)==T_VARSETDEF;
}

int PyVarIsLayerIndex(Var v) {
  return !!(v->def->flags & VF_LAYERINDEX);
}

int PyMeshUnsaved(App a) {
  return a->mesh!=NULL && a->mesh->alt;
}

/* One autosave of AutosaveProc (x_view.c), after its wait: the model
   saved as its name with ~ appended, while the model carries that name
   (in Motif the name is extended in place). Returns 1 when the autosave
   of this view ends. */
int PyAutosave(View w) {
  char* s,* fName;
  int err;

  if (w==NULL) return 1;
  if (w->app==NULL) return 1;

  if (DetectFileType(w->app->fName)==FT_DG_CONFIG) {
    SetViewMsg(w,GetStr(w,ERR_NOSAVECONFIG));
    return 1;
  }

  /* does not lock variables while auto-saving */

  fName=w->app->fName;
  s=Malloc(strlen(fName)+2);
  strcpy(s,fName);
  strcat(s,"~");
  w->app->fName=s;

  err=SaveApp(w->app,s,DGFM_APP);
  if (err) {
    SetViewMsg(w,GetStr(w,err));
  } else {
    SetViewMsg(w,GetStr(w,MSG_FILEAUTOSAVED));
    w->app->alt=0;
  }

  w->app->fName=fName;                 /* avoid multiple tildes */
  Free(s);
  return 0;
}

