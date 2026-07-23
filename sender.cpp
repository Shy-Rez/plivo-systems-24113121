#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

class UdpSocket {
public:
    int fd;
    UdpSocket() {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }
    }
    ~UdpSocket() {
        if (fd >= 0) close(fd);
    }
};

int main() {
    try {
        UdpSocket in_sock;
        struct sockaddr_in in_addr{};
        in_addr.sin_family = AF_INET;
        in_addr.sin_port = htons(47010);
        in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        if (bind(in_sock.fd, reinterpret_cast<struct sockaddr*>(&in_addr), sizeof(in_addr)) < 0) {
            perror("bind 47010");
            return 1;
        }

        UdpSocket out_sock;
        struct sockaddr_in relay{};
        relay.sin_family = AF_INET;
        relay.sin_port = htons(47001);
        relay.sin_addr.s_addr = inet_addr("127.0.0.1");

        std::array<uint8_t, 2048> buf{};
        std::array<uint8_t, 160> prev_payload{};
        bool has_prev = false;

        while (true) {
            ssize_t n = recvfrom(in_sock.fd, buf.data(), buf.size(), 0, nullptr, nullptr);
            if (n <= 0) continue;
            
            uint32_t seq;
            std::memcpy(&seq, buf.data(), sizeof(seq));
            uint32_t host_seq = ntohl(seq);

            std::array<uint8_t, 324> out_buf{};
            std::memcpy(out_buf.data(), buf.data(), 164);

            if (host_seq % 12 != 0 && has_prev) {
                std::memcpy(out_buf.data() + 164, prev_payload.data(), 160);
                sendto(out_sock.fd, out_buf.data(), 324, 0, reinterpret_cast<struct sockaddr*>(&relay), sizeof(relay));
            } else {
                sendto(out_sock.fd, out_buf.data(), 164, 0, reinterpret_cast<struct sockaddr*>(&relay), sizeof(relay));
            }

            std::memcpy(prev_payload.data(), buf.data() + 4, 160);
            has_prev = true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
