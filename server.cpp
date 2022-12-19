/*
 this is server code of Ink3D-fly
 it can deploy on Windows or linux
 compile with -lpthread, and -lwsock32 if on windows. 
 you can config BOARDCAST_FPS and PORT.
*/


#include <bits/stdc++.h>
#include <thread>
#include "tool.h"

#ifdef __linux__

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
typedef int SOCKET;
typedef unsigned char BYTE;
typedef unsigned long DWORD;
#define FALSE 0
#define SOCKET_ERROR (-1)
#define closesocket close
#define SEND_LAST_ATTR MSG_NOSIGNAL

#else

#include <winsock2.h>
#define SEND_LAST_ATTR 0
#pragma comment(lib,"ws2_32.lib")
typedef int socklen_t;

#endif

#define cur_time ((float)clock() / CLOCKS_PER_SEC)
#define BOARDCAST_FPS 80
#define PORT 7777

/*
 * 消息格式：Message_type Content;
 * Assign_ID    刚连接上，分配id          <id>
 * Update       用户发送更新状态信息       <id speed time px py pz rx ry rz>
 * Boardcast   Server广播所有人状态信息   <id px py pz rx ry rz,id px ...,..>
 * Logout       用户退出                 <id>
 */

std::mutex print_lock;
std::mutex update_lock;

struct Connection {
    SOCKET client;
    sockaddr_in address;
    Status status;
    float last_update_time;     /** < last updated time(server time) */
    float last_receive_time;    /** < last received time(game time), indicate game time of a player, avoid disorder message */
};
std::unordered_map<int, Connection> players;   /** < all connections <id, connection> */
int cur_player_id = 0;               /** < assign an id to each player; */
static std::ofstream logfile;

int log_detail_id = 0;       /** < control variable, to show details of connection with id. */

/**
 * print info to both console and log file, with endl.
 * \param fmt string format like printf
 * \param args mutable variiables like printf
 */
template<typename... T>
inline void Loginfo(const char* fmt, T... args) {
    char buf[128] = {0};
    sprintf(buf, fmt, args...);
    
    print_lock.lock();
    {
        std::cout << cur_time_str() << ": " << buf << std::endl;
        logfile << cur_time_str() << ": " << buf << std::endl;
    }
    print_lock.unlock();
}

/**
 * print error info to both console and log file, with endl.
 * \param fmt string format like printf
 * \param args mutable variiables like printf
 */
template<typename... T>
inline void Logerro(const char* fmt, T... args) {
    char buf[128] = {0};
    sprintf(buf, fmt, args...);

    print_lock.lock();
    {
        std::cerr << cur_time_str() << ": " << buf << std::endl;
        logfile << cur_time_str() << ": " << buf << std::endl;
    }
    print_lock.unlock();
}

/**
 * send message.
 * \param target s_client
 * \param str  info
 */
void send_message(SOCKET target, const Message &msg){
    string str = msg.type + ' ' + msg.content + ';';
    for(int i = 0;i + 1 < str.size();i++){
        if(str[i] == ';'){
            std::cout << str << std::endl;
            return;
        }
    }
    send(target, str.c_str(), str.size(), SEND_LAST_ATTR);
}

/**
 * thread for listening a client
 * \param s_client client socket
 */
void listen_thread(int id, SOCKET s_client){
    //接收数据
    char revData[2048];
    while(true){
        int ret = recv(s_client, revData, 2048, 0);
        if(ret) {
            revData[ret] = 0x00;
            auto msg_vec = fetch_message(revData);

            update_lock.lock();
            {
                if(!players.count(id)){
                    update_lock.unlock();
                    return;   //id已经被删则回收线程。
                }
                if(log_detail_id == id){
                    Loginfo("id = %d, Received: %s", id, revData);
                }
                for(Message &msg : msg_vec){
                    if(msg.type == "Logout") {

                        Loginfo("%s, id = %d has logout.", inet_ntoa(players[id].address.sin_addr), id);

                        closesocket(players[id].client);
                        players.erase(id);

                        update_lock.unlock();
                        return;
                    } else if(msg.type == "Update") {
                        Status st(msg.content);
                        if(st.time > players[id].last_receive_time){
                            players[id].last_receive_time = st.time;
                            players[id].last_update_time = cur_time;
                            players[id].status = st;
                        }
                    }
                }
            }
            update_lock.unlock();
        }
    }
}

/**
 * a thread to listen for link.
 * \param s_server server socket
 */
void wait_thread(SOCKET s_server) {

    std::cout << "Server is working...\n";

    //循环监听
    SOCKET s_client;
    sockaddr_in remoteAddr{};
    socklen_t nAddrlen = sizeof(remoteAddr);
    while(true) {
        s_client = accept(s_server, (sockaddr*)&remoteAddr, &nAddrlen);
        if(!s_client) {
            Logerro("Ink_fly: somebody link failed !");
            continue;
        }

        update_lock.lock();
        {
            cur_player_id++;
            players[cur_player_id] = Connection{s_client, remoteAddr, Status(), cur_time, 0};
            std::thread user(listen_thread, cur_player_id, s_client);
            user.detach();
        }
        update_lock.unlock();

        Loginfo("%s, id = %d join the game.", inet_ntoa(remoteAddr.sin_addr), cur_player_id);

        //连接后立即发送一个id以告知
        send_message(s_client, Message("Assign_id", str_format("%d", cur_player_id)));
    }
}

/**
 * boardcast all status to all client.
 */
void boardcast_thread(){
    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds (1000 / BOARDCAST_FPS));
        
        update_lock.lock();
        {
            string content = "";
            for(auto &[id, cnn] : players){
                content += cnn.status.to_data() + ',';
            }
            if(!content.empty()){
                Message msg("Boardcast", content);
                for(auto &[id, cnn] : players){
                    send_message(cnn.client, msg);
                }
            }
        }
        update_lock.unlock();
    }
}

/**
 * clear unupdated clients.
 */
void clear_thread(){
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        update_lock.lock();
        {
            vector<int> to_clear;
            for(auto &[id, cnn] : players){
                if(cur_time - cnn.last_update_time > 10){
                    to_clear.push_back(id);
                }
            }
            for(int id: to_clear){
                Loginfo("%s, id = %d No connection for a long time.", inet_ntoa(players[id].address.sin_addr), cur_player_id);
                closesocket(players[id].client);
                players.erase(id);
            }
        }
        update_lock.unlock();
    }
}

int main(int argc, char* argv[]) {
#ifdef WIN32
    //初始化WSA
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if(WSAStartup(sockVersion, &wsaData) != 0) {
        Logerro("Ink_fly: init error !");
        return 0;
    }
#endif

    logfile.open("log.txt");

    //创建套接字
    SOCKET s_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(!s_server) {
        Logerro("Ink_fly: socket error !");
        return 0;
    }

    //绑定IP和端口
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

#ifdef __linux__
    sin.sin_addr.s_addr = INADDR_ANY;
#else
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
#endif

    if(bind(s_server, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
        Logerro("Ink_fly: bind error !");
        return 0;
    }

    //开始监听
    if(listen(s_server, 5) == SOCKET_ERROR) {
        Logerro("Ink_fly: listen error !");
        return 0;
    }

    std::thread wait(wait_thread, s_server);
    std::thread boardcast(boardcast_thread);
    std::thread clear(clear_thread);

    while(true){
        string op;
        std::cin >> op;
        if(op == "status"){
            update_lock.lock();
            print_lock.lock();
            {
                std::cout << "-------\n";
                for(auto &[id, cnn] : players){
                    std::cout << "id = " << id << ", "
                              << "last_update = " << cnn.last_update_time << ", "
                              << "ip = " << inet_ntoa(cnn.address.sin_addr) << ", "
                              << "port = " << cnn.address.sin_port << '\n';
                }
                std::cout << "-------" << std::endl;
            }
            print_lock.unlock();
            update_lock.unlock();
        } else if (op == "detail") {
            int id;
            std::cin >> id;
            log_detail_id = id;
        } else if(op == "end"){
            break;
        } else {
            // 输入任何内容屏蔽detail
            log_detail_id = 0;
        }
    }

    logfile.close();
    closesocket(s_server);
#ifdef WIN32
    WSACleanup();
#endif
}