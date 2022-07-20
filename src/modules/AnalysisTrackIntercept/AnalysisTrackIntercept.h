#ifndef CORRYVRECKAN_TRACKINTERCEPT_ANALYSIS_H
#define CORRYVRECKAN_TRACKINTERCEPT_ANALYSIS_H

#include <TDirectory.h>
#include <TH2F.h>
#include <vector>
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class AnalysisTrackIntercept : public Module {

    public:
        // Constructors and destructors
        AnalysisTrackIntercept(Configuration& config, std::vector<std::shared_ptr<Detector>> detector);
        ~AnalysisTrackIntercept() {}

        // Functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        int n_bins_x, n_bins_y;
        double xmin, xmax, ymin, ymax;

        std::vector<double> m_planes_z;
        std::vector<TH2F*> m_intercepts;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_TRACKINTERCEPT_ANALYSIS_H
