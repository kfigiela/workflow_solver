#ifndef KVSTORE_HPP
#define KVSTORE_HPP

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <string>
#include <sstream>
#include <boost/format.hpp>
#include <libmemcached/memcached.h>

#include "graph_grammar_solver/Node.hpp"

namespace KV {
  using std::string;
  using std::cout;
  using std::endl;
  using boost::format;
  
  string prefix;
  memcached_st * memc;
  const char * config_string= "--SERVER=localhost";
  
  inline void init() {
    cout << "Connecting to memcached..." << endl;
    KV::memc = memcached(KV::config_string, strlen(KV::config_string));
    cout << "Connected." << endl;
  }
  
  inline void deinit() {
    memcached_free(KV::memc);
    cout << "Disconnected from memcached" << endl;
  }

  template<class T>
  void read(string key, T &value) { 
    char * data;
    size_t data_length;
    memcached_return rc;

    cout << format("Reading key: %s... \n") % key;
    
    data = memcached_get (KV::memc, key.data(), key.size(), &data_length, 0, &rc);
    if (rc == MEMCACHED_SUCCESS)
      cout << format("Memcached read %s successfully %d bytes\n") % key % data_length;
    else {
      cout << format("Memcached didn't store %s: %s\n") % key % memcached_strerror(KV::memc, rc);    
      throw;
    }
    string data_str(data, data_length);
    std::istringstream is(data_str);
    boost::archive::binary_iarchive ia(is);
    ia >> value;
    free(data);
  }

  template<class T>
  void write(string key, T &value) {
    std::ostringstream os(std::stringstream::out | std::stringstream::binary);
    cout << format("Writing key: %s... \n") % key;
    boost::archive::binary_oarchive ia(os);
    ia << value;
    
    std::string data = os.str();
    
    memcached_return rc = memcached_set(KV::memc, key.data(), key.size(), data.data(), data.size(), (time_t)0, (uint32_t)0);
    if (rc == MEMCACHED_SUCCESS)
      cout << format("Memcached stored %s successfully %d bytes\n") % key % data.size();
    else
      cout << format("Memcached didn't store %s: %s\n") % key % memcached_strerror(KV::memc, rc);    
  }

  inline void read_matrix(Node * node) {
    node->system = new EquationSystem();
    KV::read(str(format("%s_%d") % KV::prefix % node->getId()), node->system);
  }

  inline void write_matrix(Node * node) {
    KV::write(str(format("%s_%d") % KV::prefix % node->getId()), node->system);
  }
}
#endif