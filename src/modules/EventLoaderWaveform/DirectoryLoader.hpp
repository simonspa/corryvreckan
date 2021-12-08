#ifndef __DIRECTORYLOADER_HPP
#define __DIRECTORYLOADER_HPP

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdint>

#include "objects/Waveform.hpp"

namespace corryvreckan {
  class DirectoryLoader {
    struct Param {
      double x0, dx, y0, dy;
    };

    public:
      DirectoryLoader(const std::string& dir, std::vector<std::string> ch);

      std::vector<Waveform::waveform_t> read(void);
      size_t get_segments();

      bool end(void);

    private:
      void open_files(void);
      Param read_preamble(const std::filesystem::path& file);
      std::vector<Waveform::waveform_t> read_segment(size_t s);
      double get_timestamp(size_t s);

      std::filesystem::path path;
      std::vector<std::string> channels;
      std::vector<std::ifstream> files;
      std::ifstream timestamps;
      std::vector<Param> param;

      size_t points, segments, segment, count;
      bool _end = false;
  };
}

#endif
