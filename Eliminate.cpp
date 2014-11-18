// #include "DynamicLib.h"
#include "graph_grammar_solver/Analysis.hpp"
#include "graph_grammar_solver/Node.hpp"
#include "graph_grammar_solver/Element.hpp"
#include "graph_grammar_solver/Mesh.hpp"
#include "graph_grammar_solver/EquationSystem.hpp"

#include "Workflow.hpp"
#include "Tree.hpp"

#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <sys/time.h>
#include <iostream>
#include <fstream>

using namespace std;
using boost::format;
using boost::io::group;
namespace po = boost::program_options; 

po::variables_map config;

bool debug = false;
string prefix;

bool parseCommandLine(int argc, char ** argv) {
  po::options_description desc("Options"); 
  desc.add_options() 
       ("help,h", "Print help messages") 
       ("tree", po::value<string>(&prefix)->required(), "Input serialized tree")
       ("node", po::value<int>()->required(), "Node id") 
       ("debug,d", po::bool_switch(&debug)->default_value(false), "Debug mode (verbose)")
  ;
   
  try {
    po::store(po::parse_command_line(argc, argv, desc), config);
  
    if (config.count("help")) {
        cout << desc << endl;
        return false;
    }
    po::notify(config);  
  } catch (boost::program_options::required_option e) {
    cout << "Error: " << e.what() << endl << endl;
    cout << desc << endl;
    return false;
  }
  return true;
}

template<class T>
void kv_read(string key, T &value) {
  ifstream is(key);
  cout << format("Reading file: %s... \n") % key;
  boost::archive::text_iarchive ia(is);
  ia >> value;
}

template<class T>
void kv_write(string key, T &value) {
  ofstream os(key);
  cout << format("Writing file: %s... \n") % key;
  boost::archive::text_oarchive ia(os);
  ia << value;
}

void kv_read_matrix(Node * node) {
  node->system = new EquationSystem();
  kv_read(str(format("%s_%d") % prefix % node->getId()), node->system);
}

void kv_write_matrix(Node * node) {
  kv_write(str(format("%s_%d") % prefix % node->getId()), node->system);
}

int main(int argc, char ** argv)
{  
  if(!parseCommandLine(argc, argv)) 
    return 1;
  
  int node_id = config["node"].as<int>();

  Tree tree;
  Node * node;

  kv_read(prefix, tree);
  
  node = tree.nodes.at(node_id);

  node->allocateSystem(OLD);
    
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    kv_read_matrix(node->getLeft());
    kv_read_matrix(node->getRight());
  } else {
    // TODO: should actually read element matrix here, for now eliminate generates mock data for leaves
    // kv_read_matrix(node);
  }
  
  cout << format("Eliminaing node: %d... \n") % node->getId();

  node->eliminate();
  
  kv_write_matrix(node);
        
  cout << "Finished!\n";
  
  return 0;
}