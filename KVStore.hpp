#ifndef KVSTORE_HPP
#define KVSTORE_HPP

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <string>
#include <boost/format.hpp>
#include "graph_grammar_solver/Node.hpp"

namespace KV {
  using std::string;
  using std::cout;
  using boost::format;
  
  string prefix;

  template<class T>
  void read(string key, T &value) {
    std::ifstream is(key);
    cout << format("Reading file: %s... \n") % key;
    boost::archive::binary_iarchive ia(is);
    ia >> value;
  }

  template<class T>
  void write(string key, T &value) {
    std::ofstream os(key);
    cout << format("Writing file: %s... \n") % key;
    boost::archive::binary_oarchive ia(os);
    ia << value;
  }

  inline void read_matrix(Node * node) {
    node->system = new EquationSystem();
    KV::read(str(format("%s_%d") % KV::prefix % node->getId()), node->system);
  }

  inline void write_matrix(Node * node) {
    KV::write(str(format("%s_%d") % KV::prefix % node->getId()), node->system);
  }
}
#endif