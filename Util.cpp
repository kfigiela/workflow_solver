#include "Util.hpp"

#include <string>
#include <sstream>
#include <boost/format.hpp>
#include <iostream>
#include <fstream>

#include "graph_grammar_solver/Mesh.hpp"

using boost::format;
using namespace std;

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
  
  for(auto e: elements) {
    buf << format("<rect x='%f' y='%f' width='%f' height='%f' stroke='black' stroke-width='1' style='fill: hsl(%ld, 100%%, 50%%);'/>\n") % e->x1 % e->y1 % (e->x2-e->x1) % (e->y2-e->y1) % (e->k*50);
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
