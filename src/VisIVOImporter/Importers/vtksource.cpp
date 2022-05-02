#include "vtksource.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vtkDataReader.h>
#include <vtkNew.h>
#include <vtkInformation.h>
#include <vtkIndent.h>

#include <vtkUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkRectilinearGridReader.h>
#include <vtkRectilinearGrid.h>

#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>

#include "visivoutils.h"

VTKSource::VTKSource()
{
    std::cout << "VTKSource" << std::endl;
}

//---------------------------------------------------------------------
int VTKSource::readHeader()
//---------------------------------------------------------------------
{
    std::cout << "readHeader" << std::endl;
    vtkNew<vtkDataReader> reader;
    vtkNew<vtkInformation> outInfo;
    reader->SetFileName(m_pointsFileName.c_str());
    if (reader->IsFileStructuredPoints())
        type = structured_points;
    else if (reader->IsFilePolyData())
        type = polydata;
    else if (reader->IsFileStructuredGrid())
        type = structured_grid;
    else if (reader->IsFileUnstructuredGrid())
        type = unstructured_grid;
    else if (reader->IsFileRectilinearGrid())
        type = rectilinear_grid;
    
    std::cout << reader->GetHeader() << std::endl;
    std::cout << reader->GetFileType() << std::endl;
    
    m_fieldNames.clear();
    
    
    return 0;
}

// https://stackoverflow.com/questions/11727822/reading-a-vtk-file-with-python

//---------------------------------------------------------------------
int VTKSource::readData()
//---------------------------------------------------------------------
{
    std::cout << "READ VTK FILE" << std::endl;
    std::cout << "Loading " << m_pointsFileName.c_str() << std::endl;
    std::cout << type << std::endl;
    std::ofstream outFile(m_pointsBinaryName.c_str(), std::ofstream::binary);
    
    vtkDataReader *reader;
    if (type == unstructured_grid)
    {
        // particle
        vtkNew<vtkUnstructuredGridReader> reader;
        reader->SetFileName(m_pointsFileName.c_str());
        reader->Update();
        
        vtkUnstructuredGrid *data = reader->GetOutput();
        m_nRows =data->GetNumberOfPoints();
        std::cout << data[0] << std::endl;
        std::cout << m_nRows << std::endl;
        std::cout << data->GetNumberOfCells() << std::endl;
        std::cout << data->GetNumberOfPieces() << std::endl;
        double bounds[6];
        data->GetBounds(bounds);
        std::cout << bounds[0] << " " << bounds[1] << " " << bounds[2] << " " << bounds[3] << " " << bounds[4] << " " << bounds[5] << std::endl;
        
        double center[3];
        data->GetCenter(center);
        std::cout << center[0] << " " << center[1] << " " << center[2] << std::endl;
        
        std::cout << data->GetPointData()[0].GetNumberOfArrays() << std::endl;
        std::cout << data->GetPointData()[0].GetScalars()[0] << std::endl;
        std::cout << data->GetPointData()[0].GetVectors()[0] << std::endl;
        
        // Now check for point data
        vtkPointData *pd = data->GetPointData();
        if (pd)
        {
            std::cout << " contains point data with " << pd->GetNumberOfArrays() << " arrays."             << std::endl;
            std::string suffix;
            
            for (int i = 1; i < 2; i++)
            {
                suffix="";
                std::cout << "\tArray " << i << " is named " << (pd->GetArrayName(i) ? pd->GetArrayName(i) : "NULL") << " #ofcomp: "<<pd->GetAbstractArray(i)->GetNumberOfComponents()<<std::endl;
                
                
                
                for (int j = 0; j < pd->GetAbstractArray(i)->GetNumberOfComponents(); j++)
                {
                    if (pd->GetAbstractArray(i)->GetNumberOfComponents()!=1)
                        suffix=":"+ std::to_string(j);
                    
                    m_fieldNames.push_back(pd->GetArrayName(i)+suffix);
                    std::cout << "\t\t scrivo Array " << pd->GetArrayName(i)+suffix << " di dim " << pd->GetAbstractArray(i)->GetNumberOfTuples()<<std::endl;
                    
                    /*
                     for (vtkIdType tupleIdx = 0; tupleIdx < pd->GetAbstractArray(i)->GetNumberOfTuples(); ++tupleIdx)
                     {
                     std::cout << pd->GetArray(i)->GetComponent(tupleIdx, j) << std::endl;
                     
                     // outFile.write((char *)(std::to_string(pd->GetArray(i)->GetComponent(tupleIdx, j)).c_str()), 1);
                     
                     }
                     */
                    std::cout <<pd->GetAbstractArray(i)->GetNumberOfTuples()<<std::endl;
                    outFile.write((char *)(vtkFloatArray::SafeDownCast(pd->GetAbstractArray(i))->GetPointer(0)), sizeof(float)*pd->GetAbstractArray(i)->GetNumberOfTuples());//sizeof(vtkIntArray) * pd->GetAbstractArray(i)->GetNumberOfTuples());
                    
                    
                }
            }
            
            
            
            
        }
        /*
         // Now check for cell data
         vtkCellData *cd = data->GetCellData();
         if (cd)
         {
         std::cout << " contains cell data with " << cd->GetNumberOfArrays() << " arrays."
         << std::endl;
         for (int i = 0; i < cd->GetNumberOfArrays(); i++)
         {
         std::cout << "\tArray " << i << " is named "
         << (cd->GetArrayName(i) ? cd->GetArrayName(i) : "NULL") << std::endl;
         }
         }
         // Now check for field data
         if (data->GetFieldData())
         {
         std::cout << " contains field data with " << data->GetFieldData()->GetNumberOfArrays()
         << " arrays." << std::endl;
         for (int i = 0; i < data->GetFieldData()->GetNumberOfArrays(); i++)
         {
         std::cout << "\tArray " << i << " is named "
         << data->GetFieldData()->GetArray(i)->GetName() << std::endl;
         }
         }
         */
    }
    else if (type == rectilinear_grid)
    {
        vtkNew<vtkRectilinearGridReader> reader;
        reader->SetFileName(m_pointsFileName.c_str());
        reader->Update();
        vtkRectilinearGrid *data = reader->GetOutput();
        int dim[3] = {0};
        data->GetDimensions(dim);
        std::cout << dim[0] << " " << dim[1] << " " << dim[2] << std::endl;
        std::cout << data[0] << std::endl;
        std::cout << data->GetNumberOfPoints() << std::endl;
        std::cout << data->GetNumberOfCells() << std::endl;
    }
    
    // std::clog<<m_pointsBinaryName.c_str()<<endl;
    //  Read Data
    
    if (!outFile)
        return 1;
    outFile.close();
    
    makeHeader(m_nRows, m_pointsBinaryName, m_fieldNames, m_cellSize, m_cellComp, m_volumeOrTable);
    
    
    return 1;
}
