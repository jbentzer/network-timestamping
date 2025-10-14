// sender.cpp
// Simple UDP sender that can send messages to a specified destination and port.
// Compile with: g++ -o sender sender.cpp
// Usage: ./sender [--dest <destination>] [-d <destination>] [--port <port>] [-p <port>] [--msg <message>] [-m <message>] [-v|--verbose]
// Default destination is 127.0.0.1 and default port is 319 (PTP event messages).
// Example: ./sender --dest 192.168.1.100 --port 12345 --msg "Hello, World!"
//
#include <iostream>
#include <unordered_map>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h> 

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

int main(int argc, char *argv[]) {
    int port = 319; // PTP event messages port
    bool verbose = false;
    std::string dest = "127.0.0.1";
    std::string msg = "test packet";

    std::unordered_map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        if (startsWith(key, "-")) {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args[key] = argv[i + 1];
                ++i;
            } else {
                args[key] = "true";
            }
        } 
    }

    if (args.count("--port")) {
        std::cout << "Port: " << args["--port"] << std::endl;
        port = std::stoi(args["--port"]);
    }
    if (args.count("-p")) {
        std::cout << "Port: " << args["-p"] << std::endl;
        port = std::stoi(args["-p"]);
    }
    if (args.count("-v") || args.count("--verbose")) {
        std::cout << "Verbose mode ON\n";
        verbose = true;
    }
    if (args.count("--dest")) {
        std::cout << "Destination: " << args["--dest"] << std::endl;
        dest = args["--dest"];
    }
    if (args.count("-d")) {
        std::cout << "Destination: " << args["-d"] << std::endl;
        dest = args["-d"];
    }
    if (args.count("--msg")) {
        std::cout << "Message: " << args["--msg"] << std::endl;
        msg = args["--msg"];
    }
    if (args.count("-m")) {
        std::cout << "Message: " << args["-m"] << std::endl;
        msg = args["-m"];
    }
    if (args.count("--help") || args.count("-h")) {
        std::cout << "Usage: " << argv[0] << " [--dest <destination>] [-d <destination>] [--port <port>] [-p <port>] [--msg <message>] [-m <message>] [-v|--verbose]\n"
                  << "  --dest, -d <destination>  Destination IP address or hostname (default 127.0.0.1)\n"
                  << "  --port, -p <port>         Destination UDP port (default 319)\n"
                  << "  --msg, -m <message>       Message to send (default 'test packet')\n"
                  << "  --verbose, -v             Enable verbose output\n"
                  << "  --help, -h                Show this help message\n";
        return 0;
    }   

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    struct hostent *he = gethostbyname(dest.c_str());
    if (he) {
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    } else {
        std::cerr << "Failed to resolve hostname: " << dest << ", using direct IP.\n";
    }
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        if (inet_pton(AF_INET, dest.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << dest << std::endl;
            return 1;
        }
    }
    
    if (sendto(sock, msg.c_str(), msg.size(), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("sendto");
        return 1;
    }

    if (verbose) { 
        std::cout << "Sent message '" << msg << "' to " << dest << ":" << port << std::endl;
    }

    close(sock);
    return 0;
}