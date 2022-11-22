//
// Created by lx_tyin on 2022/11/16.
//

#include "expend.h"

void normalize_mesh(Ink::Mesh *m){
    float min_x = 1e18, min_y = 1e18, min_z = 1e18;
    float max_x = -1e18, max_y = -1e18, max_z = -1e18;
    for(auto &[x, y, z]: m->vertex){
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        min_z = std::min(min_z, z);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
        max_z = std::max(max_z, z);
    }
    float mid_x = (min_x + max_x) / 2;
    float mid_y = (min_y + max_y) / 2;
    float mid_z = (min_z + max_z) / 2;
    for(auto &[x, y, z]: m->vertex){
        x -= mid_x;
        y -= mid_y;
        z -= mid_z;
    }
}