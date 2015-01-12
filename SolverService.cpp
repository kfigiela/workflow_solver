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

#include <SimpleAmqpClient/SimpleAmqpClient.h>

using namespace std;
using boost::format;
using boost::io::group;
namespace po = boost::program_options; 

po::variables_map config;
bool debug = false;
bool profiling = false;
Tree tree;


#ifdef __MACH__

#include <mach/mach_time.h>
#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)

static double orwl_timebase = 0.0;
static uint64_t orwl_timestart = 0;
#define CLOCK_REALTIME 0
void clock_gettime(int sth, struct timespec * t) {
  if (!orwl_timestart) {
    mach_timebase_info_data_t tb = { 0 };
    mach_timebase_info(&tb);
    orwl_timebase = tb.numer;
    orwl_timebase /= tb.denom;
    orwl_timestart = mach_absolute_time();
  }
  double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
  t->tv_sec = diff * ORWL_NANO;
  t->tv_nsec = diff - (t->tv_sec * ORWL_GIGA);
}

#endif


ofstream benchmark("benchmark.txt");

bool parseCommandLine(int argc, char ** argv) {
  po::options_description desc("Options"); 
  desc.add_options() 
       ("help,h", "Print help messages") 
       ("tree", po::value<string>(&KV::prefix)->required(), "Input serialized tree")
       ("debug,d", po::bool_switch(&debug)->default_value(false), "Debug mode (verbose)")
       ("profiling", po::bool_switch(&profiling)->default_value(false), "Profiling (to stderr)")
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

void eliminate(int node_id) {
  struct timespec requestStart, requestEnd;
  Node * node;
  
  clock_gettime(CLOCK_REALTIME, &requestStart);
  
  node = tree.nodes.at(node_id);
  node->allocateSystem(OLD);
  int n  = node->system->n;
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    KV::read_matrix(node->getLeft());
    KV::read_matrix(node->getRight());
  } else {
    // TODO: should actually read element matrix here, for now eliminate generates mock data for leaves
    // kv_read_matrix(node);
  }
  
  if(debug) cout << format("Eliminaing node: %d... \n") % node->getId();

  node->eliminate();
    
  KV::write_matrix(node);
  
  node->deallocateSystem();
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    node->getLeft()->deallocateSystem();
    node->getRight()->deallocateSystem();
  }
  clock_gettime(CLOCK_REALTIME, &requestEnd);

  benchmark << "Eliminate" << "\t" << node_id << "\t" << n << "\t" << node->getSizeInMemory() << "\t" << node->getFLOPs() << "\t" << ( requestEnd.tv_sec - requestStart.tv_sec ) + ( requestEnd.tv_nsec - requestStart.tv_nsec ) / 1E9 << std::endl;
}

void bs(int node_id) {
  Node * node;
  
  node = tree.nodes.at(node_id);
    
  KV::read_matrix(node);
    
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    KV::read_matrix(node->getLeft());
    KV::read_matrix(node->getRight());
  } 
  
  if(debug) cout << format("Backward substitution on node: %d... \n") % node->getId();

  node->bs();
  
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    KV::write_matrix(node->getLeft());
    KV::write_matrix(node->getRight());
  } else {
    KV::write_matrix(node); // TODO: is it needed?
  }
  
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    node->getLeft()->deallocateSystem();
    node->getRight()->deallocateSystem();
  }
  
  node->deallocateSystem();  
}

void seq_eliminate(Node* node) {
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    seq_eliminate(node->getLeft());
    seq_eliminate(node->getRight());
  }
  eliminate(node->getId());
}

void seq_bs(Node* node) {
  bs(node->getId());
  if(node->getLeft() != NULL && node->getRight() != NULL) { 
    seq_bs(node->getLeft());
    seq_bs(node->getRight());
  }
}

int main(int argc, char ** argv)
{  
  if(!parseCommandLine(argc, argv)) 
    return 1;
  
  KV::init();
  
  KV::read(KV::prefix, tree);
  
  cout << format("Got tree of %d nodes\n") % tree.nodes.size();
  benchmark << "Task Id N Mem FLOPs Time\n";

  try
  {
    using namespace AmqpClient;
    
    Channel::ptr_t channel = Channel::Create();

    channel->DeclareQueue("hyperflow.jobs", false, true, false, false);
    channel->BasicConsume("hyperflow.jobs", "ct", true, false, false, 100);

    Envelope::ptr_t env;  
    while (channel->BasicConsumeMessage("ct", env))
    {
      if(debug){
        std::cout << "AMQP Envelope received: \n"
          << " Exchange: " << env->Exchange()
          << "\n Routing key: " << env->RoutingKey()
          << "\n Consumer tag: " << env->ConsumerTag()
          << "\n Delivery tag: " << env->DeliveryTag()
          << "\n Redelivered: " << env->Redelivered()
          << "\n ReplyTo: " << env->Message()->ReplyTo()
          << "\n Body: " << env->Message()->Body() << std::endl;
      }

      std::istringstream is(env->Message()->Body());
      boost::property_tree::ptree pt;
      boost::property_tree::read_json(is, pt);
      
      string operation = pt.get<string>("executable");
      string node_id = pt.get<string>("args");
      
      if(debug) std::cout << format("%s(%s)\n") % operation % node_id;

      if(operation == "Eliminate") {
        eliminate(atoi(node_id.c_str()));
      } else if(operation == "Backsubstitute") {
        bs(atoi(node_id.c_str()));
      } else if(operation == "SeqEliminate") {
        Node * node = tree.nodes.at(atoi(node_id.c_str()));
        seq_eliminate(node);
      } else if(operation == "SeqBacksubstitute") {
        Node * node = tree.nodes.at(atoi(node_id.c_str()));
        seq_bs(node);
      } else {
        cout << format("Unknown operation %s:\n") % operation;
      }
     
      {
        boost::property_tree::ptree reply;      
        reply.put("exit_status", "0");

        std::ostringstream buf; 
        boost::property_tree::write_json (buf, reply, true);
      
        BasicMessage::ptr_t msg = BasicMessage::Create();

        msg->Body(buf.str());
        msg->CorrelationId(env->Message()->CorrelationId());
        msg->ContentType("application/json");
        
        channel->BasicPublish("", env->Message()->ReplyTo(), msg, true);
      }
      
      channel->BasicAck(env);
    }
  }
  catch (AmqpClient::AmqpException &e)
  {
     std::cout << "Failure: " << e.what();
  }

  benchmark.close();
  cout << "Finished!\n";
   
  KV::deinit();
  
  return 0;
}
