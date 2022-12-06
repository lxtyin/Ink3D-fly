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
    std::queue<Status> dirty_status;    /** < new received status, which has not be used.*/
    std::unordered_map<int, float> latest_time;     /** < latest game time of each player, to avoid disorder. */

    void listen_thread();
public:

    /**
     * construct and link.
     * \param ip
     * \param hton
     */
    Remote(const string &ip, int hton);

    /**
     * pop one status from dirty status.
     * \return if empty, return Status.id = 0.
     */
    Status get_status();

    /**
     * send Status to remote server.
     * \param position
     * \param rotation
     * \param speed
     */
    void update(Vec3 position, Vec3 rotation, float speed);

    /**
     * send logout information.
     */
    void logout();
};

#endif //INK3D_REMOTE_H
