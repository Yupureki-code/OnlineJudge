#pragma once
#include "comm.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <ctime>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>

namespace fileUtil
{
    // 响应结构体
    using namespace oj_util;
    using namespace OnlineJudge;
    // 高性能文件 I/O 类
    class FileSystem 
    {
    public:
        static bool IsFileExist(const std::string& path_name)
        {
            return std::filesystem::exists(path_name);
        }
        // 生成唯一文件名：年月日时分秒-毫秒级时间戳 + 原子计数器
        static std::string GetUniqueFileName()
        {
             static std::atomic_uint id(0);
            id++;
            std::string ms = TimeUtil::GetTimeMs();
            std::string uniq_id = std::to_string(id);
            return ms + "_" + uniq_id;
        }
        // 读取文件内容
        Response read(const std::string& fullPath, 
                    std::string& content) 
        {
            try 
            {                
                // 检查文件是否存在
                if (!std::filesystem::exists(fullPath)) {
                    return Response(false, "文件不存在: " + fullPath);
                }
                
                // 获取文件大小
                auto fileSize = std::filesystem::file_size(fullPath);
                
                // 根据文件大小选择读取策略
                if (fileSize > 1024 * 1024) {  // 大于 1MB 使用 mmap
                    return readWithMmap(fullPath, content);
                } else {  // 小文件使用传统读取
                    return readWithTraditional(fullPath, content);
                }
            } 
            catch (const std::exception& e) 
            {
                return Response(false, std::string("读取文件异常: ") + e.what());
            }
        }
        
        // 写入文件内容
        Response write(const std::string& fullPath, 
                    const std::string& content,
                        bool is_append = false) 
        {
            try 
            {
                std::string filePath = fullPath.substr(0,fullPath.rfind('/'));   
                if (!std::filesystem::exists(filePath)) {
                    std::filesystem::create_directories(filePath);
                }
                
                if (content.size() > 1024 * 1024) {
                    return writeWithMmap(fullPath, content, is_append);
                } else {
                    return writeWithTraditional(fullPath, content, is_append);
                }
            } catch (const std::exception& e) {
                return Response(false, std::string("写入文件异常: ") + e.what());
            }
        }

        // 追加写入文件内容
        Response append(const std::string& fullPath,
                        const std::string& content)
        {
            return write(fullPath, content, true);
        }

    private:
        
        // 使用 mmap 读取文件
        Response readWithMmap(const std::filesystem::path& path, std::string& content) {
            // 打开文件
            int fd = open(path.c_str(), O_RDONLY);
            if (fd == -1) {
                return Response(false, "打开文件失败: " + std::string(strerror(errno)));
            }
            
            // 获取文件大小
            struct stat sb;
            if (fstat(fd, &sb) == -1) {
                close(fd);
                return Response(false, "获取文件信息失败: " + std::string(strerror(errno)));
            }
            
            // 内存映射
            void* addr = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (addr == MAP_FAILED) {
                close(fd);
                return Response(-1, "内存映射失败: " + std::string(strerror(errno)));
            }
            
            // 预读优化：提前加载数据到内存
            madvise(addr, sb.st_size, MADV_SEQUENTIAL);
            
            // 复制数据到 string
            content.assign(static_cast<char*>(addr), sb.st_size);
            
            // 清理资源
            munmap(addr, sb.st_size);
            close(fd);
            
            return Response(true);
        }
        
        // 使用 mmap 写入文件
        Response writeWithMmap(const std::filesystem::path& path, const std::string& content, bool is_append) 
        {
            int flags = O_RDWR | O_CREAT;
            if(is_append)
                flags |= O_APPEND;
            else
                flags |= O_TRUNC;
            int fd = open(path.c_str(), flags, 0644);
            if (fd == -1) {
                return Response(false, "创建文件失败: " + std::string(strerror(errno)));
            }

            struct stat sb;
            off_t old_size = 0;
            if (is_append && fstat(fd, &sb) == 0) {
                old_size = sb.st_size;
            }

            off_t new_size = old_size + content.size();
            if (ftruncate(fd, new_size) == -1) {
                close(fd);
                return Response(false, "设置文件大小失败: " + std::string(strerror(errno)));
            }

            void* addr = mmap(nullptr, new_size, PROT_WRITE, MAP_SHARED, fd, 0);
            if (addr == MAP_FAILED) {
                close(fd);
                return Response(false, "内存映射失败: " + std::string(strerror(errno)));
            }

            std::memcpy(static_cast<char*>(addr) + old_size, content.data(), content.size());
            msync(addr, new_size, MS_SYNC);
            munmap(addr, new_size);
            close(fd);

            return Response(true);
        }
        
        // 传统方式读取文件
        Response readWithTraditional(const std::filesystem::path& path, std::string& content) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                return Response(false, "打开文件失败: " + path.string());
            }
            
            // 获取文件大小
            auto fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            
            // 预分配内存
            content.resize(fileSize);
            
            // 读取文件内容
            if (!file.read(content.data(), fileSize)) {
                return Response(false, "读取文件失败: " + path.string());
            }
            
            return Response(true);
        }
        
        // 传统方式写入文件
        Response writeWithTraditional(const std::filesystem::path& path, const std::string& content, bool is_append = false) {
            auto mode = std::ios::binary;
            if (is_append) mode |= std::ios::app;
            std::ofstream file(path, mode);
            if (!file) {
                return Response(false, "创建文件失败: " + path.string());
            }
            
            // 写入文件内容
            if (!file.write(content.data(), content.size())) {
                return Response(false, "写入文件失败: " + path.string());
            }
            
            return Response(true);
        }
    };
}
