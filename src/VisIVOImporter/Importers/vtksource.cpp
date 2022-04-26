#include "vtksource.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vtkRectilinearGridReader.h>
#include <vtkNew.h>

VTKSource::VTKSource()
{
  std::cout << "VTKSource" << std::endl;
}

//---------------------------------------------------------------------
int VTKSource::readHeader()
//---------------------------------------------------------------------
{
  std::cout << "readHeader" << std::endl;
  return 0;
}

//---------------------------------------------------------------------
int VTKSource::readData()
//---------------------------------------------------------------------
{
  std::cout << "READ VTK FILE" << std::endl;
  std::cout << "Loading " << m_pointsFileName.c_str() << std::endl;

  vtkNew<vtkRectilinearGridReader> reader;
  reader->SetFileName(m_pointsFileName.c_str());
  reader->Update();

  return 1;
}
