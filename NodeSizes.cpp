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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <sys/time.h>
#include <iostream>
#include <fstream>

using namespace std;
using boost::format;
using boost::io::group;
namespace po = boost::program_options;

po::variables_map config;
bool debug = false;
bool in_memory = false;
Tree tree;



bool parseCommandLine(int argc, char ** argv) {
  po::options_description desc("Options");
  desc.add_options()
       ("help,h", "Print help messages")
       ("tree", po::value<string>(&KV::prefix)->required(), "Input serialized tree")
       ("debug,d", po::bool_switch(&debug)->default_value(false), "Debug mode (verbose)")
       ("mem", po::bool_switch(&in_memory)->default_value(false), "In memory computation")
       ("iters", po::value<unsigned long>()->default_value(100), "Max iterations")

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


void phase_eliminate(Node* node, int level) {
  cout << format("%1% %2%\n") % level % (node->getDofs().size()) ;
  if(node->getLeft() != NULL && node->getRight() != NULL) {
    phase_eliminate(node->getLeft(), level+1);
    phase_eliminate(node->getRight(), level+1);
  }

}



int main(int argc, char ** argv)
{
  if(!parseCommandLine(argc, argv))
    return 1;

  KV::init();

  KV::read(KV::prefix, tree);

  cout << format("Got tree of %d nodes\n") % tree.nodes.size();

  // init_state(tree.root);

  double alfa, beta, r, p, rs_old, rs_new;

  // p = r = b
  // aggregate_r
  // rs_old = r'*r
  // unsigned long num_iter = config["aggregate"].as<unsigned long>();
  // for(size_t i = 0; i < num_iter; ++i) {
    // aggregate_and_distribute p
    // dist: Ap = A * p
    // dist: pAp = p'*Ap
    // aggregate pAp
    // alpha = rs_old / pAp
    // distribute alpha
    // dist: x = x + alpha * p
    // rs_new = r'*r

  // }

  phase_eliminate(tree.root,0);
  // phase_bs(tree.root);

  KV::deinit();

  return 0;
}
