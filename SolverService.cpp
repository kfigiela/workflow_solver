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
#include <string>

#include <SimpleAmqpClient/SimpleAmqpClient.h>


using namespace std;
using boost::format;
using boost::io::group;
namespace po = boost::program_options;

po::variables_map config;
bool debug = false;
bool profiling = false;
Tree tree;

string amqp;
string queue;

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

bool parseCommandLine(int argc, char ** argv) {
  po::options_description desc("Options");
  desc.add_options()
       ("help,h", "Print help messages")
       ("tree", po::value<string>(&KV::prefix)->required(), "Input serialized tree")
       ("queue", po::value<string>(&queue)->default_value("hyperflow.jobs"), "AMQP queue name")
       ("amqp", po::value<string>(&amqp)->default_value("localhost"), "AMQP server")
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

void eliminate(int node_id, bool write = true, bool read = true) {
  Node * node;

  node = tree.nodes.at(node_id);
  node->allocateSystem(LU);
  if(node->getLeft() != NULL && node->getRight() != NULL) {
    if (read) {
      KV::read_matrix(node->getLeft());
      KV::read_matrix(node->getRight());
    }
  } else {
    // TODO: should actually read element matrix here, for now eliminate generates mock data for leaves
    // kv_read_matrix(node);
  }

  if(debug) cout << format("Eliminaing node: %d... \n") % node->getId();

  node->eliminate();

  if (write) KV::write_matrix(node);

  // node->deallocateSystem();
  // if(node->getLeft() != NULL && node->getRight() != NULL) {
  //   node->getLeft()->deallocateSystem();
  //   node->getRight()->deallocateSystem();
  // }
}

void bs(int node_id, bool read = true, bool write = true, bool readChildren = true) {
  Node * node;

  node = tree.nodes.at(node_id);

  if(debug) cout << format("Backward substitution on node: %d... %d\n") % node->getId() % node->getDofs().size();
  if(read) {
    KV::read_matrix(node);

    if(readChildren && node->getLeft() != NULL && node->getRight() != NULL) {
      KV::read_matrix(node->getLeft());
      KV::read_matrix(node->getRight());
    }
  }

  node->bs();

  if(write) {
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
}

void seq_eliminate(Node* node, bool rec = true) {
  if(node->getLeft() != NULL && node->getRight() != NULL) {
    seq_eliminate(node->getLeft(), false);
    seq_eliminate(node->getRight(), false);
  }
  eliminate(node->getId(), rec, false);
}

void seq_bs(Node* node, bool rec = true) {
  bs(node->getId(), rec, false, false);
  if(node->getLeft() != NULL && node->getRight() != NULL) {
    seq_bs(node->getLeft(), false);
    seq_bs(node->getRight(), false);
  }
}

int main(int argc, char ** argv)
{
  if(!parseCommandLine(argc, argv))
    return 1;

  KV::init();

  KV::read(KV::prefix, tree);

  cout << format("Got tree of %d nodes\n") % tree.nodes.size();
  if(profiling) std::cerr << "Task Id Time\n";

  try
  {
    using namespace AmqpClient;

    Channel::ptr_t channel = Channel::Create(amqp);

    channel->DeclareQueue(queue, false, true, false, false);
    channel->BasicConsume(queue, "ct", true, false, false, 100);

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

      struct timespec requestStart, requestEnd;

      if(profiling) clock_gettime(CLOCK_REALTIME, &requestStart);


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

      if(profiling) {
        clock_gettime(CLOCK_REALTIME, &requestEnd);
        std::cerr << operation << "\t" << node_id << "\t" << "\t" << ( requestEnd.tv_sec - requestStart.tv_sec ) + ( requestEnd.tv_nsec - requestStart.tv_nsec ) / 1E9 << std::endl;
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
