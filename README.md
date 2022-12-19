Ink3D-fly
========

此项目为Ink3D的开发实例，引擎：[Ink3D](https://github.com/HYPER-THEORY/Ink3D)



##### 开发环境

1. Windows，安装有Cmake，Git
2. clone项目
3. 设置项目目录为工作目录
4. 安装SDL2（需要可find_package）
5. 将bin文件夹下的两个dll复制到exe文件的生成目录下。
6. 运行，应该没有问题了



##### 服务端

代码中附有 `server.cpp`，在服务器上以C++17编译运行，需带上 `-lpthread`，如果是windows，还需加上 `-lwsock32` 。

在 `server.cpp` 中设置服务端端口，默认为7777

在 `config.txt` 中可以设置是否开启联机、服务器IP及端口。



##### 开发

开发时请尽量在Assert中修改或添加插件，不要修改Ink中的内容

![2](http://lxtyin.ac.cn/img/Ink3D_fly/2.png)

![1](http://lxtyin.ac.cn/img/Ink3D_fly/1.png)

![1](http://lxtyin.ac.cn/img/Ink3D_fly/3.png)

![1](http://lxtyin.ac.cn/img/Ink3D_fly/4.png)

![1](http://lxtyin.ac.cn/img/Ink3D_fly/5.png)
