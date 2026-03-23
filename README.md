# OnlineJudge

## 版权说明

此项目基于C++-责任链模式的负载均衡式在线OJ平台，该项目的代码，注释，图片，数据库，设计，布局，界面等仅用于个人和第三方学习。禁止第三方者将此项目用于商业目的，包括且不限于转让，销售等目的。严重者将受法律责任

Copyright Notice: 
This project is an online OJ platform based on C - the chain of responsibility pattern with load balancing. The project's code, comments, images, database, design, layout, interface, etc. are only for personal and third-party learning. Third parties are prohibited from using this project for commercial purposes, including but not limited to transfer, sale, etc. Serious violations will be subject to legal liability.

# 项目介绍

C++-责任链模式的负载均衡式在线OJ平台
C++-Load-Balanced Online OJ Platform Based on Chain of Responsibility Pattern

## 1. 项目整体架构

### 1.1 技术栈

后端技术栈:
- 语言 ：C++
- 操作系统 ： Linux
- HTTP 服务器 ：httplib
- JSON 处理 ：jsoncpp
- 模板引擎 ：ctemplate
- 数据库 ：MySQL
- 日志系统 ：自定义日志策略

前端技术栈:
- HTML5 ：页面结构
- CSS3 ：样式和响应式设计
- JavaScript ：交互逻辑
- ACE 编辑器 ：代码编辑
- Bootstrap ：UI 组件

### 1.2 核心组件

oj_server(主服务器):处理 HTTP 请求，管理题目，协调判题 
关键文件: oj_server.cpp , oj_control.hpp , oj_view.hpp , oj_model.hpp

compile_server(编译和判题服务器):负责代码编译和运行，执行判题逻辑
关键文件: compile_server.cpp , entry.hpp , judge.hpp , compiler.hpp

comm(公共组件):提供通用工具和功能
关键文件: comm.hpp , logstrategy.hpp

MySQL(数据库):负责题目的各项数据的存储
关键存储: questions(题目的属性，如标题，难度，描述等) tests(题目的各个测试用例，如输入输出数据等)

### 1.3 架构特点

- 分层架构 ：采用经典的 MVC 设计模式
  
  - Model ： oj_model.hpp 负责数据模型和数据库交互
  - View ： oj_view.hpp 负责 HTML 模板渲染
  - Controller ： oj_control.hpp 负责业务逻辑处理，控制数据的进出
 
- 分布式设计 ：
  
  - 主服务器负责接收用户请求和管理题目
  - 编译服务器集群负责代码编译和运行，支持多实例部署
  - 智能负载均衡机制，选择负载最小的编译服务器
 
- 守护进程设计 ：
  
  - 主服务器和编译服务器都支持作为守护进程运行
  - 适合在生产环境中持续运行
 
- 责任链模式设计 ：
  
  - 模板 ：HandlerProgram 作为后续责任链类的模板类，
  - 入口 ：HandlerEntry 作为责任链入口，实现entry->processer->compiler->runner->judge的高解耦合和高代码可读性，可维护性
  - 预处理 ：HandlerPreProcesser 负责代码源文件的生成
  - 编译 ：HandlerCompiler 负责代码的编译，生成.exe文件
  - 运行 ：HandlerRunner 负责.exe程序的运行，并生成将相应的.stdout和.stderr文件
  - 判题 ：HandlerJudge 负责最终结果的判定，检查.stderr和.stdout来判定程序是否出错以及最终的答案正确

 ## 2. 核心功能模块
 
### 2.1 题目管理模块
- 功能 ：管理题目列表、题目详情、分页显示
- 实现 ：
  - 支持分页显示，每页 5 道题
  - 题目排序功能
  - 题目详情展示，包括描述、输入输出示例等
  - 代码编辑界面，集成 ACE 编辑器
 
### 2.2 判题系统模块
- 功能 ：编译用户代码、运行测试用例、判断结果
- 实现 ：
  - 多编译服务器支持，通过负载均衡提高性能
  - 详细的判题逻辑，包括编译错误、运行时错误、正确、错误等状态
  - 支持去除空白字符影响的判题逻辑
  - 多测试用例支持，正确聚合多个测试用例结果
    
### 2.3 前端界面模块
- 功能 ：提供用户访问的界面
- 实现 ：
  - 响应式设计，支持移动端和桌面端
  - 题目列表和详情页面
  - 代码编辑器（ACE）集成
  - 判题结果展示
  - 导航功能
    
 ## 3. 部署与运行
### 3.1 部署结构
- 主服务器 ：运行在 8080 端口，处理 HTTP 请求
- 编译服务器 ：运行在 8081、8082、8083 等端口，处理编译和运行请求
- 配置文件 ： service_machine.conf 配置编译服务器地址和端口
- 日志文件 ： log/ 目录下存储运行日志




  


