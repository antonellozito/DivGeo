/* Topology dialog of the Python interface: the core part of its callbacks
   (x_topo.c), in the same order of core calls. A callback returns the name
   of the resource of its error message (NULL: none); Python reads the
   fields before, shows the box and sets the buttons after. */

#include "py_dg.h"
#include <float.h>
#include <limits.h>

#ifndef True
#define True 1
#define False 0
#endif

/* CbTopoDetectXPoints, CbTopoDetectXSegs */
void PyTopoDetect(View w,int bSegments) {
  SetActiveView(w);

  LockAppUpdate(w->app,1);
  if (bSegments) DetectXPointSegs(w->app);
  else DetectXPoints(w->app);
  LockAppUpdate(w->app,-1);

  UndoMark(w->app);
}

/* TopoDlg_NotifyExamine: the type of the examined object and what the
   dialog shows of it (an X point test: its level; a section: its grid
   point segment) */
int PyTopoExamined(View w,void* object,double* pLevel,int* pZone,
    char** pShort,char** pLong,long* pFlags) {
  XPointTest xpt;
  GridPointSeg gps;

  switch (GetObjType(object)) {
    case T_XPOINTTEST:
      xpt=object;
      *pLevel=xpt->level;
      return T_XPOINTTEST;
    case T_XPOINTSEG:
      gps=FindGridPointSegBySegment(w->app,object);
      assert(gps!=NULL);
      *pZone=gps->zone;
      *pShort=GetGridPointSegShortName(gps);
      *pLong=GetGridPointSegLongName(gps);
      *pFlags=gps->flags;
      return T_XPOINTSEG;
  }
  return GetObjType(object);
}

static int CurObjectGone(View w,void* cur,Group g,int type) {
  return cur==NULL || !InGroup(g,cur) || GetObjType(cur)!=type;
}

/* CbTopoXptLevelBtn */
char* PyTopoXptLevel(View w,void* cur,double level) {
  int cx1,cy1,cx2,cy2,id;
  XPointTest xpt;

  SetActiveView(w);

  if (CurObjectGone(w,cur,w->app->xpointTests,T_XPOINTTEST))
    return "errCurObjectGone";
  xpt=cur;

  if (level==DBL_MAX) return "errBadXptLevel";

  cx1=xpt->cx1;
  cy1=xpt->cy1;
  cx2=xpt->cx2;
  cy2=xpt->cy2;
  id=xpt->id;

  DelXPointTest(w->app,xpt);
  xpt=AddXPointTest(w->app,cx1,cy1,cx2,cy2,level,id);

  if (xpt==NULL) {
    Cancel(w->app);
    return "errBadNewLevel";
  }

  UndoMark(w->app);
  return NULL;
}

/* CbTopoXSegBtn with the fields of the section */
char* PyTopoXSeg(View w,void* cur,int zone,char* shortName,char* longName,
    int bUsed,int bTargetCW) {
  GridPointSeg gps,gps1;
  GridPointEx gpx;
  Index ix;
  long flags;

  SetActiveView(w);

  if (CurObjectGone(w,cur,w->app->xPointSegs,T_XPOINTSEG))
    return "errCurObjectGone";
  gps=FindGridPointSegBySegment(w->app,cur);
  assert(gps!=NULL);

  if (zone==INT_MAX) return "errBadXpsZone";

  for (gps1=AppGridPointSeg1st(w->app,&ix);gps1!=NULL;gps1=Next(&ix))
    if (gps1!=gps && gps1->zone==zone) return "errDupXpsZone";

  SetGridPointSegZone(gps,zone);
  SetGridPointSegShortName(gps,shortName);
  SetGridPointSegLongName(gps,longName);

  flags=gps->flags & ~GPSF_USED & ~GPSF_TARGET_CW;
  if (bUsed) flags |= GPSF_USED;
  if (bTargetCW) flags |= GPSF_TARGET_CW;
  SetGridPointSegFlags(gps,flags);

  if (!(gps->flags & GPSF_USED)) {
    for (gpx=AppGridPointEx1st(gps->app,&ix);gpx!=NULL;gpx=Next(&ix))
      if (gpx->zone==gps->zone) DelGridPointEx(gpx);
  }

  UndoMark(w->app);
  return NULL;
}

/* CbTopoXSegRevert */
char* PyTopoXSegRevert(View w,void* cur) {
  GridPointSeg gps;

  SetActiveView(w);

  if (CurObjectGone(w,cur,w->app->xPointSegs,T_XPOINTSEG))
    return "errCurObjectGone";
  gps=FindGridPointSegBySegment(w->app,cur);
  assert(gps!=NULL);

  SetGridPointSegDir(gps,-gps->dir);
  UndoMark(w->app);

  UndoMark(w->app);
  return NULL;
}

/* CbTopoSZSetBtn with the fields of the zone (bNew: the (New) item);
   gps2 -1 for an empty field */
char* PyTopoSZSet(View w,void* cur,int bNew,int zone,int gps1,int gps2,
    int orient,int bClosed,char* shortName,char* longName) {
  SurfaceZone sz,sz1;
  long flags;
  Index ix;

  SetActiveView(w);

  if (!bNew) {
    if (CurObjectGone(w,cur,w->app->surfaceZones,T_SURFACEZONE))
      return "errCurObjectGone";
    sz=cur;
  } else sz=NULL;

  if (zone==INT_MAX || zone<0) return "errBadSZone";

  for (sz1=AppSurfaceZone1st(w->app,&ix);sz1!=NULL;sz1=Next(&ix))
    if (sz1!=sz && sz1->zone==zone) return "errDupSZone";

  if (orient!=1) orient=-1;

  if (gps1==INT_MAX || gps2==INT_MAX || gps2<-1) return "errSZBadGPZone";

  if (FindGridPointSeg(w->app,gps1)==NULL ||
      (gps2>=0 && FindGridPointSeg(w->app,gps2)==NULL) )
    return "errSZNotFoundGPZone";

  flags=0;
  if (bClosed) flags |= SZF_LIMITBYSURFACE;

  if (sz!=NULL) DelSurfaceZone(sz);

  sz=AddSurfaceZone(w->app,zone,gps1,gps2,orient,-1);
  SetSurfaceZoneFlags(sz,flags);
  SetSurfaceZoneShortName(sz,shortName);
  SetSurfaceZoneLongName(sz,longName);

  UndoMark(w->app);
  return NULL;
}

/* CbTopoSZDeleteBtn */
char* PyTopoSZDelete(View w,void* cur) {
  if (CurObjectGone(w,cur,w->app->surfaceZones,T_SURFACEZONE))
    return "errCurObjectGone";
  DelSurfaceZone(cur);
  UndoMark(w->app);
  return NULL;
}

/* TopoDlgUpdateList: the zones as "number (description)" */
int PyTopoZoneItems(View w,PyItemProc proc) {
  SurfaceZone sz;
  Index ix;
  char buf[1024];
  int n=0;

  for (sz=AppSurfaceZone1st(w->app,&ix);sz!=NULL;sz=Next(&ix)) {
    sprintf(buf,"%d (%s)",sz->zone,GetSurfaceZoneLongName(sz));
    proc(buf,sz->zone);
    n++;
  }
  return n;
}

/* InitSZFromZone: the fields of a zone */
void PyTopoZoneInfo(SurfaceZone sz,int* pZone,char** pShort,char** pLong,
    int* pGps1,int* pGps2,int* pOrient,long* pFlags) {
  *pZone=sz->zone;
  *pShort=GetSurfaceZoneShortName(sz);
  *pLong=GetSurfaceZoneLongName(sz);
  *pGps1=sz->gpZone1;
  *pGps2=sz->gpZone2;
  *pOrient=sz->orient;
  *pFlags=sz->flags;
}
