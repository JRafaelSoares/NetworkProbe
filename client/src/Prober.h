#ifndef PROBER_H
#define PROBER_H

#include <vector>
#include <string>
#include <map>
#include <grpcpp/grpcpp.h>

#include "proto/prober.grpc.pb.h"

#include "Constants.h"

/**
 * @brief Entity which tests the state of the network.
 * 
 */
class Prober {

private:
    std::vector<std::string> _servers;

    std::unique_ptr<messages::Prober::Stub> _stub;

    std::map<std::string, std::vector<int64_t>>* _times;   // Mapping between addresses and times

    /**
     * @brief Finds all the processes alive and stores them.
     * 
     */
    void findProcesses();

    void createStub(std::string address);

    // Statistical methods

    int64_t averageValue(std::string address);

    int64_t stdDeviation(std::string address);

public:
    Prober();

    std::vector<std::string> servers() {
        return this->_servers;
    }

    std::map<std::string, std::vector<int64_t>>* times() {
        return this->_times;
    }    

    /**
     * @brief Sends a probing message to the requested address. Adds the time
     * it took for the message to get to the destination and back to the "times"
     * map.
     * 
     * @param address 
     */
    void sendProbingMessage(std::string address);

    /**
     * @brief Evaluates the stability of the network, including the time messages
     * take to arrive. Sends the results to the Redis database.
     * 
     */
    std::map<std::string, std::vector<int64_t>>* stability();

};

#endif