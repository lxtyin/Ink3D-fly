cmake_minimum_required(VERSION 3.4)
project(Ink3D)

set(CMAKE_CXX_STANDARD 17)

set(
        SOURCES
        ink/renderer/Renderer.cxx
        ink/probes/IBLFilter.cxx
        ink/probes/ReflectionProbe.cxx
        ink/probes/LightProbe.cxx
        ink/core/File.cxx
        ink/core/Date.cxx
        ink/core/Error.cxx
        ink/core/Format.cxx
        ink/camera/PerspCamera.cxx
        ink/camera/OrthoCamera.cxx
        ink/camera/Camera.cxx
        ink/objects/Material.cxx
        ink/objects/Defines.cxx
        ink/objects/Instance.cxx
        ink/objects/Uniforms.cxx
        ink/objects/Image.cxx
        ink/objects/Mesh.cxx
        ink/utils/ConvexHull.cxx
        ink/utils/ColorTransform.cxx
        ink/utils/Viewer.cxx
        ink/math/Euler.cxx
        ink/math/Matrix.cxx
        ink/math/Ray.cxx
        ink/math/Random.cxx
        ink/math/Vector2.cxx
        ink/math/Vector3.cxx
        ink/math/Vector4.cxx
        ink/meshes/SphereMesh.cxx
        ink/meshes/BoxMesh.cxx
        ink/meshes/CylinderMesh.cxx
        ink/meshes/PlaneMesh.cxx
        ink/audio/Audio.cxx
        ink/loader/Loader.cxx
        ink/window/Window.cxx
        ink/postprocess/FXAAPass.cxx
        ink/postprocess/ToneMapPass.cxx
        ink/postprocess/BlurPass.cxx
        ink/postprocess/LightPass.cxx
        ink/postprocess/SSAOPass.cxx
        ink/postprocess/CopyPass.cxx
        ink/postprocess/RenderPass.cxx
        ink/postprocess/BlendPass.cxx
        ink/postprocess/ColorGradePass.cxx
        ink/postprocess/SSRPass.cxx
        ink/postprocess/BloomPass.cxx
        ink/physics/Physics.cxx
        ink/scene/Scene.cxx
        ink/graphics/Gpu.cxx
        ink/graphics/ShaderLib.cxx
        ink/graphics/Software.cxx
        ink/lights/LinearFog.cxx
        ink/lights/Light.cxx
        ink/lights/HemisphereLight.cxx
        ink/lights/SpotLight.cxx
        ink/lights/PointLight.cxx
        ink/lights/Exp2Fog.cxx
        ink/lights/Shadow.cxx
        ink/lights/DirectionalLight.cxx
        libs/glad/glad.c
#        test/shading/ShadingTest.cxx
        Asserts/GetStart.cxx
        Asserts/MyCamera.cpp Asserts/MyCamera.h Asserts/expend.cpp Asserts/expend.h Asserts/ModelLoader.cpp Asserts/ModelLoader.h Asserts/ParticleInstance.cpp Asserts/ParticleInstance.h remote/Remote.cpp remote/Remote.h)


find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS} assimp remote)
link_directories(${SDL2_LIBDIR} assimp)

set(
        DEPENDENCIES
        mingw32
        SDL2main
        SDL2
        libassimp.dll.a
)

add_executable(Ink3D ${SOURCES})

target_link_libraries(Ink3D ${DEPENDENCIES} -lwsock32 -lpthread)
