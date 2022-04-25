#ifndef VTKSOURCE_h
#define VTKSOURCE_h

#include "abstractsource.h"

#include <string>
#include <vector>
#include <fstream>

class VTKSource : public AbstractSource
{
    std::string m_pointsFileName;
    std::string m_pointsBinaryName;
    unsigned long long int m_nRows;
    int m_nCols;

    std::vector<std::string> m_fieldNames;  //!column List


  public:
    int readHeader();
    int readData();
    VTKSource();
    ~VTKSource();


};

#endif
