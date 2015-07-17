#ifndef CELLAUTOMATA_H
#define CELLAUTOMATA_H 1

#include <vector>
#include <list>
#include <string>
#include <iosfwd>
#include <exception>
#include <utility>

#include "Math/Plane3D.h"
#include "Math/Vector3D.h"
#include "Math/Point3D.h"

#include "Graph.h"
#include "Cell.h"
#include "KDTree.h"
#include "TestBeamCluster.h"
#include "TestBeamProtoTrack.h"
#include "Parameters.h"

/** @class CellAutomata CellAutomata.h
*  
* 	2011-12-13 : Matthew Reid 
* 	Comments: 
* 	    Cellular automata forces links between neighbouring points creating
* 	    cells, each cell is initially given an interger weighting of 1. These
* 	    cells are updated given some specific criteria (mainly acceptance due
* 	    to scattering) where we iterate along the lattice and increase the
* 	    weighting if the conditions are met. Ultimately we can count back from
* 	    the highest integer to make a track. This method gives us vertices for
* 	    "free".
*      
*      Ideally have a cross between a tree and a graph structure so as not 
*      to search all vertexes but have some depth.
*       -Need each plane to be a list of bimaps connecting clusters to suceeding plane
*       -Need to be able to assign the acceptance with kdtree radius, remember need to 
*       factor in the dz changes so check, the fact they are planes at angles so need to 
*       be able in intersect a line and plane given some anglular acceptance based on
*       moliere for now.
*       -How to interate finding the highest weight in last plane and working back.
*       -need to be able to "skip" planes if no hit is found... This could be tricky.
*
*       - Probably best to store the bimap with information of the lattice site. 
*       such that we know where we are and whether things were skipped or not?...
*/

using namespace CA;

namespace CA
{
    using namespace ROOT::Math;

    struct Vertex
    {
        Vertex() {}
        Vertex(TestBeamCluster* c) :cluster(c) {}
        Vertex& operator=(const Vertex& other)
        {
            if ( this != &other ) // Ignore attepmts at self-assignment
            {
                cluster = other.cluster;
            }
            return *this;
        }
        
        bool operator==(Vertex clus) const
        {
            return cluster == clus.cluster;
        }
                
        bool operator!=(Vertex clus) const
        {
            return cluster != clus.cluster;
        }

        TestBeamCluster* cluster; // or whatever, maybe nothing
        typedef boost::vertex_property_tag kind;
        //boost::default_color_type color;
    };
    
    struct Edge 
    {
        Cell cell;
        //typedef boost::edge_property_tag kind;
    };

    typedef std::map<int, VecCluster> ClustersAssignedToDetPlane;

    class CellAutomata
    {
        public:
            //typedef Graph<TestBeamCluster*, Cell> CAGraph;
            typedef Graph<Vertex, Edge> CAGraph;
            typedef CAGraph::Vertex vertex_type; 
            explicit CellAutomata(ClustersAssignedToDetPlane& clustersOnDetPlane, Parameters* par);
            ~CellAutomata();

            // public functions
            void runCA();
            bool vertexSearch(TestBeamCluster* clust, CAGraph::Vertex& vertex);
            // getters and setters
            std::vector<TestBeamProtoTrack*>* getTracks() const { return m_protoTracks; }
            CAGraph getGraph() const { return m_graph; }

        private:
            // private functions
            bool checkSucceedingPlane(size_t planeNo, VecCluster& result, TestBeamCluster* seedcluster, const double radius);
            double moliere(const double xOverX0);
            double angle_between_cells_RL(Cell leftCell, Cell rightCell);
            double angle_between_cells_LR(Cell leftCell, Cell rightCell);
            void assignGraph();
            bool update(const size_t i);
            bool update();
            void nextCA(const size_t starting_plane);
            void collect_out_edges( const CAGraph::Vertex& vertex, CAGraph::edge_set& accumulator);
            bool tracks(size_t planeNo, int weight);
            bool nextCellforTrack(CAGraph::Vertex& v, std::list<CAGraph::Edge>& edges, int& previous_weight);
            int m_num_edges;
            bool search_RightEdge_given_LeftEdge(CAGraph::Edge& left, CAGraph::Edge& right);
            void weightSearch(const int weight, std::vector<CAGraph::Edge>& edges);
            bool sort_weight(const CAGraph::Edge& left, const CAGraph::Edge& right);
            bool maketracks(const int weight);
            double radialSearchWindow(const size_t plane, double xOverX0);

        protected:
            // member variables	
            Parameters* m_parameters;
            std::vector< ROOT::Math::Plane3D > sensorPlanes;
            size_t m_noPlanes;
            std::vector< boost::shared_ptr<KDTree> > KDTreeinit;
            //std::vector< KDTree* > KDTreeinit;
            //CellMapping m_cellMapping;
            CAGraph m_graph;
            unsigned int m_minCellsForTrack;
            //boost::shared_ptr<ClustersAssignedToDetPlane> m_clustersOnDetPlane;
            ClustersAssignedToDetPlane*  m_clustersOnDetPlane;
            //boost::shared_ptr<MyGraph> m_graph;
            std::vector<TestBeamProtoTrack*>* m_protoTracks;
    };

    void print_dep(std::ostream & out, CellAutomata::CAGraph& g); 

    // general functions
    double Angle(const XYZVector & v1, const XYZVector & v2);
}

//template <class T, class N>
/*class CompWeight 
  {
  public:
  CompWeight(){};
  explicit CompWeight(CAGraph::Edge& j) : m_first(j) { };
  ~CompWeight(){};
  bool operator()(CAGraph::Edge p) const
  {
  return m_first.cell.getWeight() > p.cell.getWeight();
  }
  private:
  CAGraph::Edge m_first; 
  };*/

#endif //end CELLAUTOMATA_H



