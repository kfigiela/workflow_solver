#include "Tree.hpp"
#include <boost/format.hpp>
#include <sstream>
using boost::format;

Tree::Tree(Node * root) : root(root) {
  traverseTree(root);
}

void Tree::traverseTree(Node * node) {
  nodes[node->getId()] = node;
  if(node->getLeft()) 
    traverseTree(node->getLeft());
  if(node->getRight()) 
    traverseTree(node->getRight());      
}
std::string Tree::dot() {
  std::ostringstream buf; 
  buf << "digraph t {\n";
  
  for(auto& s: nodes) {
    buf << format("n%d [label=\"\", shape=point, width=0.3]\n") % s.second->getId();
    if(s.second->getLeft() != NULL)
      buf << format("n%d -> n%d\n") % s.second->getId() % s.second->getLeft()->getId();
    if(s.second->getRight() != NULL)
      buf << format("n%d -> n%d\n") % s.second->getId() % s.second->getRight()->getId();
  }
  
  buf << "{rank=same; ";
  for(auto& s: nodes) {
    if(s.second->getLeft() == NULL && s.second->getRight() == NULL) {
      buf << format("n%d ") % s.second->getId();
    }
  }
  buf << "} \n";
  
  buf << "}\n";
  return buf.str();  
}