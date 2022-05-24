#ifndef __DIRECTORYLOADER_HPP
#define __DIRECTORYLOADER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "objects/Waveform.hpp"

namespace corryvreckan {
    class DirectoryLoader {
        struct Param {
            double x0, dx, y0, dy;
        };

    public:
        DirectoryLoader(
            const std::string& dir, std::vector<std::string> ch, std::vector<int> col, std::vector<int> row, std::string id);

        WaveformVector read(std::pair<uint32_t, double> trigger);
        size_t get_segments();

        bool end(void);

    private:
        void open_files(void);
        Param read_preamble(const std::filesystem::path& file);
        WaveformVector read_segment(size_t s, std::pair<uint32_t, double> trigger);
        double get_timestamp(size_t s);

        std::filesystem::path path;
        std::vector<std::string> channels;
        std::vector<int> columns;
        std::vector<int> rows;
        std::string detectorID;
        std::vector<std::ifstream> files;
        std::ifstream timestamps;
        std::vector<Param> param;

        size_t points, segments, segment, count;
        bool _end = false;
    };
} // namespace corryvreckan

#endif
