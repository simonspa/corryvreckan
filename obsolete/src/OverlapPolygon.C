#include "OverlapPolygon.h"

#include <dirent.h>
#include <errno.h>
#include <iomanip>
#include <algorithm>
#include <limits>

#include <boost/units/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/function.hpp>
        
static const bool m_debug(false);

//*****************************************************************************************************************
OverlapPolygon::OverlapPolygon() 
    : m_counter(0), m_chip("default"), m_centre(ROOT::Math::XYVector(0.,0.)), m_tl(ROOT::Math::XYVector(0.,0.)), m_tr(ROOT::Math::XYVector(0.,0.)), m_bl(ROOT::Math::XYVector(0.,0.)), m_br(ROOT::Math::XYVector(0.,0.))
{
    m_potential_outliers[RG::TopLeft].clear();
    m_potential_outliers[RG::TopRight].clear();
    m_potential_outliers[RG::BottomLeft].clear();
    m_potential_outliers[RG::BottomRight].clear();
}

OverlapPolygon::OverlapPolygon(const std::string chip, const ROOT::Math::XYVector centre, const ROOT::Math::XYVector tl, const ROOT::Math::XYVector tr, const ROOT::Math::XYVector bl, const ROOT::Math::XYVector br) 
: m_counter(0), m_chip(chip), m_centre(centre), m_tl(tl), m_tr(tr), m_bl(bl), m_br(br) 
{
    //m_potential_outliers = outliers; 
}

    OverlapPolygon::OverlapPolygon(const OverlapPolygon& other ) 
: m_counter(other.getCounter()), m_chip(other.getChip()), m_centre(other.getCentre()), m_tl(other.getTL()), m_tr(other.getTR()), m_bl(other.getBL()), m_br(other.getBR()) 
{
    m_potential_outliers = other.getOutliers();
}

//*****************************************************************************************************************
OverlapPolygon& OverlapPolygon::operator=(const OverlapPolygon& other)
{
    if ( this != &other ) // Ignore attepmts at self-assignment
    {
        m_counter = other.getCounter();
        m_chip = other.getChip();
        m_centre = other.getCentre();
        m_tl = other.getTL();
        m_tr = other.getTR();
        m_bl = other.getBL();
        m_br = other.getBR();
        m_potential_outliers = other.getOutliers();
    }
    return *this;
}

//*****************************************************************************************************************
OverlapPolygon::~OverlapPolygon()
{}

//*****************************************************************************************************************
std::ostream& operator<<(std::ostream& os, const OverlapPolygon& result)
{
    os << std::endl;
    os << "  Sensor : " << result.getChip() << std::endl;
    os << "  Top Left point : " << result.getTL() << std::endl;
    os << "  Top Right point : " << result.getTR() << std::endl;
    os << "  Bottom Left point : " << result.getBL() << std::endl;
    os << "  Bottom Right point : " << result.getBR() << std::endl;
    return os;
}

const std::string OverlapPolygon::RegionNames[4] = { "TopLeft", "TopRight",  "BottomRight", "BottomLeft" };

//*****************************************************************************************************************
void OverlapPolygon::setChip(const std::string chip)
{
    m_chip = chip;
}

//*****************************************************************************************************************
void OverlapPolygon::setCentre(const ROOT::Math::XYVector centre) 
{
    m_centre = centre;
}

//*****************************************************************************************************************
void OverlapPolygon::setTL(const ROOT::Math::XYVector tl) 
{
    m_tl = tl;
}

//*****************************************************************************************************************
void OverlapPolygon::setTR(const ROOT::Math::XYVector tr) 
{
    m_tr = tr;
}

void OverlapPolygon::setBL(const ROOT::Math::XYVector bl) 
{
    m_bl = bl;
}

//*****************************************************************************************************************
void OverlapPolygon::setBR(const ROOT::Math::XYVector br) 
{
    m_br = br;
}

//*****************************************************************************************************************
void OverlapPolygon::setOutlers(const std::map< RG::Region, std::vector<ROOT::Math::XYVector> > out) 
{
    m_potential_outliers = out;
}

//*****************************************************************************************************************
bool OverlapPolygon::isInside(const double valx, const double valy) 
{
    return isInside(ROOT::Math::XYVector(valx, valy));
}

//*****************************************************************************************************************
bool OverlapPolygon::isInside(ROOT::Math::XYVector cluster) 
{
    // uses the winding method to determine if point is inside quad or not.
    // polygon[n+1], with polygon[0] = polygon[n]. 
    // We make a coordinate transformation so that point cluster(x,y) |-> cluster´(0,0)
    std::vector<ROOT::Math::XYVector> polygon(5);

    polygon[0] = m_tl;
    polygon[1] = m_bl;
    polygon[2] = m_br;
    polygon[3] = m_tr;
    polygon[4] = m_tl;

    //polygon[0] = m_tl - cluster;
    //polygon[1] = m_bl - cluster;
    //polygon[2] = m_br - cluster;
    //polygon[3] = m_tr - cluster;
    //polygon[4] = m_tl - cluster;

    // insane checking here only need to pass one but lets be really sure!
    int winding = winding_n_PnPoly( polygon, cluster);
    int crossing = crossing_n_PnPoly( polygon, cluster);
    int triplecheck = InsidePolygon( polygon, cluster);
    
    polygon.clear();
    
    return (winding != 0 || crossing !=0 || triplecheck != 0) ? true : false;
}

//*****************************************************************************************************************
bool OverlapPolygon::extendCorners()
{
    // It is hard coded that we require
    //findRealOutliers();
    if (m_counter < 1760)
        return extendCornersBox();
    else
        return extendCornersPoly();
}

//*****************************************************************************************************************
bool OverlapPolygon::extendCornersBox()
{
    // Old method just find maximum rectangle that will fit around data.
    std::vector< ROOT::Math::XYVector > temp;
    temp.push_back(m_tr);
    temp.push_back(m_tl);
    temp.push_back(m_bl);
    temp.push_back(m_br);

    std::map<RG::Region, std::vector< ROOT::Math::XYVector > >::iterator it = m_potential_outliers.begin();
    const std::map<RG::Region, std::vector< ROOT::Math::XYVector > >::const_iterator endit = m_potential_outliers.end();

    for(; it != endit; ++it)
    {       
        temp.insert(temp.end(), (it->second).begin(), (it->second).end());
    }

    double xmin = (*std::min_element(temp.begin(), temp.end(), CompareX)).X();
    double xmax = (*std::max_element(temp.begin(), temp.end(), CompareX)).X();
    double ymax = (*std::max_element(temp.begin(), temp.end(), CompareY)).Y();
    double ymin = (*std::min_element(temp.begin(), temp.end(), CompareY)).Y();

    m_tl = ROOT::Math::XYVector(xmin, ymax);
    m_bl = ROOT::Math::XYVector(xmin, ymin);
    m_br = ROOT::Math::XYVector(xmax, ymin);
    m_tr = ROOT::Math::XYVector(xmax, ymax);
    return true;
}

//*****************************************************************************************************************
bool OverlapPolygon::extendCornersPoly()
{
    // before running make sure we have real outliers afer the lines are drawn.
    // find the maximum points that lie outside the boundaries, will use these to 
    // project a new corner point.
    // This all coule be written much neater and not be as stupid but hey, time is of
    // the essence.
    //bool outliersRemain(false);
    //std::map<RG::Region, std::vector< ROOT::Math::XYVector > >::iterator iter;
    //const std::map<RG::Region, std::vector< ROOT::Math::XYVector > >::const_iterator enditer = m_potential_outliers.end();
    //int iterations(0);
    //do
    //{
    //outliersRemain = findRealOutliers();

    //for ( int i = RG::TopLeft; i <= RG::BottomRight; ++i )
    //for ( ; iter != enditer; ++iter )
    //
    //switch(static_cast<RG::Region>(i))
    //


    //std::sort(m_potential_outliers[RG::TopLeft].begin(), m_potential_outliers[RG::TopLeft].end(), CompareX);

    ROOT::Math::XYVector xminA1(0,0), xminA2(0,0), yminB1(0,0), yminB2(0,0), xmaxC1(0,0), xmaxC2(0,0), ymaxD1(0,0), ymaxD2(0,0); 

    if(m_potential_outliers[RG::TopLeft].size() == 0)
    {
        xminA1 = m_tl;
        ymaxD2 = m_tl;
    }
    else
    {
        xminA1 = *std::min_element( m_potential_outliers[RG::TopLeft].begin(), m_potential_outliers[RG::TopLeft].end(), CompareX );
        if ( !( xminA1.X() < m_tl.X() && xminA1.Y() < m_tl.Y() ))
            xminA1 = m_tl;

        ymaxD2 = *std::max_element( m_potential_outliers[RG::TopLeft].begin(), m_potential_outliers[RG::TopLeft].end(), CompareY );
        if( !( ymaxD2.Y() > m_tl.Y() && ymaxD2.X() > m_tl.X() ))
            ymaxD2 = m_tl;
    }

    if(m_potential_outliers[RG::BottomLeft].size() == 0)
    {
        xminA2 = m_bl;
        yminB1 = m_bl;
    }
    else
    {
        xminA2 = *std::min_element( m_potential_outliers[RG::BottomLeft].begin(), m_potential_outliers[RG::BottomLeft].end(), CompareX );
        if ( !(xminA2.X() < m_bl.X() && xminA2.Y() > m_bl.Y() ))
            xminA2 = m_bl;

        yminB1 = *std::min_element( m_potential_outliers[RG::BottomLeft].begin(), m_potential_outliers[RG::BottomLeft].end(), CompareY );
        if ( !( yminB1.Y() < m_bl.Y() && yminB1.X() > m_bl.X() ))
            yminB1 = m_bl;
    }

    if(m_potential_outliers[RG::BottomRight].size() == 0)
    {
        xmaxC1 = m_br;
        yminB2 = m_br;
    }
    else
    {
        xmaxC1 = *std::max_element( m_potential_outliers[RG::BottomRight].begin(), m_potential_outliers[RG::BottomRight].end(), CompareX );
        if ( !( xmaxC1.X() > m_br.X() && xmaxC1.Y() > m_br.Y()))
            xmaxC1 = m_br;

        yminB2 = *std::min_element( m_potential_outliers[RG::BottomRight].begin(), m_potential_outliers[RG::BottomRight].end(), CompareY );
        if ( !( yminB2.Y() < m_br.Y() && yminB2.X() < m_br.X() ))
            yminB2 = m_br;
    }

    if(m_potential_outliers[RG::TopRight].size() == 0)
    {
        ymaxD1 = m_tr;
        xmaxC2 = m_tr;
    }
    else
    {
        ymaxD1 = *std::max_element( m_potential_outliers[RG::TopRight].begin(), m_potential_outliers[RG::TopRight].end(), CompareY );
        if ( !( ymaxD1.Y() > m_tr.Y() && ymaxD1.X() < m_tr.X() ))
            ymaxD2 = m_tr ;

        xmaxC2 = *std::max_element( m_potential_outliers[RG::TopRight].begin(), m_potential_outliers[RG::TopRight].end(), CompareX);
        if( !( xmaxC2.X() > m_tr.X() && xmaxC2.Y() < m_tr.Y() ))
            xmaxC2 = m_tr;
    }

    m_tl = getIntersection(ymaxD1, ymaxD2, xminA1, xminA2);
    m_bl = getIntersection(xminA1, xminA2, yminB1, yminB2);
    m_br = getIntersection(yminB1, yminB2, xmaxC1, xmaxC2);      
    m_tr = getIntersection(xmaxC1, xmaxC2, ymaxD1, ymaxD2);
    //++iterations;
    //} 
    //while(outliersRemain && iterations < 50000);
    return true;
}

//*****************************************************************************************************************
ROOT::Math::XYVector point_on_line_at_t(const ROOT::Math::XYVector& p1, const ROOT::Math::XYVector& p2, const double t)
{
    return p1 + (p2 - p1) *  t;
}

//*****************************************************************************************************************
double find_t_given_point(const ROOT::Math::XYVector& p1, ROOT::Math::XYVector& p2, const ROOT::Math::XYVector& cluster)
{
    // only need solve for 1 dimension
    double div(0.);
    try 
    {
        div = division(cluster.X() - p1.X(), (p2 - p1).X());
    }
    catch (const char* msg) 
    {
        std::cerr << msg << std::endl;
    }
    if(div == 0)
    {
        try 
        {
            div = division(cluster.Y() - p1.Y(), (p2 - p1).Y());
        }
        catch (const char* msg) 
        {
            std::cerr << msg << std::endl;
        }
    }
    //std::cout << div << std::endl;
    return div ;
}

//*****************************************************************************************************************
bool OverlapPolygon::findRealOutliers()
{
    bool foundoutlier(false);
    // create a temp to hold the info
    std::map<RG::Region, std::vector< ROOT::Math::XYVector > > temp;

    std::map<RG::Region, std::vector< ROOT::Math::XYVector > >::iterator it = m_potential_outliers.begin();
    const std::map<RG::Region, std::vector< ROOT::Math::XYVector > >::const_iterator endit = m_potential_outliers.end();

    for(; it != endit; ++it)
    {
        std::vector< ROOT::Math::XYVector >::iterator iter = it->second.begin();
        const std::vector< ROOT::Math::XYVector >::const_iterator enditer = it->second.end();
        for(; iter != enditer; ++iter) 
            if(!isInside(*iter))
            {
                temp[it->first].push_back(*iter); 
                foundoutlier = true;    
            }
    }
    m_potential_outliers.clear();
    m_potential_outliers = temp;
    return foundoutlier;
}

//*****************************************************************************************************************
bool OverlapPolygon::setCorner(const double valx, const double valy) 
{
    return setCorner(ROOT::Math::XYVector(valx, valy));
}

//*****************************************************************************************************************
bool OverlapPolygon::setCorner(const ROOT::Math::XYVector& cluster)
{
    bool found(false);
    switch ( isRegion(cluster) )
    {
        case RG::TopLeft :
            if(cluster.X() < m_tl.X() && cluster.Y() > m_tl.Y())
                m_tl.SetXY(cluster.X(), cluster.Y());
            if( (cluster.X() < m_tl.X() && cluster.Y() < m_tl.Y()) || ( cluster.X() > m_tl.X() && cluster.Y() > m_tl.Y() ) )
            //else
            {
                m_potential_outliers[RG::TopLeft].push_back(cluster);  
                ++m_counter;
            }
            found=true;
            break;
        case RG::TopRight :
            if(cluster.X() > m_tr.X() && cluster.Y() > m_tr.Y())
                m_tr.SetXY(cluster.X(), cluster.Y());
            if( (cluster.X() > m_tr.X() && cluster.Y() < m_tr.Y() ) || ( cluster.X() < m_tr.X() && cluster.Y() > m_tr.Y()) )
            //else
            {
                m_potential_outliers[RG::TopRight].push_back(cluster);  
                ++m_counter;
            }
            found=true;
            break;
        case RG::BottomLeft :
            if(cluster.X() < m_bl.X() && cluster.Y() < m_bl.Y())
                m_bl.SetXY(cluster.X(), cluster.Y());
            if( ( cluster.X() > m_bl.X() && cluster.Y() < m_bl.Y() ) || ( cluster.X() < m_bl.X() && cluster.Y() > m_bl.Y() ) )
            //else
            {
                m_potential_outliers[RG::BottomLeft].push_back(cluster);  
                ++m_counter;
            }
            found=true;
            break;
        case RG::BottomRight : 
            if(cluster.X() > m_br.X() && cluster.Y() < m_br.Y())
                m_br.SetXY(cluster.X(), cluster.Y());
            if( ( cluster.X() < m_br.X() && cluster.Y() < m_br.Y() ) || (cluster.X() > m_br.X() && cluster.Y() > m_br.Y()) )
            //else
            {
                m_potential_outliers[RG::BottomRight].push_back(cluster);  
                ++m_counter;
            }
            found=true;
            break;
        default :
            std::cerr << "Error! No match for region type!" << std::endl;
            found=false;
            break;
    }
    return found;
}

//*****************************************************************************************************************
RG::Region OverlapPolygon::isRegion(const ROOT::Math::XYVector& cluster) const
{ 
    if(cluster.X() <= m_centre.X() && cluster.Y() > m_centre.Y())
        return RG::TopLeft;
    else if(cluster.X() > m_centre.X() && cluster.Y() >= m_centre.Y()) 
        return RG::TopRight;
    else if(cluster.X() < m_centre.X() && cluster.Y() <= m_centre.Y()) 
        return RG::BottomLeft;
    else
        return RG::BottomRight;
}

//*****************************************************************************************************************
ROOT::Math::XYVector getIntersection(const ROOT::Math::XYVector p1, const ROOT::Math::XYVector p2, const ROOT::Math::XYVector p3, const ROOT::Math::XYVector p4)
{
    // solve 2d line intersection point. Consider two lines pa and pb
    //
    //  pa = p1 + ua(p2 - p1)    &&    pb = p3 + ub(p4 - p3)
    //
    // calculate differences  
    ROOT::Math::XYVector D1 = p2 - p1;
    ROOT::Math::XYVector D2 = p4 - p3;
    ROOT::Math::XYVector D3 = p1 - p3;

    // Find the tightest interval along the x-axis defined by the four points
    //double xrange_min = std::max(std::min(x1, x2), std::min(ax1, ax2));
    //double xrange_max = std::min(std::max(x1, x2), std::max(ax1, ax2));

    // If points from the two lines overlap, they are trivially intersecting
    //if ((x1 == ax1 and y1 == ay1) or (x2 == ax2 and y2 == ay2))
    //    return ROOT::Math::XYVector( (x1 == ax1 and y1 == ay1) ? x1 : x2, (x1 == ax1 and y1 == ay1) ? y1 : y2);
    // If the intersection between the two lines is within the tight range, add it to the list of intersections.
    //else if(x > xrange_min && x < xrange_max)
    //    return ROOT::Math::XYVector(x,y);
    // calculate the lengths of the two lines  */
    double len1 = sqrt(D1.Mag2());  
    double len2 = sqrt(D2.Mag2());

    // calculate angle between the two lines.  
    double dot = D1.Dot(D2); // dot product  
    double deg = dot / (len1*len2); 

    // if abs(angle)==1 then the lines are parallell,  
    // so no intersection is possible  
    if(fabs(deg)==1) 
        return ROOT::Math::XYVector(-9999999,-9999999);  

    // find intersection Pt between two lines    
    ROOT::Math::XYVector pt(0,0);  

    double div = D2.Y() * D1.X() - D2.X() * D1.Y();  
    double ua =   ( D2.X() * D3.Y() - D2.Y() * D3.X() ) / div;  
    //    double ub = ( D1.X() * D3.Y() - D1.Y() * D3.X() ) / div;  

    double x = p1.X() + ua * D1.X();
    double y = p1.Y() + ua * D1.Y();
    pt.SetXY( x, y);

    // calculate the combined length of the two segments  
    // between Pt-p1 and Pt-p2  
    return pt;
}

double fx( double x, double c, double m )
{
    //boost::function<double(double)> y = boost::lambda::var(c) + boost::lambda::var(m) * boost::lambda::_1;
    //return y(x);
    return (m * x + c);
}

//*****************************************************************************************************************
double division(double a, double b)
{
    if( b == 0 )
    {
        throw "Division by zero condition!";
    }
    return (a/b);
}

//*****************************************************************************************************************
bool CompareX(  ROOT::Math::XYVector i, ROOT::Math::XYVector j)
{
    return (i.X() < j.X());
}

//*****************************************************************************************************************
bool CompareY(  ROOT::Math::XYVector i, ROOT::Math::XYVector j)
{
    return (i.Y() < j.Y());
}

//*****************************************************************************************************************
bool ComparePair( const std::pair< double, std::string>&  i, const std::pair< double, std::string>&  j)
{
    return i.first < j.first;
}

//*****************************************************************************************************************
double distance( ROOT::Math::XYVector a, ROOT::Math::XYVector b)
{
    return std::sqrt( (a.X() - b.X())*(a.X() - b.X()) + (a.Y() - b.Y())*(a.Y() - b.Y()) );
}

//*****************************************************************************************************************
inline double isLeft( ROOT::Math::XYVector P0, ROOT::Math::XYVector P1, ROOT::Math::XYVector P2 )
{
    // isLeft(): tests if a point is Left|On|Right of an infinite line.
    //    Input:  three points P0, P1, and P2
    //    Return: >0 for P2 left of the line through P0 and P1
    //            =0 for P2 on the line
    //            <0 for P2 right of the line
    return ( (P1.x() - P0.x()) * (P2.y() - P0.y())  - (P2.x() - P0.x()) * (P1.y() - P0.y()) );
}

//*****************************************************************************************************************
int crossing_n_PnPoly( std::vector<ROOT::Math::XYVector> V, ROOT::Math::XYVector P )
{
// crossing number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  0 = outside, 1 = inside
    int    cn = 0;    // the crossing number counter
    // loop through all edges of the polygon
    for (int i=0; i<static_cast<int>(V.size()); i++) {    // edge from V[i] to V[i+1]
        if (((V[i].y() <= P.y()) && (V[i+1].y() > P.y()))    // an upward crossing
                || ((V[i].y() > P.y()) && (V[i+1].y() <= P.y()))) { // a downward crossing
            // compute the actual edge-ray intersect x-coordinate
            float vt = (float)(P.y() - V[i].y()) / (V[i+1].y() - V[i].y());
            if (P.x() < V[i].x() + vt * (V[i+1].x() - V[i].x())) // P.x < intersect
                ++cn;   // a valid crossing of y=P.y right of P.x
        }
    }
    return (cn&1);    // 0 if even (out), and 1 if odd (in)

}

//*****************************************************************************************************************
int winding_n_PnPoly( std::vector<ROOT::Math::XYVector> V, ROOT::Math::XYVector P)
{
// winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (==0 only if P is outside V[])
    int    wn = 0;    // the winding number counter

    // loop through all edges of the polygon
    for (int i(0); i<static_cast<int>(V.size()-1); ++i) 
    {   // edge from V[i] to V[i+1]
        if (V[i].y() <= P.y())
        {         // start y <= P.y
            if (V[i+1].y() > P.y())      // an upward crossing
                if (isLeft( V[i], V[i+1], P) > 0)  // P left of edge
                    ++wn;            // have a valid up intersect
        }
        else 
        {                       // start y > P.y (no test needed)
            if (V[i+1].y() <= P.y())     // a downward crossing
                if (isLeft( V[i], V[i+1], P) < 0)  // P right of edge
                    --wn;            // have a valid down intersect
        }
    }
    return wn;
}

int InsidePolygon( std::vector<ROOT::Math::XYVector> polygon, ROOT::Math::XYVector p)
{
    //cross points count of x
    int __count(0);

    // number of points defining polygon
    size_t N(polygon.size()) ;

    //neighbour bound vertices
    ROOT::Math::XYVector p1(0.,0.), p2(0.,0.);

    //left vertex
    p1 = polygon[0];
    
    //check all rays
    for(size_t i(1); i <= N; ++i)
    {
        //point is an vertex
        if(p == p1) return 1;

        //right vertex
        p2 = polygon[i % N];

        //ray is outside of our interests
        if(p.y() < std::min(p1.y(), p2.y()) || p.y() > std::max(p1.y(), p2.y()))
        {
            //next ray left point
            p1 = p2; continue;
        }

        //ray is crossing over by the algorithm (common part of)
        if(p.y() > std::min(p1.y(), p2.y()) && p.y() < std::max(p1.y(), p2.y()))
        {
            //x is before of ray
            if(p.x() <= std::max(p1.x(), p2.x()))
            {
                //overlies on a horizontal ray
                if(p1.y() == p2.y() && p.x() >= std::min(p1.x(), p2.x())) return true;

                //ray is vertical
                if(p1.x() == p2.x())
                {
                    //overlies on a ray
                    if(p1.x() == p.x()) return 1;
                    //before ray
                    else ++__count;
                }

                //cross point on the left side
                else
                {
                    //cross point of x
                    double xinters = (p.y() - p1.y()) * (p2.x() - p1.x()) / (p2.y() - p1.y()) + p1.x();

                    //overlies on a ray
                    if(fabs(p.x() - xinters) < __DBL_EPSILON__) return 1;

                    //before ray
                    if(p.x() < xinters) ++__count;
                }
            }
        }
        //special case when ray is crossing through the vertex
        else
        {
            //p crossing over p2
            if(p.y() == p2.y() && p.x() <= p2.x())
            {
                //next vertex
                const ROOT::Math::XYVector& p3 = polygon[(i+1) % N];

                //p.y lies between p1.y & p3.y
                if(p.y() >= std::min(p1.y(), p3.y()) && p.y() <= std::max(p1.y(), p3.y()))
                {
                    ++__count;
                }
                else
                {
                    __count += 2;
                }
            }
        }

        //next ray left point
        p1 = p2;
    }

    //EVEN
    if(__count % 2 == 0) return 0;
    //ODD
    else return 1;
}
