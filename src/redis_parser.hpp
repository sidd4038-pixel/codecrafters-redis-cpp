#ifndef REDIS_PARSER_HPP
#define REDIS_PARSER_HPP
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>
#include <vector>
#include <string>


void parse_redis_command(const char* command, std::vector<std::string>& result);
void execute_redis_command(const std::vector<std::string>& result);
std::string to_lowercase(const std::string& str);

std::string to_lowercase(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower_str;
}

void parse_redis_command(const char* command, std::vector<std::string>& result) {
    std::istringstream stream(command);
    std::string line;

    // Read the first line to determine the type of RESP data
    if (!std::getline(stream, line)) {
        return;
    }

    if (line[0] == '*') {
        // Array of Bulk Strings
        int num_elements = std::stoi(line.substr(1));
        for (int i = 0; i < num_elements; ++i) {
            // Read the length of the bulk string
            if (!std::getline(stream, line) || line[0] != '$') {
                return;
            }
            int bulk_length = std::stoi(line.substr(1));

            // Read the bulk string itself
            std::string bulk_string(bulk_length, '\0');
            stream.read(&bulk_string[0], bulk_length);

            // Read the trailing \r\n
            char crlf[2];
            stream.read(crlf, 2);
            if (crlf[0] != '\r' || crlf[1] != '\n') {
                return;
            }

            result.push_back(bulk_string);
        }
    } else {
        // Handle other RESP types if needed
        std::cerr << "Unsupported RESP type: " << line[0] << std::endl;
    }
}

// Function to execute the parsed Redis command
void execute_redis_command(int client_fd, const std::vector<std::string>& result) {
    if (result.empty()) {
        std::cerr << "Empty command" << std::endl;
        return;
    }
    
    const std::string& command = to_lowercase(result[0]);
    if (command == "ping") {
        send(client_fd, "+PONG\r\n", 7, 0);
    } else if (command == "echo") {
        if (result.size() < 2) {
            std::cerr << "ECHO command requires an argument" << std::endl;
            return;
        }
        // Print the argument(s) of the ECHO command
        for (int i = 1; i < result.size(); ++i) {
            std::string response = "$" + std::to_string(result[i].size()) + "\r\n" + result[i] + "\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }
    } else {
        std::cerr << "Unsupported command: " << command << std::endl;
    }
}
#endif // REDIS_PARSER_HPP