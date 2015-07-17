// Include files 
#include <iostream>

#include "TROOT.h"
#include "TVirtualFitter.h"
#include "TMinuit.h"
#include "TMatrixD.h"
#include "TGraph2DErrors.h"
#include "TVectorD.h"

#include "Clipboard.h"
#include "TestBeamTransform.h"
#include "TestBeamTrack.h"

//-----------------------------------------------------------------------------
// Implementation file for class : TestBeamTrackFitter
//
//-----------------------------------------------------------------------------

TestBeamTrack::TestBeamTrack(TestBeamProtoTrack* p, Parameters* par) : 
    TestBeamObject(), 
    m_prototrack(0),
    m_trigger(0),
    m_hastrigger(false),
    m_deleteProto(false),
    m_deleteTrigger(false),
    parameters(0),
    m_debug(false) {

  parameters = par;
  m_chi2 = 0.;
  m_ndof = 0;
  m_prob = 0.;
  m_covMatrix.ResizeTo(TMatrixD(4,4));

  m_alignmentclusters = new TestBeamClusters();
  m_vetraalignmentclusters = new VetraClusters();
  m_fei4alignmentclusters = new FEI4Clusters();
  m_scifialignmentclusters = new SciFiClusters();

  m_prototrack = p;
  m_tdctimestamp = 0;
  m_toatimestamp = 0;

  m_errorToT = 0.004;
  m_errorToA = 0.016;

} 

TestBeamTrack::TestBeamTrack(TestBeamTrack& t, bool action) : 
    TestBeamObject(), 
    m_prototrack(0),
    m_trigger(0),
    m_hastrigger(false),
    m_deleteProto(false), 
    m_deleteTrigger(false),
    m_debug(false) {

  if (action == COPY) return;
  parameters = t.parameters;
  m_chi2 = t.chi2();
  m_ndof = t.ndof();
  m_prob = t.probability();
  m_covMatrix.ResizeTo(t.covMatrix());
  m_covMatrix = t.covMatrix();
  m_tdctimestamp = t.tdctimestamp();
  m_toatimestamp = t.toatimestamp();
  m_state = ROOT::Math::XYZPoint(t.firstState()->X(), 
                                 t.firstState()->Y(), 
                                 t.firstState()->Z());
  m_dir = ROOT::Math::XYZVector(t.direction()->X(),
                                t.direction()->Y(),
                                t.direction()->Z());

  m_prototrack = new TestBeamProtoTrack(*t.protoTrack(), CLONE);
  if (t.hastrigger()) {
    m_trigger = new TDCTrigger((*t.tdcTrigger()).timeAfterShutterOpen(), 
                               (*t.tdcTrigger()).coarseTimeWord(), 
                               (*t.tdcTrigger()).fineTimeWord(), 
                               (*t.tdcTrigger()).syncDelay());
  }
  m_hastrigger = t.hastrigger();
  m_deleteTrigger = true;
  m_deleteProto = true;

  m_alignmentclusters = new TestBeamClusters();
  m_vetraalignmentclusters = new VetraClusters();
  m_fei4alignmentclusters = new FEI4Clusters();
  m_scifialignmentclusters = new SciFiClusters();

  TestBeamClusters::iterator itc;
  for (itc = t.alignmentclusters()->begin(); 
       itc != t.alignmentclusters()->end(); ++itc) {
    TestBeamCluster* copiedCluster = new TestBeamCluster(**itc, CLONE);
    m_alignmentclusters->push_back(copiedCluster);
  }

  VetraClusters::iterator itv;
  for (itv = t.vetraalignmentclusters()->begin(); 
       itv != t.vetraalignmentclusters()->end(); ++itv) {
    VetraCluster* copiedCluster = new VetraCluster(**itv, CLONE);
    m_vetraalignmentclusters->push_back(copiedCluster);
  }

  FEI4Clusters::iterator itf;
  for (itf = t.fei4alignmentclusters()->begin(); 
       itf != t.fei4alignmentclusters()->end(); ++itf) {
    FEI4Cluster *copiedCluster = new FEI4Cluster(**itf, CLONE);
    m_fei4alignmentclusters->push_back(copiedCluster);
  }

  SciFiClusters::iterator its;
  for (its = t.scifialignmentclusters()->begin(); 
       its != t.scifialignmentclusters()->end(); ++its) {
    SciFiCluster *copiedCluster = new SciFiCluster(**its, CLONE);
    m_scifialignmentclusters->push_back(copiedCluster);
  }

  m_errorToT = t.m_errorToT;
  m_errorToA = t.m_errorToA;

}

TestBeamTrack::~TestBeamTrack() {

  if (m_deleteProto && m_prototrack != 0) delete m_prototrack;
  if (m_deleteTrigger && m_trigger != 0) delete m_trigger;

  m_alignmentclusters->clear();
  delete m_alignmentclusters;

  m_vetraalignmentclusters->clear();
  delete m_vetraalignmentclusters;

  m_fei4alignmentclusters->clear();
  delete m_fei4alignmentclusters;

  m_scifialignmentclusters->clear();
  delete m_scifialignmentclusters;

}

void TestBeamTrack::fit() {

  if (parameters->useMinuitFit) {
    minuitfit();
    return;
  }

  // Straight line fit as implemented by Marco
  double vecx[2] = {0., 0.};
  double vecy[2] = {0., 0.};
  double matx[2][2] = {{0., 0.}, {0., 0.}};
  double maty[2][2] = {{0., 0.}, {0., 0.}};

  std::vector<TestBeamCluster*>* clusters = m_prototrack->clusters(); 
  std::vector<TestBeamCluster*>::iterator it;
  int nd(clusters->size());
  for (it = clusters->begin(); it != clusters->end(); ++it) {
    if ((*it) == 0) continue;
    // Skip planes which are excluded from the track fit.
    if (parameters->excludedFromTrackFit[(*it)->detectorId()]) {
      --nd; 
      continue;
    }
    const double x = (*it)->globalX();
    const double y = (*it)->globalY();
    const double z = (*it)->globalZ();
    // By default, assume that the plane is in ToT mode.
    double error = m_errorToT;
    if (parameters->trackFitError.count((*it)->detectorId()) > 0) {
      // User defined error
      error = parameters->trackFitError[(*it)->detectorId()];
    } else if (parameters->toa[(*it)->detectorId()]) {
      // ToA plane
      error = m_errorToA;
    }
    const double wx = 1. / (error * error);
    const double wy = wx;
    vecx[0] += wx * x;
    vecx[1] += wx * x * z;
    vecy[0] += wy * y;
    vecy[1] += wy * y * z;
    matx[0][0] += wx;
    matx[1][0] += wx * z;
    matx[1][1] += wx * z * z;
    maty[0][0] += wy;
    maty[1][0] += wy * z;
    maty[1][1] += wy * z * z;
  }

  // Invert the matrix and compute track parameters.
  double detx = matx[0][0] * matx[1][1] - matx[1][0] * matx[1][0];
  double dety = maty[0][0] * maty[1][1] - maty[1][0] * maty[1][0];
  // Check for singularities.
  if (detx == 0. || dety == 0.) return;

  double slopex = (vecx[1] * matx[0][0] - vecx[0] * matx[1][0]) / detx;
  double slopey = (vecy[1] * maty[0][0] - vecy[0] * maty[1][0]) / dety;

  double interceptx = (vecx[0] * matx[1][1] - vecx[1] * matx[1][0]) / detx;
  double intercepty = (vecy[0] * maty[1][1] - vecy[1] * maty[1][0]) / dety;

  firstState(interceptx, intercepty, 0);
  direction(slopex, slopey, 1);

  // Compute chi2.
  m_chi2 = 0.;
  for (it = clusters->begin(); it != clusters->end(); ++it) {
    const float x = (*it)->globalX();
    const float y = (*it)->globalY();
    const float z = (*it)->globalZ();
    // Skip planes which are excluded from the track fit.
    if (parameters->excludedFromTrackFit[(*it)->detectorId()]) continue;
    // By default, assume that the plane is in ToT mode.
    double error = m_errorToT;
    if (parameters->trackFitError.count((*it)->detectorId()) > 0) {
      // User defined error
      error = parameters->trackFitError[(*it)->detectorId()];
    } else if (parameters->toa[(*it)->detectorId()]) {
      // ToA plane
      error = m_errorToA;
    }
    const double wx = error;
    const double wy = error;

    const double dz = fabs(z - firstState()->Z());
    const float xfit = firstState()->X() + dz * slopeXZ();
    const float yfit = firstState()->Y() + dz * slopeYZ();
    const double dx = x - xfit;
    const double dy = y - yfit;
    m_chi2 += (dx * dx) / (wx * wx) + (dy * dy) / (wy * wy);
  }
  m_ndof = 2 * nd - 4;
  prob();

}

void SumDistance2(int&, double*, double& sum, double* par, int) { 

  // function to be minimized 
  // the TGraph must be a global variable
  TGraph2DErrors* gr = dynamic_cast<TGraph2DErrors*>((TVirtualFitter::GetFitter())->GetObjectFit());
  assert(gr != 0);
  double* x = gr->GetX();
  double* y = gr->GetY();
  double* z = gr->GetZ();
  double* ex = gr->GetEX();
  double* ey = gr->GetEY();
  const int npoints = gr->GetN();
  sum = 0;
  XYZVector x0(par[0], par[2], 0.); 
  XYZVector x1(par[0] + par[1], par[2] + par[3], 1. ); 
  XYZVector u = (x1 - x0).Unit(); 
  for (int i = 0; i < npoints; ++i) {
    // calculate distance line-point 
    // distance line point is D = | (xp - x0) cross ux | 
    // where ux is direction of line and x0 is a point in the line (like t = 0) 
    XYZVector xp(x[i], y[i], z[i]);
    XYZVector res = ((xp - x0).Cross(u));
    const double xres = res.X();
    const double yres = res.Y();
    const double d = (xres * xres) / (ex[i] * ex[i]) + 
                     (yres * yres) / (ey[i] * ey[i]);
    sum += d;
    const bool debug = false;
    if (debug) {
      std::cout << "point " << i << "\t" 
                << x[i] << "\t" 
                << y[i] << "\t" 
                << z[i] << "\t" 
                << std::sqrt(d) << std::endl;
    }
  }

}

void TestBeamTrack::minuitfit() {
  // Implementation of TMinuit fit to track (using TGraph2DErrors).
  
  std::vector<TestBeamCluster*>* clusters = m_prototrack->clusters();
  int nd = clusters->size();
  std::vector<TestBeamCluster*>::iterator it;
  for (it = clusters->begin(); it != clusters->end(); ++it) {
    // Skip planes which are excluded from the track fit.
    if (parameters->excludedFromTrackFit[(*it)->detectorId()]) --nd;
  }

  TGraph2DErrors* gr = new TGraph2DErrors(nd);
  //std::cout << "Number of points, nd=" << nd << " size =" << clusters->size() << std::endl;

  int k(0);
  for (it = clusters->begin(); it != clusters->end(); ++it) {
    if ((*it) == 0) continue;
    // Skip planes which are excluded from the track fit.
    if (parameters->excludedFromTrackFit[(*it)->detectorId()]) continue;
    // By default, assume that the plane is in ToT mode.
    double xyerror = m_errorToT;
    //std::cout << "k=" << k << " and number of points is nd=" << nd << std::endl;
    if (parameters->trackFitError.count((*it)->detectorId()) > 0) {
      // User defined error
      xyerror = parameters->trackFitError[(*it)->detectorId()];
    } else if (parameters->toa[(*it)->detectorId()]) {
      // ToA plane
      xyerror = m_errorToA;
    }
    double zerror = 1.; 
    gr->SetPoint(k, double((*it)->globalX()), double((*it)->globalY()), double((*it)->globalZ()));
    gr->SetPointError(k, xyerror, xyerror, zerror);
    ++k;
  }

  // Fit the graph now.
  TVirtualFitter* gMinuit = TVirtualFitter::Fitter(0, 4);
  gMinuit->SetObjectFit(gr);
  gMinuit->SetFCN(SumDistance2);
  double arglist[10];
  // set PRINT level
  // -1 : no output except from SHOW commands
  //  0 : minimum output (no starting values or intermediate results)
  //  1 : default value, normal output
  //  2 : additional output giving intermediate results.
  //  3 : maximum output, showing progress of minimizations.
  arglist[0] = -1;
  gMinuit->ExecuteCommand("SET PRINT", arglist, 1);

  double pStart[4] = {1., 1., 1., 1.};
  gMinuit->SetParameter(0, "x", pStart[0], 0.01, 0, 0);
  gMinuit->SetParameter(1, "tx", pStart[1], 0.01, 0, 0);
  gMinuit->SetParameter(2, "y", pStart[2], 0.01, 0, 0);
  gMinuit->SetParameter(3, "ty", pStart[3], 0.01, 0, 0);

  arglist[0] = 1000; // number of function calls 
  arglist[1] = 0.0001; // tolerance 
  gMinuit->ExecuteCommand("MIGRAD", arglist, 2);
  // if (minos) gMinuit->ExecuteCommand("MINOS",arglist,0);

  //----------------------------------------------------------------------------------------------
  //  return global fit parameters
  //    - amin     : chisquare
  //    - edm      : estimated distance to minimum
  //    - errdef
  //    - nvpar    : number of variable parameters
  //    - nparx    : total number of parameters
  int nvpar, nparx; 
  double chi2, edm, errdef;
  gMinuit->GetStats(chi2, edm, errdef, nvpar, nparx);
  TMatrixD mat(4, 4, gMinuit->GetCovarianceMatrix());
  m_covMatrix.ResizeTo(mat);
  m_covMatrix = mat;
  if (m_debug) {
    m_covMatrix.Print();
    std::cout << "npar = " << nparx << std::endl;
    std::cout << "Log likelyhood = " << gMinuit->GetSumLog(nd) << std::endl;
    // std::cout << "chi2 = " << gMinuit->Chisquare(nparx, parFit) << std::endl;
    std::cout << "chi2 = " << chi2 << std::endl;
  }
  double x0 = gMinuit->GetParameter(0);
  double xm = gMinuit->GetParameter(1);
  double y0 = gMinuit->GetParameter(2);
  double ym = gMinuit->GetParameter(3);
  double e_x0 = gMinuit->GetParError(0);
  double e_xm = gMinuit->GetParError(1);
  double e_y0 = gMinuit->GetParError(2);
  double e_ym = gMinuit->GetParError(3); 
  if (m_debug) {
    std::cout << "First state: (" 
              << x0 << " +/- " << e_x0 << ", "
              << y0 << " +/- " << e_y0 << ")" << std::endl;
    std::cout << "Direction: (" 
              << xm << " +/- " << e_xm << ", "
              << ym << " +/- " << e_ym << ")" << std::endl;
  }

  firstState(x0, y0, 0);
  direction(xm, ym, 1);
  m_chi2 = chi2; 
  // We have nd points in both (x,y) coordinates and 4 degrees of freedom (x, tx, y, ty)
  m_ndof = 2 * nd - 4;
  prob();
  if (m_debug) {
    std::cout << "chi2=" << m_chi2 << " and ndof=" << m_ndof << std::endl;
    if (m_ndof != 0) std::cout << "chi2/ndof=" << m_chi2/m_ndof << std::endl;
    std::cout << "P(chi2 due to chance) = (1-P(chi2,ndof)) = " << m_prob << std::endl;
  }
  delete gr; gr = 0;

}
