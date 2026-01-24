# ics2025-nemu

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/你的用户名/ics2025-nemu)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/你的用户名/ics2025-nemu)](https://github.com/你的用户名/ics2025-nemu/stargazers)

**个人 NEMU 教育模拟器项目**
完整实现了南京大学计算机体系结构课程的 NEMU 模拟器及所有 PA 实验。该项目用于深入理解 CPU 指令执行、内存管理、抽象机实现及操作系统基础原理。

---

## 功能特性

* 完整 NEMU 模拟器内核
* 抽象机（Abstract Machine）实现
* 多 PA 实验（PA0、PA1、…）支持
* Git 分支管理实验版本
* 程序编译、运行与调试
* FCEUX 调试环境集成

---

## 仓库结构

```
ics2025-nemu/
├── nemu/                 # NEMU 模拟器核心
├── abstract-machine/     # 抽象机实现
├── fceux-am/             # FCEUX 调试工具
├── init.sh               # 初始化脚本
├── Makefile              # 根目录编译文件
├── README.md             # 仓库说明文档
```

---

## 安装与初始化

1. 克隆仓库：

```bash
git clone git@github.com:你的用户名/ics2025-nemu.git
cd ics2025-nemu
```

2. 初始化环境：

```bash
bash init.sh nemu
bash init.sh abstract-machine
source ~/.bashrc
```

3. 编译 NEMU：

```bash
cd $NEMU_HOME
make
```

4. 编译抽象机：

```bash
cd $AM_HOME
make
```

---

## 使用说明

* **切换分支进行实验**

```bash
git checkout -b pa0       # 创建 PA0 分支
# 修改代码 / 完成实验
git add .
git commit -m "完成 PA0"
git push -u origin pa0
```

* **切换回 master 分支整合实验**

```bash
git checkout master
git merge pa0
git push
```

* **运行模拟器**

```bash
cd $NEMU_HOME
./nemu <program>
```

---

## 可视化示例

实验运行截图：

![NEMU Running](docs/nemu_run.png)

PA 分支管理示例：

```
* pa2
  pa1
  pa0
  master
```

---

## 成果与亮点

* 全面完成 NEMU 内核及抽象机实验
* 分支隔离，每个 PA 独立开发
* 本地和远程仓库同步，便于备份与展示
* 实验均可复现，适合作为教育参考

---

## 联系与说明

* 本仓库为个人练习项目，与学校官方仓库无直接关联
* 欢迎参考、学习或提出改进建议

