// $Id: FEI4Cluster.h,v 1.10 2010/06/07 11:08:34 mjohn Exp $
#ifndef FEI4Cluster_H 
#define FEI4Cluster_H 1

// Include files
#include "TestBeamObject.h"
#include "FEI4RowColumnEntry.h"
#include "TDCTrigger.h"

#include "TMatrixD.h"
#include "TVectorD.h"
#include "TMath.h"
#include "Math/PositionVector3D.h"
//#include "Point.h"
/** @class FEI4Cluster FEI4Cluster.h
 *  
 *  2011-10-04 : Daniel Hynds
 *  
 */

using namespace ROOT::Math;

class FEI4Cluster : public TestBeamObject {
	public: 
		FEI4Cluster(); 
		FEI4Cluster(FEI4Cluster&,bool=COPY);
		virtual ~FEI4Cluster( ){delete m_clusterHits;}
		void rowPosition(float r){row_pos=r;}
		float rowPosition(){return row_pos;}
		void colPosition(float c){col_pos=c;}
		float colPosition(){return col_pos;}

		void rowRMS(float r){row_rms=r;}
		float rowRMS(){return row_rms;}
		void colRMS(float r){col_rms=r;}
		float colRMS(){return col_rms;}

		void totalADC(float t){totalADCcount=t;}
		float totalADC(){return totalADCcount;}

		void globalX(float g){global_x=g; compute_globalXYZ();}
		float globalX(){return global_x;}
		void globalY(float g){global_y=g; compute_globalXYZ();}
		float globalY(){return global_y;}
		void globalZ(float g){global_z=g; compute_globalXYZ();}
		float globalZ(){return global_z;}

		void rowWidth(float w){row_wid=w;} 
		float rowWidth(){return row_wid;}
		void colWidth(float w){col_wid=w;} 
		float colWidth(){return col_wid;}
		
                float gettimeStamp(){return time_stamp;}
                void  settimeStamp(float w){time_stamp=w;}

		void GlobalXYZ(PositionVector3D< Cartesian3D<float> > w){global_XYZ=w;} 
		PositionVector3D< Cartesian3D<float> > GlobalXYZ(){return global_XYZ;}
		
//		TestBeamEventElement *element(){return detElement;}
		std::string detectorId(){return dId;}
//		std::string sourceName(){return source;}
		std::vector<FEI4RowColumnEntry*>* getClusterHits(){return m_clusterHits;}  
		int getNumHitsInCluster(){return m_clusterHits->size();}

		void addHitToCluster(FEI4RowColumnEntry* hit){m_clusterHits->push_back(hit);}  

	protected:

	private:
//		TestBeamEventElement *detElement;
		std::string dId;
//		std::string source;
		float totalADCcount;
		float row_pos;
		float col_pos;
		float row_rms;
		float col_rms;
		float global_x;
		float global_y;
		float global_z;
		float row_wid;
		float col_wid;
                float time_stamp;
		PositionVector3D< Cartesian3D<float> > global_XYZ;
		std::vector<FEI4RowColumnEntry*>* m_clusterHits;  
		void compute_globalXYZ();
};

inline FEI4Cluster::FEI4Cluster(/*TestBeamEventElement *e*/)
{
	m_name=std::string("FEI4Cluster");
	m_clusterHits = 0;
	m_clusterHits = new std::vector<FEI4RowColumnEntry*>;  
	m_clusterHits->clear();  
//	detElement=e;
//	dId=e->detectorId();
	dId="FEI4";
//	source=e->sourceName();
}

inline void FEI4Cluster::compute_globalXYZ()
{
	PositionVector3D< Cartesian3D<double> > a(global_x,global_y,global_z);
	global_XYZ = a;
}

inline FEI4Cluster::FEI4Cluster(FEI4Cluster& c,bool action) : TestBeamObject()
{
	if(action==COPY) return;
//	detElement=0;
	dId=c.detectorId();
//	source=c.sourceName();
	totalADCcount=c.totalADC();
	row_pos=c.rowPosition();
	col_pos=c.colPosition();
	row_rms=c.rowRMS();
	col_rms=c.colRMS();
	global_x=c.globalX();
	global_y=c.globalY();
	global_z=c.globalZ();
	row_wid=c.rowWidth();
	col_wid=c.colWidth();
	global_XYZ=c.GlobalXYZ();
} 

typedef std::vector<FEI4Cluster*> FEI4Clusters;

#endif // FEI4Cluster_H
