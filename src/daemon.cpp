#include "../include/search_engine.h"
#include "fs_hook.h"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>   // provides socket(), bind(), listen(), accept(), send(), recv()
#include <sys/un.h>       // struct sockaddr_un (Unix domain socket addresses)
#include <unistd.h>       // close(), unlink()
#include <poll.h>         // poll() for multiplexing socket + filesystem events

#define SOCKET_PATH "/tmp/indexer.sock"

/*
    Serializes search results into a JSON string.

    Example output:
    {"results":[{"id":0,"name":"math.txt","path":"/data/math.txt","score":15}]}
*/
std::string to_json(const std::vector<SearchResult>& results) {
    std::string json = "{\"results\":[";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) json += ",";  // comma between array elements
        json += "{\"id\":" + std::to_string(results[i].f_id);
        json += ",\"name\":\"" + results[i].name + "\"";
        json += ",\"path\":\"" + results[i].path + "\"";
        json += ",\"score\":" + std::to_string(results[i].score) + "}";
    }
    json += "]}";
    return json;
}

int main(int argc, char* argv[]) {
    std::string dir_path = std::filesystem::weakly_canonical((argc >= 2) ? argv[1] : "data").string();

    SearchEngine engine;
    engine.build(dir_path);

    // Hook into filesystem events for live index updates
    FsHook hook(dir_path, [&](const std::string& name, int ev) {
        std::string full_path = dir_path + "/" + name;
        if (ev == 1)      std::cout << "Added: " << full_path << "\n";
        else if (ev == 2) std::cout << "Modified: " << full_path << "\n";
        else if (ev == 3) std::cout << "Deleted: " << full_path << "\n";
        
        engine.update_changes(engine.has_changed(dir_path));
    });

    // remove stale socket file from previous run (if it exists)
    unlink(SOCKET_PATH);

    // create a Unix domain socket (it is a local IPC, not a network stack - its faster)
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // Bind the socket to a filesystem path
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind to " << SOCKET_PATH << "\n";
        return 1;
    }

    // Mark socket as passive — it will accept incoming connections
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Failed to listen\n";
        return 1;
    }

    std::cout << "Indexer daemon listening on " << SOCKET_PATH << "\n";

    // watch both socket and filesystem events with poll()
    struct pollfd fds[2];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    fds[1].fd = hook.get_fd();
    fds[1].events = POLLIN;

    // Event loop: handle client requests and filesystem changes
    while (true) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) break;

        //new client connection
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd < 0) continue;

            // Read the query from the client
            char buffer[4096] = {0};
            int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                close(client_fd);
                continue;
            }

            std::string query(buffer);

            auto results = engine.search_query(query);
            std::string json = to_json(results);

            // Send JSON response back to client
            send(client_fd, json.c_str(), json.size(), 0);

            close(client_fd);
        }

        // Filesystem event
        if (fds[1].revents & POLLIN) {
            hook.process_events();
        }
    }

    // Cleanup (unreachable in current loop, but good practice)
    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}