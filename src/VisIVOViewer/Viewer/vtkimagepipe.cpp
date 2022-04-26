/***************************************************************************
 *   Copyright (C) 2008 by Gabriella Caniglia *
 *  gabriella.caniglia@oact.inaf.it *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <cstdlib>
#include <cstring>

#include "vtkimagepipe.h"

#include "visivoutils.h"
#include "luteditor.h"

#include "extendedglyph3d.h"

#include <sstream>
#include <algorithm>

#include "vtkSphereSource.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkCubeSource.h"

#include "vtkCamera.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkLookupTable.h"

#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkGlyph3D.h"
#include "vtkScalarBarActor.h"
#include "vtkOutlineCornerFilter.h"
#include "vtkProperty.h"

#include "vtkGenericRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkAxesActor.h"
#include "vtkGaussianSplatter.h"
#include "vtkSmartPointer.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkImageData.h"
#include "vtkDoubleArray.h"
//---------------------------------------------------------------------
VtkImagePipe::VtkImagePipe(VisIVOServerOptions options)
//---------------------------------------------------------------------
{
  m_visOpt = options;
  constructVTK();
  m_polyData = vtkPolyData::New();
}
//---------------------------------
VtkImagePipe::~VtkImagePipe()
//---------------------------------
{
  destroyVTK();
  if (m_polyData != 0)
    m_polyData->Delete();
}
//---------------------------------
void VtkImagePipe::destroyAll()
//---------------------------------
{
  if (m_polyData != 0)
    m_polyData->Delete();
}

//-----------------------------------------------------------------------------------
int VtkImagePipe::createPipe()
//------------------------------------------------------------------------------------
{
  int i = 0;
  int j = 0;

  std::ifstream inFile;

  vtkFloatArray *radiusArrays = vtkFloatArray::New();
  vtkFloatArray *xAxis = vtkFloatArray::New();
  vtkFloatArray *yAxis = vtkFloatArray::New();
  vtkFloatArray *zAxis = vtkFloatArray::New();

  vtkSmartPointer<vtkDoubleArray> normalsArray =
      vtkSmartPointer<vtkDoubleArray>::New();
  normalsArray->SetNumberOfComponents(3); // 3d normals (ie x,y,z)

  xAxis->SetNumberOfTuples(m_visOpt.nRows);
  yAxis->SetNumberOfTuples(m_visOpt.nRows);
  zAxis->SetNumberOfTuples(m_visOpt.nRows);

  xAxis->SetName(m_visOpt.xField.c_str());
  yAxis->SetName(m_visOpt.yField.c_str());
  zAxis->SetName(m_visOpt.zField.c_str());

  int xIndex, yIndex, zIndex, vxIndex = -1, vyIndex = -1, vzIndex = -1;
  bool haveVector = false;
  if (m_visOpt.dataRead && m_visOpt.goodAllocation)
  {
    std::map<std::string, int>::iterator p;
    for (p = m_visOpt.columns.begin(); p != m_visOpt.columns.end(); p++)
    {
      if (p->first == m_visOpt.xField)
        xIndex = p->second;
      if (p->first == m_visOpt.yField)
        yIndex = p->second;
      if (p->first == m_visOpt.zField)
        zIndex = p->second;
      if (p->first == m_visOpt.xVectorField)
        vxIndex = p->second;
      if (p->first == m_visOpt.yVectorField)
        vyIndex = p->second;
      if (p->first == m_visOpt.zVectorField)
        vzIndex = p->second;
    }
    if (vxIndex >= 0 && vyIndex >= 0 && vzIndex >= 0)
      haveVector = true;
    for (i = 0; i < m_visOpt.nRows; i++)
    {
      xAxis->SetValue(i, m_visOpt.tableData[xIndex][i]); //! this is the row that fill xAxis array
      yAxis->SetValue(i, m_visOpt.tableData[yIndex][i]);
      zAxis->SetValue(i, m_visOpt.tableData[zIndex][i]);
    }
    if (haveVector)
    {
      normalsArray->SetNumberOfTuples(m_visOpt.nRows);
      double cN1[3];

      for (i = 0; i < m_visOpt.nRows; i++)
      {
        double norm;
        norm = m_visOpt.tableData[vxIndex][i] * m_visOpt.tableData[vxIndex][i];
        norm = norm + (m_visOpt.tableData[vyIndex][i] * m_visOpt.tableData[vyIndex][i]);
        norm = norm + (m_visOpt.tableData[vzIndex][i] * m_visOpt.tableData[vzIndex][i]);
        norm = sqrt(norm);
        cN1[0] = m_visOpt.tableData[vxIndex][i] / norm;
        cN1[1] = m_visOpt.tableData[vyIndex][i] / norm;
        cN1[2] = m_visOpt.tableData[vzIndex][i] / norm;
        // add the data to the normals array
        normalsArray->SetTuple(i, cN1);
      }
    }
  }
  else
  {
    inFile.open(m_visOpt.path.c_str(), ios::binary); //! open binary file. m_visOpt is the structure (parameter of the constructor) that contain alla data to be visualized
    // std::clog<<m_visOpt.path.c_str()<<std::endl;
    if (!inFile.is_open())
      return -1;

    float tmpAxis[1];

    for (i = 0; i < m_visOpt.nRows; i++)
    {
      inFile.seekg((m_visOpt.x * m_visOpt.nRows + i) * sizeof(float));

      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));

      xAxis->SetValue(i, tmpAxis[0]); //! this is the row that fill xAxis array

      inFile.seekg((m_visOpt.y * m_visOpt.nRows + i) * sizeof(float));
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));

      yAxis->SetValue(i, tmpAxis[0]);

      inFile.seekg((m_visOpt.z * m_visOpt.nRows + i) * sizeof(float));
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));

      zAxis->SetValue(i, tmpAxis[0]);
    }
  } // else

  xAxis->GetRange(m_xRange); //! minimum and maximum value
  yAxis->GetRange(m_yRange); //! minimum and maximum value
  zAxis->GetRange(m_zRange); //! minimum and maximum value

  SetXYZ(xAxis, yAxis, zAxis);

  m_polyData->SetPoints(m_points);
  if (haveVector)
    m_polyData->GetCellData()->SetNormals(normalsArray);
  m_points->Delete();

  std::string tmpstrname = m_visOpt.imageName;
  if (tmpstrname.find(".vti") == std::string::npos)
    tmpstrname.append(".vti");

  std::string OutputFilename = tmpstrname;

  double bounds[6];
  double volumeScalarBounds[2];
  double volumeOpacityBounds[2];
  vtkSmartPointer<vtkGaussianSplatter> popSplatter = vtkSmartPointer<vtkGaussianSplatter>::New();
  //***********************************
  int ngridX = m_visOpt.vtkImSizeValue[0], ngridY = m_visOpt.vtkImSizeValue[1], ngridZ = m_visOpt.vtkImSizeValue[2];
  // int ngridX=128, ngridY=128, ngridZ=128;
  // Setting the splatterRadius as R=1.5*(bounds[1]-bounds[0])/ngridX
  // float splatterRadius in %
  m_polyData->GetBounds(bounds);
  double dx = bounds[1] - bounds[0];
  double dy = bounds[3] - bounds[2];
  double dz = bounds[5] - bounds[4];
  double maxDelta = 0.;
  int indexDelta = 1;
  if (maxDelta <= dx)
  {
    maxDelta = dx;
    indexDelta = ngridX;
  }
  if (maxDelta <= dy)
  {
    maxDelta = dy;
    indexDelta = ngridY;
  }
  if (maxDelta <= dz)
  {
    maxDelta = dz;
    indexDelta = ngridZ;
  }
  float splatterRadius = m_visOpt.vtkSpacingFact * maxDelta * maxDelta / (indexDelta * 100.0);
  /* VTK9 migration
  popSplatter->SetInput(m_polyData);
  */

  popSplatter->SetInputData(m_polyData);

  popSplatter->SetSampleDimensions(ngridX, ngridY, ngridZ);
  popSplatter->SetRadius(splatterRadius);
  //   popSplatter->SetAccumulationModeToSum();
  popSplatter->SetEccentricity(m_visOpt.vtkEcc);
  //   popSplatter->SetScaleFactor(m_visOpt.vtkScale);
  popSplatter->SetExponentFactor(m_visOpt.vtkGausExp);

  vtkXMLImageDataWriter *writer = vtkXMLImageDataWriter::New();
  /* VTK9 migration
  writer->SetInput(popSplatter->GetOutput());
  */
  writer->SetInputData(popSplatter->GetOutput());

  writer->SetFileName(OutputFilename.c_str());
  writer->Write();

  return 0;
}

//---------------------------------------------------------------------
void VtkImagePipe::setGlyphs()
//---------------------------------------------------------------------
{
  int max = 1000;

  if (m_visOpt.nRows < max)
  {
    /* VTK9 migration
m_glyph->SetInput (m_polyData );
*/

    m_glyph->SetInputData(m_polyData);

    if (m_visOpt.scale == "yes")
      m_glyph->SetScaleFactor(0.04);

    else
      m_glyph->SetScaleFactor(2.5);

    m_pConeMapper->SetInputConnection(m_glyph->GetOutputPort());

    if (m_visOpt.nGlyphs == 1)
    {
      m_sphere = vtkSphereSource::New();
      setResolution();
      setRadius();
      /* VTK9 migration
      m_glyph->SetSource ( m_sphere->GetOutput() );
      */
      m_glyph->SetSourceData(m_sphere->GetOutput());

      m_sphere->Delete();
    }

    else if (m_visOpt.nGlyphs == 2)
    {
      m_cone = vtkConeSource::New();
      setResolution();
      setRadius();
      /* VTK9 migration
     m_glyph->SetSource ( m_cone->GetOutput() );
     */
      m_glyph->SetSourceData(m_cone->GetOutput());
      m_cone->Delete();
    }

    else if (m_visOpt.nGlyphs == 3)
    {
      m_cylinder = vtkCylinderSource::New();
      setResolution();
      setRadius();
      /* VTK9 migration
      m_glyph->SetSource ( m_cylinder->GetOutput() );
      */
      m_glyph->SetSourceData(m_cylinder->GetOutput());
      m_cylinder->Delete();
    }

    else if (m_visOpt.nGlyphs == 4)
    {
      m_cube = vtkCubeSource::New();
      setRadius();
      /* VTK9 migration
m_glyph->SetSource ( m_cube->GetOutput() );
*/
      m_glyph->SetSourceData(m_cube->GetOutput());
      m_cube->Delete();
    }
  }
  return;
}

//---------------------------------------------------------------------
void VtkImagePipe::setLookupTable()
//---------------------------------------------------------------------
{

  double b[2];
  m_polyData->GetPointData()->SetActiveScalars(m_visOpt.colorScalar.c_str());

  m_polyData->GetPointData()->GetScalars(m_visOpt.colorScalar.c_str())->GetRange(b);

  m_lut->SetTableRange(m_polyData->GetPointData()->GetScalars()->GetRange());
  m_lut->GetTableRange(b);
  if (m_visOpt.isColorRangeFrom)
    b[0] = m_visOpt.colorRangeFrom;
  if (m_visOpt.isColorRangeTo)
    b[1] = m_visOpt.colorRangeTo;
  m_lut->SetTableRange(b[0], b[1]);

  if (m_visOpt.uselogscale == "yes")
    m_lut->SetScaleToLog10();
  else
    m_lut->SetScaleToLinear();

  m_lut->Build();

  SelectLookTable(&m_visOpt, m_lut);

  m_pConeMapper->SetLookupTable(m_lut);

  m_pConeMapper->SetScalarVisibility(1);
  m_pConeMapper->UseLookupTableScalarRangeOn();

  m_pConeActor->SetMapper(m_pConeMapper);

  if (m_visOpt.showLut)
    colorBar();
}

//---------------------------------------------------------------------
void VtkImagePipe::setRadius()
//---------------------------------------------------------------------
{
  if (m_visOpt.nGlyphs == 1)
    m_sphere->SetRadius(m_visOpt.radius);

  else if (m_visOpt.nGlyphs == 2)
  {

    m_cone->SetRadius(m_visOpt.radius);
    m_cone->SetHeight(m_visOpt.height);
  }

  else if (m_visOpt.nGlyphs == 3)
  {

    m_cylinder->SetRadius(m_visOpt.radius);
    m_cylinder->SetHeight(m_visOpt.height);
  }
  else if (m_visOpt.nGlyphs == 4)
  {
    m_cube->SetXLength(m_visOpt.radius);
    m_cube->SetYLength(m_visOpt.height);
    m_cube->SetZLength(1);
  }
}

//---------------------------------------------------------------------
void VtkImagePipe::setResolution()
//---------------------------------------------------------------------
{
  if (m_visOpt.nGlyphs == 1)
  {
    m_sphere->SetPhiResolution(10);
    m_sphere->SetThetaResolution(20);
  }

  else if (m_visOpt.nGlyphs == 2)
    m_cone->SetResolution(10);

  else if (m_visOpt.nGlyphs == 3)
    m_cylinder->SetResolution(10);
}

//-------------------------------------------------------------------------
bool VtkImagePipe::SetXYZ(vtkFloatArray *xField, vtkFloatArray *yField, vtkFloatArray *zField)
//-------------------------------------------------------------------------
{
  double scalingFactors[3];
  scalingFactors[0] = scalingFactors[1] = scalingFactors[2] = 0;

  m_points = vtkPoints::New();
  m_points->SetNumberOfPoints(m_visOpt.nRows);

  if (xField->GetNumberOfComponents() != yField->GetNumberOfComponents())
  {
    if (zField && (xField->GetNumberOfComponents() != zField->GetNumberOfComponents() || yField->GetNumberOfComponents() != zField->GetNumberOfComponents()))
    {
      false;
    }
    false; // component mismatch, do nothing
  }

  if (m_visOpt.scale == "yes")
  {

    double size = 0;

    size = (m_xRange[1] - m_xRange[0] != 0 ? m_xRange[1] - m_xRange[0] : m_xRange[1]);
    scalingFactors[0] = size * 0.1;

    size = (m_yRange[1] - m_yRange[0] != 0 ? m_yRange[1] - m_yRange[0] : m_yRange[1]);
    scalingFactors[1] = size * 0.1;

    size = (m_zRange[1] - m_zRange[0] != 0 ? m_zRange[1] - m_zRange[0] : m_zRange[1]);
    scalingFactors[2] = size * 0.1;
  }

  double scalingFactorsInv[3];

  int i = 0;
  for (i = 0; i < 3; i++)
    scalingFactorsInv[i] = ((scalingFactors && scalingFactors[i] != 0) ? 1 / scalingFactors[i] : 0);

  // Set the points data
  if (m_visOpt.scale == "yes")
  {
    for (i = 0; i < m_visOpt.nRows; i++)
    {
      float inPoint[3];
      float outPoint[3];
      inPoint[0] = outPoint[0] = xField->GetValue(i) * scalingFactorsInv[0];
      inPoint[1] = outPoint[1] = yField->GetValue(i) * scalingFactorsInv[1];
      inPoint[2] = outPoint[2] = zField->GetValue(i) * scalingFactorsInv[2];

      m_points->SetPoint(i, outPoint);
    }
  }
  else
    for (i = 0; i < m_visOpt.nRows; i++)
    {
      float outPoint[3];

      outPoint[0] = xField->GetValue(i);
      outPoint[1] = yField->GetValue(i);
      outPoint[2] = zField->GetValue(i);

      m_points->SetPoint(i, outPoint);
    }

  return true;
}

//---------------------------------------------------------------------
void VtkImagePipe::setScaling()
//---------------------------------------------------------------------
{
  m_glyphFilter->SetUseSecondScalar(true);
  m_glyphFilter->SetUseThirdScalar(true);

  m_glyphFilter->SetScaling(1);

  if (m_visOpt.heightscalar != "none" && m_visOpt.scaleGlyphs != "none" && m_visOpt.nGlyphs != 0 && m_visOpt.nGlyphs != 1)
    m_glyphFilter->SetInputScalarsSelectionY(m_visOpt.heightscalar.c_str());

  if (m_visOpt.radiusscalar != "none" && m_visOpt.scaleGlyphs != "none" && m_visOpt.nGlyphs != 0)
    m_glyphFilter->SetInputScalarsSelectionXZ(m_visOpt.heightscalar.c_str());

  if (m_visOpt.nGlyphs != 0)
    m_glyphFilter->SetScaleModeToScaleByScalar();
  else
    m_glyphFilter->ScalarVisibilityOff();
}
