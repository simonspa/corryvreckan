#ifndef StraightLineTrack_H
#define StraightLineTrack_H 1

#include "Track.hpp"
namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief StraightLineTrack object
     *
     * This class is a simple track class which knows how to fit itself. It holds a collection of clusters, which may or may
     * not be included in the track fit.
     */

    class StraightLineTrack : public Track {

    public:
        // Constructors and destructors
        StraightLineTrack();

        // Minimisation operator used by Minuit. Minuit passes the current iteration of the parameters and checks if the chi2
        // is better or worse
        double operator()(const double* parameters);

        // Print an ASCII representation of the StraightLineTrack to the given stream
        void print(std::ostream& out) const override;

        // Fit the StraightLineTrack (linear regression)
        void fit() override;
        double distance2(const Cluster* cluster) const override;
        ROOT::Math::XYZPoint intercept(double z) const override;
        ROOT::Math::XYZPoint state(std::string) const override { return m_state; }
        ROOT::Math::XYZVector direction(std::string) const override { return m_direction; }

    private:
        // Calculate the chi2 of the StraightLineTrack
        void calculateChi2();

        // Member variables
        ROOT::Math::XYZVector m_direction;
        ROOT::Math::XYZPoint m_state;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(StraightLineTrack, 0)
    };

    // Vector type declaration
    using StraightLineTrackVector = std::vector<StraightLineTrack*>;
} // namespace corryvreckan

#endif // StraightLineTrack_H
