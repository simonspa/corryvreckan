#include "ImproveTimestamp.h"
#include "objects/SpidrSignal.h"

using namespace corryvreckan;
using namespace std;

ImproveTimestamp::ImproveTimestamp(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    m_method = m_config.get<int>("improvementMethod", 1);
}

void ImproveTimestamp::initialise() {
    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode ImproveTimestamp::run(Clipboard* clipboard) {

    // Recieved triggers
    std::vector<long long int> trigger_times;

    // Get trigger signals
    SpidrSignals* spidrData = (SpidrSignals*)clipboard->get("W0013_G02", "SpidrSignals");
    if(spidrData != NULL) {
        // Loop over all signals registered
        int nSignals = spidrData->size();
        for(int iSig = 0; iSig < nSignals; iSig++) {
            // Get the signal
            SpidrSignal* signal = (*spidrData)[iSig];
            if(signal->type() == "trigger") {
                trigger_times.push_back(signal->timestamp());
            }
        }
        LOG(DEBUG) << "Number of triggers found: " << trigger_times.size();
    }

    // Get the tracks from the clipboard
    Tracks* tracks = (Tracks*)clipboard->get("tracks");
    if(tracks == NULL) {
        LOG(DEBUG) << "No tracks on the clipboard";
        return Success;
    }
    LOG(DEBUG) << "Number of tracks found: " << tracks->size();

    // Loop over all tracks
    for(auto& track : (*tracks)) {

        long long int improved_time = track->timestamp();

        if(m_method == 0) {
            // Find trigger timestamp clostest in time
            long long int diff = 999999999999999;
            int imin = -1;
            if(trigger_times.size() > 0) {
                for(int i = 0; i < trigger_times.size(); i++) {
                    LOG(DEBUG) << " track: " << track->timestamp() << " trigger: " << trigger_times.at(i)
                               << " diff: " << abs((int)(trigger_times.at(i) - track->timestamp()))
                               << " diff stored cycles: " << diff
                               << " diff stored secs: " << (double)(diff) / (4096. * 40000000.);
                    if(abs((int)(trigger_times.at(i) - track->timestamp())) < diff) {
                        imin = i;
                        diff = abs((int)(trigger_times.at(i) - track->timestamp()));
                    }
                }
                improved_time = trigger_times.at(imin);
                // trigger latency is 175 ns, still missing
            }
        }

        if(m_method == 1) {
            int nhits = 0;
            long long int avg_track_time = 0;
            for(auto& cluster : track->clusters()) {
                avg_track_time += cluster->timestamp();
                nhits++;
            }
            avg_track_time = round(avg_track_time / nhits);
            LOG(DEBUG) << setprecision(12) << "Reference track time " << (double)(track->timestamp()) / (4096. * 40000000.);
            LOG(DEBUG) << setprecision(12) << "Average track time " << (double)(avg_track_time) / (4096. * 40000000.);
        }

        // Set improved reference timestamp
        track->setTimestamp(improved_time);

        LOG(DEBUG) << "End of track";
    }

    // Increment event counter
    m_eventNumber++;

    // Return value telling analysis to keep running
    LOG(DEBUG) << "End of event";
    return Success;
}

void ImproveTimestamp::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
