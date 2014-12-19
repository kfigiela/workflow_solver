#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>

class Mesh;

typedef std::tuple<uint8_t, uint8_t, uint8_t> rgb;
class Util {
public:
  static rgb hsv2rgb(double h, double s, double v);
  static std::string rgb2str(rgb color);
  static std::string mesh_svg(Mesh * m);
  static void writeFile(std::string name, std::string content);
};

#endif