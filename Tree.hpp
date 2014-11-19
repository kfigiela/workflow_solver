#ifndef TREE_HPP
#define TREE_HPP

#include <map>
#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>


#include "graph_grammar_solver/Node.hpp"

struct Tree {
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & root;
    ar & BOOST_SERIALIZATION_NVP(nodes);
  };
    
  Node * root;
  std::map<int, Node*> nodes;
  
  Tree(): root(NULL) { };
  Tree(Node * root);
  
  void traverseTree(Node * node);
};

#endif