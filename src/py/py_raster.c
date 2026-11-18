/* Raster drawing of the view records for the Python interface
   (lib/python/divgeo/view/raster.py and scaled.py): the pixels of lines,
   rectangles and ellipses written into the picture (0xAARRGGBB) in the
   order of the records, with the rules of the Python rasterizer, which
   follows the X server the Motif view draws on:
   - picture of the view (scaled==0): integer screen positions, 16-bit as in
     the protocol, zero-width lines as Bresenham lines with the end point,
     wide lines and rectangles as the pixels whose centres lie inside the
     shape (a centre on the border counts when the inside is right of it,
     or below it), full ellipses sampled along the curve;
   - finer picture (scaled!=0, f pixels per pixel of the view, dpr device
     pixels per pixel of the view, ss supersampling): positions s*f-1/2,
     lines one device pixel wide at zero width, outlines of rectangles and
     ellipses of the given width.
   COPY writes the colour, XOR exclusive-ors it (each pixel of a primitive
   once). Texts are drawn by Python. Compiled without contraction of
   multiplications and additions, so that every pixel is the one of the
   Python rasterizer. */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "py_dg.h"

#define OPAQUE 0xFF000000u
#define EPS_X 1e-6
#define EPS_Y 1e-9

typedef struct {
  unsigned int* img;
  long w,h;
  unsigned int color;
  int xorMode;
} Target;

static void Put(Target* t,long x,long y) {
  unsigned int* p;

  if (x<0 || y<0 || x>=t->w || y>=t->h) return;
  p=t->img+y*t->w+x;
  if (t->xorMode) *p^=t->color; else *p=OPAQUE|t->color;
}

/* Floor division of the Python rasterizer (b>0) */
static long FloorDiv(long a,long b) {
  long q=a/b;

  if (a%b!=0 && a<0) q--;
  return q;
}

static long I16(long v) {
  return ((v+32768)&0xFFFF)-32768;
}

static long C16(long v) {
  return v&0xFFFF;
}

/* Zero-width line, end point included (zero_lines) */
static void ZeroLine(Target* t,long x1,long y1,long x2,long y2) {
  long dx=x2-x1,dy=y2-y1,adx=labs(dx),ady=labs(dy);
  long sx=dx<0 ? -1 : 1,sy=dy<0 ? -1 : 1;
  int ymajor=ady>=adx;
  long amaj=ymajor ? ady : adx,amin=ymajor ? adx : ady;
  long e1=2*amin,dd=2*amaj,e0=e1-amaj,k,m;

  for (k=0;k<=amaj;k++) {
    m=k==0 ? 0 : FloorDiv(e0+(k-1)*e1,dd)+1;
    if (ymajor) Put(t,x1+m*sx,y1+k*sy);
    else Put(t,x1+k*sx,y1+m*sy);
  }
}

/* Wide line with butt caps: the centres inside the rectangle of the given
   width around the segment, per column (rows for steep lines)
   (wide_lines); returns the number of pixels */
static long WideLine(Target* t,double x1,double y1,double x2,double y2,
    double width) {
  double hw=width/2.0,lx=x2-x1,ly=y2-y1,len=hypot(lx,ly);
  double a1,b1,la,lb,ea,eb,ua,ub,a2,ext,amin,amax,i,a,s1,s2,lo,hi,t1,t2;
  double base,jmin,jmax,j;
  long n,k,c,cnt,count=0;
  int steep,flat,inside;

  if (!(len>0)) return 0;
  steep=fabs(ly)>fabs(lx);
  a1=steep ? y1 : x1; b1=steep ? x1 : y1;
  la=steep ? ly : lx; lb=steep ? lx : ly;
  ea=steep ? EPS_Y : EPS_X; eb=steep ? EPS_X : EPS_Y;
  ua=la/len; ub=lb/len;
  a2=a1+la;
  ext=hw*fabs(ub);
  amin=floor(fmin(a1,a2)-ext)-1;
  amax=ceil(fmax(a1,a2)+ext)+1;
  n=(long)(amax-amin+1);
  flat=ub==0;
  base=b1-eb;
  for (k=0;k<n;k++) {
    i=amin+(double)k;
    a=i+ea-a1;
    t1=(a*ub-hw)/ua;
    t2=(a*ub+hw)/ua;
    lo=fmin(t1,t2);
    hi=fmax(t1,t2);
    if (!flat) {
      s1=-a*ua/ub;
      s2=(len-a*ua)/ub;
      lo=fmax(lo,fmin(s1,s2));
      hi=fmin(hi,fmax(s1,s2));
    }
    inside=!flat || (a*ua>0 && a*ua<len);
    if (!inside) continue;
    jmin=floor(lo+base)+1;
    jmax=ceil(hi+base)-1;
    cnt=(long)(jmax-jmin+1);
    for (c=0;c<cnt;c++) {
      j=jmin+(double)c;
      if (steep) Put(t,(long)j,(long)i); else Put(t,(long)i,(long)j);
    }
    if (cnt>0) count+=cnt;
  }
  return count;
}

/* Outline of a rectangle (XDrawRectangle) at integer positions
   (rectangle) */
static void Rectangle(Target* t,long x,long y,long w,long h,long width) {
  long i,j,pad;
  double hw,cx,cy;
  int outer,inner;

  if (width==0) {
    for (i=x;i<=x+w;i++) Put(t,i,y);
    for (j=y+1;j<y+h;j++) Put(t,x+w,j);
    if (h) for (i=x+w;i>=x;i--) Put(t,i,y+h);
    if (w) for (j=y+h-1;j>y;j--) Put(t,x,j);
    return;
  }
  hw=width/2.0;
  pad=(long)ceil(hw)+1;
  for (j=y-pad;j<=y+h+pad;j++) for (i=x-pad;i<=x+w+pad;i++) {
    cx=(double)i+EPS_X;
    cy=(double)j+EPS_Y;
    outer=cx>(double)x-hw && cx<(double)(x+w)+hw &&
          cy>(double)y-hw && cy<(double)(y+h)+hw;
    inner=cx>(double)x+hw && cx<(double)(x+w)-hw &&
          cy>(double)y+hw && cy<(double)(y+h)-hw;
    if (outer && !inner) Put(t,i,j);
  }
}

static int CmpLong(const void* a,const void* b) {
  long x=*(const long*)a,y=*(const long*)b;
  return x<y ? -1 : x>y;
}

/* Full ellipse in the box (x, y, w, h), sampled, each pixel once
   (ellipse_pixels) */
static void Ellipse(Target* t,long x,long y,long w,long h) {
  long n,k,m=0,*key;
  double step,tt,px,py;

  if (w==0 && h==0) { Put(t,x,y); return; }
  n=(long)(4*(w+h))+8;
  key=malloc(n*sizeof(long));
  if (key==NULL) return;
  step=(2*M_PI)/(double)n;
  for (k=0;k<n;k++) {
    tt=(double)k*step;
    px=rint((double)x+(double)w/2.0+(double)w/2.0*cos(tt));
    py=rint((double)y+(double)h/2.0+(double)h/2.0*sin(tt));
    key[k]=(long)py*70000+(long)px;
  }
  qsort(key,n,sizeof(long),CmpLong);
  for (k=0;k<n;k++) {
    if (k>0 && key[k]==key[k-1]) continue;
    /* the decomposition of np.unique(...) % 70000, // 70000 */
    m=key[k]-FloorDiv(key[k],70000)*70000;
    Put(t,m,FloorDiv(key[k],70000));
  }
  free(key);
}

/* Outline of a rectangle of the given width around corners at fine
   positions (frame of scaled.py) */
static void Frame(Target* t,double x0,double y0,double x1,double y1,
    double width) {
  double hw=width/2.0,v,cx,cy;
  long i,j,i0,i1,j0,j1;
  int outer,inner;

  if (x0>x1) { v=x0; x0=x1; x1=v; }
  if (y0>y1) { v=y0; y0=y1; y1=v; }
  i0=(long)floor(x0-hw)-1; i1=(long)ceil(x1+hw)+1;
  j0=(long)floor(y0-hw)-1; j1=(long)ceil(y1+hw)+1;
  for (j=j0;j<=j1;j++) for (i=i0;i<=i1;i++) {
    cx=(double)i+EPS_X;
    cy=(double)j+EPS_Y;
    outer=cx>x0-hw && cx<x1+hw && cy>y0-hw && cy<y1+hw;
    inner=cx>x0+hw && cx<x1-hw && cy>y0+hw && cy<y1-hw;
    if (outer && !inner) Put(t,i,j);
  }
}

/* Outline of an ellipse (centre, half axes) of the given width (ring of
   scaled.py) */
static void Ring(Target* t,double cx,double cy,double a,double b,
    double width) {
  double hw=width/2.0,dx,dy,dist,aa=fmax(a,1e-9),bb=fmax(b,1e-9);
  long i,j,i0,i1,j0,j1;

  i0=(long)floor(cx-a-hw)-1; i1=(long)ceil(cx+a+hw)+1;
  j0=(long)floor(cy-b-hw)-1; j1=(long)ceil(cy+b+hw)+1;
  for (j=j0;j<=j1;j++) for (i=i0;i<=i1;i++) {
    dx=((double)i-cx)/aa;
    dy=((double)j-cy)/bb;
    dist=fabs(sqrt(dx*dx+dy*dy)-1.0)*fmin(aa,bb);
    if (dist<hw) Put(t,i,j);
  }
}

/* One primitive of a record with the graphics context in t (kind of the
   record: PYR_LINE or PYR_SLINE, PYR_SRECT, PYR_CIRCLE) */
static void Primitive(Target* t,int op,const double* v,int wd,int scaled,
    int f,int dpr,int ss) {
  long x1,y1,x2,y2,wh,hh;
  double e[4],dev;

  if (!scaled) {
    switch (op) {
    case PYR_LINE: case PYR_SLINE:
      x1=I16((long)v[0]); y1=I16((long)v[1]);
      x2=I16((long)v[2]); y2=I16((long)v[3]);
      if (wd==0) ZeroLine(t,x1,y1,x2,y2);
      else WideLine(t,(double)x1,(double)y1,(double)x2,(double)y2,
                    (double)wd);
      break;
    case PYR_SRECT:
      x1=(long)v[0]; y1=(long)v[1]; x2=(long)v[2]; y2=(long)v[3];
      if (x1>x2) { wh=x1; x1=x2; x2=wh; }
      if (y1>y2) { wh=y1; y1=y2; y2=wh; }
      Rectangle(t,I16(x1),I16(y1),C16(x2-x1),C16(y2-y1),wd);
      break;
    case PYR_CIRCLE:
      wh=(long)v[2]; hh=(long)v[3];
      Ellipse(t,I16((long)v[0]-wh),I16((long)v[1]-hh),C16(wh*2-1),
              C16(hh*2-1));
      break;
    }
    return;
  }
  e[0]=v[0]*f-0.5; e[1]=v[1]*f-0.5; e[2]=v[2]*f-0.5; e[3]=v[3]*f-0.5;
  switch (op) {
  case PYR_LINE: case PYR_SLINE:
    dev=wd==0 ? 1 : wd*dpr;
    if (wd==0 && ss==1)
      ZeroLine(t,(long)floor(e[0]+0.5),(long)floor(e[1]+0.5),
               (long)floor(e[2]+0.5),(long)floor(e[3]+0.5));
    else if (WideLine(t,e[0],e[1],e[2],e[3],dev*ss)==0)
      /* a dot where a wide line has no pixel */
      Put(t,(long)floor(e[0]+0.5),(long)floor(e[1]+0.5));
    break;
  case PYR_SRECT:
    Frame(t,e[0],e[1],e[2],e[3],(double)((wd*dpr ? wd*dpr : 1)*ss));
    break;
  case PYR_CIRCLE:
    Ring(t,e[0],e[1],v[2]*f,v[3]*f,(double)((wd*dpr ? wd*dpr : 1)*ss));
    break;
  }
}

/* The records of a view from index k on, in order (those of other views
   and hidden ones left out): a mode record sets the graphics context
   from the table (gc*, index 2*mode+highlight; a mode without a context
   keeps the one before), lines, screen rectangles and ellipses (not
   clipped; ellipses only without squares) are drawn, a clear fills the
   picture with bg. Returns the index of the first text record, or of the
   first clear when stopAtClear (drawn by Python), or n at the end; the
   context (function, colour, width) is kept in *pFunc, *pFg, *pWidth. */
long PyRasterDraw(unsigned int* img,int w,int h,const struct _PyRec* recs,
    long n,long k,int view,int hiddenFlag,int clippedFlag,int hlFlag,
    int squares,int stopAtClear,unsigned int bg,const int* gcValid,
    const int* gcFunc,const unsigned int* gcFg,const int* gcWidth,
    int nModes,int* pFunc,unsigned int* pFg,int* pWidth,int scaled,int f,
    int dpr,int ss) {
  Target t;
  const struct _PyRec* r;
  long m,i;
  int func=*pFunc,width=*pWidth;
  unsigned int fg=*pFg;

  t.img=img; t.w=w; t.h=h;
  for (;k<n;k++) {
    r=recs+k;
    if (r->view!=view || (r->flags&hiddenFlag)) continue;
    switch (r->op) {
    case PYR_MODE:
      m=2L*r->i+((r->flags&hlFlag)!=0);
      if (r->i>=0 && r->i<nModes && gcValid[m]) {
        func=gcFunc[m]; fg=gcFg[m]; width=gcWidth[m];
      }
      break;
    case PYR_TEXT:
      *pFunc=func; *pFg=fg; *pWidth=width;
      return k;
    case PYR_CLEAR:
      if (stopAtClear) {
        *pFunc=func; *pFg=fg; *pWidth=width;
        return k;
      }
      for (i=0;i<(long)w*h;i++) img[i]=OPAQUE|bg;
      break;
    case PYR_CIRCLE:
      if (squares) break;
      /* fall through */
    case PYR_LINE: case PYR_SLINE:
      if (r->flags&clippedFlag) break;
      /* fall through */
    case PYR_SRECT:
      t.color=fg; t.xorMode=func==1;
      Primitive(&t,r->op,r->s,width,scaled,f,dpr,ss);
      break;
    }
  }
  *pFunc=func; *pFg=fg; *pWidth=width;
  return n;
}

/* The lines drawing elements (mode elemMode, not highlighted, the mode set
   by a record of the batch) of a view from index start up to the next
   clear: their screen positions into out (4 values each, at most max
   lines); returns their number. */
long PyRasterElemLines(const struct _PyRec* recs,long n,long start,
    int view,int hiddenFlag,int hlFlag,int elemMode,double* out,long max) {
  const struct _PyRec* r;
  long k,count=0;
  int elem=0;

  for (k=0;k<n;k++) {
    r=recs+k;
    if (r->view!=view || (r->flags&hiddenFlag)) continue;
    if (r->op==PYR_MODE) elem=r->i==elemMode && !(r->flags&hlFlag);
    else if (k>=start && r->op==PYR_CLEAR && k>start) break;
    else if (k>=start && elem && (r->op==PYR_LINE || r->op==PYR_SLINE)) {
      if (count<max) memcpy(out+4*count,r->s,4*sizeof(double));
      count++;
    }
  }
  return count;
}

/* The picture at device resolution: the mean of each ss x ss block and
   channel, rounded half to even (as np.rint), opaque. */
void PyRasterAverage(const unsigned int* src,int w,int h,int ss,
    unsigned int* dst) {
  long x,y,dx,dy,ow=w/ss,oh=h/ss,n=(long)ss*ss;
  unsigned long r,g,b,q,rr,out[3],sum[3];
  unsigned int p;
  int c;

  if (ss==2) {
    /* 2x2: red and blue summed in one word, green in another; v/4
       rounded half to even: v>>2, plus one when (v&3)+((v>>2)&1)>2 */
    for (y=0;y<oh;y++) {
      const unsigned int* r0=src+2*y*w;
      const unsigned int* r1=r0+w;
      unsigned int* o=dst+y*ow;
      for (x=0;x<ow;x++) {
        unsigned int p0=r0[2*x],p1=r0[2*x+1],p2=r1[2*x],p3=r1[2*x+1];
        unsigned int rb=(p0&0xFF00FFu)+(p1&0xFF00FFu)+(p2&0xFF00FFu)+
                        (p3&0xFF00FFu);
        unsigned int gg=((p0>>8)&0xFFu)+((p1>>8)&0xFFu)+((p2>>8)&0xFFu)+
                        ((p3>>8)&0xFFu);
        unsigned int vr=rb>>16,vb=rb&0x3FFu;
        unsigned int cr=(vr>>2)+(((vr&3)+((vr>>2)&1))>2);
        unsigned int cg=(gg>>2)+(((gg&3)+((gg>>2)&1))>2);
        unsigned int cb=(vb>>2)+(((vb&3)+((vb>>2)&1))>2);
        o[x]=OPAQUE|(cr<<16)|(cg<<8)|cb;
      }
    }
    return;
  }
  for (y=0;y<oh;y++) for (x=0;x<ow;x++) {
    r=g=b=0;
    for (dy=0;dy<ss;dy++) for (dx=0;dx<ss;dx++) {
      p=src[(y*ss+dy)*w+x*ss+dx];
      r+=(p>>16)&255; g+=(p>>8)&255; b+=p&255;
    }
    sum[0]=r; sum[1]=g; sum[2]=b;
    for (c=0;c<3;c++) {
      q=sum[c]/n; rr=sum[c]%n;
      out[c]=q+((2*rr>(unsigned long)n) || (2*rr==(unsigned long)n && (q&1)));
    }
    dst[y*ow+x]=OPAQUE|(out[0]<<16)|(out[1]<<8)|out[2];
  }
}
