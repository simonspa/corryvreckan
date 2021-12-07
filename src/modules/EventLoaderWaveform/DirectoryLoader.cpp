#include "DirectoryLoader.hpp"

#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>

DirectoryLoader::DirectoryLoader(std::string&& dir, std::vector<std::string> channels) : path(dir), channels(channels), segment(0), count(0) {
  open_files();
}

void DirectoryLoader::open_files(void) {
  files.clear();

  for(auto &i : channels) {
    std::filesystem::path p = path / ("data_" + std::to_string(count) + "_" + i + ".dat");
    files.emplace_back(p);

    if(files.back().fail()) {
      _end = true;
      return;
    }
  }

  param.clear();

  for(auto &i : channels) {
    param.push_back(read_preamble(path / ("data_" + std::to_string(count) + "_" + i + ".dat")));
  }

  timestamps.close();
  std::filesystem::path p = path / ("data_" + std::to_string(count) + "_" + channels[0] + "_time.dat");
  timestamps.open(p, std::ios::binary);
}

Param DirectoryLoader::read_preamble(std::filesystem::path&& file) {
  std::filesystem::path p_txt = file;
  p_txt.replace_extension(".txt");

  std::cout << p_txt << '\n';

  std::ifstream preamble(p_txt);
  std::vector<std::string> p;
  std::string s;

  while(std::getline(preamble, s, ',')) {
    p.push_back(s);
  }

  Param out{std::stod(p[5])*1e9, std::stod(p[4])*1e9, std::stod(p[8]), std::stod(p[7])};

  points = std::stoi(p[2]);
  segments = p.size() == 25 ? std::stoi(p[24]) : 1;

  return out;
}

std::vector<Waveform> DirectoryLoader::read_segment(size_t s) {
  std::vector<Waveform> out;

  for(size_t i = 0; i<files.size(); i++) {
    Waveform o;

    o.x0 = param[i].x0;
    o.dx = param[i].dx;
    o.timestamp = get_timestamp(s);

    files[i].seekg(points*2*s);

    std::vector<int16_t> buffer(points);
    files[i].read(reinterpret_cast<char*>(buffer.data()), points*2);

    o.waveform.resize(points);

    std::transform (buffer.begin(), buffer.end(), o.waveform.begin(), [&](int16_t j){ return param[i].y0+j*param[i].dy; });

    out.push_back(o);
  }

  return out;
}

size_t DirectoryLoader::get_segments(void) {
  return segments;
}

std::vector<Waveform> DirectoryLoader::read(void) {
  auto out = read_segment(segment);
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
    timestamps.seekg(s*8);

    double timestamp;
    timestamps.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));

    return timestamp;
}
