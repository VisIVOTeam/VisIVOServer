#include "vtksource.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>


//---------------------------------------------------------------------
int VTKSource::readHeader()
//---------------------------------------------------------------------
{
  return 1;
}

//---------------------------------------------------------------------
int VTKSource::readData()
//---------------------------------------------------------------------
{
    std::cout<<"READ VTK FILE"<<std::endl;
    return 1;
}

