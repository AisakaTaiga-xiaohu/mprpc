/*
    问题：./include/mprpcconfig.h 无法被正确地引用
    原因：使用aux_source_directory(. SRC_LIST)后，当新建.cc文件，如果此时build中存在makefile
          CMake并不会重新编译，这样编译的项目就不包含新添加的.cc文件
    解决办法：
        1. set(SRC_LIST 所有xx.cc文件)  -- 不适合文件多的情况
        2. 删除buile文件下的所有文件，是CMake重头编译
*/
#include "./include/mprpcconfig.h"

#include <iostream>
#include <string>

// 负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE *pf = fopen(config_file, "r");
    if (pf == nullptr)
    {
        std::cout << config_file << "is not exist! " << std::endl;
        exit(EXIT_FAILURE);
    }

    // 1.注释   2.正确的配置项=  3.去掉开头多余的空格
    while (!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf);

        // 去掉字符串前面多余的空格
        std::string read_buf(buf);
        Trim(read_buf);

        // 判断#的注释 || 空行
        if (read_buf[0] == '#' || read_buf.empty())
        {
            continue;
        }

        // 解析配置项
        int idx = read_buf.find('=');
        if (idx == -1)
        {
            // 配置不合法
            continue;
        }

        std::string key;
        std::string value;
        key = read_buf.substr(0, idx);
        Trim(key);
        // rpcserverip=127.0.0.1\n
        int endidx = read_buf.find('\n', idx);
        value = read_buf.substr(idx + 1, endidx - idx - 1);
        Trim(value);
        m_configMap.insert({key, value});
    }
}

// 查询配置项信息
std::string MprpcConfig::Load(std::string key)
{
    std::unordered_map<std::string, std::string>::iterator it = m_configMap.find(key);
    if (it == m_configMap.end())
    {
        return "";
    }
    else
    {
        return it->second;
    }
}

// 去掉字符串前后的空格
/*
    问题1：读取配置文件遇到额外的换行
    问题2：截取子串的位置错误
    原因1：发现有额外的换行符符'\n'
    原因2：截取子串的endidx与idx的错误估计
    解决办法：都使用gdb调试，查找问题地址
*/
void MprpcConfig::Trim(std::string &src_buf)
{
    int idx = src_buf.find_first_not_of(' ');
    if (idx != -1)
    {
        //  说明字符串前面有空格
        src_buf = src_buf.substr(idx, src_buf.size() - idx);
    }
    // 去掉字符串后面多余的空格
    idx = src_buf.find_last_not_of(' ');
    if (idx != -1)
    {
        //  说明字符串后面有空格
        src_buf = src_buf.substr(0, idx + 1);
    }
}