#include <chrono>
#include <thread>
#include <random>
#include <atomic>
#include "ping-pong.hpp"

std::atomic_int m(0); // last value
int ping = 1, pong = -1;
std::atomic_bool has_ping(false);
std::atomic<critical_section_callback_t> critical_section_callback(nullptr);

inline bool link_fails_simulation(int tag, MPI::Comm &comm) {
    // Lost token with gauss distribution
    static std::default_random_engine generator;
    static std::normal_distribution<float> distribution(0.0, 1.0);
    auto value = distribution(generator);

    return abs(value) > 0.5 && comm.Get_rank() == 4 &&
           tag == PONG_TAG; // simulate link 4->next fail (avoid loose of 2 tokens in one round)
}

inline bool want_to_enter_critical_section() {
    return critical_section_callback != nullptr;
}

inline void pass_token(int token, int tag, MPI::Comm &comm) {
    m.store(token, std::memory_order_relaxed);
    int next = (comm.Get_rank() + 1) % comm.Get_size();
    if (link_fails_simulation(tag, comm))
        std::cout << "link " << comm.Get_rank() << " -> " << next << " fails. Token lost.\n";
    else
        comm.Send(&token, 1, MPI::INT, next, tag);
}

inline void pass_ping(MPI::Comm &comm) {
    std::cout << comm.Get_rank() << " - send ping " << ping << "\n";
    pass_token(ping, PING_TAG, comm);
}

inline void pass_pong(MPI::Comm &comm) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << comm.Get_rank() << " - send pong " << pong << "\n";

    pass_token(pong, PONG_TAG, comm);
}

void init(MPI::Comm &comm) {
    if (comm.Get_rank() == 0) {
        pass_ping(comm);
        pass_pong(comm);
    } else if (comm.Get_rank() == 2) {
        critical_section_callback = [](MPI::Comm &comm) {
            std::cout << comm.Get_rank() << " - in critical section. Go sleep.\n";
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::cout << comm.Get_rank() << " - in critical section. Weak up.\n";
        };
    }
}

void register_callbacks(tag_callback_map &map) {
    map[PING_TAG] = on_ping_rcv;
    map[PONG_TAG] = on_pong_rcv;
}

void regenerate(int val, MPI::Comm &comm) {
    ping = (abs(val) % comm.Get_size()) + 1;
    pong = -ping;
}

void incarnate(int val, MPI::Comm &comm) {
    ping = ((abs(val) + 1) % comm.Get_size()) + 1;
    pong = -ping;
}

void try_enter_critical_section(MPI::Comm &comm) {
    if (want_to_enter_critical_section()) {
        has_ping = true;
        auto f = [&comm]() {
            auto callback = critical_section_callback.load();
            callback(comm);
            pass_ping(comm);
            has_ping = false;
        };

        std::thread thread(f);
        thread.detach();
    } else {
        pass_ping(comm);
    }
}

bool on_ping_rcv(int buf[BUF_SIZE], MPI::Status &status, MPI::Comm &comm) {
    int token = buf[0];
    if (m.load(std::memory_order_relaxed) == token) { // pong lost, regenerate it
        std::cout << comm.Get_rank() << " - pong lost\n";

        regenerate(token, comm);
        try_enter_critical_section(comm);
        pass_pong(comm);
    } else { // ping ok
        std::cout << comm.Get_rank() << " - ping received\n";

        ping = token;
        try_enter_critical_section(comm);
    }

    return true;
}

bool on_pong_rcv(int buf[BUF_SIZE], MPI::Status &status, MPI::Comm &comm) {
    int token = buf[0];
    if (has_ping) { // pong meet ping
        std::cout << comm.Get_rank() << " - pong meet ping\n";
        incarnate(token, comm);

        pass_pong(comm);
    } else if (m.load(std::memory_order_relaxed) == token) { // ping lost, regenerate it
        std::cout << comm.Get_rank() << " - ping lost\n";

        regenerate(token, comm);

        pass_ping(comm);
        pass_pong(comm);
    } else { // pong, ok
        std::cout << comm.Get_rank() << " - pong received\n";

        pong = token;
        pass_pong(comm);
    }

    return true;
}
