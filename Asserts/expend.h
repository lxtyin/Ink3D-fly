//
// Created by lx_tyin on 2022/11/16.
//

#include "../ink/utils/Everything.h"

/**
 * Move the center of mesh to the origin point.
 * \param m the mesh
 */
void normalize_mesh(Ink::Mesh *m);

/**
 * rebuild vertexs without index.
 * \param m origin mesh
 * \param idx index array
 */
void Fuck_ids(Ink::Mesh *m, std::vector<int> &idx);

