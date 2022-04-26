#include "vtksource.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

VTKSource::VTKSource()
{
    std::cout<<"VTKSource"<<std::endl;
}


//---------------------------------------------------------------------
int VTKSource::readHeader()
//---------------------------------------------------------------------
{
    std::cout<<"readHeader"<<std::endl;
    return 0;
}

//---------------------------------------------------------------------
int VTKSource::readData()
//---------------------------------------------------------------------
{
    std::cout<<"READ VTK FILE"<<std::endl;
    return 1;
}

