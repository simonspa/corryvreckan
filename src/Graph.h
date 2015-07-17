#ifndef GRAPH_H
#define GRAPH_H 1

#include <vector>
#include <algorithm>
#include <set>

#include "boost/config.hpp"
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/adjacency_list_io.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/property_map/property_map.hpp"
#include "boost/graph/graph_utility.hpp" // for boost::make_list
#include "boost/graph/property_iter_range.hpp"
#include "boost/graph/lookup_edge.hpp"
#include "boost/graph/adjacency_iterator.hpp"

/** @class Graph Graph.h
*  
* 	2011-12-20 : Matthew Reid 
* 	Comments: 
* 	    This is a wrapper to the boost graph class for an adjacency list. Allows us to connect a 
* 	    series of vertexes (clusters) via an attachment known as an edge (Cell class which main purpose
* 	    is to provide the weighting of the edge). Here we use a directed graph, storing a std::set of edges as 
* 	    only need one in one direction (non parallel) and a std::list of vertices for easy removal. A property map
* 	    is implemented as a template and various accessor functions are defined.
*
*/

using namespace boost;

/* definition of basic boost::graph properties */
enum vertex_properties_t { vertex_properties };
enum edge_properties_t { edge_properties };

namespace boost {
        BOOST_INSTALL_PROPERTY(vertex, properties);
        BOOST_INSTALL_PROPERTY(edge, properties);
}

/*template<class T>
bool equal_to(const T& first, const T& second)
{
    if(first == second) return true;
    else return false;
}*/

template <typename vertex_t>
class ConsolePrinter 
{
    public:
        template <typename iter_t>
        void print(iter_t vbegin, iter_t vend) const 
        {
            std::copy(vbegin, vend, std::ostream_iterator<vertex_t>(std::cout, "\r\n"));
        }
};

/* the graph base class template */
template < typename VERTEXPROPERTIES, typename EDGEPROPERTIES >
class Graph
{
    public:

    /* an adjacency_list like we need it */
    typedef adjacency_list<
        setS, // disallow parallel edges, edge container
        setS,//listS, // vertex container
        directedS, // directed graph
        property<vertex_properties_t, VERTEXPROPERTIES>,
        property<edge_properties_t, EDGEPROPERTIES>
            > GraphContainer;


    /* a bunch of graph-specific typedefs */
    typedef typename graph_traits<GraphContainer>::vertex_descriptor Vertex;
    typedef typename graph_traits<GraphContainer>::edge_descriptor Edge;
    typedef std::pair<Edge, Edge> EdgePair;

    typedef typename graph_traits<GraphContainer>::vertex_iterator vertex_iter;
    typedef typename graph_traits<GraphContainer>::edge_iterator edge_iter;
    typedef typename graph_traits<GraphContainer>::adjacency_iterator adjacency_iter;
    typedef typename graph_traits<GraphContainer>::out_edge_iterator out_edge_iter;
    typedef typename graph_traits<GraphContainer>::in_edge_iterator in_edge_iter;

    typedef typename graph_traits<GraphContainer>::degree_size_type degree_t;

    typedef std::pair<adjacency_iter, adjacency_iter> adjacency_vertex_range_t;
    typedef std::pair<out_edge_iter, out_edge_iter> out_edge_range_t;
    typedef std::pair<in_edge_iter, in_edge_iter> in_edge_range_t;
    typedef std::pair<vertex_iter, vertex_iter> vertex_range_t;
    typedef std::pair<edge_iter, edge_iter> edge_range_t;
    typedef std::set< Edge > edge_set;
    typedef std::list< Vertex > vertex_list;

    /* constructors etc. */
    Graph()
    {}
    
    Graph(const int& init) : initalVs(init), graph(init)
    {}
    
    Graph(const Graph& g) :
        graph(g.graph)
    {}

    virtual ~Graph()
    {}

    /*vertex_t find_vertex_named(const graph_t& graph, const std::string& name)
    {
        graph_t::vertex_iterator begin, end;
        boost::tie(begin, end) = vertices(graph);
        for (graph_t::vertex_iterator i = begin; i != end; ++i)
        {
            if (get(vertex_name, graph, *i) == name)
                return *i;
        }

        return -1;
    }*/

    /* structure modification methods */
    void Clear()
    {
        graph.clear();
    }

    Vertex AddVertex(const VERTEXPROPERTIES& prop)
    {
        Vertex v = add_vertex(graph);
        properties(v) = prop;
        return v;
    }

    void RemoveVertex(const Vertex& v)
    {
        clear_vertex(v, graph);
        remove_vertex(v, graph);
    }

    void RemoveEdge(const Edge& e)
    {
        remove_edge(e, graph);
    }

    void ModifyEdge(const Edge& e, const EDGEPROPERTIES& value)
    {
        properties(e) = value;
    }

    void RemoveEdge(const Vertex& u, const Vertex& v)
    {
        remove_edge(u, v, graph);
    }

    Edge AddEdge(const Vertex& v1, const Vertex& v2, const EDGEPROPERTIES& prop_12)
    {
        /* TODO: maybe one wants to check if this edge could be inserted */
        Edge addedEdge1 = add_edge(v1, v2, graph).first;

        properties(addedEdge1) = prop_12;

        return addedEdge1;
    }

    EdgePair AddEdge(const Vertex& v1, const Vertex& v2, const EDGEPROPERTIES& prop_12, const EDGEPROPERTIES& prop_21)
    {
        /* TODO: maybe one wants to check if this edge could be inserted */
        Edge addedEdge1 = add_edge(v1, v2, graph).first;
        Edge addedEdge2 = add_edge(v2, v1, graph).first;

        properties(addedEdge1) = prop_12;
        properties(addedEdge2) = prop_21;

        return EdgePair(addedEdge1, addedEdge2);
    }

    /* property access */
    VERTEXPROPERTIES& properties(const Vertex& v)
    {
        typename property_map<GraphContainer, vertex_properties_t>::type param = get(vertex_properties, graph);
        return param[v];
    }

    const VERTEXPROPERTIES& properties(const Vertex& v) const
    {
        typename property_map<GraphContainer, vertex_properties_t>::const_type param = get(vertex_properties, graph);
        return param[v];
    }

    EDGEPROPERTIES& properties(const Edge& e)
    {
        typename property_map<GraphContainer, edge_properties_t>::type param = get(edge_properties, graph);
        return param[e];
    }

    const EDGEPROPERTIES& properties(const Edge& e) const
    {
        typename property_map<GraphContainer, edge_properties_t>::const_type param = get(edge_properties, graph);
        return param[e];
    }

    edge_set out_edges_from_vertex(const VERTEXPROPERTIES& focus_vertex_name)
    {
        Vertex focus_vertex;
        vertex_iter begin, end;
        boost::tie(begin, end) = getVertices();
        for (vertex_iter i = begin; i != end; ++i)
        {
            //if ( equal_to(properties(*i),name))  return *i;
            if ( properties(*i)==focus_vertex_name)  focus_vertex = *i;
        }

        edge_set edges;
        collect_out_edges(focus_vertex, edges);

        return edges;
    }

    // Helpers
    void collect_out_edges( const Vertex& vertex, edge_set& accumulator)
    {
        out_edge_iter begin, end;
        boost::tie(begin, end) = getOutEdges(vertex);
        for (out_edge_iter i = begin; i != end; ++i)
        {
            if (accumulator.find(*i) == accumulator.end())
            {
                accumulator.insert(*i);
                collect_out_edges(getTarget(*i), accumulator);
            }
        }
    }

    Edge find_edge_named(const EDGEPROPERTIES& name)
    {
        edge_iter begin, end;
        boost::tie(begin, end) = getEdges();
        for (edge_iter i = begin; i != end; ++i)
        {
            if (properties(*i)==name) return *i;
        }
        return -1;
    }

        
    /* selectors and properties */
    const GraphContainer& getGraph() const
    {
        return graph;
    }

    // target is a vertex lower down the network (a priori)
    Vertex getTarget(const Edge& edge)
    {
        return target(edge, graph);
    }

    // source is a vertex higher up the network (a posteriori)
    Vertex getSource(const Edge& edge)
    {
        return source(edge, graph);
    }
 
    // return edges in this example will be the same as out_edge as directed graph
    edge_range_t getEdges() const
    {
        return edges(graph);
    }

    // return outgoing edges from vertex
    out_edge_range_t getOutEdges(const Vertex& vertex) const
    {
        return out_edges(vertex, graph);
    }
    
    // return ingoing edges from vertex
    in_edge_range_t getInEdges(const Vertex& vertex) const
    {
        return in_edges(vertex, graph);
    }

    // return outgoing edges from vertex
    int getEdgeCount() const
    {
        return num_edges(graph);
    }
    
    vertex_range_t getVertices() const
    {
        return vertices(graph);
    }

    adjacency_vertex_range_t getAdjacentVertices(const Vertex& v) const
    {
        return adjacent_vertices(v, graph);
    }

    int getVertexCount() const
    {
        return num_vertices(graph);
    }

    int getVertexDegree(const Vertex& v) const
    {
        return out_degree(v, graph);
    }

    int getInitVs() const
    {
        return initalVs;
    }

    /* operators */
    Graph& operator=(const Graph &rhs)
    {
        graph = rhs.graph;
        return *this;
    }

    protected:
    int initalVs;
    GraphContainer graph;
};

//**************************************************************
/*struct print_edge 
  {
  typedef typename Graph<VERTEXPROPERTIES, EDGEPROPERTIES> MyGraph;
  print_edge( MyGraph& g) : G(g) {}
  typedef typename Graph<VERTEXPROPERTIES, EDGEPROPERTIES>::Edge Edge;
  typedef typename Graph<VERTEXPROPERTIES, EDGEPROPERTIES>::Vertex Vertex;
  void operator() (Edge e) const
  {
  typedef boost::property_map<Graph, vertex_index_t>::type id = 
  Vertex src = 
  }
  MyGraph& G; 
  };*/

//**************************************************************
/*template<class VERTEXPROPERTIES, class EDGEPROPERTIES>
  std::ostream& operator<<(std::ostream& out, const Graph<VERTEXPROPERTIES, EDGEPROPERTIES>& g)
  {
  typedef Graph<VERTEXPROPERTIES, EDGEPROPERTIES> MyGraph;

  typename MyGraph::vertex_range_t vertices_range = g.getVertices(); 
  ConsolePrinter<typename MyGraph::Vertex> printer;   
  out << printer.print(vertices_range.first,vertices_range.second);
// out << depth_first_search( G, visitor(make_dfs_visitor(  ) ) ) << std::endl;
return out;
}*/

#endif // GRAPH_H
