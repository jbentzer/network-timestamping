//
// A simple UDP receiver that listens on a specified port and prints received
// messages along with their software and hardware timestamps.
// Compile with: g++ -o receiver receiver.cpp
// Usage: ./receiver [--port <port>] [-p <port>] [-v|--verbose]
// Default port is 319 (PTP event messages).
// Note: Requires root privileges to receive hardware timestamps.
// Example: sudo ./receiver --port 12345
//
// The timestamping capabilities can be checked using ethtool:
// sudo ethtool -T <interface>
//
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>
#include <arpa/inet.h>
#include <netinet/in.h>

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

int main(int argc, char *argv[]) {
    int port = 319; // PTP event messages port
    bool verbose = false;

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

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) { perror("socket"); return 1; }

    // Enable timestamping
    int ts_flags = 
        SOF_TIMESTAMPING_RX_HARDWARE  |
//        SOF_TIMESTAMPING_TX_HARDWARE  |
        SOF_TIMESTAMPING_RAW_HARDWARE |
//        SOF_TIMESTAMPING_TX_SOFTWARE  |
        SOF_TIMESTAMPING_RX_SOFTWARE  |
        SOF_TIMESTAMPING_SOFTWARE;
    if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING,
                   &ts_flags, sizeof(ts_flags)) < 0) {
        perror("setsockopt SO_TIMESTAMPING");
        return 1;
    }

    // Bind to port 319 (PTP event messages) for example
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    std::cout << "Listening on port " << port << " IP " << inet_ntoa(addr.sin_addr) << std::endl;

    while (true) {
        char buf[2048];
        char ctrl[1024];

        iovec iov { buf, sizeof(buf) };
        msghdr msg {};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = ctrl;
        msg.msg_controllen = sizeof(ctrl);

        ssize_t len = recvmsg(sock, &msg, 0);
        if (verbose) {
            std::cout << "Received " << len << " bytes" << std::endl;
        }

        if (len < 0) { perror("recvmsg"); continue; }

        for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
            cmsg != nullptr;
            cmsg = CMSG_NXTHDR(&msg, cmsg)) {

            if (cmsg->cmsg_level == SOL_SOCKET &&
                cmsg->cmsg_type == SO_TIMESTAMPING) {

                timespec* ts = (timespec*) CMSG_DATA(cmsg);
                // Kernel gives three timespecs, last one is the HW timestamp
                std::cout 
                    << "SW timestamp: " << ts[0].tv_sec << "." << std::setfill('0') << std::setw(9) << ts[0].tv_nsec << std::endl
                    << "HW timestamp: " << ts[2].tv_sec << "." << std::setfill('0') << std::setw(9) << ts[2].tv_nsec << std::endl
                    ;
            }
            else {
                if (verbose) {
                    std::cout << "Unhandled cmsg_level " << cmsg->cmsg_level
                              << " cmsg_type " << cmsg->cmsg_type << std::endl;
                }
            }
        }
    }
}