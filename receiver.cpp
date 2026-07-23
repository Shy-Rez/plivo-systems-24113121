#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr size_t MAX_FRAMES = 1000000;

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
        in_addr.sin_port = htons(47002);
        in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        if (bind(in_sock.fd, reinterpret_cast<struct sockaddr*>(&in_addr), sizeof(in_addr)) < 0) {
            perror("bind 47002");
            return 1;
        }

        UdpSocket out_sock;
        struct sockaddr_in player{};
        player.sin_family = AF_INET;
        player.sin_port = htons(47020);
        player.sin_addr.s_addr = inet_addr("127.0.0.1");

        std::array<uint8_t, 2048> buf{};
        std::vector<uint8_t> received(MAX_FRAMES, 0);

        while (true) {
            ssize_t n = recvfrom(in_sock.fd, buf.data(), buf.size(), 0, nullptr, nullptr);
            if (n <= 0) continue;
            
            if (n == 164 || n == 324) {
                uint32_t seq;
                std::memcpy(&seq, buf.data(), sizeof(seq));
                uint32_t host_seq = ntohl(seq);

                if (host_seq < MAX_FRAMES && !received[host_seq]) {
                    received[host_seq] = 1;
                    sendto(out_sock.fd, buf.data(), 164, 0, reinterpret_cast<struct sockaddr*>(&player), sizeof(player));
                }

                if (n == 324 && host_seq > 0) {
                    uint32_t prev_seq = host_seq - 1;
                    if (prev_seq < MAX_FRAMES && !received[prev_seq]) {
                        received[prev_seq] = 1;

                        std::array<uint8_t, 164> out_buf{};
                        uint32_t net_prev_seq = htonl(prev_seq);
                        std::memcpy(out_buf.data(), &net_prev_seq, 4);
                        std::memcpy(out_buf.data() + 4, buf.data() + 164, 160);

                        sendto(out_sock.fd, out_buf.data(), 164, 0, reinterpret_cast<struct sockaddr*>(&player), sizeof(player));
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
