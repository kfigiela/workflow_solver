#include <iostream>
#include <sstream>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>


#include "Workflow.hpp"

using namespace std;
using boost::format;

string Workflow::json(string common_args) {
  using boost::property_tree::ptree;
  using boost::property_tree::write_json;
  
  ptree pt;
  ptree signals;
  ptree processes;
  ptree ins;
  ptree outs;

  for(auto& s: this->signals) {
    ptree signal;
    signal.put("name", s.second.name);
    signal.put("id", s.second.id);
    signals.push_back(std::make_pair("", signal));
  }
  
  pt.add_child("signals",   signals);

  for(auto& p: this->processes) {
    ptree process, ins, outs, config, executor;
    process.put("name", p.command);
    process.put("function", "amqpCommand");
    process.put("type", "dataflow");
    process.put("executor", "syscommand");
    
    executor.put("executable", p.command);
    executor.put("args", common_args + p.args);    
    
    config.add_child("executor", executor);
    config.put("mem", p.mem);
    config.put("flops", p.flops);
    process.add_child("config", config);
    
    for(auto s: p.ins) {
      ptree el;
      el.put_value(s->name);
      ins.push_back(std::make_pair("", el));
    }
    process.add_child("ins", ins);
    
    for(auto s: p.outs) {
      ptree el;
      el.put_value(s->name);
      outs.push_back(std::make_pair("", el));
    }
    process.add_child("outs", outs);
    
    processes.push_back(std::make_pair("", process));
  }
  
  
  for(auto s: this->ins) {
    ptree el;
    el.put_value(s->name);
    ins.push_back(std::make_pair("", el));
  }  
  pt.add_child("ins", ins);
  
  for(auto s: this->outs) {
    ptree el;
    el.put_value(s->name);
    outs.push_back(std::make_pair("", el));
  }
  pt.add_child("outs", outs);
  
  
  ptree exit_process, empty;
  
  exit_process.put("name", "Done");
  exit_process.put("function", "exit");
  exit_process.put("type", "dataflow");
  exit_process.put("executor", "syscommand");
  exit_process.add_child("ins", outs);
  exit_process.add_child("outs", empty);
  
  
  processes.push_back(std::make_pair("", exit_process));
  
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


Workflow * buildWorkflow(Node * root, unsigned long threshold) {
  Workflow * w = new Workflow();
  
  workflowElimination(w, root, threshold);
  workflowBackwardSubstitution(w, root, threshold);
    
  return w;
}



void workflowElimination(Workflow* w, Node *node, unsigned long threshold)
{  
  bool isLeaf = (node->getLeft() == NULL || node->getRight() == NULL);
  bool aggregate = node->getFLOPs(true) < threshold;

  if (!isLeaf && !aggregate) {
      workflowElimination(w, node->getLeft(), threshold);
      workflowElimination(w, node->getRight(), threshold);
  }
  
  Process p(aggregate?"SeqEliminate":"Eliminate", node->getSizeInMemory(false), node->getFLOPs(true));
  if(isLeaf || aggregate) {
    Signal * elementMatrix = w->signal(str(format("%05d_element") % node->getId()));
    p.ins.push_back(elementMatrix);
    w->ins.push_back(elementMatrix);
  } else {
    Signal * leftMatrix = w->signal(str(format("%05d_schur") % node->getLeft()->getId()));
    Signal * rightMatrix = w->signal(str(format("%05d_schur") % node->getRight()->getId()));
    p.ins.push_back(leftMatrix);
    p.ins.push_back(rightMatrix);    
  }

  Signal * eliminatedMatrix = w->signal(str(format("%05d_schur"  ) % node->getId()));
  p.outs.push_back(eliminatedMatrix);            

  p.args = str(format("%d") % node->getId());
  w->processes.push_back(p);  
}

void workflowBackwardSubstitution(Workflow* w, Node *node, unsigned long threshold)
{
  bool isLeaf = (node->getLeft() == NULL || node->getRight() == NULL);
  bool aggregate = node->getFLOPs(true) < threshold;
    
  Process p(aggregate?"SeqBacksubstitute":"Backsubstitute");
  
  Signal * nodeMatrix = w->signal(str(format("%05d_%s") % node->getId() % (node->getParent() == NULL?"schur":"bs")));
  p.ins.push_back(nodeMatrix);

  if (isLeaf || aggregate) {
    Signal * solutionMatrix = w->signal(str(format("%05d_sol") % node->getId()));
    p.outs.push_back(solutionMatrix);
    w->outs.push_back(solutionMatrix);
  } else {
    Signal * leftMatrix = w->signal(str(format("%05d_bs") % node->getLeft()->getId()));
    p.outs.push_back(leftMatrix);

    Signal * rightMatrix = w->signal(str(format("%05d_bs") % node->getRight()->getId()));
    p.outs.push_back(rightMatrix);
  } 
  
  p.args = str(format("%d") % node->getId());
  w->processes.push_back(p);      
  
  if (node->getLeft() != NULL && node->getRight() != NULL && !aggregate) {
    workflowBackwardSubstitution(w, node->getLeft(), threshold);
    workflowBackwardSubstitution(w, node->getRight(), threshold);
  }
}
