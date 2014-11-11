#include <iostream>
#include <sstream>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Workflow.hpp"

using namespace std;
using boost::format;

string Workflow::json() {
  using boost::property_tree::ptree;
  using boost::property_tree::write_json;
  
  ptree pt;
  ptree signals;
  ptree processes;

  for(auto& s: this->signals) {
    ptree signal;
    signal.put("name", s.second.name);
    signal.put("id", s.second.id);
    signals.push_back(std::make_pair("", signal));
  }
  
  pt.add_child("signals",   signals);

  for(auto& p: this->processes) {
    ptree process, ins, outs;
    process.put("command", p.command);
    for(auto s: p.ins) {
      ptree el;
      el.put_value(s->id);
      ins.push_back(std::make_pair("", el));
    }
    process.add_child("ins", ins);
    
    for(auto s: p.outs) {
      ptree el;
      el.put_value(s->id);
      outs.push_back(std::make_pair("", el));
    }
    process.add_child("outs", outs);
    
    processes.push_back(std::make_pair("", process));
  }
  pt.add_child("processes", processes);
  
  std::ostringstream buf; 
  write_json (buf, pt, true);
  return buf.str();
}

string Workflow::dot() {
  std::ostringstream buf; 
  buf << "digraph g {\n";
  
  for(auto& s: this->signals) {
    buf << format("s%d [label=\"%s\", shape=box];\n") % s.second.id % s.second.name;
  }

  int i = 0;
  for(auto& p: this->processes) {
    buf << format("p%d [label=\"%s\"];\n") % ++i % p.command;
    
    for(auto s: p.ins) {
      buf << format("s%d -> p%d;\n") % s->id % i;      
    }
    
    for(auto s: p.outs) {
      buf << format("p%d -> s%d;\n") % i % s->id;
    }
  }
  
  buf << "}\n";
  return buf.str();  
}

Signal * Workflow::signal(string name) {
  if(this->signals.count(name) == 0) {
    this->signals.insert(std::make_pair(name, Signal(this->signals.size(), name)));
  }
  return &this->signals.at(name);
}


Workflow * buildWorkflow(Node * root) {
  Workflow * w = new Workflow();
  
  workflowElimination(w, root);
  workflowBackwardSubstitution(w, root);
  
  return w;
}


void workflowElimination(Workflow* w, Node *node)
{
  bool isLeaf = (node->getLeft() == NULL || node->getRight() == NULL);

  if (!isLeaf) {
      workflowElimination(w, node->getLeft());
      workflowElimination(w, node->getRight());
  }
  
  Process p(str(format("Eliminate %d") % node->getElements().size()));
  if(isLeaf) {
    Signal * elementMatrix = w->signal(str(format("%05d_element.bin") % node->getId()));
    p.ins.push_back(elementMatrix);
  } else {
    Signal * leftMatrix = w->signal(str(format("%05d_schur.bin") % node->getLeft()->getId()));
    Signal * rightMatrix = w->signal(str(format("%05d_schur.bin") % node->getRight()->getId()));
    p.ins.push_back(leftMatrix);
    p.ins.push_back(rightMatrix);    
  }
  Signal * eliminatedMatrix = w->signal(str(format("%05d_schur.bin"  ) % node->getId()));
  p.outs.push_back(eliminatedMatrix);            
  
  w->processes.push_back(p);  
}

void workflowBackwardSubstitution(Workflow* w, Node *node)
{
  if(node->getParent() != NULL) {
    Process p("BS");
    
    Signal * parentMatrix = w->signal(str(format("%05d_%s.bin") % node->getParent()->getId() % (node->getParent()->getParent() == NULL?"schur":"bs")));
    p.ins.push_back(parentMatrix);
  
    Signal * nodeMatrix = w->signal(str(format("%05d_bs.bin") % node->getId()));
    p.outs.push_back(nodeMatrix);

    w->processes.push_back(p);      
  }
  if (node->getLeft() != NULL && node->getRight() != NULL) {
    workflowBackwardSubstitution(w, node->getLeft());
    workflowBackwardSubstitution(w, node->getRight());
  }
}
