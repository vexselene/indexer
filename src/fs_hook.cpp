#include "./include/fs_hook.h"
#include <sys/inotify.h>
#include <unistd.h>
#include <iostream>

FsHook::FsHook(const std::string& dir_path, Callback cb) : callback(cb) {
    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        std::cerr << "inotify_init failed\n";
        return;
    }
    
    watch_fd = inotify_add_watch(inotify_fd, dir_path.c_str(),
        IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
    
    if (watch_fd < 0) {
        std::cerr << "Failed to watch: " << dir_path << "\n";
    }
}

FsHook::~FsHook() {
    if (watch_fd >= 0) inotify_rm_watch(inotify_fd, watch_fd);
    if (inotify_fd >= 0) close(inotify_fd);
}

int FsHook::get_fd() const {
    return inotify_fd;
}

void FsHook::process_events() {
    char buffer[4096];
    int len = read(inotify_fd, buffer, sizeof(buffer));
    if (len <= 0) return;
    
    int i = 0;
    while (i < len) {
        struct inotify_event* event = (struct inotify_event*)&buffer[i];
        
        if (event->len > 0 && event->name[0] != '.') {
            int ev = 0;
            if (event->mask & (IN_CREATE | IN_MOVED_TO))       ev = 1;
            else if (event->mask & (IN_MODIFY))                 ev = 2;
            else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) ev = 3;
            
            if (ev > 0) {
                callback(event->name, ev);
            }
        }
        
        i += sizeof(struct inotify_event) + event->len;
    }
}