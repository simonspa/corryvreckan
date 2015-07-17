#include "Cell.h"
#include <cmath>
//#include "Math/VectorUtil_Cint.h"

using namespace CA;

//**************************************************************
CA::Cell::Cell()
    : m_weight(1), m_to_update(false)
{}

//**************************************************************
CA::Cell::Cell(const Cell& other)
    : m_weight(other.getWeight()), m_to_update(other.to_update()), m_leftCluster(other.getLeftCluster()), m_rightCluster(other.getRightCluster())
{}

//**************************************************************
CA::Cell::Cell(const int& w_, bool to_update, TestBeamCluster* left_, TestBeamCluster* right_)
    : m_weight(w_),  m_to_update(to_update), m_leftCluster(left_), m_rightCluster(right_)
{}

//**************************************************************
CA::Cell::~Cell()
{}

//**************************************************************
Cell& CA::Cell::operator=(const Cell& other)
{
	if ( this != &other ) // Ignore attepmts at self-assignment
	{
		m_weight = other.getWeight();
		m_to_update = other.to_update();
		m_leftCluster = other.getLeftCluster();
		m_rightCluster = other.getRightCluster();
	}
	return *this;
}

//**************************************************************
void CA::Cell::addOneToWeight()
{
    if(m_to_update) m_weight += 1;
}


//**************************************************************
void CA::Cell::setWeight( const int w_)
{
    m_weight = w_;
}
            
//**************************************************************
void CA::Cell::to_update( const bool to_update )
{
    m_to_update = to_update;
}

//**************************************************************
void CA::Cell::setLeftCluster(TestBeamCluster* const left_)
{
    m_leftCluster = left_;
	compute_length();
}

//**************************************************************
void CA::Cell::setRightCluster(TestBeamCluster* const right_)
{
    m_rightCluster = right_;
	compute_length();    
}

//**************************************************************
void CA::Cell::compute_length()
{
    m_len = std::sqrt( ( ROOT::Math::XYZVector( m_leftCluster->globalX(), m_leftCluster->globalY(), m_leftCluster->globalZ() ) 
                - ROOT::Math::XYZVector( m_rightCluster->globalX(), m_rightCluster->globalY(), m_rightCluster->globalZ() ) ).Mag2() );
}

//**************************************************************
ROOT::Math::XYZVector CA::Cell::direction_norm_RL()
{
    return ( ROOT::Math::XYZVector( m_leftCluster->globalX(), m_leftCluster->globalY(), m_leftCluster->globalZ() ) 
                - ROOT::Math::XYZVector( m_rightCluster->globalX(), m_rightCluster->globalY(), m_rightCluster->globalZ() ) ).Unit() ;
}

//**************************************************************
ROOT::Math::XYZVector CA::Cell::direction_norm_LR()
{
    return ( ROOT::Math::XYZVector( m_rightCluster->globalX(), m_rightCluster->globalY(), m_rightCluster->globalZ() ) 
                - ROOT::Math::XYZVector( m_leftCluster->globalX(), m_leftCluster->globalY(), m_leftCluster->globalZ() ) ).Unit() ;
}

//**************************************************************
std::ostream& CA::operator<<(std::ostream& out, const Cell& cell)
{
	out << "Weight                  :  " << cell.getWeight() << std::endl;
    out << "Update Status           :  " << cell.to_update() << std::endl;
    out << "Left Cluster  (x,y,z,t) :  " << "(" << cell.getLeftCluster()->globalX() << "," << cell.getLeftCluster()->globalY() << "," << cell.getLeftCluster()->globalZ() << "," << cell.getLeftCluster()->element()->timeStamp() << ")" << std::endl;
    out << "Right Cluster  (x,y,z,t) :  " << "(" << cell.getRightCluster()->globalX() << "," << cell.getRightCluster()->globalY() << "," << cell.getRightCluster()->globalZ() << "," << cell.getRightCluster()->element()->timeStamp() << ")" << std::endl;
	return out;
}

