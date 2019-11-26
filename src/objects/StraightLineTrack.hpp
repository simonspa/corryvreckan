#ifndef StraightLineTrack_H
#define StraightLineTrack_H 1

#include "Track.hpp"
namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief StraightLineTrack object
     *
     * This class is a simple track class which knows how to fit itself. It is dervied from Track
     */

    class StraightLineTrack : public Track {

    public:
        // Constructors and destructors
        StraightLineTrack();

        // copy constructor
        StraightLineTrack(const StraightLineTrack& track);

        virtual StraightLineTrack* copy() const override { return new StraightLineTrack(*this); }

        // Minimisation operator used by Minuit. Minuit passes the current iteration of the parameters and checks if the chi2
        // is better or worse
        double operator()(const double* parameters);

        void print(std::ostream& out) const override;

        /**
         * @brief The fiting routine
         */
        void fit() override;

        /**
         * @brief  Get the distance between cluster and track
         * @param Cluster* Pointer to the cluster
         * @return distance between cluster and track
         */
        double distance2(const Cluster* cluster) const override;

        /**
         * @brief Get the track position for a certain z position
         * @param z positon
         * @return ROOT::Math::XYZPoint at z position
         */
        ROOT::Math::XYZPoint intercept(double z) const override;

        /**
         * @brief Get the track state at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint state at detetcor layer
         */
        ROOT::Math::XYZPoint state(std::string) const override { return m_state; }

        /**
         * @brief Get the track direction at a detector
         * @param name of detector
         * @return ROOT::Math::XYZPoint direction at detetcor layer
         */
        ROOT::Math::XYZVector direction(std::string) const override { return m_direction; }

    private:
        /**
         * @brief calculate the chi2 of the linear regression
         */
        void calculateChi2();

        // Member variables
        ROOT::Math::XYZVector m_direction;
        ROOT::Math::XYZPoint m_state;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(StraightLineTrack, 1)
    };

    // Vector type declaration
    using StraightLineTrackVector = std::vector<StraightLineTrack*>;
} // namespace corryvreckan

#endif // StraightLineTrack_H
