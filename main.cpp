#include "ping-pong.hpp"

inline bool loop(tag_callback_map &callback_map, MPI::Comm &comm) {
    int buf[BUF_SIZE];

    MPI::Status status;
    comm.Recv(buf, BUF_SIZE, MPI::INT, MPI::ANY_SOURCE, MPI::ANY_TAG, status);

    //TODO select callback
    auto it = callback_map.find(status.Get_tag());
    if (it == callback_map.end()) { //not found
        std::cerr << "TAG " << status.Get_tag() << " not found in map";
        return true;
    }

    on_message_rcv_t callback = it->second;
    return callback(buf, status, comm);
}

int main(int argc, char **argv) {
    // Initialize the MPI environment
    MPI::Init(argc, argv);
    MPI::Intracomm comm = MPI::COMM_WORLD;

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI::Get_processor_name(processor_name, name_len);

    // Print off a hellBYTEo world message
    printf("Hello world from processor %s, rank %d out of %d processors\n",
           processor_name, comm.Get_rank(), comm.Get_size());

    tag_callback_map map;
    register_callbacks(map);

    init(comm);
    while (loop(map, comm));

    // Finalize the MPI environment.
    MPI::Finalize();
}
