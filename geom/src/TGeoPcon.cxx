/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
// Author :  Andrei Gheata  - date Thu 31 Jan 2002 01:47:40 PM CET
// TGeoPcon::Contains() implemented by Mihaela Gheata

#include "TROOT.h"

#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TGeoPainter.h"
#include "TGeoTube.h"
#include "TGeoCone.h"
#include "TGeoPcon.h"


/*************************************************************************
 * TGeoPcon - a polycone. It has at least 9 parameters :
 *            - the lower phi limit;
 *            - the range in phi;
 *            - the number of z planes (at least two) where the inner/outer 
 *              radii are changing;
 *            - z coordinate, inner and outer radius for each z plane
 *
 *************************************************************************/
//Begin_Html
/*
<img src="gif/TGeoPcon.gif">
*/
//End_Html

ClassImp(TGeoPcon)

//-----------------------------------------------------------------------------
TGeoPcon::TGeoPcon()
{
// dummy ctor
   SetBit(TGeoShape::kGeoPcon);
   fRmin = 0;
   fRmax = 0;
   fZ    = 0;
}   
//-----------------------------------------------------------------------------
TGeoPcon::TGeoPcon(Double_t phi, Double_t dphi, Int_t nz)
         :TGeoBBox(0, 0, 0)
{
// Default constructor
   SetBit(TGeoShape::kGeoPcon);
   fPhi1 = phi;
   fDphi = dphi;
   fNz   = nz;
   fRmin = new Double_t [nz];
   fRmax = new Double_t [nz];
   fZ    = new Double_t [nz];
   ComputeBBox();
}
//-----------------------------------------------------------------------------
TGeoPcon::TGeoPcon(Double_t *param)
{
// Default constructor in GEANT3 style
// param[0] = phi1
// param[1] = dphi
// param[2] = nz
//
// param[3] = z1
// param[4] = Rmin1
// param[5] = Rmax1
// ...
   SetBit(TGeoShape::kGeoPcon);
   SetDimensions(param);
   ComputeBBox();
}
//-----------------------------------------------------------------------------
TGeoPcon::~TGeoPcon()
{
// destructor
   if (fRmin) {delete[] fRmin; fRmin = 0;}
   if (fRmax) {delete[] fRmax; fRmax = 0;}
   if (fZ)    {delete[] fZ; fZ = 0;}
}
//-----------------------------------------------------------------------------   
void TGeoPcon::ComputeBBox()
{
// compute bounding box of the pcon
   Double_t zmin = TMath::Min(fZ[0], fZ[fNz-1]);
   Double_t zmax = TMath::Max(fZ[0], fZ[fNz-1]);
   // find largest rmax an smallest rmin
   Double_t rmin, rmax;
   rmin = fRmin[TMath::LocMin(fNz, fRmin)];
   rmax = fRmax[TMath::LocMax(fNz, fRmax)];
   Double_t phi1 = fPhi1;
   Double_t phi2 = phi1 + fDphi;
   if (phi2 > 360) phi2-=360;
   
   Double_t xc[4];
   Double_t yc[4];
   xc[0] = rmax*TMath::Cos(phi1*kDegRad);
   yc[0] = rmax*TMath::Sin(phi1*kDegRad);
   xc[1] = rmax*TMath::Cos(phi2*kDegRad);
   yc[1] = rmax*TMath::Sin(phi2*kDegRad);
   xc[2] = rmin*TMath::Cos(phi1*kDegRad);
   yc[2] = rmin*TMath::Sin(phi1*kDegRad);
   xc[3] = rmin*TMath::Cos(phi2*kDegRad);
   yc[3] = rmin*TMath::Sin(phi2*kDegRad);

   Double_t xmin = xc[TMath::LocMin(4, &xc[0])];
   Double_t xmax = xc[TMath::LocMax(4, &xc[0])]; 
   Double_t ymin = yc[TMath::LocMin(4, &yc[0])]; 
   Double_t ymax = yc[TMath::LocMax(4, &yc[0])];

   Double_t ddp = -phi1;
   if (ddp<0) ddp+= 360;
   if (ddp>360) ddp-=360;
   if (ddp<=fDphi) xmax = rmax;
   ddp = 90-phi1;
   if (ddp<0) ddp+= 360;
   if (ddp>360) ddp-=360;
   if (ddp<=fDphi) ymax = rmax;
   ddp = 180-phi1;
   if (ddp<0) ddp+= 360;
   if (ddp>360) ddp-=360;
   if (ddp<=fDphi) xmin = -rmax;
   ddp = 270-phi1;
   if (ddp<0) ddp+= 360;
   if (ddp>360) ddp-=360;
   if (ddp<=fDphi) ymin = -rmax;
   fOrigin[0] = (xmax+xmin)/2;
   fOrigin[1] = (ymax+ymin)/2;
   fOrigin[2] = (zmax+zmin)/2;
   fDX = (xmax-xmin)/2;
   fDY = (ymax-ymin)/2;
   fDZ = (zmax-zmin)/2;
}   
//-----------------------------------------------------------------------------
Bool_t TGeoPcon::Contains(Double_t *point)
{
// test if point is inside this shape
   // check total z range
   if ((point[2]<fZ[0]) || (point[2]>fZ[fNz-1])) return kFALSE;
   // check R squared
   Double_t r2 = point[0]*point[0]+point[1]*point[1];
   
   Int_t izl = 0;
   Int_t izh = fNz-1;
   Int_t izt = (fNz-1)/2;
   while ((izh-izl)>1) {
      if (point[2] > fZ[izt]) izl = izt;     
      else izh = izt;
      izt = (izl+izh)/2;
   }
   // the point is in the section bounded by izl and izh Z planes
   
   // compute Rmin and Rmax and test the value of R squared
   Double_t rmin, rmax;  
   if ((fZ[izl]==fZ[izh]) && (point[2]==fZ[izl])) {
      rmin = TMath::Min(fRmin[izl], fRmin[izh]);
      rmax = TMath::Max(fRmax[izl], fRmax[izh]);
   } else {
      Double_t dz = fZ[izh] - fZ[izl];
      Double_t dz1 = point[2] - fZ[izl];
      rmin = (fRmin[izl]*(dz-dz1)+fRmin[izh]*dz1)/dz;
      rmax = (fRmax[izl]*(dz-dz1)+fRmax[izh]*dz1)/dz;
   }
   if ((r2<rmin*rmin) || (r2>rmax*rmax)) return kFALSE;
   // now check phi 
   Double_t phi = fPhi1+0.5*fDphi;
   if ((point[1]!=0.0) || (point[0] != 0.0))
      phi = TMath::ATan2(point[1], point[0]) * kRadDeg;
   if (phi < fPhi1) phi+=360.0;
   if ((phi<fPhi1) || ((phi-fPhi1)>fDphi)) return kFALSE;
   return kTRUE;
}
//-----------------------------------------------------------------------------
Int_t TGeoPcon::DistancetoPrimitive(Int_t px, Int_t py)
{
// compute closest distance from point px,py to each corner
   Int_t n = TGeoManager::kGeoDefaultNsegments+1;
   const Int_t numPoints = 2*n*fNz;
   return ShapeDistancetoPrimitive(numPoints, px, py);
}
//-----------------------------------------------------------------------------
Double_t TGeoPcon::DistToOut(Double_t *point, Double_t *dir, Int_t iact, Double_t step, Double_t *safe)
{
// compute distance from inside point to surface of the polycone
   Double_t saf[4];
   Double_t snxt = kBig;
//   Double_t r2 = point[0]*point[0] + point[1]*point[1];
//   Double_t r = TMath::Sqrt(r2);
   
   Double_t zmin = fZ[0];
   Double_t zmax = fZ[fNz-1];
   Double_t safz1 = point[2]-zmin;
   Double_t safz2 = zmax-point[2];
   saf[0] = TMath::Min(safz1, safz2);
   // determine which z segment contains the point
   Int_t ipl = TMath::BinarySearch(fNz, fZ, point[2]);
   if (ipl==(fNz-1)) {
      // point on end z plane
      if (safe) *safe=0;
      return 0;
   }
   Double_t dz = 0.5*(fZ[ipl+1]-fZ[ipl]);
   // determine if the current segment is a tube or a cone
   Bool_t intub = kTRUE;
   if (fRmin[ipl]!=fRmin[ipl+1]) intub=kFALSE;
   else if (fRmax[ipl]!=fRmax[ipl+1]) intub=kFALSE;
   // determine phi segmentation
   Bool_t inphi=kTRUE;
   if (fDphi==360) inphi=kFALSE;     
   Double_t point_new[3];
   memcpy(&point_new[0], &point[0], 2*sizeof(Double_t));
   // new point in reference system of the current segment
   point_new[2] = point[2]-0.5*(fZ[ipl]+fZ[ipl+1]);
   
   Double_t phi1 = fPhi1;
   Double_t phi2 = fPhi1+fDphi;
   if (intub) {
      if (inphi) snxt=TGeoTubeSeg::DistToOutS(&point_new[0], dir, iact, step, &saf[1], 
                                              fRmin[ipl], fRmax[ipl],dz, phi1, phi2); 
      else snxt=TGeoTube::DistToOutS(&point_new[0], dir, iact, step, &saf[1], fRmin[ipl], fRmax[ipl],dz);
   } else {
      if (inphi) snxt=TGeoConeSeg::DistToOutS(&point_new[0], dir, iact, step, &saf[1],
                                 dz, fRmin[ipl], fRmax[ipl], fRmin[ipl+1], fRmax[ipl+1], phi1,phi2);
      else snxt=TGeoCone::DistToOutS(&point_new[0], dir, iact, step, &saf[1],
                                 dz, fRmin[ipl], fRmax[ipl], fRmin[ipl+1], fRmax[ipl+1]);
   }                              

   if (iact<3 && safe) {
      *safe = TMath::Min(saf[0],saf[1]);
      if (iact==0) return kBig;
      if ((iact==1) && (*safe>step)) return step;
   }
   return snxt;
}
//-----------------------------------------------------------------------------
Double_t TGeoPcon::DistToSegZ(Double_t *point, Double_t *dir, Int_t &iz, Double_t c1, Double_t s1, 
                              Double_t c2, Double_t s2, Double_t cfio, Double_t sfio, Double_t cdfi)
{
// compute distance to a pcon Z slice. Segment iz must be valid
   Double_t zmin=fZ[iz];
   Double_t zmax=fZ[iz+1];
   if (zmin==zmax) {
      if (dir[2]==0) return kBig;
      Int_t istep=(dir[2]>0)?1:-1;
      iz+=istep;
      if (iz<0 || iz>(fNz-2)) return kBig;
      return DistToSegZ(point,dir,iz,c1,s1,c2,s2,cfio,sfio,cdfi);
   }
   Double_t dz=0.5*(zmax-zmin);
   Double_t local[3];
   memcpy(&local[0], point, 3*sizeof(Double_t));
   local[2]=point[2]-0.5*(zmin+zmax);
   Double_t snxt;
   Double_t rmin1=fRmin[iz];
   Double_t rmax1=fRmax[iz];
   Double_t rmin2=fRmin[iz+1];
   Double_t rmax2=fRmax[iz+1];
   Bool_t is_seg=(fDphi==360)?kFALSE:kTRUE;
   Double_t r2=point[0]*point[0]+point[1]*point[1];
   
   if ((rmin1==rmin2) && (rmax1==rmax2)) {
      if (!is_seg) snxt=TGeoTube::DistToInS(&local[0], dir, rmin1, rmax1, dz);
      else snxt=TGeoTubeSeg::DistToInS(&local[0], dir, rmin1, rmax1, dz, c1, s1, c2, s2, cfio, sfio, cdfi);
   } else {  
      Double_t ro1=0.5*(rmin1+rmin2);
      Double_t tg1=0.5*(rmin2-rmin1)/dz;
      Double_t cr1=1./TMath::Sqrt(1.0+tg1*tg1);
      Double_t zv1=kBig;
      if (rmin1!=rmin2) zv1=-ro1/tg1;
      Double_t ro2=0.5*(rmax1+rmax2);
      Double_t tg2=0.5*(rmax2-rmax1)/dz;
      Double_t cr2=1./TMath::Sqrt(1.0+tg2*tg2);
      Double_t zv2=kBig;
      if (rmax1!=rmax2) zv2=-ro2/tg2;
      Double_t rin=TMath::Abs(tg1*local[2]+ro1);
      Double_t rout=TMath::Abs(tg2*local[2]+ro2);
   
      if (!is_seg) snxt=TGeoCone::DistToInS(&local[0],dir,rmin1, rmax1, rmin2, rmax2, dz,
                                              ro1,tg1,cr1,zv1,ro2,tg2,cr2,zv2,r2,rin,rout);
      else snxt=TGeoConeSeg::DistToInS(&local[0],dir,rmin1, rmax1, rmin2, rmax2, dz,
                           ro1,tg1,cr1,zv1,ro2,tg2,cr2,zv2,r2,rin,rout,c1, s1, c2, s2, cfio,sfio,cdfi);
   }
   if (snxt<1E20) return snxt;
   // check next segment
   if (dir[2]==0) return kBig;
   Int_t istep=(dir[2]>0)?1:-1;
   iz+=istep;
   if (iz<0 || iz>(fNz-2)) return kBig;
   return DistToSegZ(point,dir,iz,c1,s1,c2,s2,cfio,sfio,cdfi);
}      
//-----------------------------------------------------------------------------
Double_t TGeoPcon::DistToIn(Double_t *point, Double_t *dir, Int_t iact, Double_t step, Double_t *safe)
{
// compute distance from outside point to surface of the tube
   Bool_t cross = kTRUE;
   // check if ray intersect outscribed cylinder
   if ((point[2]<fZ[0]) && (dir[2]<=0)) {
      if (iact==3) return kBig;
      cross = kFALSE;
   }
   if (cross) {
      if ((point[2]>fZ[fNz-1]) && (dir[2]>=0)) {
         if (iact==3) return kBig;
         cross = kFALSE;
      }
   }   

   Double_t r2 = point[0]*point[0]+point[1]*point[1];
   Double_t radmax=0;
   radmax=fRmax[TMath::LocMax(fNz, fRmax)];
   if (cross) {
      if (r2>(radmax*radmax)) {
         Double_t rpr=point[0]*dir[0]+point[1]*dir[1];
         if (rpr>TMath::Sqrt(r2-radmax*radmax)) {
            if (iact==3) return kBig;
            cross = kFALSE;
         }
      }
   }
   Double_t saf[6];
   Double_t r = TMath::Sqrt(r2); 

   // find in which Z segment we are
   Int_t ipl = TMath::BinarySearch(fNz, fZ, point[2]);
   Int_t ifirst = ipl;
   if (ifirst<0) {
      ifirst=0;
      saf[0]=fZ[0]-point[2];
      saf[1]=-kBig;
   } else {
      if (ifirst>=(fNz-1)) {
         ifirst=fNz-2;
         saf[0]=-kBig;
         saf[1]=point[2]-fZ[fNz-1];
      } else {
         saf[0]=point[2]-fZ[ifirst];
         saf[1]=fZ[ifirst+1]-point[2];
      }
   }
   // find if point is in the phi gap
   Double_t phi=0;
   Double_t phi1=0;
   Double_t phi2=0;
   Double_t safp1=kBig, safp2=kBig;
   Double_t c1=0., s1=0., c2=0., s2=0., cfio=0., sfio=0., cdfi=0.;
   Bool_t inhole=kFALSE, outhole=kFALSE, inphi=kFALSE;
   if (fDphi!=360) {
      phi1=fPhi1*kDegRad;
      phi2=(fPhi1+fDphi)*kDegRad;
      phi=TMath::ATan2(point[1], point[0]);
      if (phi<phi1) phi+=2.*TMath::Pi();
      safp1 = -r*TMath::Sin(phi-phi1);
      safp2 = -r*TMath::Sin(phi2-phi);
      if ((phi-phi1)>(phi2-phi1)) {
         // point in gap
         saf[4]=safp1;                             
         saf[5]=safp2;
      } else {
         saf[4]=saf[5]=-kBig;
         inphi=kTRUE;
      }
      c1=TMath::Cos(phi1);
      s1=TMath::Sin(phi1);
      c2=TMath::Cos(phi2);
      s2=TMath::Sin(phi2);
      Double_t fio=0.5*(phi1+phi2);
      cfio=TMath::Cos(fio);
      sfio=TMath::Sin(fio);
      cdfi=TMath::Cos(0.5*(phi2-phi1));
   } else {
      saf[4]=saf[5]=-kBig;
      inphi=kTRUE;
   }
//   if (inphi1) printf("INPHI\n");
//   printf("phi1=%f phi2=%f phi=%f\n", fPhi1, fPhi1+fDphi, phi);
   Double_t dz=fZ[ifirst+1]-fZ[ifirst];
   Double_t dzrat=(point[2]-fZ[ifirst])/dz;
   Double_t rmin=fRmin[ifirst]+(fRmin[ifirst+1]-fRmin[ifirst])*dzrat;
   Double_t rmax=fRmax[ifirst]+(fRmax[ifirst+1]-fRmax[ifirst])*dzrat;
   if ((rmin>0) && (r<rmin)) inhole=kTRUE;
   if (r>rmax) outhole=kTRUE;
   Double_t tin=(fRmin[ifirst+1]-fRmin[ifirst])/dz;
   Double_t cin=1./TMath::Sqrt(1.0+tin*tin);
   Double_t tou=(fRmax[ifirst+1]-fRmax[ifirst])/dz;
   Double_t cou=1./TMath::Sqrt(1.0+tou*tou);
   if (inphi) {
      saf[2] = (inhole)?((rmin-r)*cin):-kBig;
      saf[3] = (outhole)?((r-rmax)*cou):-kBig;
   } else {
      saf[2]=saf[3]=-kBig;
   }
   if ((iact<3) && safe) {
      *safe = saf[TMath::LocMax(6, &saf[0])];
      if ((iact==1) && (*safe>step)) return step;
      if (iact==0) return kBig;
   }
   // compute distance to boundary
   if (!cross) return kBig;
   return DistToSegZ(point,dir,ifirst, c1,s1,c2,s2,cfio,sfio,cdfi);

/*
   Double_t snxt=0.;
   // if outside, propagate to first plane
   
   // first find out intersection axial plane normal to dir
   Double_t calf = 0;
   if (r!=0) calf=(point[0]*dir[0]+point[1]*dir[1])/r;
//   Double_t s=kBig;
   cross = kFALSE;
   Double_t s=0;
   if (calf<0) {
      // ray is getting closer
      Double_t dcl=-r*calf;
      if (dir[2]!=0) {
         s=dcl/TMath::Sqrt(1.0-dir[2]*dir[2]);
         cross=kTRUE;
      }
   }
   Double_t xi=0,yi=0,zi=0;
   Int_t icross=-1;
   Double_t rcross=r;
   if (cross) {
      xi = point[0]+s*dir[0];
      yi = point[1]+s*dir[1];
      zi = point[2]+s*dir[2];
      rcross=xi*xi+yi*yi;
      icross=TMath::BinarySearch(fNz, fZ, zi);
      if (icross>=(fNz-1)) icross = -1;
      if (icross>0) {
         dz=fZ[icross+1]-fZ[icross];
         dzrat=(point[2]-fZ[icross])/dz;
         rmax=fRmax[icross]+(fRmax[icross+1]-fRmax[icross])*dzrat;
         if (rcross>(rmax*rmax)) cross=kFALSE;  
      }
//      printf("cross axial plane : rmin=%f ipl=%i xi=%f yi=%f zi=%f\n", TMath::Sqrt(rcross), icross, xi,yi,zi);
   }
   // find intersection with phi planes (if any)
   Int_t iph=-1;
   Double_t rph=kBig;
   Double_t rminph=0;
   Double_t rmaxph=kBig;
   Bool_t crossph=kFALSE;
   if (fDphi!=360) {
      Double_t phi1=fPhi1*kDegRad;
      Double_t phi2=(fPhi1+fDphi)*kDegRad;
      Double_t cos1=TMath::Cos(phi1);
      Double_t sin1=TMath::Sin(phi1);
      Double_t cos2=TMath::Cos(phi2);
      Double_t sin2=TMath::Sin(phi2);
      calf=dir[0]*sin1-dir[1]*cos1;
      Double_t s1=-safp1/calf;
      calf=-dir[0]*sin2+dir[1]*cos2;
      Double_t s2=-safp2/calf;
      s=-kBig;
      Bool_t cross1=kFALSE, cross2=kFALSE;
      if (s1>=0) {
         xi=point[0]+s1*dir[0];
         yi=point[1]+s1*dir[1];
         if ((xi*cos1+yi*sin1)>0) cross1=kTRUE;
      }   
      if (s2>=0) {
         xi=point[0]+s2*dir[0];
         yi=point[1]+s2*dir[1];
         if ((xi*cos2+yi*sin2)>0) cross2=kTRUE;
      }
      if (cross1) {
         if (cross2) s=TMath::Min(s1,s2);
         else        s=s1;
      } else {
         if (cross2) s=s2;
      }        
      // propagate until cross phi
      if (s>0) {
         crossph=kTRUE;
         xi = point[0]+s*dir[0];
         yi = point[1]+s*dir[1];
         zi = point[2]+s*dir[2];
         rph=xi*xi+yi*yi;
         iph=TMath::BinarySearch(fNz, fZ, zi);
         if (iph>=(fNz-1)) iph = -1;
         if (iph>0) {
            dz=fZ[iph+1]-fZ[iph];
            dzrat=(point[2]-fZ[iph])/dz;
            rminph=fRmin[iph]+(fRmin[iph+1]-fRmin[iph])*dzrat;
            rmaxph=fRmax[iph]+(fRmax[iph+1]-fRmax[iph])*dzrat;
//            printf("CROSSPHI\n");
         }
      }
   }      
   // propagate to next Z plane if point is outside Z range   
   Double_t dpl=-kBig, phipl=phi, rpl=r;
   Double_t rminpl=0, rmaxpl=0;
   if (down) {
      dpl = ((fZ[0]-point[2])/dir[2]);
      rminpl=fRmin[0];
      rmaxpl=fRmax[0];
   } else {
      if (up) {         
         dpl = ((point[2]-fZ[fNz-2])/dir[2]);
         rminpl=fRmin[fNz-2];
         rmaxpl=fRmax[fNz-2];
      }
   }
   if (up || down) {
      xi=point[0]+dpl*dir[0];
      yi=point[1]+dpl*dir[1];
      rpl=xi*xi+yi*yi;
      if ((rminpl>0) && (rpl<rminpl*rminpl)) {
         if (up) inhole2=kTRUE;
         else inhole1=kTRUE;
      }   
      if (rpl>rmaxpl*rmaxpl) {
         if (up) outhole2=kTRUE;
         else outhole1=kTRUE;
      }   
      phipl=TMath::ATan2(yi,xi)*kRadDeg;
      if (phipl<fPhi1) phipl+=360;
      if ((phipl-fPhi1)<fDphi) {
         if (up) inphi2=kTRUE;
         else inphi1=kTRUE;
      }   
   }
    
   // now find intersections with Z planes
   Bool_t is_crossing=kFALSE;
   Int_t icrt=ifirst;
   
   while (icrt>=0 && icrt<(fNz-1)) {
      if (dir[2]==0) {
         is_crossing=kTRUE;
         break;
      }   
      inext=ifirst+istep;
      dpl = (fZ[inext]-point[2])/dir[2];
      if (dpl>0) {
         xi=point[0]+dpl*dir[0];
         yi=point[1]+dpl*dir[1];
         rpl=TMath::Sqrt(xi*xi+yi*yi);
         if ((fRmin[inext]>0) && (rpl<fRmin[inext])) {
            if (dir[2]>0) inhole2=kTRUE;
            else inhole1=kTRUE;
         }   
         if (rpl>fRmax[icrt]) {
            if (dir[2]>0) outhole2=kTRUE;
            else outhole1=kTRUE;
         }   
         phipl=TMath::ATan2(yi,xi)*kRadDeg;
         if (phipl<fPhi1) phipl+=360;
         if ((phipl-fPhi1)<fDphi) {
            if (dir[2]>0) inphi2=kTRUE;
            else inphi1=kTRUE;
         }
         printf("           phi1=%f phi2=%f\n", fPhi1, fPhi1+fDphi); 
         printf("plane %i : z=%f rmin=%f rmax=%f r=%f phi=%f\n", icrt, fZ[icrt],
                fRmin[icrt], fRmax[icrt], rpl, phipl);
      }   
      // check if ray actually crosses current segment
      printf("Checking segment %i\n", icrt);
      printf("point : %f %f %f\n", point[0], point[1], point[2]);
      printf("dir   : %f %f %f\n", dir[0], dir[1], dir[2]);
      if (up) printf("---UP\n");
      if (down) printf("---DOWN\n");
      printf(" point1 :\n");
      if (inhole1) printf("---inhole");
      if (outhole1) printf("---outhole");
      if (inphi1) printf("---inphi, iph=%i rph=%f\n", iph, rph);
      printf(" point2 :\n");
      if (inhole2) printf("---inhole");
      if (outhole2) printf("---outhole");
      if (inphi2) printf("---inphi, iph=%i rph=%f\n", iph, rph);

      if (inhole1) {
         if (!inhole2) {
            if (inphi2) is_crossing=kTRUE;
            else {
               if (inphi1) {
                  if (crossph) {
                     if ((iph==icrt) && (rph>rminph)) is_crossing=kTRUE;
                  }
               }
            }
         }
      } else {
         if (inhole2) {
            if (inphi1) is_crossing=kTRUE;
            else {
               if (inphi2) {
                  if (crossph) {
                     if ((iph==icrt) && (rph>rminph)) is_crossing=kTRUE;
                  }
               }
            }
         } else {
            if (outhole1) {
               if (outhole2) {
                  if (cross && (icross==icrt)) {
                     if ((iph==icrt) && (rph<rmaxph)) is_crossing=kTRUE;
                  }   
               } else {
                  if (inphi1) {
                     if ((iph==icrt) && (rph<rmaxph)) is_crossing=kTRUE;
                  }   
               }
            } else {
               if (outhole2) {
                  if (inphi2) {
                     if ((iph==icrt) && (rph<rmaxph)) is_crossing=kTRUE;
                  }
               }
            }         
         }
      }
      if (is_crossing) {
         printf("CROSSING %i\n", icrt);
         break;
      }   
      icrt+=istep;
      return kBig;               
   }   
   return kBig;
*/
}
//-----------------------------------------------------------------------------
Double_t TGeoPcon::DistToSurf(Double_t *point, Double_t *dir)
{
// computes the distance to next surface of the sphere along a ray
// starting from given point to the given direction.
   return kBig;
}
//-----------------------------------------------------------------------------
void TGeoPcon::DefineSection(Int_t snum, Double_t z, Double_t rmin, Double_t rmax)
{
// defines z position of a section plane, rmin and rmax at this z.
   if ((snum<0) || (snum>=fNz)) return;
   fZ[snum]    = z;
   fRmin[snum] = rmin;
   fRmax[snum] = rmax;
   if (rmin>rmax) {
      Warning("DefineSection", "invalid rmin/rmax");
      printf("rmin=%f rmax=%f\n", rmin, rmax);
   }
   if (snum==(fNz-1)) ComputeBBox();
}
//-----------------------------------------------------------------------------
Int_t TGeoPcon::GetNsegments()
{
   return TGeoManager::kGeoDefaultNsegments;
}
//-----------------------------------------------------------------------------
void TGeoPcon::Draw(Option_t *option)
{
// draw this shape according to option
}
//-----------------------------------------------------------------------------
void TGeoPcon::InspectShape()
{
// print shape parameters
   printf("*** TGeoPcon parameters ***\n");
   printf("    Nz    = %i\n", fNz);
   printf("    phi1  = %11.5f\n", fPhi1);
   printf("    dphi  = %11.5f\n", fDphi);
   for (Int_t ipl=0; ipl<fNz; ipl++)
      printf("     plane %i: z=%11.5f Rmin=%11.5f Rmax=%11.5f\n", ipl, fZ[ipl], fRmin[ipl], fRmax[ipl]);
   TGeoBBox::InspectShape();
}
//-----------------------------------------------------------------------------
void TGeoPcon::Paint(Option_t *option)
{
// paint this shape according to option
   TGeoPainter *painter = (TGeoPainter*)gGeoManager->GetMakeDefPainter();
   if (!painter) return;
   TGeoVolume *vol = gGeoManager->GetCurrentVolume();
   if (vol->GetShape() != (TGeoShape*)this) return;
   painter->PaintPcon(vol, option);
}
//-----------------------------------------------------------------------------
void TGeoPcon::NextCrossing(TGeoParamCurve *c, Double_t *point)
{
// computes next intersection point of curve c with this shape
}
//-----------------------------------------------------------------------------
Double_t TGeoPcon::Safety(Double_t *point, Double_t *spoint, Option_t *option)
{
// computes the closest distance from given point to this shape, according
// to option. The matching point on the shape is stored in spoint.
   return 0.0;
}
//-----------------------------------------------------------------------------
void TGeoPcon::SetDimensions(Double_t *param)
{
   fPhi1    = param[0];
   fDphi    = param[1];
   fNz      = (Int_t)param[2];
   if (!fRmin) fRmin = new Double_t [fNz];
   if (!fRmax) fRmax = new Double_t [fNz];
   if (!fZ)    fZ    = new Double_t [fNz];
   for (Int_t i=0; i<fNz; i++) 
      DefineSection(i, param[3+3*i], param[4+3*i], param[5+3*i]);
}   
//-----------------------------------------------------------------------------
void TGeoPcon::SetPoints(Double_t *buff) const
{
// create polycone mesh points
    Double_t phi, dphi;
    Int_t n = TGeoManager::kGeoDefaultNsegments + 1;
    dphi = fDphi/(n-1);
    Int_t i, j;
    Int_t indx = 0;

    if (buff) {
        for (i = 0; i < fNz; i++)
        {
            for (j = 0; j < n; j++)
            {
                phi = (fPhi1+j*dphi)*kDegRad;
                buff[indx++] = fRmin[i] * TMath::Cos(phi);
                buff[indx++] = fRmin[i] * TMath::Sin(phi);
                buff[indx++] = fZ[i];
            }
            for (j = 0; j < n; j++)
            {
                phi = (fPhi1+j*dphi)*kDegRad;
                buff[indx++] = fRmax[i] * TMath::Cos(phi);
                buff[indx++] = fRmax[i] * TMath::Sin(phi);
                buff[indx++] = fZ[i];
            }
        }
    }
}
//-----------------------------------------------------------------------------
void TGeoPcon::SetPoints(Float_t *buff) const
{
// create polycone mesh points
    Double_t phi, dphi;
    Int_t n = TGeoManager::kGeoDefaultNsegments + 1;
    dphi = fDphi/(n-1);
    Int_t i, j;
    Int_t indx = 0;

    if (buff) {
        for (i = 0; i < fNz; i++)
        {
            for (j = 0; j < n; j++)
            {
                phi = (fPhi1+j*dphi)*kDegRad;
                buff[indx++] = fRmin[i] * TMath::Cos(phi);
                buff[indx++] = fRmin[i] * TMath::Sin(phi);
                buff[indx++] = fZ[i];
            }
            for (j = 0; j < n; j++)
            {
                phi = (fPhi1+j*dphi)*kDegRad;
                buff[indx++] = fRmax[i] * TMath::Cos(phi);
                buff[indx++] = fRmax[i] * TMath::Sin(phi);
                buff[indx++] = fZ[i];
            }
        }
    }
}
//-----------------------------------------------------------------------------
void TGeoPcon::Sizeof3D() const
{
// fill size of this 3-D object
    Int_t n;

    n = TGeoManager::kGeoDefaultNsegments+1;

    gSize3D.numPoints += fNz*2*n;
    gSize3D.numSegs   += 4*(fNz*n-1+(fDphi == 360));
    gSize3D.numPolys  += 2*(fNz*n-1+(fDphi == 360));
}

