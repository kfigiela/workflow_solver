// #include "DynamicLib.h"
#include "graph_grammar_solver/Analysis.hpp"
#include "graph_grammar_solver/Node.hpp"
#include "graph_grammar_solver/Element.hpp"
#include "graph_grammar_solver/Mesh.hpp"
#include "graph_grammar_solver/EquationSystem.hpp"

#include "Workflow.hpp"

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

bool parseCommandLine(int argc, char ** argv) {
  po::options_description desc("Options"); 
  desc.add_options() 
       ("help,h", "Print help messages") 
       ("treefile", po::value<string>()->required(), "Tree file") 
       ("json", po::value<string>(), "Output workflow JSON")
       ("dot", po::value<string>(), "Output DOT file")
       ("debug,d", "Debug mode (verbose)")
  ;
   
  try {
    po::store(po::parse_command_line(argc, argv, desc), config);
  
    if (config.count("help")) {
        cout << desc << endl;
        return false;
    }
    po::notify(config);  
  } catch (boost::program_options::required_option e) {
    cerr << "Error: " << e.what() << endl << endl;
    cerr << desc << endl;
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
    cerr << format("Failed writing %s: %s") % name % e.what() << endl;    
  }
}

int main(int argc, char ** argv)
{  
  if(!parseCommandLine(argc, argv)) 
    return 1;
  
  string treefile = config["treefile"].as<string>();
  
  cerr << format("Mesh file: %s\n") % treefile;
  
  Mesh *m  = Mesh::loadFromFile(treefile.c_str());
  if (m == NULL) {
      cerr << "Could not load the mesh. Exiting.\n";
      exit(1);
  }
  
  Analysis::enumerateDOF(m);
  
  cerr << "DOF enumeration done.\n";
  
  Analysis::doAnalise(m);
  
  cerr << "Analysis done.\n";
  
  if(config.count("debug")) {
    Analysis::printTree(m->getRootNode());

    for (Element *e : m->getElements()) {
        Analysis::printElement(e);
    }
    cerr << format("Root size: %d\n") % m->getRootNode()->getDofs().size();
  }
  
  cerr << "Generating workflow graph..." << endl;
  
  Workflow * w = buildWorkflow(m->getRootNode());
  
  if(config.count("json")) {
    cerr << "Writing JSON..." << endl;
    writeFile(config["json"].as<string>(), w->json());
  }

  if(config.count("dot")) {
    cerr << "Writing DOT..." << endl;
    writeFile(config["dot"].as<string>(), w->dot());
  }
        
  cerr << "Done." << endl;
  
  return 0;
}