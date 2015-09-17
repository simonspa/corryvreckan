// $Id: Analysis.h,v 1.5 2009-08-08 13:51:55 mjohn Exp $

// Include files
#include <vector>
#include <map>

#include "Algorithm.h"
#include "Clipboard.h"
#include "Parameters.h"
#include "TestBeamEvent.h"


/** @class Analysis Analysis.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-01
 */

#ifndef ANALYSIS_H 
#define ANALYSIS_H 1

class Analysis 
{
    public: 
        Analysis(){}
        Analysis(Parameters*);
        virtual ~Analysis();
        void add(Algorithm*);
        void run();
        void timing(int);
    protected:

    private:
        Parameters* parameters;
        Clipboard* clipboard;
        std::vector<Algorithm*> algorithms;
};
#endif // ANALYSIS_H
