#ifndef CELL_H
#define CELL_H 1

#include <string>
#include <iosfwd>

#include "Cell.h"
#include "KDTree.h"
#include "TestBeamCluster.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"

using namespace ROOT::Math;
/** @class Cell Cell.h
*  
* 	2011-12-13 : Matthew Reid 
* 	Comments: 
* 	    Base class for a Cell. Each Cell has a left <-> right cluster and
* 	    associated weight. It also has a boolean to determine whether its 
* 	    weight should be updated after iteration of the set.
*
*/

namespace CA
{
    class Cell
    {
        public:
            // c´tors
            Cell();
            Cell(const Cell& other);
		    Cell(const int& w_, bool to_update, TestBeamCluster* left_, TestBeamCluster* right_);
            
            // d´tor
            ~Cell();

            // Copy assignment operator
            Cell& operator=(const Cell& other);

            // member funcions
            void addOneToWeight();
            ROOT::Math::XYZVector direction_norm_RL();
            ROOT::Math::XYZVector direction_norm_LR();

            // access functions getters and setter
            int getWeight() const { return m_weight; }
            bool to_update() const { return m_to_update; }
            TestBeamCluster* getLeftCluster() const { return m_leftCluster; }
            TestBeamCluster* getRightCluster() const { return m_rightCluster; }
            ROOT::Math::XYZVector getLeftXYZ() const { return ROOT::Math::XYZVector(m_leftCluster->globalX(), m_leftCluster->globalY(), m_leftCluster->globalZ() ); }
            ROOT::Math::XYZVector getRightXYZ() const { return ROOT::Math::XYZVector(m_rightCluster->globalX(), m_rightCluster->globalY(), m_rightCluster->globalZ() ); }
            double length() const { return m_len;};

            void setWeight( const int w_);
            void to_update( const bool to_update );
            void setLeftCluster(TestBeamCluster* const left_);
            void setRightCluster(TestBeamCluster* const right_);

        private:
            // Private member functions
		    void compute_length();
            
            // Private member variables
            int m_weight;
            bool m_to_update;
            TestBeamCluster* m_leftCluster;
            TestBeamCluster* m_rightCluster;

		    double m_len;
    };
    
    // Streaming operators
    std::ostream& operator<<(std::ostream& out, const Cell& cell);

}

#endif // end CELL_H


