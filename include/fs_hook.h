#pragma once
#include <string>
#include <functional>

class FsHook {
public:
    // event: 1 = added, 2 = modified, 3 = deleted
    using Callback = std::function<void(const std::string& name, int event)>;

    FsHook(const std::string& dir_path, Callback cb);
    ~FsHook();
    
    int get_fd() const;        // for poll()
    void process_events();     // call after poll() signals

private:
    int inotify_fd;
    int watch_fd;
    Callback callback;
};