#include "Util.hpp"

#include <string>
#include <sstream>
#include <boost/format.hpp>
#include <iostream>
#include <fstream>
#include <tuple>
#include <cmath>

#include "graph_grammar_solver/Mesh.hpp"

using boost::format;
using namespace std;


rgb Util::hsv2rgb( double h, double s, double v ) {
  double r,g,b;

  int i = int(h * 6);
  double f = h * 6 - i;
  double p = v * (1 - s);
  double q = v * (1 - f * s);
  double t = v * (1 - (1 - f) * s);

  switch(i % 6){
      case 0: r = v, g = t, b = p; break;
      case 1: r = q, g = v, b = p; break;
      case 2: r = p, g = v, b = t; break;
      case 3: r = p, g = q, b = v; break;
      case 4: r = t, g = p, b = v; break;
      case 5: r = v, g = p, b = q; break;
  }
 
  return rgb(r*255, g*255, b*255);
}

std::string Util::rgb2str(rgb color) {
  return (format("#%02x%02x%02x") % (int)std::get<0>(color) % (int)std::get<1>(color) % (int)std::get<2>(color)).str();
}

std::string Util::mesh_svg(Mesh * m) {
  std::ostringstream buf;
  
  auto elements = m->getElements();
    
  auto min_x = std::min_element(begin(elements), end(elements), [] (Element * const& s1, Element * const& s2) { return s1->x1 < s2->x1; });
  auto max_x = std::max_element(begin(elements), end(elements), [] (Element * const& s1, Element * const& s2) { return s1->x2 < s2->x2; });
  auto min_y = std::min_element(begin(elements), end(elements), [] (Element * const& s1, Element * const& s2) { return s1->y1 < s2->y1; });
  auto max_y = std::max_element(begin(elements), end(elements), [] (Element * const& s1, Element * const& s2) { return s1->y2 < s2->y2; });
    
  buf << "<?xml version=\"1.0\" standalone=\"no\"?>\n";
  buf << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
  buf << format("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1024\" height=\"1024\" viewBox=\"%d %d %d %d\" version=\"1.1\">\n") % (*min_x)->x1 % (*min_y)->y1 % (*max_x)->x2 % (*max_y)->y2;
  
  auto color = [](int k) { return rgb2str(hsv2rgb(fmod(k*0.2, 1), 1, 1)); };
  
  for(auto e: elements) {
    buf << format("<rect x='%f' y='%f' width='%f' height='%f' stroke='black' stroke-width='1' fill=\"%s\"/>\n") % e->x1 % e->y1 % (e->x2-e->x1) % (e->y2-e->y1) % color(e->k);
  }
  
  buf << "</svg>";
  return buf.str();
}

void Util::writeFile(string name, string content) {
  try {
    ofstream f(name);
    f.exceptions ( ifstream::eofbit | ifstream::failbit | ifstream::badbit );    
    f << content;
    f.close();    
  } catch (std::exception e) {
    cout << format("Failed writing %s: %s") % name % e.what() << endl;    
  }
}
