//
// Created by lx_tyin on 2022/12/4.
//

#include "Remote.h"
#include <thread>

void Remote::listen_thread() {
    while(true){
        int ret = recv(s_client, revData, 20480, 0);
        if(ret != SOCKET_ERROR) {
            revData[ret] = 0x00;

            auto msg_vec = fetch_message(revData);
            for(Message &msg : msg_vec){
                if(msg.type == "Boardcast") {
                    update_lock.lock();
                    {
                        for(auto& st : fetch_status(msg.content)) {
                            if(st.id != local_id && st.time > latest_time[st.id]){
                                latest_time[st.id] = st.time;
                                dirty_status.push(st);
                            }
                        }
                    }
                    update_lock.unlock();
                } else {
                    std::cout << "Remote: unknow message: " << revData << std::endl;
                }
            }
        } else {
            std::cout << "Remote: server error" << std::endl;
            return;
        }
    }
}

Remote::Remote(const string &ip, int hton) {
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if(WSAStartup(sockVersion, &data)!=0) {
        s_client = -1;
        return;
    }

    s_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s_client == INVALID_SOCKET) {
        std::cerr << "Remote: invalid socket!" << std::endl;
        s_client = -1;
        return;
    }

    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(hton);//端口
    serAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());//IP
    if(connect(s_client, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
        //连接失败
        std::cerr << "Remote: connect error !" << std::endl;
        closesocket(s_client);
        s_client = -1;
        return;
    }

    // 接收一个id
    int ret = recv(s_client, revData, 20480, 0);
    if(ret > 0) {
        revData[ret] = 0x00;
        Message msg = fetch_message(revData)[0];
        if(msg.type == "Assign_id"){
            local_id = to_int(msg.content);
            std::thread listen(&Remote::listen_thread, this);
            listen.detach();
            std::cout << "Remote: Connection succeeded!" << std::endl;
            std::cout << "Remote: id = " << msg.content << std::endl;
            return;
        }
    }
    std::cerr << "Remote: id error !" << std::endl;
    closesocket(s_client);
    s_client = -1;
}

Status Remote::get_status() {
    Status res;
    update_lock.lock();
    {
        if(dirty_status.empty()){
            res.id = 0;
        } else {
            res = dirty_status.front();
            dirty_status.pop();
        }
    }
    update_lock.unlock();
    return res;
}

void Remote::logout() {
    string str = str_format("Logout %d;", local_id);
    std::cout << "Remote: logout" << std::endl;
    send(s_client, str.c_str(), str.size(), 0);
}

void Remote::update(Vec3 position, Vec3 rotation, float speed) {
    Status st;
    st.speed = speed;
    st.id = local_id;
    st.position = position;
    st.rotation = rotation;
    st.time = cur_time;
    string str = "Update " + st.to_data() + ';';
    send(s_client, str.c_str(), str.size(), 0);
}
