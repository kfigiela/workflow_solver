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
Tree tree;


bool parseCommandLine(int argc, char ** argv) {
  po::options_description desc("Options"); 
  desc.add_options() 
       ("help,h", "Print help messages") 
       ("tree", po::value<string>(&KV::prefix)->required(), "Input serialized tree")
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

void eliminate(int node_id) {
  Node * node;
  
  node = tree.nodes.at(node_id);
  node->allocateSystem(OLD);
    
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

int main(int argc, char ** argv)
{  
  if(!parseCommandLine(argc, argv)) 
    return 1;
  
  KV::init();
  
  KV::read(KV::prefix, tree);
  
  cout << format("Got tree of %d nodes\n") % tree.nodes.size();
  
  
  try
  {
    using namespace AmqpClient;
    
    Channel::ptr_t channel = Channel::Create();

    channel->DeclareQueue("hyperflow.jobs", false, true, false, false);
    channel->BasicConsume("hyperflow.jobs", "ct", true, false, false, 10);

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

        
  cout << "Finished!\n";
   
  KV::deinit();
  
  return 0;
}