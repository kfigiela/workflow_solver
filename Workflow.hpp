#ifndef WORKFLOW_HPP
#define WORKFLOW_HPP

#include "graph_grammar_solver/Node.hpp"
#include "Workflow.hpp"

#include <vector>
#include <map>
#include <string>

using std::string;
using std::vector;
using std::map;

class Signal {
  
public:
  unsigned int id;
  string name;
  Signal(unsigned int id, string name): id(id), name(name) {};
};

class Process {
public:
  vector<Signal*> ins;
  vector<Signal*> outs;
  const string command;
  string args;
  unsigned long mem;
  unsigned long flops;
  
  Process(string command): command(command), mem(0), flops(0) {};
  Process(string command, unsigned long mem, unsigned long flops): command(command), mem(mem), flops(flops) {};
};

class Workflow {
public:
  vector<Process> processes;
  map<string, Signal> signals;

  vector<Signal*> ins;
  vector<Signal*> outs;  
  
  string json(string common_args = "");
  string dot();
  Signal * signal(string name);  
};

void workflowElimination(Workflow* w, Node *node, unsigned long threshold);
void workflowBackwardSubstitution(Workflow* w, Node *node, unsigned long threshold);
Workflow * buildWorkflow(Node * root, unsigned long threshold);

#endif