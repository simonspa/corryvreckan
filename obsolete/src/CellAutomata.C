// $Id: CellAutomata.C,v 1.0 13/12/2011 mreid Exp $
// Include files 

#include "CellAutomata.h"
#include "TMath.h"
#include "SystemOfUnits.h"
#include "PhysicalConstants.h"
#include <math.h>

using namespace CA;
using namespace boost;

//**************************************************************
CA::CellAutomata::CellAutomata(ClustersAssignedToDetPlane& clustersOnDetPlane, Parameters* par)
{
    std::cout<< "Initialising CA algortihm" << std::endl;
    
    m_parameters = new Parameters();
    m_parameters = par;
    m_protoTracks = new std::vector<TestBeamProtoTrack*>;

    // initalise a new graph on the heap
    //m_graph = new CAGraph;

    //m_clustersOnDetPlane = boost::make_shared<ClustersAssignedToDetPlane>(clustersOnDetPlane);
    m_clustersOnDetPlane = new ClustersAssignedToDetPlane(clustersOnDetPlane);
    m_noPlanes = m_clustersOnDetPlane->size();
    m_minCellsForTrack = m_parameters->numclustersontrack-1; // seen as have n-1 cells
    //std::cout<< "size CLUSTERSONDETPLANE " << m_noPlanes <<  std::endl;

    //const int noIVs = m_clustersOnDetPlane->find(0)->second.size();
    //m_graph = CAGraph(noIVs);
    size_t combinatorics(0);
    size_t initial_hits((m_clustersOnDetPlane->find(0)->second).size());
    size_t hitslast(0);
    for (size_t i(0); i < m_noPlanes; ++i) 
    {
        size_t hits = (m_clustersOnDetPlane->find(i)->second).size();
        if(i==0) combinatorics = hits;
        //else if (i==m_noPlanes) combinatorics *= hits ;
        else 
        {
            combinatorics += ( hitslast*hits );
            hitslast = hits;
        }
    }
    std::cout<< "Maximum combinatorics in graph are " << initial_hits*combinatorics << std::endl;

    // set up the KDTree Algorithm for searching the NN within search window
    for (size_t i(1); i < m_noPlanes; ++i)
    {
        VecCluster succeedingPlane = m_clustersOnDetPlane->find(i)->second;
        KDTreeinit.push_back( boost::make_shared<KDTree>(succeedingPlane) );
        //KDTreeinit.push_back( new KDTree(succeedingPlane) );
    }

    assignGraph( );
}

//**************************************************************
CA::CellAutomata::~CellAutomata()
{
    delete m_parameters; /*delete m_protoTracks;*/ delete m_clustersOnDetPlane;// delete m_graph; 
    m_parameters=0;/*m_protoTracks=0;*/ m_clustersOnDetPlane=0;//m_graph=0;
    KDTreeinit.clear();//m_cellMapping.clear();
}

//**************************************************************
void CA::CellAutomata::assignGraph()
{
    std::cout<< "Assigning graph components" << std::endl;
    int count(0);
    for (size_t i(0); i < (m_noPlanes-1); ++i)

    {
        //std::cout<< "begin iteration " << i+1 << std::endl;
        //std::cout<< "Print if enter first loop" << std::endl;
        //VecCluster planes =  (m_clustersOnDetPlane->find(i)->second).begin();
        //VecCluster::iterator preit =  (m_clustersOnDetPlane->find(i)->second).begin();
        //const VecCluster::const_iterator endit =  (m_clustersOnDetPlane->find(i)->second).end();
        //TestBeamCluster* test = *preit;
        //std::cout<< "First loop test " << test->globalX()<< std::endl;
        double xOverX0 = m_parameters->xOverX0;
        std::string isDUT = ((m_clustersOnDetPlane->find(i)->second)[0])->element()->detectorId();
        if(isDUT == m_parameters->dut) 
        {
            xOverX0 = m_parameters->xOverX0_dut;
        }

        size_t succeedingPlaneNo(i+1);
        
        for(VecCluster::iterator preit =  (m_clustersOnDetPlane->find(i)->second).begin(); preit != (m_clustersOnDetPlane->find(i)->second).end(); ++preit)
        {
            //std::cout<< "Print if enter second loop" << std::endl;
            TestBeamCluster* seedClusterL = *preit;
            //std::cout<< "Second loop test " << seedClusterL->globalX()<< std::endl;

            // Setup kNN to return all clusters within the specified radius
            VecCluster result;
            //size_t succeedingPlaneNo(i+1);
            Vertex vp1; vp1.cluster = seedClusterL;
            CAGraph::Vertex v1;
            if(count == 0)
            {
                v1 = m_graph.AddVertex(vp1);
                //std::cout<< "ADDED FIRST VERTEX" << std::endl;
                ++count;
            }
            else
            {
                bool found = vertexSearch(seedClusterL, v1);
                if (!found) 
                {
                    v1 = m_graph.AddVertex(vp1);
                }
                //else continue;
            }

            // search the next plane to return NN
            double radius = radialSearchWindow(i, xOverX0);
            //std::cout<< "RADIUS" << radius << std::endl;//double radius = 2.;
            bool check = checkSucceedingPlane( succeedingPlaneNo, result, seedClusterL, radius);
            // If no hits returned then try the next plane with wider window set to 1.5xradius
            if(!check && (succeedingPlaneNo+1)<m_noPlanes) 
            {
                isDUT = ((m_clustersOnDetPlane->find(i)->second)[0])->element()->detectorId();
                if(isDUT == m_parameters->dut) 
                {
                    xOverX0 = m_parameters->xOverX0_dut;
                }
                radius = radialSearchWindow(succeedingPlaneNo, xOverX0);
                checkSucceedingPlane( succeedingPlaneNo+1, result, seedClusterL, radius); 
                //std::cout<< "Print if last check failed" << std::endl;
            }
            //else std::cout<< "Check OK" << std::endl;//continue;
            // Loop through nearest clusters and add them to Cell
            //VecCluster::iterator resiter = result.begin();
            //const VecCluster::const_iterator endresiter = result.end();
            if(result.empty()) std::cout<< "No clusters returned within radius specified. " << std::endl;
            for (VecCluster::iterator resiter = result.begin(); resiter != result.end(); ++resiter)
            {
                TestBeamCluster* NNClusterR = *resiter;
                Vertex vp2; vp2.cluster = NNClusterR;
                CAGraph::Vertex v2;
                bool found = vertexSearch(NNClusterR, v2);
                if (!found) 
                {
                    v2 = m_graph.AddVertex(vp2);
                    //std::cout<< "ADDED ANOTHER VERTEX" << std::endl;
                }
                //else continue;//Cell cell =  Cell(1, false, seedClusterL, NNClusterR);
                Edge joinLR; joinLR.cell = Cell(1, false, seedClusterL, NNClusterR);
                //Edge joinRL; joinRL.cell = Cell(1, false, NNClusterR, seedClusterL);
                //std::cout<< "Cell Print...." << join.cell << std::endl;

                //CAGraph::Vertex v1 = m_graph.AddVertex(vp1);
                //m_graph.AddEdge(v1,v2,joinLR,joinRL);
                m_graph.AddEdge(v1,v2,joinLR);
            }
        }
    }

    /*CAGraph::edge_range_t vertices_range = m_graph.getEdges(); 
      ConsolePrinter<CAGraph::Edge> printer;   
      std::cout<< "Testing Printer...." << std::endl;
      printer.print(vertices_range.first,vertices_range.second);*/
    //print_edge<Vertex, Edge>(m_graph);
    //std::cout << std::endl;
    //CA::print_dep(std::cout, m_graph);

}

//**************************************************************
double CA::CellAutomata::radialSearchWindow(const size_t plane, double xOverX0)
{
    std::string chip1 =  (m_clustersOnDetPlane->find(plane)->second[0])->detectorId();
    std::string chip2 = (m_clustersOnDetPlane->find(plane+1)->second[0])->detectorId();
    double dz = m_parameters->alignment[chip2]->displacementZ() - m_parameters->alignment[chip1]->displacementZ();
    return dz * tan( m_parameters->molieresigmacut * moliere(xOverX0) );
    //return radius;
}

//**************************************************************
void CA::print_dep(std::ostream & out, CellAutomata::CAGraph& g)
{
    CellAutomata::CAGraph::vertex_iter i, end;
    CellAutomata::CAGraph::out_edge_iter ei, edge_end;
    out << "Graph has " << g.getVertexCount() << " vertices and " << g.getEdgeCount() << " edges:- " << std::endl;
    for (boost::tie(i,end) = g.getVertices(); i != end; ++i) 
    {
        out << "Vertex " << g.properties(*i).cluster->globalZ() << " has "<< g.getVertexDegree(*i) << " out edges:-" <<std::endl;
        for (boost::tie(ei,edge_end) = out_edges(*i,g.getGraph()); ei != edge_end; ++ei)
            out << "    " << g.properties(*ei).cell << " connecting vertex:  " << g.properties(g.getSource(*ei)).cluster->globalZ()
                << "---->" << g.properties(g.getTarget(*ei)).cluster->globalZ() << std::endl;//id[target(*ei, g)] << "  ";
    }
}

//**************************************************************
void CA::CellAutomata::runCA()
{
    // start at the 2nd cell we look to the left, hence we look up the 3rd plane vertices as queries
    //size_t lastPlane(3); 
    std::cout<< "Executing CA algorithm" << std::endl;
    //if(3<m_noPlanes)  std::cout << "Oh dear, you have too few planes for the CA algorithm to work -- go and re-configure the parameters file" << std::endl; return ;
    //std::cout<< "Executing CA algorithm" << std::endl;

    bool updateAll(false);
    int count(0);
    size_t init(0); // starts on the 3rd plane
    do
    {
        //for (size_t i(m_noPlanes-1); i > init ; --i) nextCA(i);
        if (init == m_noPlanes-1)init=0;
        for (size_t i(init); i < (m_noPlanes-2) ; ++i)
        {
            //std::cout<<std::endl<<"Looping planes and executing CA"<<std::endl<<std::endl;
            nextCA(i);
        }
        //std::cout<< "Next CA worked!!!!" << std::endl;
        // update all cells with new weights, if no more can be done then weŕe finished
        updateAll = update();
        //updateAll = update(init);
        ++count;
        ++init;
    }while((updateAll==true) && (count < 1.e+4) ); // stop anything or infinite iterations (general crazyness);

    std::cout << "Total number of full iterations until no more updates = " << count << std::endl;
    /*CAGraph::edge_iter edge, edge_end;
      std::cout << "PRINTING OUT THE EDGES:-" << std::endl;
      for (boost::tie(edge,edge_end) = m_graph.getEdges(); edge != edge_end; ++edge)
      {
      std::cout << m_graph.properties(*edge).cell << std::endl;
      }
      std::cout << "PRINTING FINISHED!" << std::endl << std::endl;
      std::cout << "CHECK OUT EDGES!!!!!!!" << std::endl;

      TestBeamCluster* clust = (m_clustersOnDetPlane->find(3)->second)[0];    
      std::cout<<"NEW CLUSTER"<<std::endl;
      CAGraph::out_edge_iter begin,end;
      CAGraph::Vertex v;
      vertexSearch(clust,v);
    //boost::tie(begin,end) = out_edges(v, m_graph.getGraph());//m_graph.getOutEdges(v);
    std::cout << "number of out edges" << out_degree(v, m_graph.getGraph());
    int no(1);
    for (boost::tie(begin,end) = out_edges(v, m_graph.getGraph()); begin != end; ++begin)
    {  
    std::cout<<"Edge counter = " << no << std::endl;
    std::cout << m_graph.properties(*begin).cell << std::endl;
    }*/
    //std::cout << std::endl;
    //CA::print_dep(std::cout, m_graph);
    /*CAGraph::Vertex v;
      bool checking =  vertexSearch((m_clustersOnDetPlane->find(0)->second)[0],v);
      CAGraph::out_edge_iter begin,end;
      boost::tie(begin, end) = m_graph.getOutEdges(v);
      for (CAGraph::out_edge_iter i = begin; i != end; ++i)
      {   
      std::cout << "FOR THE " << m_noPlanes-1 << " PLANE THERE ARE THE FOLLOWING CELLS:-"<<std::endl;
      std::cout <<  m_graph.properties(*i).cell << std::endl;
      std::cout << m_graph.properties(m_graph.getSource(*i)).cluster->globalZ() << " --> "
      << (m_graph.properties(m_graph.getTarget(*i))).cluster->globalZ() << std::endl;
      }*/


    //CAGraph::in_edge_iter begin, end;
    //CAGraph::Vertex v;
    //vertexSearch((m_clustersOnDetPlane->find(1)->second)[0], v);
    //boost::tie(begin,end) = in_edges(v,m_graph.getGraph());
    // Starting at the 1st plane iterate to the nth-1 
    //
    //   for(size_t i(m_noPlanes-1); i > 0; --i)
    //bool OK;
    //do{
    //for (size_t i(2); i<m_noPlanes;++i) 
    for(size_t weight(m_noPlanes-1); weight >= m_minCellsForTrack ; --weight)
    {
        bool track = maketracks(weight);
        if(track) std::cout <<  "Tracks iter OK" << std::endl;
        else std::cout <<  "Tracks iter bad" << std::endl;
    }
    //std::cout << std::endl;
    std::cout << std::endl;
    //std::cout << std::endl;
    //CA::print_dep(std::cout, m_graph);
    /*
                                         std::cout <<  "ENTER LOOP ON PLANE: " << i << std::endl;
                                         for(int maxWeight(m_noPlanes); maxWeight>=m_minCellsForTrack; --maxWeight)
                                         {
    // Execute the track finding algorithm.
    //size_t i(m_noPlanes-1); 
    std::cout <<  "SEARCHING FOR WEIGHTS: " << maxWeight-1 << std::endl;
    OK = tracks(i,maxWeight);
    std::cout <<  "Just ran tracks algo" << std::endl;
    if(OK)  std::cout <<  "Tracks iter OK" << std::endl;
    else
    {
    std::cout <<  "Tracks iter is fucked after " << maxWeight <<"  iteration/s" << std::endl;

    continue;
    }
    }
    }
    //--maxWeight;
    //}while(OK==true && maxWeight >=m_minCellsForTrack && i>0 );
    std::cout << "Size of m_prototracks is: " << m_protoTracks->size() << std::endl;
    */
    //TestBeamProtoTracks::iterator iter = m_protoTracks->begin();
    //std::cout << "No Clusters on ProtoTrack: " << m_protoTracks->getNumClustersOnTrack() << std::endl;
    }

//**************************************************************
void CA::CellAutomata::weightSearch(const int weight, std::vector<CAGraph::Edge>& edges)
{
    // find all edges with the given weight
    CAGraph::edge_iter ei, edge_end;
    for (boost::tie(ei,edge_end) = m_graph.getEdges(); ei != edge_end; ++ei)
    {
        if( m_graph.properties(*ei).cell.getWeight() == weight )
        {
            edges.push_back(*ei);
        }
    }
}

//**************************************************************
bool CA::CellAutomata::search_RightEdge_given_LeftEdge(CAGraph::Edge& left, CAGraph::Edge& right)
{
    CAGraph::edge_iter ei, edge_end;
    CAGraph::Edge smallest_angle_edge;
    double angle(1.e2);
    bool found(false);
    for (boost::tie(ei,edge_end) = m_graph.getEdges(); ei != edge_end; ++ei)
    {
        // may need to take properties of src and targ as =operator
        //if( ( m_graph.properties(m_graph.getTarget(*ei)).cluster == m_graph.properties(m_graph.getSource(right)).cluster ) && ( m_graph.properties(right).cell.getWeight() == m_graph.properties(*ei).cell.getWeight()+1 ) )   
        if( ( m_graph.getTarget(*ei) == m_graph.getSource(right) ) && ( m_graph.properties(right).cell.getWeight() == m_graph.properties(*ei).cell.getWeight()+1 ) )   
        {
            //std::cout<<"search for left edge!!"<< std::endl;
            //std::cout << "right edge has weight!!"<< m_graph.properties(right).cell.getWeight()<< std::endl;
            //std::cout << "left edge has weight!!"<< m_graph.properties(*ei).cell.getWeight()<< std::endl;
            if (angle_between_cells_RL(m_graph.properties(*ei).cell, m_graph.properties(right).cell) < angle)
            {
                angle = angle_between_cells_RL(m_graph.properties(*ei).cell, m_graph.properties(right).cell);
                left = *ei;
                found = true;
            }
        }
    }
    
    if(found) return true;
    return false;
}

//**************************************************************
bool CA::CellAutomata::vertexSearch(TestBeamCluster* clust, CAGraph::Vertex& v)
{
    Vertex vertex; vertex.cluster = clust;
    CAGraph::vertex_iter begin, end;
    boost::tie(begin, end) = m_graph.getVertices();
    // find the seed vertex
    for (CAGraph::vertex_iter i = begin; i != end; ++i)
    {
        CAGraph::Vertex ve = *i;// only grab vertices that match the query
        if (m_graph.properties(ve)==vertex)  
        {
            v = *i;
            return true;
        }
        // no point looping once found 
    }
    return false;
}

//**************************************************************
bool CA::CellAutomata::sort_weight(const CAGraph::Edge& left, const CAGraph::Edge& right)
{
    Edge leftE = m_graph.properties(left);
    Edge rightE = m_graph.properties(right);

    return leftE.cell.getWeight() < rightE.cell.getWeight();
}

//**************************************************************
bool CA::CellAutomata::nextCellforTrack(CAGraph::Vertex& v, std::list<CAGraph::Edge>& edges, int& previous_weight)
{
    std::cout << "Entering nextCellForTrack!!!! " <<std::endl;
    //std::cout << "Previous WEIGHT was: " << previous_weight << std::endl;

    CAGraph::edge_set edgeset;
    collect_out_edges(v,edgeset);
    if (edgeset.empty())
    {
        std::cout << "No further edges - end of plane " << std::endl;
        return false;
    }

    //int weight(0);
    CAGraph::Edge highest_edge;
    // loop through out going edges to get the one with the highest
    // weight, sort both the weight and the edge
    int highest_weight(0);
    bool found;//(false);
    for (CAGraph::edge_set::iterator c_iter = edgeset.begin( ); c_iter != edgeset.end( ); ++c_iter)
    {
        //edges.push_back(*c_iter);
        Cell cell = m_graph.properties(*c_iter).cell;
        //std::cout << "iterating though out edges " << std::endl;
        //std::cout << m_graph.properties(m_graph.getSource(*c_iter)).cluster->globalZ() << " --> " << (m_graph.properties(m_graph.getTarget(*c_iter))).cluster->globalZ() << std::endl;
        // wish to return the maximum weighted edge
        if ((m_graph.properties(*c_iter).cell.getWeight() == previous_weight-1) ) 
        {
            //std::cout << "if statement success " << std::endl;
            highest_weight = cell.getWeight();
            highest_edge = *c_iter;
            //++next_weight;
            found = true;
        }
        else found=false;
        //CAGraph::Edge;
    }
    if(!found)return false;
    edges.push_back(*(edgeset.find(highest_edge)));
    //std::cout << "pushback went OK:...edges size " << edges.size() << std::endl;
    Cell cell = m_graph.properties(highest_edge).cell;
    //std::cout << "Edge assignment also OK, cell is:... " << cell << std::endl;
    //TestBeamCluster* Lcluster = dynamic_cast<TestBeamCluster*>((m_graph.properties(highest_edge)).cell.getLeftCluster());
    //TestBeamCluster* cluster = ((m_graph.properties(temp_edge)).cell.getRightCluster());
    bool foundV = vertexSearch( m_graph.properties(highest_edge).cell.getLeftCluster(), v );
    //if(!foundV) return false;
    if (found && foundV) 
    {
        previous_weight = highest_weight;
        return true;
    }
    else return false;
    //std::cout << " " << std::endl;
    //std::sort(edges.begin(), edges.end(), sort_weight);
    //std::sort(edgeset.begin(), edgeset.end(), sort_weight);
}

//**************************************************************
bool CA::CellAutomata::maketracks(const int weight)
{
    bool tracksfound(false);
    std::vector<CAGraph::Edge> weighted_edges;
    weightSearch(weight, weighted_edges);

    if(weighted_edges.empty()) 
    {
        std::cout<<"NO Weights found!!"<< std::endl;
        return false;
    }

    std::vector<CAGraph::Edge>::const_iterator it = weighted_edges.begin();
    const std::vector<CAGraph::Edge>::const_iterator endit = weighted_edges.end();
    for (;  it != endit; ++it)
    {
        //std::cout<<"INSIDE LOOP MAKETRAKCS!!"<< std::endl;
        //std::cout<<"Current weight is !!" << m_graph.properties(*it).cell.getWeight() << std::endl;
        TestBeamProtoTrack *temp_track = 0;
        temp_track = new TestBeamProtoTrack;
        temp_track->clearTrack();

        temp_track->addClusterToTrack(m_graph.properties(*it).cell.getRightCluster());
        temp_track->addClusterToTrack(m_graph.properties(*it).cell.getLeftCluster());

        CAGraph::Edge left;
        CAGraph::Edge right = (*it);
        int count(0);
        //int c(m_noPlanes-2);
        bool found(false);
        do{
            found = search_RightEdge_given_LeftEdge(left, right);
            if (found) 
            {
                if(count == 0)
                {
                    m_graph.RemoveEdge(*it);
                    count=1;
                }


                temp_track->addClusterToTrack(m_graph.properties(left).cell.getLeftCluster());
                if(count > 0)m_graph.RemoveEdge(right);
                right = left;
            }
        }while(found);

        if(temp_track->getNumClustersOnTrack() >= m_parameters->numclustersontrack) 
        {
            m_protoTracks->push_back(temp_track);
            tracksfound = true;//   else continue;
            //delete temp_track; temp_track=0;
        }
    }

    if(tracksfound) return true;
    else return false;
}

// if(edge.cell.getLeftCluster() == clusters.end()) continue;
//**************************************************************
bool CA::CellAutomata::tracks(size_t planeNo, int weight)
{
    // loop through the graph and count down from end plane from 
    // highest weight down to the lowest, remove the edges as theyre found
    // to make the next iteration faster.
    //RemoveEdge(const Edge& e)
    //RemoveEdge(const Vertex& u, const Vertex& v)
    //std::cout<<"Entered TRACKS ALGO!!"<< std::endl;

    VecCluster::const_iterator succeedit =  m_clustersOnDetPlane->find(planeNo)->second.begin();
    const VecCluster::const_iterator endsucceedit =  m_clustersOnDetPlane->find(planeNo)->second.end();

    bool tracksfound(false);
    // pass in the clusters from nth plane and find the corresponding vertex
    for(; succeedit != endsucceedit; ++succeedit)
    {
        //Vertex vertex; vertex.cluster = (*succeedit);
        CAGraph::Vertex v;
        vertexSearch(*succeedit, v );

        std::list<CAGraph::Edge> edges;
        // set the initial weight to find
        bool ok(true);
        //do
        //{
        //std::cout << "INSIDE DO-WHILE LOOP:    "<< std::endl;
        do
        {
            //std::cout<<"INSIDE WHILE LOOP!!!!"<< std::endl;
            //std::cout<<"Weight is: "<< weight << std::endl;
            if(weight == 1) {ok=false;break;};//weight=holdhighWeight;
            ok = nextCellforTrack(v, edges, weight);
        }while(ok);
        //}
        //while( (ok==true) && (edges.size()<m_noPlanes) );

        //if(edges.empty()) continue;
        if(edges.size()<=m_minCellsForTrack) continue;
        // Create instance of TestBeamProtoTrack to add clusters
        TestBeamProtoTrack *temp_track = 0;
        temp_track = new TestBeamProtoTrack;
        temp_track->clearTrack();

        std::list<CAGraph::Edge>::iterator it = edges.begin();
        const std::list<CAGraph::Edge>::const_iterator endit = edges.end();
        int begin(0);
        for(; it != endit; ++it)
        {
            if (begin == 0) 
            {
                //std::cout << "Cells are:    "<< std::endl;
                //std::cout << m_graph.properties(*it).cell << std::endl;
                TestBeamCluster* useRCluster = m_graph.properties(*it).cell.getRightCluster();
                temp_track->addClusterToTrack(useRCluster);
            }
            TestBeamCluster* useLCluster = m_graph.properties(*it).cell.getLeftCluster();
            //std::cout << m_graph.properties(*it).cell << std::endl;
            temp_track->addClusterToTrack(useLCluster);
            m_graph.RemoveEdge(*it);
            begin=1;
        }
        if(temp_track->getNumClustersOnTrack() >= m_parameters->numclustersontrack) 
        {
            m_protoTracks->push_back(temp_track);
            tracksfound = true;//   else continue;
            //delete temp_track; temp_track=0;
        }
        //delete temp_track; temp_track=0;
    }
    //CAGraph::edge_iter begin, end;
    if(tracksfound) return true;
    else return false;
}

//**************************************************************
void CA::CellAutomata::collect_out_edges( const CAGraph::Vertex& vertex, CAGraph::edge_set& accumulator)
{
    CAGraph::out_edge_iter begin, end;
    boost::tie(begin, end) = out_edges(vertex,m_graph.getGraph());
    //boost::tie(begin, end) = m_graph.getOutEdges(vertex);
    for (CAGraph::out_edge_iter i = begin; i != end; ++i)
    {
        if (accumulator.find(*i) == accumulator.end())
        {
            accumulator.insert(*i);
            collect_out_edges(m_graph.getTarget(*i), accumulator);
        }
    }
}

//**************************************************************
void CA::CellAutomata::nextCA(const size_t starting_plane)
{
    // The best way to do this part would be to use a standard boost graph search.
    // Such as the visitor example where you can return all connected vertices given a
    // query. This would be the most efficient as oppossed to multiple nested loops
    // and is most likely written in a much more efficient manor.
    // start with the final plane and work backwards 8->7->6->...->3. finish at 3 as 
    // we need to search two planes further down then network to see if they lie 
    // inside the angular acceptance
    //std::cout<< "Call to Next CA and the size of the VecCluster is: " << m_noPlanes <<  std::endl;

    VecCluster::iterator preceedit =  m_clustersOnDetPlane->find(starting_plane)->second.begin();
    const VecCluster::const_iterator endpreceedit =  m_clustersOnDetPlane->find(starting_plane)->second.end();

    // Thickness of plane is important for scattering angle acceptance
    //double planethickness = m_parameters->telescope_thickness;
    double xOverX0 = m_parameters->xOverX0;
    std::string isDUT = ((m_clustersOnDetPlane->find(starting_plane+1)->second)[0])->element()->detectorId();
    if(isDUT == m_parameters->dut) 
    {
        xOverX0 = m_parameters->xOverX0_dut;
    }

    //std::cout<< "Continue Next CA" << std::endl;
    // pass in the clusters from nth plane and find the corresponding vertex
    //std::cout<< "print preeceedit" << std::endl;
    for(; preceedit != endpreceedit; ++preceedit)
    {
        //std::cout<< "begin vertex search with query" << std::endl;
        //std::cout<< "start searching for vertex match" << std::endl;
        //std::cout<<"Cluster has z position "<<(*preceedit)->global_z<<std::endl;
        // find the corresponding left vertex
        CAGraph::Vertex vL;
        bool found_vertex = vertexSearch( *preceedit, vL );
        /*if (found_vertex)  std::cout<<"Found vertex that matches Left cluster!"<<std::endl;
        else std::cout<<"RUBBISH NO VERTICES FOUND!!!!"<<std::endl;*/
        // once matched vertex find its out going edges (directed graph so ok))
        // loop through these one at a time.
        CAGraph::edge_set edgesetL;
        collect_out_edges(vL,edgesetL);
        for (CAGraph::edge_set::iterator eL = edgesetL.begin( ); eL != edgesetL.end( ); ++eL)
        {
            Edge cellL = m_graph.properties(*eL);
            //std::cout<<"Outgoing edges are "<< std::endl;
            //std::cout << cellL.cell <<std::endl;

            // find the corresponding right vertex
            CAGraph::Vertex vR;
            found_vertex = vertexSearch( cellL.cell.getRightCluster(), vR ); //dhynds from Right to Left
            /*if (found_vertex)  std::cout<<"Found vertex that matches Right cluster!"<<std::endl;
            else std::cout<<"RUBBISH NO VERTICES FOUND!!!!"<<std::endl;*/
            CAGraph::edge_set edgesetR;
            collect_out_edges(vR,edgesetR);

            // similary now compare the angles between the single L cell and the set of
            // R cells
            for (CAGraph::edge_set::iterator eR = edgesetR.begin( ); eR != edgesetR.end(); ++eR)
            {
                Edge cellR = m_graph.properties(*eR);
                //std::cout<<"Right hand cell is"<<std::endl;
                //std::cout<<cellR.cell<<std::endl;
                //if( ( angle_between_cells_LR( cellL.cell, cellR.cell ) == 0.) && ( cellL.cell.getWeight() == cellR.cell.getWeight() ) )
                if( ( angle_between_cells_LR( cellL.cell, cellR.cell ) <= moliere(xOverX0) ) && ( cellL.cell.getWeight() == cellR.cell.getWeight() ) )
                    //if( ( angle_between_cells_LR( cellL.cell, cellR.cell ) != 0 ) )
                {
                    //std::cout<< "Found cell to update" << std::endl;
                    //std::cout<<"angle between cells is " << (angle_between_cells_LR( cellL.cell, cellR.cell )* TMath::RadToDeg()) << "*deg and moliere angle is " << moliere(xOverX0)*TMath::RadToDeg() << "*deg" << std::endl;
                    //std::cout<< "right cell update status " << cellR.cell.to_update()<<std::endl;
                    //std::cout<<"The left cell has weight "<<cellL.cell.getWeight() << " and the right cell has weight " << cellR.cell.getWeight()<<std::endl;
                    //std::cout << "Cell should be false... " <<m_graph.properties(*eL).cell.to_update() << std::endl;// mark the R cell for update and modify the graph
                    cellR.cell.to_update(true); //dhynds changed R to L
                    m_graph.ModifyEdge( *eR, cellR);
                    //std::cout << "Update cell should be true(1)... " <<m_graph.properties(*eR).cell.to_update() << std::endl;
                }

            }
        }
    }

}

//**************************************************************
bool CA::CellAutomata::update(size_t i)
{

    bool areAnyToBeUpdated(false);

    for (size_t n(i); n < (m_noPlanes-1); ++n)
    {
        VecCluster::iterator preceedit =  m_clustersOnDetPlane->find(n)->second.begin();
        const VecCluster::const_iterator endpreceedit =  m_clustersOnDetPlane->find(n)->second.end();

        for(; preceedit != endpreceedit; ++preceedit)
        {
            CAGraph::Vertex vL;
            bool found_vertex = vertexSearch( *preceedit, vL );
            /*if (found_vertex)  std::cout<<"Found vertex that matches Left cluster!"<<std::endl;
            else std::cout<<"RUBBISH NO VERTICES FOUND!!!!"<<std::endl;*/
            // once matched vertex find its out going edges (directed graph so ok))
            // loop through these one at a time.
            CAGraph::edge_set edgesetL;
            collect_out_edges(vL,edgesetL);
            for (CAGraph::edge_set::iterator eL = edgesetL.begin( ); eL != edgesetL.end( ); ++eL)
            {
                Edge cellL = m_graph.properties(*eL);

                // find the corresponding right vertex
                CAGraph::Vertex vR;
                found_vertex = vertexSearch( cellL.cell.getRightCluster(), vR ); //dhynds from Right to Left
                /*if (found_vertex)  std::cout<<"Found vertex that matches Right cluster!"<<std::endl;
                else std::cout<<"RUBBISH NO VERTICES FOUND!!!!"<<std::endl;*/
                CAGraph::edge_set edgesetR;
                collect_out_edges(vR,edgesetR);

                // similary now compare the angles between the single L cell and the set of
                // R cells
                for (CAGraph::edge_set::iterator eR = edgesetR.begin( ); eR != edgesetR.end(); ++eR)
                {
                    Cell cellR = m_graph.properties(*eR).cell;

                    if( cellL.cell.getWeight() == cellR.getWeight() && cellR.to_update())
                    {
                        areAnyToBeUpdated = true;
                        cellR.addOneToWeight();
                        cellR.to_update(false); // reset the new cell
                        Edge join; join.cell = cellR;
                        m_graph.ModifyEdge( *eR, join);
                    }
                }
            }
        }
    }
    if(areAnyToBeUpdated) return true;
    else return false;
}

//**************************************************************
bool CA::CellAutomata::update()
{
    // Loop through all Edges and see if they have been set for update. If so add 1
    CAGraph::edge_iter ei, edge_end;

    // use a flag to note once all Cells are updated
    bool areAnyToBeUpdated(false);
    //std::cout<<"Update all edges if boolean true"<<std::endl;
    for (boost::tie(ei,edge_end) = m_graph.getEdges(); ei != edge_end; ++ei)
    {
        //std::cout<<"loop edges"<<std::endl;

        Cell cell = m_graph.properties(*ei).cell;
        if(cell.to_update()==true) 
        {
            areAnyToBeUpdated = true;
            cell.addOneToWeight();
            cell.to_update(false); // reset the new cell
            Edge join; join.cell = cell;
            m_graph.ModifyEdge( *ei, join);
            //std::cout<<"Updating cell"<<std::endl<< cell <<std::endl;
            //std::cout<<"Cell updated, weight is now: " << m_graph.properties(*ei).cell.getWeight()<< std::endl;
        }
    }

    if(areAnyToBeUpdated) return true;
    else return false;
}

//**************************************************************
bool CA::CellAutomata::checkSucceedingPlane(size_t planeNo, VecCluster& result, TestBeamCluster* seedcluster, const double radius)
{
    //KDTree* nn = dynamic_cast<KDTree*>(KDTreeinit[planeNo]);
    //KDTree* nn = dynamic_cast<KDTree*>(KDTreeinit[planeNo]);
    KDTreeinit[planeNo-1]->allNeighboursInRadius( seedcluster, radius, result);
    //VecCluster succeedingPlane = m_clustersOnDetPlane->find(planeNo)->second;
    //KDTree* nn = new KDTree(succeedingPlane);
    //nn->allNeighboursInRadius( seedcluster, radius, result);
    //delete nn; nn=0;
    if(!result.empty()) return true;
    else return false;
}

//**************************************************************
double CA::Angle(const XYZVector & v1, const XYZVector & v2)
{
    // return the angle w.r.t. another 3-vector
    double ptot2 = v1.Mag2()*v2.Mag2();
    if(ptot2 <= 0) 
    {
        return 0.0;
    }
    else 
    {
        double arg = v1.Dot(v2)/std::sqrt(ptot2);
        if(arg >  1.0) arg =  1.0;
        if(arg < -1.0) arg = -1.0;
        return std::acos(arg);
    } 
}

//**************************************************************
double CA::CellAutomata::angle_between_cells_RL(Cell leftCell, Cell rightCell)
{
    // a.b = |a| |b| cos(theta)
    // we have the normal vector of each cell pointing R->L hence
    // theta = cos^(-1)( n_a . n_b )
    return CA::Angle( leftCell.direction_norm_RL(), rightCell.direction_norm_RL() );
}

//**************************************************************
double CA::CellAutomata::angle_between_cells_LR(Cell leftCell, Cell rightCell)
{
    // a.b = |a| |b| cos(theta)
    // we have the normal vector of each cell pointing R->L hence
    // theta = cos^(-1)( n_a . n_b )
    return CA::Angle( leftCell.direction_norm_LR(), rightCell.direction_norm_LR() );
}

//**************************************************************
//*****************************************************************************************************************
double CA::CellAutomata::moliere(const double xOverX0)
{
    // v/c ratio say it is almost speed of light at
    // Particle momentum [MeV/c]
    double momentum = m_parameters->momentum ;

    // Particle mass [MeV]
    double mass = PhysicalConstants::pion_mass_c2 ;
    if (m_parameters->particle == "mu")
        mass = PhysicalConstants::muon_mass_c2 ;
    if (m_parameters->particle == "e")
        mass = PhysicalConstants::electron_mass_c2 ;
    if (m_parameters->particle == "p")
        mass = PhysicalConstants::proton_mass_c2 ;

    // Particle charge using +- particles
    double charge = 1;
    // Particle energy [MeV]
    double energy = sqrt(mass * mass + momentum * momentum);
    double gamma = energy / mass;
    double beta = sqrt( 1. - 1. / (gamma * gamma));

    const double c_highland = 13.6 * SystemOfUnits::MeV;

    double betacp = beta * momentum;
    double theta0 = c_highland * std::abs(charge) * sqrt(xOverX0)/betacp;

    //xOverX0 = log(xOverX0);
    //theta0 *= sqrt(1. + xOverX0 * (0.105 + 0.0035* xOverX0) );
    theta0 *= (1. + 0.038 * log(xOverX0));

    //if (m_debug)
    //    std::cout << " *** Mult. scattering angle (deg): " << theta0 / SystemOfUnits::degree << std::endl;

    return theta0;
}

