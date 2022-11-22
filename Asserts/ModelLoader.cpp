//
// Created by lx_tyin on 2022/11/22.
//

#include "ModelLoader.h"
#include "expend.h"
#include "../libs/stb/stb_image.h"

#include <iostream>

namespace Ink{

    //loading directory
    string directory;

    Image* processImage(const string &name, const aiScene *scene){
        Image *result = new Image(0, 0, 0, 1);
        if(name[0] == '*'){
            // embedded texture
            auto tex = scene->GetEmbeddedTexture(name.c_str());
            int siz = tex->mHeight ? tex->mWidth * tex->mHeight : tex->mWidth; //maybe compressed
            unsigned char *buff = stbi_load_from_memory(reinterpret_cast<unsigned char*>(tex->pcData),
                                                        siz,
                                                        &result->width,
                                                        &result->height,
                                                        &result->channel, 0);

            siz = result->width * result->height * result->channel;
            result->data.resize(siz);
            memcpy(result->data.data(), buff, siz);
            stbi_image_free(buff);
        } else {
            // local image file.
            string path = directory + name;
            result = new Image(Ink::Loader::load_image(path));
            // ..
        }

        return result;
    }

    Material* processMaterial(aiMaterial *mat, const aiScene *scene){
        Material *result = new Material(mat->GetName().C_Str());

        // 获取各项属性
        aiColor3D color;
        if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, color)){
            result->color = Vec3(color.r, color.g, color.b);
        }

        // 获取纹理
        for(int i = 0;i < mat->GetTextureCount(aiTextureType_DIFFUSE);i++){
            aiString str;
            mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
            result->color_map = processImage(str.C_Str(), scene);
        }
        return result;
    }

    Instance* processMesh(aiMesh *mesh, const aiScene *scene, Scene &target) {
        Instance *result = Instance::create();
        Mesh *cur = new Mesh("name?");
        result->mesh = cur;

        // 读取所有顶点数据
        for(int i = 0; i < mesh->mNumVertices; i++){
            float tex[2] = {0, 0};
            if(mesh->mTextureCoords[0]) { //如果有纹理坐标就取第一个 ?
                tex[0] = mesh->mTextureCoords[0][i].x;
                tex[1] = mesh->mTextureCoords[0][i].y;
            }
            cur->vertex.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            cur->normal.emplace_back(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            cur->uv.emplace_back(tex[0], tex[1]);
        }
        vector<int> ids;
        for(int i = 0; i < mesh->mNumFaces; i++){
            aiFace &face = mesh->mFaces[i];
            for(int j = 0; j < face.mNumIndices; j++){
                ids.push_back(face.mIndices[j]);
            }
        }
        Fuck_ids(cur, ids);
        cur->create_tangents();

        // 确定材质
        // 暂定单一分组
        if(mesh->mMaterialIndex >= 0){
            aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
            string name = mat->GetName().C_Str();
            cur->groups.push_back({name, 0, (int)cur->vertex.size()});

            Material *my_mat = processMaterial(mat, scene);

//            // 如果存在顶点颜色，生成一张漫反射贴图
//            if(mesh->mColors[0]){
////                processVertexColorMap(mesh);
//                my_mat->color_map = processVertexColorMap(mesh);
//            }
            target.set_material(cur, name, my_mat);
        } else {
            cur->groups.push_back({"defalut", 0, (int)cur->vertex.size()});
        }
        return result;
    }

    Instance* processNode(aiNode *node, const aiScene *scene, Scene &target) {
        Instance *cur = Instance::create();

        aiVector3D position, rotation, scale;
        node->mTransformation.Decompose(scale, rotation, position);
        cur->position = Vec3(position.x, position.y, position.z);
        cur->rotation = Vec3(rotation.x, rotation.y, rotation.z);
        cur->scale    = Vec3(scale.x,    scale.y,    scale.z    );

        for(int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            cur->add(processMesh(mesh, scene, target));
        }
        for(int i = 0; i < node->mNumChildren; i++) {
            cur->add(processNode(node->mChildren[i], scene, target));
        }
        return cur;
    }

    Instance* load_model(const string &path, Scene &target){
        Assimp::Importer import;
        const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
        // 第二个参数为预处理指令，此处指定：全部转换为三角形 | 翻转纹理y轴 | 自动生成法线

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
            std::cerr<<"ERROR::ASSIMP::"<<import.GetErrorString()<<'\n';
            return nullptr;
        }
        directory = path;
        while(!directory.empty() && directory.back() != '/' && directory.back() != '\\') directory.pop_back();

        Instance *result = processNode(scene->mRootNode, scene, target);
        target.add(result);
        return result;
    }
}
