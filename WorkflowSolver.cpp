// #include "DynamicLib.h"
#include "graph_grammar_solver/Analysis.hpp"
#include "graph_grammar_solver/Node.hpp"
#include "graph_grammar_solver/Element.hpp"
#include "graph_grammar_solver/Mesh.hpp"
#include "graph_grammar_solver/EquationSystem.hpp"

#include "Workflow.hpp"

#include <boost/format.hpp>
// #include <llvm/Support/CommandLine.h>
#include <sys/time.h>
#include <iostream>

// #ifdef BOOST_NO_EXCEPTIONS
// namespace boost {
//   void throw_exception( std::exception const & e ) { std::cerr << "EXCEPTION!!\n"; }
// }
// #endif


// const char* const name = "WorkflowSolver";
// const char* const desc = "Workflow interface for mesh-based FEM solver";
// const char* const url = NULL;
//
// namespace cl = llvm::cl;
//
// static cl::opt<std::string> treefile("treefile", cl::desc("File with tree definition"), cl::value_desc("filename"), cl::Required);


using std::cout; using std::cerr;
using std::string;
using std::endl; using std::flush;

using boost::format;
using boost::io::group;

int main(int argc, char ** argv)
{
  // cl::ParseCommandLineOptions(argc, argv);
  
  string treefile = argv[1];
  
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
  
  if(0) {
    Analysis::printTree(m->getRootNode());

    for (Element *e : m->getElements()) {
        Analysis::printElement(e);
    }
    cerr << format("Root size: %d\n") % m->getRootNode()->getDofs().size();
  }
  
  cerr << "Generating graph...\n";
  
  Workflow * w = buildWorkflow(m->getRootNode());
  // cout << w->json();
  // cout << endl << endl << endl;
  cout << w->dot();
  
  
  return 0;
}