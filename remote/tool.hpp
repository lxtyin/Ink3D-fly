#pragma once

#include <bits/stdc++.h>
#include <cstdarg>
#include "../ink/math/Vector3.h"
using Ink::Vec3;
using std::string;
using std::vector;
#define cur_time ((float)clock() / CLOCKS_PER_SEC)

template<typename... T>
inline string str_format(const char* fmt, T... args) {
    char buf[128] = {0};
    sprintf(buf, fmt, args...);
    return buf;
}

inline int to_int(const string &str){
    int n = 0;
    for(char c: str){
        n = n * 10 + (c - '0');
    }
    return n;
}

struct Message {
    string type;
    string content;
    Message(const string &t, const string &c) : type(t), content(c) {}
    Message(const char *data){
        int n = strlen(data);
        type = "";
        content = "";
        int i = 0;
        for(;i < n;i++){
            if(data[i] == ' ') break;
            type.push_back(data[i]);
        }
        for(i++;i < n;i++) content.push_back(data[i]);
    }
    Message(const string &data){
        int idx = data.find_first_of(' ');
        type = data.substr(0, idx);
        content = data.substr(idx + 1, data.size() - idx - 1);
    }
};

inline vector<Message> fetch_message(const string &data){
    vector<Message> res;
    int st = 0;
    for(int i = 0;i < data.size();i++){
        if(data[i] == ';'){
            res.emplace_back(data.substr(st, i - st));
            st = i + 1;
        }
    }
    return res;
}

struct Status {
    int id;
    float time; //此状态时间戳，判断旧消息
    float speed;
    Vec3 position;
    Vec3 rotation;
    Status() = default;
    Status(const string &data){
        std::stringstream ss(data);
        ss >> id >> speed >> time;
        ss >> position.x >> position.y >> position.z;
        ss >> rotation.x >> rotation.y >> rotation.z;
    }
    string to_data() const{
        return str_format("%d %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f", id, speed, time,
                          position.x, position.y, position.z,
                          rotation.x, rotation.y, rotation.z);
    }
};

inline vector<Status> fetch_status(const string &data){
    vector<Status> res;
    int st = 0;
    for(int i = 0;i < data.size();i++){
        if(data[i] == ','){
            res.emplace_back(data.substr(st, i - st));
            st = i + 1;
        }
    }
    return res;
}

