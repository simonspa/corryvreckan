#ifndef __DIRECTORYLOADER_HPP
#define __DIRECTORYLOADER_HPP

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdint>

struct Waveform {
  std::vector<double> waveform;
  double x0, dx, timestamp;
};

struct Param {
  double x0, dx, y0, dy;
};

class DirectoryLoader {
  public:
    DirectoryLoader(std::string&& dir, std::vector<std::string> channels);

    std::vector<Waveform> read(void);
    size_t get_segments();

    bool end(void);

  private:
    void open_files(void);
    Param read_preamble(std::filesystem::path&& file);
    std::vector<Waveform> read_segment(size_t s);
    double get_timestamp(size_t s);

    std::vector<std::string> channels;
    std::vector<std::ifstream> files;
    std::ifstream timestamps;
    std::vector<Param> param;

    size_t points, segments, segment, count;
    bool _end = false;

    std::filesystem::path path;
};

#endif
