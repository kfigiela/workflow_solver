#include "graph_grammar_solver/Analysis.hpp"
#include "graph_grammar_solver/Node.hpp"
#include "graph_grammar_solver/Element.hpp"
#include "graph_grammar_solver/Mesh.hpp"
#include "graph_grammar_solver/EquationSystem.hpp"

#include "Workflow.hpp"
#include "Tree.hpp"
#include "KVStore.hpp"

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

bool parseCommandLine(int argc, char ** argv) {
  po::options_description desc("Options"); 
  desc.add_options() 
       ("help,h", "Print help messages") 
       ("tree", po::value<string>(&KV::prefix)->required(), "Input serialized tree")
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

int main(int argc, char ** argv)
{  
  if(!parseCommandLine(argc, argv)) 
    return 1;
  
  KV::init();
  
  int node_id = config["node"].as<int>();

  Tree tree;
  Node * node;

  KV::read(KV::prefix, tree);
  
  cout << format("Got tree of %d nodes\n") % tree.nodes.size();
  
  node = tree.nodes.at(node_id);

  node->allocateSystem(OLD);
    
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    KV::read_matrix(node->getLeft());
    KV::read_matrix(node->getRight());
  } else {
    // TODO: should actually read element matrix here, for now eliminate generates mock data for leaves
    // kv_read_matrix(node);
  }
  
  cout << format("Eliminaing node: %d... \n") % node->getId();

  node->eliminate();
  
  KV::write_matrix(node);
        
  cout << "Finished!\n";
   
  KV::deinit();
  
  return 0;
}