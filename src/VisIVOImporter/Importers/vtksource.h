#ifndef VTKSOURCE_h
#define VTKSOURCE_h

#include "abstractsource.h"

#include <string>
#include <vector>
#include <fstream>

class VTKSource : public AbstractSource
{

  public:
    int readHeader();
    int readData();
    VTKSource();
    ~VTKSource() = default;


};

#endif
