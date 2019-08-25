#ifndef AlignmentMillepede_H
#define AlignmentMillepede_H 1

#include "core/module/Module.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {

    /** @ingroup Modules
     *
     *  Implementation of the Millepede algorithm.
     *
     *  @author Christoph Hombach
     *  @date   2012-06-19
     */
    class AlignmentMillepede : public Module {
    public:
        /// Constructor
        AlignmentMillepede(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        /// Destructor
        virtual ~AlignmentMillepede();

        void initialise();
        void finalise();
        StatusCode run(std::shared_ptr<Clipboard>);

        virtual void updateGeometry();

    private:
        struct Equation {
            double rmeas;
            double weight;
            std::vector<int> indG;
            std::vector<double> derG;
            std::vector<int> indL;
            std::vector<double> derL;
            std::vector<int> indNL;
            std::vector<double> derNL;
            std::vector<double> slopes;
        };
        struct Constraint {
            /// Right-hand side (Lagrange multiplier)
            double rhs;
            /// Coefficients
            std::vector<double> coefficients;
        };

        /// (Re-)initialise matrices and vectors.
        bool reset(const size_t nPlanes, const double startfact);
        /// Setup the constraint equations.
        void setConstraints(const size_t nPlanes);
        /// Define a single constraint equation
        void addConstraint(const std::vector<double>& dercs, const double rhs);
        /// Add the equations for one track and do the local fit.
        bool putTrack(Track* track, const size_t nPlanes);
        /// Store the parameters for one measurement.
        void addEquation(std::vector<Equation>& equations,
                         const std::vector<double>& derlc,
                         const std::vector<double>& dergb,
                         const std::vector<double>& dernl,
                         const std::vector<int>& dernli,
                         const std::vector<double>& dernls,
                         const double rmeas,
                         const double sigma);

        // Perform local parameters fit using the equations for one track.
        bool fitTrack(const std::vector<Equation>& equations,
                      std::vector<double>& trackParams,
                      const bool singlefit,
                      const unsigned int iteration);

        // Perform global parameters fit.
        bool fitGlobal();
        /// Print the results of the global parameters fit.
        bool printResults();

        /// Matrix inversion and solution for global fit.
        int invertMatrix(std::vector<std::vector<double>>& v, std::vector<double>& b, const size_t n);
        // Matrix inversion and solution for local fit.
        int invertMatrixLocal(std::vector<std::vector<double>>& v, std::vector<double>& b, const size_t n);

        /// Return the limit in chi2 / ndof for n sigmas.
        double chi2Limit(const int n, const int nd) const;
        /// Multiply matrix and vector
        bool multiplyAX(const std::vector<std::vector<double>>& a,
                        const std::vector<double>& x,
                        std::vector<double>& y,
                        const unsigned int n,
                        const unsigned int m);
        /// Multiply matrices
        bool multiplyAVAt(const std::vector<std::vector<double>>& v,
                          const std::vector<std::vector<double>>& a,
                          std::vector<std::vector<double>>& w,
                          const unsigned int n,
                          const unsigned int m);

        TrackVector m_alignmenttracks;
        size_t m_numberOfTracksForAlignment;

        /// Number of global derivatives
        unsigned int m_nagb;
        /// Number of local derivatives
        unsigned int m_nalc = 4;

        /// Degrees of freedom
        std::vector<bool> m_dofs;
        /// Default degrees of freedom
        std::vector<bool> m_dofsDefault;

        /// Equations for each track
        std::vector<std::vector<Equation>> m_equations;
        /// Constraint equations
        std::vector<Constraint> m_constraints;

        ///  Flag for each global parameter whether it is fixed or not.
        std::vector<bool> m_fixed;
        /// Sigmas for each global parameter.
        std::vector<double> m_psigm;

        std::vector<std::vector<double>> m_cgmat;
        std::vector<std::vector<double>> m_corrm;
        std::vector<std::vector<double>> m_clcmat;

        std::vector<double> m_bgvec;
        std::vector<double> m_corrv;
        std::vector<double> m_diag;

        /// Difference in misalignment parameters with respect to initial values.
        std::vector<double> m_dparm;

        /// Mapping of internal numbering to geometry service planes.
        std::map<std::string, unsigned int> m_millePlanes;

        /// Flag to switch on/off iterations in the global fit.
        bool m_iterate = true;
        /// Residual cut after the first iteration.
        double m_rescut;
        /// Residual cut in the first iteration.
        double m_rescut_init;
        /// Factor in chisquare / ndof cut.
        double m_cfactr;
        /// Reference value for chisquare / ndof cut factor.
        double m_cfactref;
        // Number of standard deviations for chisquare / ndof cut.
        int m_nstdev;
        // Value of convergence to interrupting iterations
        double m_convergence;
        /// Number of "full" iterations (with geometry updates).
        size_t m_nIterations;
        /// Sigmas for each degree of freedom
        std::vector<double> m_sigmas;
        /// Planes to be kept fixed
        std::vector<unsigned int> m_fixedPlanes;
        /// Flag to fix all degrees of freedom or only the translations.
        bool m_fix_all;
        /// It can be also reasonable to include the DUT in the alignemnt
        bool m_excludeDUT;
    };
} // namespace corryvreckan

#endif // AlignmentMillepede_H
