/* Dialogs of the Python interface: the core part of the dialog callbacks
   of the Motif front end (x_rotmov.c, x_creobj.c, x_fileio.c, x_info.c,
   x_setup.c, x_mesh.c), in the same order of core calls. Python reads the
   dialog fields, calls these functions and shows the box whose text they
   return (NULL: no box). */

#include "py_dg.h"

#ifndef True
#define True 1
#define False 0
#endif

/* Move/Rotate (x_rotmov.c) *******************************************/

/* CbRotMoveDlgRotate after SetActiveView */
char* PyRotMoveRotate(View w,int bN,int bT) {
  Node n;
  Index ix;
  double angle;

  if (!bT && !bN) return GetStr(w,ERR_NOTHINGTODO);
  LockAppUpdate(w->app,1);
  if (bT) {
    angle=w->app->template->angle+M_PI/2;
    if (angle>=M_PI*2) angle-=M_PI*2;
    ChangeTemplate(w->app,w->app->template->xIncr,
      w->app->template->yIncr,angle,w->app->template->scale);
    SetViewFlags(w,w->showFlags | SHW_TEMPLATE);
  }
  if (bN) {
    for (n=AppNode1st(w->app,&ix);n!=NULL;n=Next(&ix)) {
      if (IsLocked(n)) {
        Cancel(w->app);
        return WhyLocked(w,n);
      }
      ChangeNode(w->app,n,-n->y,n->x);
    }
    SetViewFlags(w,w->showFlags | SHW_ELEMS);
  }
  LockAppUpdate(w->app,-1);
  UndoMark(w->app);
  return NULL;
}

/* CbRotMoveDlgMove after the fields are read and checked */
char* PyRotMoveMove(View w,double x,double y,double scale,int bN,int bT) {
  Node n;
  Index ix;

  if (!bT && !bN) return GetStr(w,ERR_NOTHINGTODO);
  LockAppUpdate(w->app,1);
  if (bT) {
    ChangeTemplate(w->app,w->app->template->xIncr*scale+x,
      w->app->template->yIncr*scale+y,w->app->template->angle,
      w->app->template->scale*scale);
    SetViewFlags(w,w->showFlags | SHW_TEMPLATE);
  }
  if (bN) for (n=AppNode1st(w->app,&ix);n!=NULL;n=Next(&ix)) {
    if (IsLocked(n)) {
      Cancel(w->app);
      return WhyLocked(w,n);
    }
    ChangeNode(w->app,n,n->x*scale+x,n->y*scale+y);
    SetViewFlags(w,w->showFlags | SHW_ELEMS);
  }
  LockAppUpdate(w->app,-1);
  UndoMark(w->app);
  return NULL;
}

/* CbStretchDlgSetFactors after the factors are read and checked */
void PyStretchDlgSet(View w,double xScale,double yScale) {
  SetViewRect(w,
    1/xScale*w->minX+(1-1/xScale)*w->centerX,
    1/yScale*w->minY+(1-1/yScale)*w->centerY,
    1/xScale*w->maxX+(1-1/xScale)*w->centerX,
    1/yScale*w->maxY+(1-1/yScale)*w->centerY);
  UndoMark(w->app);
}

/* Create point, source, chord (x_creobj.c) ***************************/

/* CbCreateNode after the numbers are read */
char* PyCreateNode(View w,double x,double y) {
  Node n;
  Index ix;

  for (n=AppNode1st(w->app,&ix);n!=NULL;n=Next(&ix))
      if (n->x==x && n->y==y) {
    LabelObject(w,n,GetStr(w,STR_ERRLABEL),True);
    UndoMark(w->app);
    return GetStr(w,ERR_ALREADYEXISTS);
  }

  n=AddNode(w->app,x,y);
  LabelObject(w,n,GetStr(w,STR_NEWLABEL),True);
  SetViewFlags(w,w->showFlags | SHW_NODES | SHW_IRRNODES);
  UndoMark(w->app);
  return NULL;
}

/* CbCreateSource after the numbers are read */
char* PyCreateSource(View w,double x,double y) {
  Source src;
  Index ix;

  for (src=AppSource1st(w->app,&ix);src!=NULL;src=Next(&ix))
      if (src->x==x && src->y==y) {
    LabelObject(w,src,GetStr(w,STR_ERRLABEL),True);
    UndoMark(w->app);
    return GetStr(w,ERR_ALREADYEXISTS);
  }

  src=AddSource(w->app,x,y);
  LabelObject(w,src,GetStr(w,STR_NEWLABEL),True);
  SetViewFlags(w,w->showFlags | SHW_SOURCES);
  UndoMark(w->app);
  return NULL;
}

/* CbCreateChord after the numbers are read: Cartesian x, y, z, or
   cylindrical r, z, phi (degrees) */
char* PyCreateChord(View w,int bCartes,double x1,double y1,double z1,
    double x2,double y2,double z2) {
  double ph1,ph2;
  Chord ch;
  Index ix;

  if (!bCartes) {
    ph1=z1;
    ph2=z2;
    z1=x1*sin(M_PI/180*ph1);x1=x1*cos(M_PI/180*ph1);
    z2=x2*sin(M_PI/180*ph2);x2=x2*cos(M_PI/180*ph2);
  }

  for (ch=AppChord1st(w->app,&ix);ch!=NULL;ch=Next(&ix))
      if (ch->x1==x1 && ch->y1==y1 && ch->x2==x2 && ch->y2==y2
          && ch->z1==z1 && ch->z2==z2) {
    LabelObject(w,ch,GetStr(w,STR_ERRLABEL),True);
    UndoMark(w->app);
    return GetStr(w,ERR_ALREADYEXISTS);
  }
  ch=AddChord3D(w->app,x1,y1,z1,x2,y2,z2);
  if (ch==NULL) return NULL;
  LabelObject(w,ch,GetStr(w,STR_NEWLABEL),True);
  SetViewFlags(w,w->showFlags | SHW_CHORDS);
  UndoMark(w->app);
  return NULL;
}

/* Create surface(s) (x_creobj.c) *************************************/

/* CSD_CreateSingle by level, after the level is read */
char* PyCreateSurfaceByLevel(View w,int area,double level) {
  int i;
  SurfaceEx surfex;
  Index ix;

  for (surfex=AppSurfaceEx1st(w->app,&ix);surfex!=NULL;surfex=Next(&ix))
      if (surfex->zone==area && surfex->level==level) {
    LabelObject(w,surfex,GetStr(w,STR_ERRLABEL),True);
    UndoMark(w->app);
    return GetStr(w,ERR_ALREADYEXISTS);
  }

  surfex=AddSurfaceEx(w->app,area,level,&i);
  if (surfex==NULL) return GetStr(w,i);

  LabelObject(w,surfex,GetStr(w,STR_NEWLABEL),True);
  SetViewFlags(w,w->showFlags | SHW_SURFACES);
  UndoMark(w->app);
  return NULL;
}

/* CSD_CreateSingle through a point, after x and y are read */
char* PyCreateSurfaceByXY(View w,double x,double y) {
  int i;
  SurfaceEx surfex;
  Index ix;

  for (surfex=AppSurfaceEx1st(w->app,&ix);surfex!=NULL;surfex=Next(&ix))
      if (surfex->bDrawn && surfex->originX==x && surfex->originY==y) {
    LabelObject(w,surfex,GetStr(w,STR_ERRLABEL),True);
    UndoMark(w->app);
    return GetStr(w,ERR_ALREADYEXISTS);
  }

  surfex=AddSurfaceExByXY(w->app,x,y,&i);
  if (surfex==NULL) return GetStr(w,i);

  LabelObject(w,surfex,GetStr(w,STR_NEWLABEL),True);
  SetViewFlags(w,w->showFlags | SHW_SURFACES);
  UndoMark(w->app);
  return NULL;
}

/* CSD_CreateMultiple, Sonnet mode: the surfaces made with the old creator
   id deleted; returns their number */
int PyDelSurfacesOfCreator(View w,char* oldCreatorId) {
  SurfaceEx sex;
  Index ix;
  int i;

  for (i=0,sex=AppSurfaceEx1st(w->app,&ix);sex!=NULL;sex=Next(&ix)) {
    if (!strcmp(StripPrefix(GetSurfaceExCreatorId(sex),"+"),
        StripPrefix(oldCreatorId,"+")))
      {DelSurfaceEx(sex);i++;}
  }
  return i;
}

/* The surface remembered by the Examine tool: False if none or deleted */
int PyExaminedSurfaceEx(View w,int* pZone,double* pLevel,char** pCreatorId) {
  SurfaceEx surfex;

  surfex=w->lastExaminedSurfaceEx;
  if (surfex==NULL || !InGroup(w->app->surfacesEx,surfex)) return False;
  *pZone=surfex->zone;
  *pLevel=surfex->level;
  *pCreatorId=GetSurfaceExCreatorId(surfex);
  return True;
}

/* The grid point remembered by the Examine tool: False if none */
int PyExaminedGridPointEx(View w,char** pCreatorId) {
  GridPointEx gpx;

  gpx=w->lastExaminedGridPointEx;
  if (gpx==NULL || !InGroup(w->app->gridPointsEx,gpx)) return False;
  *pCreatorId=GetGridPointExCreatorId(gpx);
  return True;
}

/* CbCSD_MarkBoundingElem after the area is read */
void PyMarkBoundingElem(View w,int area) {
  SurfaceZone sz;

  sz = FindSurfaceZone(w->app,area);

  MarkGroup(w->app,w->app->mark,0);
  if (sz->innermost!=NULL) MarkObject(w->app,sz->innermost,1);
  else SetViewMsg(w,GetStr(w,MSG_NOBOUNDINGELEMS));
  SetViewFlags(w,w->showFlags | SHW_ELEMS);
  UndoMark(w->app);
}

/* The items of the area menus (DwCSD): the used surface zones */
int PySurfaceZoneItems(View w,PyItemProc proc) {
  SurfaceZone sz;
  Index ix;
  int n=0;

  for (sz=AppSurfaceZone1st(w->app,&ix);sz!=NULL;sz=Next(&ix)) {
    if (!SurfaceZoneIsUsed(sz)) continue;
    proc(GetSurfaceZoneDescription(sz),sz->zone);
    n++;
  }
  return n;
}

/* The items of the zone menus (DwCGPD): the used separatrix segments */
int PyGridPointSegItems(View w,PyItemProc proc) {
  GridPointSeg gps;
  Index ix;
  int n=0;

  for (gps=AppGridPointSeg1st(w->app,&ix);gps!=NULL;gps=Next(&ix)) {
    if (!GridPointSegIsUsed(gps)) continue;
    proc(GetGridPointSegDescription(gps),gps->zone);
    n++;
  }
  return n;
}

/* Create grid point(s) (x_creobj.c) **********************************/

/* CGPD_CreateSingle after the value is read and checked */
char* PyCreateGridPoint(View w,int area,double value) {
  GridPointEx gpx;
  Index ix;

  for (gpx=AppGridPointEx1st(w->app,&ix);gpx!=NULL;gpx=Next(&ix))
      if (gpx->zone==area && gpx->value==value) {
    LabelObject(w,gpx,GetStr(w,STR_ERRLABEL),True);
    UndoMark(w->app);
    return GetStr(w,ERR_ALREADYEXISTS);
  }
  gpx=AddGridPointEx(w->app,area,value);
  if (gpx==NULL) return GetStr(w,ERR_INTERNALGP);
  LabelObject(w,gpx,GetStr(w,STR_NEWLABEL),True);

  SetViewFlags(w,w->showFlags | SHW_GRIDPOINTS);
  UndoMark(w->app);
  return NULL;
}

/* CGPD_CreateMultiple: the grid points of the zone deleted */
void PyDelGridPointsOfZone(View w,int zone) {
  GridPointEx gpx;
  Index ix;

  for (gpx=AppGridPointEx1st(w->app,&ix);gpx!=NULL;gpx=Next(&ix))
    if (gpx->zone==zone) DelGridPointEx(gpx);
}

/* Export elements as template (x_fileio.c) ***************************/

/* EEAT_OkCheck: 0 ok, 1 no elements (box on the dialog), 2 another
   object type (labelled; box on the main window) */
int PyExportElemsCheck(View w,Group g) {
  void* p;
  Index ix;

  if (IsEmptyGroup(g)) return 1;

  for (p=Group1st(g,&ix);p!=NULL;p=Next(&ix))
    if (GetObjType(p)!=T_ELEM) {
      LabelObject(w,p,GetStr(w,STR_ERRLABEL),1);
      UndoMark(w->app);
      return 2;
    }

  return 0;
}

/* File names and values the dialogs show *****************************/

/* The file of the equilibrium (1), template (2) or mesh (3), NULL if the
   model has none */
char* PyAppFileName(App a,int which) {
  switch (which) {
    case 1: return a->equil!=NULL ? a->equil->fName : NULL;
    case 2: return a->template!=NULL ? a->template->fName : NULL;
    case 3: return a->mesh!=NULL ? a->mesh->fileName : NULL;
  }
  return NULL;
}

/* The values of the Statistics dialog (UpdateInfoDlg) as numbers: nodes,
   elements, sources, unused numbers, separators, chords */
void PyInfoCounts(App a,unsigned* counts) {
  counts[0]=(unsigned)GroupCount(a->nodes);
  counts[1]=(unsigned)GroupCount(a->elems);
  counts[2]=(unsigned)GroupCount(a->sources);
  counts[3]=(unsigned)GetUnusedNumberCount(a);
  counts[4]=(unsigned)GroupCount(a->separators);
  counts[5]=(unsigned)GroupCount(a->chords);
}

/* The mesh options of the model (ResetMeshOptionsDlg) */
void PyMeshOptions(App a,int* pMode,double* pThreshold,int* pDoubleBorder) {
  *pMode=a->meshSlidingMode;
  *pThreshold=a->meshSlidingThreshold;
  *pDoubleBorder=a->bDoubleMeshBorder;
}

/* The header of a mesh */
char* PyMeshHeader(Mesh m) {
  return m->headerString;
}
