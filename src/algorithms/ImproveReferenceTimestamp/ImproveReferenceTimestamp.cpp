#include "ImproveReferenceTimestamp.h"
#include "objects/SpidrSignal.h"

using namespace corryvreckan;
using namespace std;

ImproveReferenceTimestamp::ImproveReferenceTimestamp(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {
    m_method = m_config.get<int>("improvementMethod", 1);
    m_source = m_config.get<std::string>("signalSource", "W0013_G02");
    m_stop = m_config.get<bool>("allowOutoftimeEvents", true);
}

void ImproveReferenceTimestamp::initialise() {
    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode ImproveReferenceTimestamp::run(Clipboard* clipboard) {

    // Recieved triggers
    std::vector<long long int> trigger_times;

    // Get trigger signals
    SpidrSignals* spidrData = (SpidrSignals*)clipboard->get(m_source, "SpidrSignals");
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

        // Use trigger timestamp
        if(m_method == 0) {
            // Find trigger timestamp clostest in time
            long long int diff = std::numeric_limits<long long int>::max();
            for(auto& trigger_time : trigger_times) {
                LOG(DEBUG) << " track: " << track->timestamp() << " trigger: " << trigger_time
                           << " diff: " << abs((long long int)(trigger_time - track->timestamp()))
                           << " diff stored cycles: " << diff
                           << " diff stored secs: " << (double)(diff) / (4096. * 40000000.);
                if(abs((long long int)(trigger_time - track->timestamp())) < diff) {
                    improved_time = trigger_time;
                    diff = abs((long long int)(trigger_time - track->timestamp()));
                }
            }
            // if(m_stop == false) {
            //     if(diff > 5e-7) {
            //         return NoData;
            //     }
            // }
            // trigger latency is ~175 ns, still missing
        }

        // Use average track timestamp
        else if(m_method == 1) {
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

void ImproveReferenceTimestamp::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
