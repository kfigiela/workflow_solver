#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <sys/time.h>
#include <sstream>
#include <algorithm>
#include <tuple>

#include "graph_grammar_solver/Analysis.hpp"
#include "graph_grammar_solver/Node.hpp"
#include "graph_grammar_solver/Element.hpp"
#include "graph_grammar_solver/Mesh.hpp"
#include "graph_grammar_solver/EquationSystem.hpp"

#include "Workflow.hpp"
#include "Tree.hpp"
#include "KVStore.hpp"
#include "Util.hpp"

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
       ("treefile", po::value<string>()->required(), "Input tree file") 
       ("json", po::value<string>(), "Output workflow JSON filename")
       ("dot", po::value<string>(), "Output workflow DOT filename ")
       ("tree", po::value<string>(), "Output tree prefix in Memcached store")
       ("treedot", po::value<string>(), "Output tree DOT filename")
       ("mesh", po::value<string>(), "Output mesh SVG filename")
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
    
  string treefile = config["treefile"].as<string>();
  
  cout << format("Reading mesh file: %s... \n") % treefile;
  
  Mesh *m  = Mesh::loadFromFile(treefile.c_str());
  if (m == NULL) {
    cout << "Failed. Could not load the mesh. Exiting.\n";
    exit(1);
  }
  
  cout << "DOF enumeration... \n";
  Analysis::enumerateDOF(m);  
  
  cout << "Analysis... \n";
  Analysis::doAnalise(m);
  
  if(debug) {
    Analysis::printTree(m->getRootNode());

    for (Element *e : m->getElements()) {
        Analysis::printElement(e);
    }
    cout << format("Root size: %d\n") % m->getRootNode()->getDofs().size();
  }
  
  cout << "Generating workflow graph... \n";  
  Workflow * w = buildWorkflow(m->getRootNode());
  
  Tree t(m->getRootNode());
  
  cout << format("Generated %d tasks.\n") % w->processes.size();
  
  if(config.count("json")) {
    cout << "Writing JSON...\n";
    Util::writeFile(config["json"].as<string>(), w->json());
  }

  if(config.count("dot")) {
    cout << "Writing DOT...\n";
    Util::writeFile(config["dot"].as<string>(), w->dot());
  }

  if(config.count("mesh")) {
    cout << "Writing Mesh SVG...\n";
    Util::writeFile(config["mesh"].as<string>(), Util::mesh_svg(m));
  }
    
  if(config.count("tree")) {
    cout << "Writing Tree...\n";
    KV::init();    
    KV::write(config["tree"].as<string>(), t);
    KV::deinit();
  }
  
  if(config.count("treedot")) {
    Util::writeFile(config["treedot"].as<string>(), t.dot());
  }
        
  cout << "Finished!\n";
  
  return 0;
}