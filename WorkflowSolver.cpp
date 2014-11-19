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

bool parseCommandLine(int argc, char ** argv) {
  po::options_description desc("Options"); 
  desc.add_options() 
       ("help,h", "Print help messages") 
       ("treefile", po::value<string>()->required(), "Tree file") 
       ("json", po::value<string>(), "Output workflow JSON")
       ("dot", po::value<string>(), "Output DOT file")
       ("tree", po::value<string>()->required(), "Output serialized tree")
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

void writeFile(string name, string content) {
  try {
    ofstream f(name);
    f.exceptions ( ifstream::eofbit | ifstream::failbit | ifstream::badbit );    
    f << content;
    f.close();    
  } catch (std::exception e) {
    cout << format("Failed writing %s: %s") % name % e.what() << endl;    
  }
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
  
  cout << format("Generated %d tasks.\n") % w->processes.size();
  
  if(config.count("json")) {
    cout << "Writing JSON...\n";
    writeFile(config["json"].as<string>(), w->json(str(format("--tree %s ") % config["tree"].as<string>())));
  }

  if(config.count("dot")) {
    cout << "Writing DOT...\n";
    writeFile(config["dot"].as<string>(), w->dot());
  }
  
  if(config.count("tree")) {
    cout << "Writing Tree...\n";
    Tree t(m->getRootNode());
    try {
      ofstream f(config["tree"].as<string>());
      f.exceptions ( ifstream::eofbit | ifstream::failbit | ifstream::badbit );    
      boost::archive::binary_oarchive oa(f);
      oa << t;
      f.close();
    } catch (std::exception e) {
      cout << format("Failed writing %s: %s") % config["tree"].as<string>() % e.what() << endl;    
    }
  }
        
  cout << "Finished!\n";
  
  return 0;
}