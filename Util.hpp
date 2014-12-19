#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>

class Mesh;

class Util {
public:
  static std::string mesh_svg(Mesh * m);
};

#endif