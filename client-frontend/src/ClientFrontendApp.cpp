#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <sw/redis++/redis++.h>

// Redis information
const std::string REDIS_ADDRESS = "localhost";
const int REDIS_PORT = 6379;

/**
 * @brief Reads the log list containing the logs for each server divided by its
 * respective address.
 * 
 * @param logs Map between the server address and a vector of logs for it.
 * @param logsList List of strings containing the logs.
 *                 Form: ["SERVER address1", "log1", "log2", ..., "SERVER address2", ...]
 */
void readLogs(std::map<std::string, std::vector<std::string>> *logs, std::vector<std::string> *logsList) {
    for (auto it = logsList->begin(); it != logsList->end(); it++) {
        std::string token;
        std::stringstream ss(*it);
        getline(ss, token, ' ');

        if (!token.compare("SERVER")) {
            getline(ss, token, ' ');    // get new server address
            logs->insert(std::pair<std::string, std::vector<std::string>>(token, std::vector<std::string>()));
            continue;
        }

        logs->at(token).push_back(token);   // push log info
    }
}

/**
 * @brief Prints the logs and statistics for each server in a human-friendly way.
 * 
 * @param logs Map between the server address and a vector of logs for it.
 */
void printLogs(std::map<std::string, std::vector<std::string>> *logs) {
    for (auto it = logs->begin(); it != logs->end(); it++) {
        std::cout << "=> " << it->first << '\n';
        for (auto vecIt = it->second.begin(); vecIt != it->second.end(); vecIt++) {
            std::cout << "\t- " << *vecIt << '\n';
        }
        std::cout << std::endl;
    }
}

void waitConsume(sw::redis::Subscriber *sub, bool *consumed) {
    do {
        try {
            sub->consume();
        
        } catch (const sw::redis::Error &err) {
            std::cerr << "Redis error: " << err.what() << std::endl;
        }

    } while (!(*consumed));

    *consumed = false;  // reset flag
}

/**
 * @brief Frontend program to serve as a means of communication with the real
 * client, through a Redis database.
 * 
 * @return int 
 */
int main() {
    bool consumed = false;
    std::map<std::string, std::vector<std::string>> *logs = new std::map<std::string, std::vector<std::string>>();
    sw::redis::Redis *redis;

    try {
        sw::redis::ConnectionOptions config;
        config.host = REDIS_ADDRESS;
        config.port = REDIS_PORT;
        redis = new sw::redis::Redis(config);

    } catch (const std::exception &e) {
        std::cout << "Error while starting up Redis: " << e.what() << std::endl;
    }

    // Redis subscriber
    sw::redis::Subscriber sub = redis->subscriber();
    sub.subscribe("to-exp");

    sub.on_message([&consumed, &logs, &redis](std::string channel, std::string msg) {
        if (msg.compare("benchmarks") == 0) {
            std::vector<std::string> *logsList = new std::vector<std::string>();
            redis->lrange("logs", 0, -1, std::back_inserter(*logsList));

            readLogs(logs, logsList);
            printLogs(logs);
        }

        consumed = true;
    });

    while (true) {
        std::string cmd, msgN;
        std::cin >> cmd;

        if (cmd.compare("begin")) {
            std::cin >> msgN;
            redis->publish("to-exp", "begin " + msgN);
        
        } else if (cmd.compare("fetch")) {
            redis->publish("to-exp", "fetch");
            waitConsume(&sub, &consumed);

        } else {
            std::cerr << "Invalid command specified.\n" << std::endl;
        }
    }

    do {
        try {
            sub.consume();
        
        } catch (const sw::redis::Error &err) {
            std::cerr << "Redis error: " << err.what() << std::endl;
        }

    } while (true);

    delete redis;
    return 0;
}