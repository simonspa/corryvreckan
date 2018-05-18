#include "RegionOfInterest.h"

using namespace corryvreckan;
using namespace std;

RegionOfInterest::RegionOfInterest(Configuration config, std::vector<Detector*> detectors)
    : Module(std::move(config), std::move(detectors)) {
    m_DUT = m_config.get<std::string>("DUT");
    m_roi = m_config.getMatrix<int>("roi", std::vector<std::vector<int>>());
}

StatusCode RegionOfInterest::run(Clipboard* clipboard) {

    // Get the telescope tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }

    // Get the DUT detector:
    auto detector = get_detector(m_DUT);

    // Loop over all tracks
    for(auto& track : (*tracks)) {
        // Check if it intercepts the DUT
        auto globalIntercept = detector->getIntercept(track);
        if(!detector->hasIntercept(track, 1.)) {
            LOG(DEBUG) << "Track outside DUT";
            track->set_within_roi(false);
            continue;
        }

        // If it does, we define the track as being inside the ROI
        track->set_within_roi(true);

        // Check that track is within region of interest using winding number algorithm
        auto localIntercept = detector->getLocalIntercept(track);
        auto coordinates = std::make_pair(detector->getColumn(localIntercept), detector->getRow(localIntercept));
        if(winding_number(coordinates, m_roi) == 0) {
            LOG(DEBUG) << "Track outside DUT ROI";
            track->set_within_roi(false);
        } else {
            LOG(DEBUG) << "Track within DUT ROI";
        }
    }

    return Success;
}

/* isLeft(): tests if a point is Left|On|Right of an infinite line.
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *    Input:  three points P0, P1, and P2
 *    Return: >0 for P2 left of the line through P0 and P1
 *            =0 for P2  on the line
 *            <0 for P2  right of the line
 *    See: Algorithm 1 "Area of Triangles and Polygons"
 */
int RegionOfInterest::isLeft(std::pair<int, int> pt0, std::pair<int, int> pt1, std::pair<int, int> pt2) {
    return ((pt1.first - pt0.first) * (pt2.second - pt0.second) - (pt2.first - pt0.first) * (pt1.second - pt0.second));
}

/* Winding number test for a point in a polygon
 * via: http://geomalgorithms.com/a03-_inclusion.html
 *      Input:   x, y = a point,
 *               polygon = vector of vertex points of a polygon V[n+1] with V[n]=V[0]
 *      Return:  wn = the winding number (=0 only when P is outside)
 */
int RegionOfInterest::winding_number(std::pair<int, int> probe, std::vector<std::vector<int>> polygon) {
    // Two points don't make an area
    if(polygon.size() < 3) {
        LOG(DEBUG) << "No ROI given.";
        return 0;
    }

    int wn = 0; // the  winding number counter

    // loop through all edges of the polygon

    // edge from V[i] to  V[i+1]
    for(int i = 0; i < polygon.size(); i++) {
        auto point_this = std::make_pair(polygon.at(i).at(0), polygon.at(i).at(1));
        auto point_next = (i + 1 < polygon.size() ? std::make_pair(polygon.at(i + 1).at(0), polygon.at(i + 1).at(1))
                                                  : std::make_pair(polygon.at(0).at(0), polygon.at(0).at(1)));

        // start y <= P.y
        if(point_this.second <= probe.second) {
            // an upward crossing
            if(point_next.second > probe.second) {
                // P left of  edge
                if(isLeft(point_this, point_next, probe) > 0) {
                    // have  a valid up intersect
                    ++wn;
                }
            }
        } else {
            // start y > P.y (no test needed)

            // a downward crossing
            if(point_next.second <= probe.second) {
                // P right of  edge
                if(isLeft(point_this, point_next, probe) < 0) {
                    // have  a valid down intersect
                    --wn;
                }
            }
        }
    }
    return wn;
}
