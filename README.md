# MySpyder


## 仿spyder的python IDE， ( • ̀ω•́ )✧~。对应spyder的v3.3.3版本

### 功能:
* Editor。
* File explorer。
* Outline。
* Project explorer。
* Find in files。
* History log。

## 截图:
<img src="https://github.com/quan12jiale/MySpyder/blob/master/images/screenshot.png" alt="MySpyder运行截图" />

### 安装:
```
$ git clone git@github.com/quan12jiale/MySpyder.git
```

### 运行:
```
第一种：用Qt creator打开spyder.pro项目文件，开发环境是Qt creator 12.0 mingw，把images文件夹拷贝到exe的上层目录
第二种：把CMake路径“C:\Program Files\CMake\bin”添加到系统环境Path变量；
添加QTDIR变量，变量值为QT安装目录“F:\QT\qt563\msvc2015”；
双击vs_genSln.bat批处理文件，用Visual Studio打开build文件夹下的MySpyder.sln，把images文件夹拷贝到exe的上层目录
```

#### 功能TODO:
- [ ] IPython console.
- [ ] Variable explorer.
