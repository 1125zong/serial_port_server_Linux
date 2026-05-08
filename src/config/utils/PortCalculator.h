#ifndef PORTCALCULATOR_H
#define PORTCALCULATOR_H

#include <QPair>

class PortCalculator
{
public:
    PortCalculator();
    
    // Calculate data and command ports for a given port index (1-16) and start port
    // Returns: <dataPort, cmdPort>
    static QPair<int, int> calculatePorts(int portIndex, int startPort);
    
    // Get port index (1-16) from data and command ports
    // Returns: port index, or -1 if ports don't match the pattern
    static int getPortIndexFromPorts(int dataPort, int cmdPort, int startPort);
    
private:
    // Port calculation formula:
    // dataPort = startPort + (portIndex - 1) * 2
    // cmdPort = dataPort + 1
};

#endif // PORTCALCULATOR_H
