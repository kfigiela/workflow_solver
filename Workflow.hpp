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
  
  Process(string command): command(command) {};
};

class Workflow {
public:
  vector<Process> processes;
  map<string, Signal> signals;
  
  string json();
  string dot();
  Signal * signal(string name);  
};

void workflowElimination(Workflow* w, Node *node);
void workflowBackwardSubstitution(Workflow* w, Node *node);
Workflow * buildWorkflow(Node * root);

#endif