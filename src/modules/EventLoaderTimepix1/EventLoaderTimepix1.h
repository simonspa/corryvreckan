/**
 * @file
 * @brief Definition of module EventLoaderTimepix1
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef EventLoaderTimepix1_H
#define EventLoaderTimepix1_H 1

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "core/module/Module.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EventLoaderTimepix1 : public Module {

    public:
        // Constructors and destructors
        EventLoaderTimepix1(Configuration config, std::vector<std::shared_ptr<Detector>> detectors);
        ~EventLoaderTimepix1() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalize(const std::shared_ptr<ReadonlyClipboard>& clipboard) override;

    private:
        static bool sortByTime(std::string filename1, std::string filename2);
        void processHeader(std::string, std::string&, long long int&);

        // Member variables
        int m_eventNumber;
        std::string m_inputDirectory;
        std::vector<std::string> m_inputFilenames;
        bool m_fileOpen;
        size_t m_fileNumber;
        long long int m_eventTime;
        std::ifstream m_currentFile;
        std::string m_currentDevice;
        std::string m_prevHeader;
    };
} // namespace corryvreckan
#endif // EventLoaderTimepix1_H
