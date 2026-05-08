#include "PortCalculator.h"

PortCalculator::PortCalculator()
{
}

QPair<int, int> PortCalculator::calculatePorts(int portIndex, int startPort)
{
    // 端口计算公式:
    // dataPort = startPort + (portIndex - 1) * 2
    // cmdPort = dataPort + 1
    
    int dataPort = startPort + (portIndex - 1) * 2;
    int cmdPort = dataPort + 1;
    
    return QPair<int, int>(dataPort, cmdPort);
}

int PortCalculator::getPortIndexFromPorts(int dataPort, int cmdPort, int startPort)
{
    // 验证cmdPort = dataPort + 1
    if (cmdPort != dataPort + 1) {
        return -1; // 无效的端口对
    }
    
    // 从数据端口计算端口索引
    // dataPort = startPort + (portIndex - 1) * 2
    // portIndex = ((dataPort - startPort) / 2) + 1
    
    int portIndex = ((dataPort - startPort) / 2) + 1;
    
    // 验证端口索引范围 (1-16)
    if (portIndex < 1 || portIndex > 16) {
        return -1;
    }
    
    return portIndex;
}
