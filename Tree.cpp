#include "Tree.hpp"

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
