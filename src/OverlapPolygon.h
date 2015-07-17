// $Id: OverlapPolygon.h,v 1.0 2012-07-10 $
#ifndef OVERLAPPOLYGON_H 
#define OVERLAPPOLYGON_H 1

// Include files
#include <map>
#include <vector>
#include <iosfwd>
#include <string>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>

#include "Math/Point2D.h"
#include "Math/Vector2D.h"

namespace RG
{
    enum Region
    {   
        TopLeft = 0,
        TopRight,
        BottomRight,
        BottomLeft
    };
}

class OverlapPolygon
{
    /*  
     *  OverlapPolygon - class to store the corners of the overlap region from the dataset.
     *
     *  tl(x1,y1) *----------------* tr(x2,y2)
     *            |               |  
     *            |              |   
     *            |             |    
     *            |            |
     *  bl(x3,y3) *-----------*  br(x4,y4)
     *
     */
    
    public:
        OverlapPolygon();
        OverlapPolygon(const std::string chip, const ROOT::Math::XYVector center, const ROOT::Math::XYVector tl, const ROOT::Math::XYVector tr, const ROOT::Math::XYVector bl, const ROOT::Math::XYVector br);
        OverlapPolygon(const OverlapPolygon& other );
        OverlapPolygon& operator=(const OverlapPolygon& other);

        ~OverlapPolygon();

        bool isInside(const double valx, const double valy) ;
        bool isInside( ROOT::Math::XYVector cluster) ;
        bool setCorner(const double valx, const double valy) ;
        bool setCorner(const ROOT::Math::XYVector& cluster) ;
        bool extendCorners();
        bool extendCornersBox();
        bool extendCornersPoly();

        // getter functions
        int getCounter() const {return this->m_counter;}
        std::string getChip() const {return this->m_chip;}
        ROOT::Math::XYVector getCentre() const {return this->m_centre;}
        ROOT::Math::XYVector getTL() const {return this->m_tl;}
        ROOT::Math::XYVector getTR() const {return this->m_tr;}
        ROOT::Math::XYVector getBL() const {return this->m_bl;}
        ROOT::Math::XYVector getBR() const {return this->m_br;}
        std::map< RG::Region, std::vector<ROOT::Math::XYVector> > getOutliers() const {return this->m_potential_outliers;}
        int getNOutliers(const RG::Region region) {return static_cast<int>(this->m_potential_outliers[region].size());}

        static const std::string RegionNames[4];

        // setter functions
        void setChip(const std::string chip); 
        void setCentre(const ROOT::Math::XYVector centre); 
        void setTL(const ROOT::Math::XYVector tl); 
        void setTR(const ROOT::Math::XYVector tr); 
        void setBL(const ROOT::Math::XYVector bl); 
        void setBR(const ROOT::Math::XYVector br); 
        void setOutlers(const std::map< RG::Region, std::vector<ROOT::Math::XYVector> > out); 

        //! print the information 
        friend std::ostream& operator<<(std::ostream& os, const OverlapPolygon& result);

    private:
        // private meember functions
        RG::Region isRegion(const ROOT::Math::XYVector& cluster) const ; 
        bool findRealOutliers();

        int m_counter;
        std::string m_chip;
        ROOT::Math::XYVector m_centre;
        ROOT::Math::XYVector m_tl;
        ROOT::Math::XYVector m_tr;
        ROOT::Math::XYVector m_bl;
        ROOT::Math::XYVector m_br;
        std::map<RG::Region, std::vector< ROOT::Math::XYVector > > m_potential_outliers;
};

// exterior functions
ROOT::Math::XYVector getIntersection(const ROOT::Math::XYVector p1, const ROOT::Math::XYVector p2, const ROOT::Math::XYVector p3, const ROOT::Math::XYVector p4);
ROOT::Math::XYVector point_on_line_at_t(const ROOT::Math::XYVector& p1, const ROOT::Math::XYVector& p2, const double t);
double find_t_given_point( const ROOT::Math::XYVector& p1, ROOT::Math::XYVector& p2, const ROOT::Math::XYVector& cluster);
double division(double a, double b);
bool CompareX( ROOT::Math::XYVector i, ROOT::Math::XYVector j);
bool CompareY( ROOT::Math::XYVector i, ROOT::Math::XYVector j);
double fx(double x, double c, double m );
int winding_n_PnPoly( std::vector< ROOT::Math::XYVector> poly, ROOT::Math::XYVector p);
int crossing_n_PnPoly( std::vector< ROOT::Math::XYVector> poly, ROOT::Math::XYVector p);
double distance( ROOT::Math::XYVector a, ROOT::Math::XYVector b);
inline double isLeft( ROOT::Math::XYVector P0, ROOT::Math::XYVector P1, ROOT::Math::XYVector P2 );
int InsidePolygon( std::vector<ROOT::Math::XYVector> polygon, ROOT::Math::XYVector p);

bool ComparePair( const std::pair< double, std::string>&  i, const std::pair< double, std::string>&  j);


#endif
