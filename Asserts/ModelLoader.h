//
// Created by lx_tyin on 2022/11/22.
//

#ifndef INK3D_MODELLOADER_H
#define INK3D_MODELLOADER_H

#include "../ink/utils/Everything.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
using std::vector;
using std::string;

namespace Ink{
    Instance* processNode(aiNode *node, const aiScene *scene, Instance *t_node, Scene &target);
    Instance* processMesh(aiMesh *mesh, const aiScene *scene, Instance *t_node, Scene &target);
    Instance* load_model(const string &path, Scene &target);
};

#endif //INK3D_MODELLOADER_H
