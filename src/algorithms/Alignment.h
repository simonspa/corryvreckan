#ifndef ALIGNMENT_H
#define ALIGNMENT_H 1

// ROOT includes
#include "Minuit2/Minuit2Minimizer.h"
#include "Math/Functor.h"
#include "TError.h"
// Local includes
#include "core/Algorithm.h"
#include "objects/Cluster.h"
#include "objects/Track.h"

namespace corryvreckan {
  class Alignment : public Algorithm {

  public:
    // Constructors and destructors
    Alignment(Configuration config, Clipboard* clipboard);
    ~Alignment(){}

    // Functions
    void initialise(Parameters*);
    StatusCode run(Clipboard*);
    void finalise();

    // Member variables
    Tracks m_alignmenttracks;
    int nIterations;
    int m_numberOfTracksForAlignment;

  };
}
#endif // ALIGNMENT_H
