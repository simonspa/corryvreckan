#ifndef KDTREE_WRAPPER_HPP
#define KDTREE_WRAPPER_HPP

#include <iosfwd>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <boost/multi_array.hpp>
#include <boost/array.hpp>

#include "TestBeamCluster.h"
#include "kdtree2.h"
//#include "Point.h"

//-----------------------------------------------------------------------------
// Class : KDTree
// 
// Info : Wrapper interface to the kdtree2 class, to be passed a vector of
// clusters on a given plane. Will return the kNN to a given seed point.
// 
// 2011-07-21 : Matthew Reid
//
//-----------------------------------------------------------------------------

// How to use:-
// initialise KDTree nn(<VecCluster> plane_i);
// VecCluster result(N);
// nn2.nearestNeighbours(<TestBeamCluster> seed, N, result);
// Where N is the number of nearest neighbours to be returned


// Define some typedefs for use later
typedef std::vector <TestBeamCluster* > VecCluster;

// build the class
class KDTree 
{
	public:
		typedef kdtree2::KDTreeResultVector KDTreeResultVector;


		explicit KDTree(const VecCluster& pts);
		~KDTree();

		void nearestNeighbours(TestBeamCluster* pt, int N, VecCluster& result);
		void allNeighboursInRadius(TestBeamCluster* pt, const double radius, VecCluster& result);

	private:
		void transformResults(KDTreeResultVector& vec, VecCluster& result);

		static const int k;
		boost::multi_array<double, 2> array; 
		kdtree2::KDTree* tree;
		VecCluster det;
};

bool distComparator(const kdtree2::KDTreeResult& a, const kdtree2::KDTreeResult& b);


#endif
