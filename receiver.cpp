//
// A simple UDP receiver that listens on a specified port and prints received
// messages along with their software and hardware timestamps.
// Compile with: g++ -o receiver receiver.cpp
// Usage: ./receiver [--port <port>] [-p <port>] [-t <type>] [-v|--verbose]
// Default port is 319 (PTP event messages).
// Type can be "hw" for hardware timestamps or "sw" for software timestamps.
// Default is "sw".
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
#include <linux/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <vector>

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

std::string getIpv4Address(ifreq *ifr) {
    struct sockaddr_in *addr_in = (struct sockaddr_in *) &ifr->ifr_addr;
    return addr_in->sin_addr.s_addr ? inet_ntoa(addr_in->sin_addr) : "N/A";
}

std::unordered_map<std::string, ifreq> getNetworkInterfaces(int sock) {
    struct ifconf ifc;
    char buf[1024];
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    std::unordered_map<std::string, ifreq> interfaces;

    // Get the list of interfaces
    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
        std::cerr << "Failed to get network interfaces (SIOCGIFCONF)" << std::endl;
        return interfaces;
    }

    // Iterate through the list and store in the map
    struct ifreq* ifr = ifc.ifc_req;
    int interfacesCount = ifc.ifc_len / sizeof(struct ifreq);
    for (int i = 0; i < interfacesCount; i++) {
        interfaces[ifr[i].ifr_name] = ifr[i];
    }

    return interfaces;
}

void enableHwTimestamps(int sock, const char* ifname) {
    struct ifreq ifr;
    struct hwtstamp_config config;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    
    // Set up the hardware timestamping configuration
    memset(&config, 0, sizeof(config));
    config.flags = 0;
    config.tx_type = HWTSTAMP_TX_OFF;
    config.rx_filter = HWTSTAMP_FILTER_ALL;
    
    // Attach the hwtstamp_config to the ifreq structure
    ifr.ifr_data = (void *)&config;

    // Call ioctl to apply the configuration
    if (ioctl(sock, SIOCSHWTSTAMP, &ifr) < 0) {
        std::cerr << "Failed to enable hardware timestamps (SIOCSHWTSTAMP) on interface " << ifname << std::endl;
    }
    else {
        std::cout << "Enabled hardware timestamps on interface " << ifname << std::endl;
    }
}

void enableHwTimestampsOnAllInterfaces(int sock) {
    auto interfaces = getNetworkInterfaces(sock);
    for (const auto& [ifname, in_addr] : interfaces) {
        enableHwTimestamps(sock, ifname.c_str());
    }
}

int main(int argc, char *argv[]) {
    int port = 319; // PTP event messages port
    bool verbose = false;
    bool useHwTimestamps = false;
    std::vector<std::string> interfacesToEnable;

    // Simple argument parsing
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
    if (args.count("-t")) {
        std::string type = args["-t"];
        std::cout << "Timestamp type: " << type << std::endl;
        if (type == "hw") {
            useHwTimestamps = true;
        } else if (type == "sw") {
            useHwTimestamps = false;
        } else {
            std::cerr << "Unknown timestamp type: " << type << ", use 'hw' or 'sw'" << std::endl;
            return 1;
        }
    }
    if (args.count("--type")) {
        std::string type = args["--type"];
        std::cout << "Timestamp type: " << type << std::endl;
        if (type == "hw") {
            useHwTimestamps = true;
        } else if (type == "sw") {
            useHwTimestamps = false;
        } else {
            std::cerr << "Unknown timestamp type: " << type << ", use 'hw' or 'sw'" << std::endl;
            return 1;
        }
    }
    if (args.count("-i")) {
        if (args.count("-i") + args.count("--ifc") > 1) {
            std::cerr << "Multiple interface options are not supported in this version." << std::endl;
            return 1;
        }
        std::string ifname = args["-i"];
        std::cout << "Using interface: " << ifname << std::endl;
        interfacesToEnable.push_back(ifname);
    }
    if (args.count("--ifc")) {
        if (args.count("--ifc") + args.count("-i") > 1) {
            std::cerr << "Multiple interface options are not supported in this version." << std::endl;
            return 1;
        }
        std::string ifname = args["--ifc"];
        std::cout << "Using interface: " << ifname << std::endl;
        interfacesToEnable.push_back(ifname);
    }
    if (args.count("-v") || args.count("--verbose")) {
        std::cout << "Verbose mode ON\n";
        verbose = true;
    }
    if (args.count("--help") || args.count("-h")) {
        std::cout << "Usage: " << argv[0] << " [--port <port>] [-p <port>] [-t <type>] [-v|--verbose]\n"
                  << "  --port, -p <port>    UDP port to listen on (default 319)\n"
                  << "  --type, -t <type>    Timestamp type: 'hw' for hardware, 'sw' for software (default 'sw')\n"
                  << "  --ifc, -i <ifname>   Network interface to use (default all interfaces)\n"
                  << "  --verbose, -v        Enable verbose output\n"
                  << "  --help, -h           Show this help message\n";
        return 0;
    }   

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) { 
        std::cerr << "Failed to create socket" << std::endl;
        return 1; 
    }

    // Enable timestamping
    int ts_flags = 0;
    int opt_name = SO_TIMESTAMPING_NEW;

    if (useHwTimestamps) {
        ts_flags = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
        if (interfacesToEnable.empty()) {
            enableHwTimestampsOnAllInterfaces(sock);
        } else {
            for (const auto& ifname : interfacesToEnable) {
                enableHwTimestamps(sock, ifname.c_str());
            }
        }
    } else {
        ts_flags = SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
    }
    if (setsockopt(sock, SOL_SOCKET, opt_name,
                   &ts_flags, sizeof(ts_flags)) < 0) {
        std::cerr << "Failed to set socket options (SO_TIMESTAMPING)" << std::endl;
        return 1;
    }

    // Bind to specific interfaces if provided
    for (const auto& ifname : interfacesToEnable) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name) - 1);
        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
            std::cerr << "Failed to bind to interface " << ifname << std::endl;
            return 1;
        }
    }

    // Bind to the specified port on all interfaces or a specific one
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (interfacesToEnable.size() == 1) {
        auto interfaces = getNetworkInterfaces(sock);
        if (interfaces.count(interfacesToEnable[0])) {
            std::string ip = getIpv4Address(&interfaces[interfacesToEnable[0]]);
            addr.sin_addr.s_addr = inet_addr(ip.c_str());
        } else {
            std::cerr << "Interface " << interfacesToEnable[0] << " not found, binding to INADDR_ANY" << std::endl;
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
    } else {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return 1;
    }
    std::cout << "Listening on port " << port << " IP " << inet_ntoa(addr.sin_addr) << std::endl;

    // Receive loop
    while (true) {
        char buf[2048];
        char ctrl[1024];

        iovec iov { buf, sizeof(buf) };
        msghdr msg {};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = ctrl;
        msg.msg_controllen = sizeof(ctrl);

        // Receive message and ancillary data
        ssize_t len = recvmsg(sock, &msg, 0);
        buf[len < 0 ? 0 : len==sizeof(buf) ? len-1 : len] = '\0'; // Null-terminate the received data
        if (verbose) {
            std::cout << "Received " << len << " bytes: `" << buf << "`" << std::endl;
        }

        if (len < 0) { 
            std::cerr << "Failed to receive message" << std::endl;
            continue; 
        }

        // Process control messages
        for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
            cmsg != nullptr;
            cmsg = CMSG_NXTHDR(&msg, cmsg)) {

            // Check for timestamping messages
            if (cmsg->cmsg_level == SOL_SOCKET) {
                if (cmsg->cmsg_type == SO_TIMESTAMPING_NEW) {
                    struct scm_timestamping64 *ts = (struct scm_timestamping64*) CMSG_DATA(cmsg);
                    // Kernel gives three timespecs, first one is the SW timestamp
                    std::cout 
                        << "SW timestamp: " << ts->ts[0].tv_sec << "." << std::setfill('0') << std::setw(9) << ts->ts[0].tv_nsec << std::endl
                        << "HW timestamp: " << ts->ts[2].tv_sec << "." << std::setfill('0') << std::setw(9) << ts->ts[2].tv_nsec << std::endl
                        ;
                }
                else {
                    if (verbose) {
                        std::cout << "Unhandled cmsg_type " << cmsg->cmsg_type << std::endl;
                    }
                }
            }
            else {
                if (verbose) {
                    std::cout << "Unhandled cmsg_level " << cmsg->cmsg_level << std::endl;
                }
            }
        }
    }
}