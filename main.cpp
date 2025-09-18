#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    // Enable timestamping
    int ts_flags = SOF_TIMESTAMPING_RX_HARDWARE |
                   SOF_TIMESTAMPING_TX_HARDWARE |
                   SOF_TIMESTAMPING_RAW_HARDWARE;
    if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING,
                   &ts_flags, sizeof(ts_flags)) < 0) {
        perror("setsockopt SO_TIMESTAMPING");
        return 1;
    }

    // Bind to port 319 (PTP event messages) for example
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(319);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

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
        if (len < 0) { perror("recvmsg"); continue; }

        for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
             cmsg != nullptr;
             cmsg = CMSG_NXTHDR(&msg, cmsg)) {

            if (cmsg->cmsg_level == SOL_SOCKET &&
                cmsg->cmsg_type == SO_TIMESTAMPING) {

                timespec* ts = (timespec*) CMSG_DATA(cmsg);
                // Kernel gives three timespecs, last one is the HW timestamp
                std::cout << "HW timestamp: "
                          << ts[2].tv_sec << "." << ts[2].tv_nsec << "\n";
            }
        }
    }
}