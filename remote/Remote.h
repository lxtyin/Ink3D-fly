//
// Created by lx_tyin on 2022/12/4.
//

#ifndef INK3D_REMOTE_H
#define INK3D_REMOTE_H

#include "tool.hpp"
#include "../ink/math/Euler.h"
#include <thread>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

class Remote {
    SOCKET s_client;
    int local_id;
    std::mutex update_lock;
    vector<Status> players;  /** < id : player_status, will be automatically maintained.*/

    void listen_thread();

public:

    Remote(const string &ip, int hton);

    vector<Status> get_status();

    void update(Vec3 position, Vec3 rotation);

    void logout();
};


#endif //INK3D_REMOTE_H
