#include "DirectoryLoader.hpp"
#include "core/utils/log.h"

#include <algorithm>
#include <cmath>
#include <numeric>

using namespace corryvreckan;

DirectoryLoader::DirectoryLoader(
    const std::string& dir, std::vector<std::string> ch, std::vector<int> col, std::vector<int> row, std::string id)
    : path(dir), channels(ch), columns(col), rows(row), detectorID(id), segment(0), count(0) {
    open_files();
}

void DirectoryLoader::open_files(void) {
    files.clear();

    LOG(INFO) << "Reached waveform block " << count;

    for(auto& i : channels) {
        std::filesystem::path p = path / ("data_" + std::to_string(count) + "_" + i + ".dat");
        files.emplace_back(p);
        LOG(INFO) << "Opening waveform file " << p.filename();

        if(files.back().fail()) {
            _end = true;
            files.clear();
            LOG(INFO) << "Reached end of waveform data";
            return;
        }
    }

    param.clear();

    for(auto& i : channels) {
        param.push_back(read_preamble(path / ("data_" + std::to_string(count) + "_" + i + ".dat")));
    }

    timestamps.close();
    std::filesystem::path p = path / ("data_" + std::to_string(count) + "_" + channels[0] + "_time.dat");
    timestamps.open(p, std::ios::binary);
}

DirectoryLoader::Param DirectoryLoader::read_preamble(const std::filesystem::path& file) {
    std::filesystem::path p_txt = file;
    p_txt.replace_extension(".txt");

    std::ifstream preamble(p_txt);
    std::vector<std::string> p;
    std::string s;

    while(std::getline(preamble, s, ',')) {
        p.push_back(s);
    }

    DirectoryLoader::Param out{std::stod(p[5]) * 1e9, std::stod(p[4]) * 1e9, std::stod(p[8]), std::stod(p[7])};

    points = std::stoull(p[2]);
    segments = p.size() == 25 ? std::stoull(p[24]) : 1;

    return out;
}

WaveformVector DirectoryLoader::read_segment(size_t s, std::pair<uint32_t, double> trigger) {
    WaveformVector out;

    for(size_t i = 0; i < files.size(); i++) {
        Waveform::waveform_t o;

        o.x0 = param[i].x0;
        o.dx = param[i].dx;

        files[i].seekg(static_cast<std::streamoff>(points * 2 * s));
        if(files[i].rdstate() & (/*std::ifstream::eofbit |*/ std::ifstream::failbit | std::ifstream::badbit)) {
            LOG(ERROR) << "Error reading data!";
            break;
        }

        std::vector<int16_t> buffer(points);
        files[i].read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(points * 2));

        if(files[i].rdstate() & (/*std::ifstream::eofbit |*/ std::ifstream::failbit | std::ifstream::badbit)) {
            LOG(ERROR) << "Error reading data!";
            break;
        }

        o.waveform.resize(points);

        std::transform(
            buffer.begin(), buffer.end(), o.waveform.begin(), [&](int16_t j) { return param[i].y0 + j * param[i].dy; });

        out.emplace_back(std::make_shared<Waveform>(detectorID, columns[i], rows[i], 0, 0, trigger.second, o));
    }

    return out;
}

size_t DirectoryLoader::get_segments(void) {
    return segments;
}

WaveformVector DirectoryLoader::read(std::pair<uint32_t, double> trigger) {
    auto out = read_segment(segment, trigger);
    segment++;

    if(segment == segments) {
        segment = 0;
        count++;
        open_files();
    }

    return out;
}

bool DirectoryLoader::end(void) {
    return _end;
}

double DirectoryLoader::get_timestamp(size_t s) {
    timestamps.seekg(static_cast<std::streamoff>(s * 8));

    double timestamp;
    timestamps.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));

    return timestamp;
}
