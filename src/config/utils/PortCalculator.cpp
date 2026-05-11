#include "PortCalculator.h"

PortCalculator::PortCalculator()
{
}

QPair<int, int> PortCalculator::calculatePorts(int portIndex, int startPort)
{
    int dataPort = startPort + (portIndex - 1);
    int cmdPort = startPort + 16 + (portIndex - 1);

    return QPair<int, int>(dataPort, cmdPort);
}

int PortCalculator::getPortIndexFromPorts(int dataPort, int cmdPort, int startPort)
{
    if (cmdPort != dataPort + 16) {
        return -1;
    }

    int portIndex = (dataPort - startPort) + 1;

    if (portIndex < 1 || portIndex > 16) {
        return -1;
    }

    return portIndex;
}
