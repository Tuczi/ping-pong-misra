#ifndef PING_PONG_MISRA_PING_PONG_HPP
#define PING_PONG_MISRA_PING_PONG_HPP

#include <unordered_map>
#include <mpi.h>

const int BUF_SIZE = 10;
const int PING_TAG = 1;
const int PONG_TAG = 2;

typedef bool (*on_message_rcv_t)(int buf[BUF_SIZE], MPI::Status &status, MPI::Comm &comm);

typedef void (*critical_section_callback_t)(MPI::Comm &comm);

typedef std::unordered_map<int, on_message_rcv_t> tag_callback_map;

void init(MPI::Comm &comm);

void register_callbacks(tag_callback_map &map);

bool on_ping_rcv(int buf[BUF_SIZE], MPI::Status &status, MPI::Comm &comm);

bool on_pong_rcv(int buf[BUF_SIZE], MPI::Status &status, MPI::Comm &comm);

#endif //PING_PONG_MISRA_PING_PONG_HPP
