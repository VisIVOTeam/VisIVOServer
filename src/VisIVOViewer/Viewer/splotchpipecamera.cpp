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

#include "splotchpipecamera.h"
#include <cstdlib>
#include <cstring>

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
#include "vtkCubeAxesActor2D.h"
#include "vtkProperty.h"
#include "vtkAxisActor2D.h"
#include "vtkGenericRenderWindowInteractor.h"
//#include"vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#define min(x0, x1) (((x0) < (x1)) ? (x0) : (x1))
#define max(x0, x1) (((x0) > (x1)) ? (x0) : (x1))

//---------------------------------------------------------------------
SplotchPipeCamera::SplotchPipeCamera(VisIVOServerOptions options)
//---------------------------------------------------------------------
{
  m_visOpt = options;
  constructVTK();
  m_glyphFilter = ExtendedGlyph3D::New();
  m_glyph = vtkGlyph3D::New();
  m_pConeActor = vtkActor::New();
  m_polyData = vtkPolyData::New();
  m_pConeMapper = vtkPolyDataMapper::New();
}
//---------------------------------
SplotchPipeCamera::~SplotchPipeCamera()
//---------------------------------
{
  destroyVTK();
  if (m_glyph != 0)
    m_glyph->Delete();
  if (m_glyphFilter != 0)
    m_glyphFilter->Delete();
  if (m_pConeMapper != 0)
    m_pConeMapper->Delete();
  if (m_pConeActor != 0)
    m_pConeActor->Delete();
  if (m_polyData != 0)
    m_polyData->Delete();
}
//-----------------------------------------------------------------------------------
void SplotchPipeCamera::setMirror()
//------------------------------------------------------------------------------------
{
  if (m_visOpt.goodAllocation)
  {
    int xIndex, yIndex, zIndex;
    std::map<std::string, int>::iterator p;
    for (p = m_visOpt.columns.begin(); p != m_visOpt.columns.end(); p++)
    {
      if (p->first == m_visOpt.xField)
        xIndex = p->second;
      if (p->first == m_visOpt.yField)
        yIndex = p->second;
      if (p->first == m_visOpt.zField)
        zIndex = p->second;
    }
    if (m_visOpt.nRows <= 1000)
    {
      m_mirrorEle = m_visOpt.nRows;
      for (int i = 0; i < m_mirrorEle; i++)
      {
        m_mirror[0][i] = m_visOpt.tableData[xIndex][i];
        m_mirror[1][i] = m_visOpt.tableData[yIndex][i];
        m_mirror[2][i] = m_visOpt.tableData[zIndex][i];
      }
      return;
    }
    else
    {
      float xmax = m_visOpt.tableData[xIndex][0];
      float xmin = m_visOpt.tableData[xIndex][0];
      float ymax = m_visOpt.tableData[yIndex][0];
      float ymin = m_visOpt.tableData[yIndex][0];
      float zmax = m_visOpt.tableData[zIndex][0];
      float zmin = m_visOpt.tableData[zIndex][0];
      for (int i = 1; i < m_visOpt.nRows; i++)
      {
        xmax = max(xmax, m_visOpt.tableData[xIndex][i]);
        ymax = max(ymax, m_visOpt.tableData[yIndex][i]);
        zmax = max(zmax, m_visOpt.tableData[zIndex][i]);
        xmin = min(xmin, m_visOpt.tableData[xIndex][i]);
        ymin = min(ymin, m_visOpt.tableData[yIndex][i]);
        zmin = min(zmin, m_visOpt.tableData[zIndex][i]);
      }

      m_mirrorEle = 1000;

      m_mirror[0][0] = xmin;
      m_mirror[1][0] = ymin;
      m_mirror[2][0] = zmin;
      m_mirror[0][999] = xmax;
      m_mirror[1][999] = ymax;
      m_mirror[2][999] = zmax;

      for (int i = 1; i < m_mirrorEle - 1; i++)
      {
        int ele = (((float)rand()) / RAND_MAX) * (m_visOpt.nRows - 1);
        m_mirror[0][i] = m_visOpt.tableData[xIndex][ele];
        m_mirror[1][i] = m_visOpt.tableData[yIndex][ele];
        m_mirror[2][i] = m_visOpt.tableData[zIndex][ele];
      }
    }
  }
  else // if(m_visOpt.goodAllocation)
  {
    ifstream inFile;
    inFile.open(m_visOpt.path.c_str(), ios::binary);
    if (!inFile)
    {
      std::cerr << "Unable to open " << m_visOpt.path << std::endl;
      return;
    }
    float tmpAxis[1];
    float xmax;
    float xmin;
    float ymax;
    float ymin;
    float zmax;
    float zmin;
    int nOfEle = m_visOpt.nRows / m_mirrorEle;
    int counterMirror = 1;

    // X of mirror
    inFile.seekg((m_visOpt.x * m_visOpt.nRows, std::ios::beg) * sizeof(float));
    for (int i = 0; i < m_visOpt.nRows; i++)
    {
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      if (i == 0)
      {
        xmax = tmpAxis[0];
        xmin = tmpAxis[0];
      }
      else
      {
        xmax = max(xmax, tmpAxis[0]);
        xmin = min(xmin, tmpAxis[0]);
        if (i % nOfEle == 0 && counterMirror < 999)
        {
          m_mirror[0][counterMirror] = tmpAxis[0];
          counterMirror++;
        }
      }
    }
    m_mirror[0][0] = xmin;
    m_mirror[0][999] = xmax;

    // Y of mirror
    counterMirror = 1;
    inFile.seekg((m_visOpt.y * m_visOpt.nRows, std::ios::beg) * sizeof(float));
    for (int i = 0; i < m_visOpt.nRows; i++)
    {
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      if (i == 0)
      {
        ymax = tmpAxis[0];
        ymin = tmpAxis[0];
      }
      else
      {
        ymax = max(ymax, tmpAxis[0]);
        ymin = min(ymin, tmpAxis[0]);
        if (i % nOfEle == 0 && counterMirror < 999)
        {
          m_mirror[1][counterMirror] = tmpAxis[0];
          counterMirror++;
        }
      }
    }
    m_mirror[1][0] = ymin;
    m_mirror[1][999] = ymax;
    // Z of mirror
    inFile.seekg((m_visOpt.z * m_visOpt.nRows, std::ios::beg) * sizeof(float));
    for (int i = 0; i < m_visOpt.nRows; i++)
    {
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      if (i == 0)
      {
        zmax = tmpAxis[0];
        zmin = tmpAxis[0];
      }
      else
      {
        zmax = max(zmax, tmpAxis[0]);
        zmin = min(zmin, tmpAxis[0]);
        if (i % nOfEle == 0 && counterMirror < 999)
        {
          m_mirror[2][counterMirror] = tmpAxis[0];
          counterMirror++;
        }
      }
    }

    m_mirror[2][0] = zmin;
    m_mirror[2][999] = zmax;

  } // else  if(m_visOpt.goodAllocation)
}

//-----------------------------------------------------------------------------------
int SplotchPipeCamera::vtkCameraData(SplotchCamera *splCamera)
//------------------------------------------------------------------------------------
{

  int i = 0;
  int j = 0;

  std::ifstream inFile;

  vtkFloatArray *radiusArrays = vtkFloatArray::New();

  vtkFloatArray *xAxis = vtkFloatArray::New();
  vtkFloatArray *yAxis = vtkFloatArray::New();
  vtkFloatArray *zAxis = vtkFloatArray::New();

  xAxis->SetNumberOfTuples(m_mirrorEle);
  yAxis->SetNumberOfTuples(m_mirrorEle);
  zAxis->SetNumberOfTuples(m_mirrorEle);

  xAxis->SetName(m_visOpt.xField.c_str());
  yAxis->SetName(m_visOpt.yField.c_str());
  zAxis->SetName(m_visOpt.zField.c_str());

  int xIndex, yIndex, zIndex;

  for (i = 0; i < m_mirrorEle; i++)
  {
    xAxis->SetValue(i, m_mirror[0][i]); //! this is the row that fill xAxis array
    yAxis->SetValue(i, m_mirror[1][i]);
    zAxis->SetValue(i, m_mirror[2][i]);
  }

  xAxis->GetRange(m_xRange); //! minimum and maximum value
  yAxis->GetRange(m_yRange); //! minimum and maximum value
  zAxis->GetRange(m_zRange); //! minimum and maximum value

  SetXYZ(xAxis, yAxis, zAxis);

  m_polyData->SetPoints(m_points);

  m_points->Delete();

  // connect m_pRendererderer and m_pRendererder window and configure m_pRendererder window
  m_pRenderWindow->AddRenderer(m_pRenderer);

  int nPoints = m_polyData->GetNumberOfPoints();
  //  // std::clog<<nPoints<<std::endl;

  vtkCellArray *newVerts = vtkCellArray::New();
  newVerts->EstimateSize(1, nPoints);
  newVerts->InsertNextCell(nPoints);

  for (int i = 0; i < nPoints; i++)
    newVerts->InsertCellPoint(i);

  m_polyData->SetVerts(newVerts);

  xAxis->Delete();
  yAxis->Delete();
  zAxis->Delete();

  float tmp[1];

  inFile.close();

  setBoundingBox(m_polyData);
  m_pConeMapper->SetInput(m_polyData);
  m_pConeActor->SetMapper(m_pConeMapper);
  m_pRenderer->AddActor(m_pConeActor);

  if (m_visOpt.opacity < 0)
    m_visOpt.opacity = 0;

  else if (m_visOpt.opacity > 1)
    m_visOpt.opacity = 1;

  m_pConeActor->GetProperty()->SetOpacity(m_visOpt.opacity);

  /*  if (m_visOpt.nGlyphs!=0)
      setGlyphs (  );

    if(m_visOpt.scaleGlyphs!="none")
      setScaling ();

    if ( m_visOpt.colorScalar!="none" && m_visOpt.color!="none")
      setLookupTable ( );

    if(m_visOpt.scaleGlyphs!="none"&& m_visOpt.nGlyphs!=0 ||( (m_visOpt.heightscalar!="none" ||  (m_visOpt.radiusscalar!="none" && m_visOpt.nGlyphs!=1))  ))
      m_glyph->ScalingOn();
    else
      m_glyph->ScalingOff(); */

  m_pRenderer->SetBackground(0.0, 0.0, 0.0);
  m_pRenderWindow->SetSize(792, 566);
  m_pRenderWindow->SetWindowName("VisIVOServer View");

  // open view
  //------------------------------------------------------------------
  /**/ vtkGenericRenderWindowInteractor *inter = vtkGenericRenderWindowInteractor::New(); // generic/**/
  m_pRenderWindow->SetInteractor(inter);
  m_pRenderWindow->Render();
  //--------------------------------------------------------------------
  //  m_pRenderer->Render();

  setCamera(splCamera);
  //     QUI
  /*      setAxes ( );
                  //open view
                  //-----------------------------------
        inter->Start();
        inter->ExitEvent();*/
  //-----------------------------------

  //   vtkVRMLExporter *writer= vtkVRMLExporter::New();
  //   writer->SetInput(m_pRenderWindow);
  //   writer->SetFilePointer(stdout);
  //   writer->Write();
  // write->Delete();

  if (inter != 0)
    inter->Delete();
  if (newVerts != 0)
    newVerts->Delete();

  return 0;
}

//---------------------------------------------------------------------
void SplotchPipeCamera::setLookupTable()
//---------------------------------------------------------------------
{

  double b[2];
  m_polyData->GetPointData()->SetActiveScalars(m_visOpt.colorScalar.c_str());

  m_polyData->GetPointData()->GetScalars(m_visOpt.colorScalar.c_str())->GetRange(b);

  m_lut->SetTableRange(m_polyData->GetPointData()->GetScalars()->GetRange());
  m_lut->GetTableRange(b);

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

  colorBar();
}

//---------------------------------------------------------------------
void SplotchPipeCamera::setAxes()
//---------------------------------------------------------------------
{
  vtkCubeAxesActor2D *axesActor = vtkCubeAxesActor2D::New();

  axesActor->SetInput(m_polyData);
  axesActor->UseRangesOn();

  axesActor->SetBounds(m_xRange[0], m_xRange[1], m_yRange[0], m_yRange[1], m_zRange[0], m_zRange[1]);
  axesActor->SetRanges(m_xRange[0], m_xRange[1], m_yRange[0], m_yRange[1], m_zRange[0], m_zRange[1]);

  axesActor->SetViewProp(NULL);
  axesActor->SetScaling(0);
  axesActor->SetPickable(0);
  axesActor->SetCamera(m_pRenderer->GetActiveCamera());
  axesActor->SetCornerOffset(0.1);
  axesActor->SetLabelFormat("%6.5g");
  axesActor->SetInertia(100);
  axesActor->SetFlyModeToOuterEdges();
  axesActor->SetVisibility(true);

  axesActor->SetXLabel(m_visOpt.xField.c_str());
  axesActor->SetYLabel(m_visOpt.yField.c_str());
  axesActor->SetZLabel(m_visOpt.zField.c_str());
  axesActor->Modified();

  m_pRenderer->AddActor2D(axesActor);

  if (axesActor != 0)
    axesActor->Delete();
}

//---------------------------------------------------------------------
void SplotchPipeCamera::setRadius()
//---------------------------------------------------------------------
{
}

//---------------------------------------------------------------------
void SplotchPipeCamera::setResolution()
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
bool SplotchPipeCamera::SetXYZ(vtkFloatArray *xField, vtkFloatArray *yField, vtkFloatArray *zField)
//-------------------------------------------------------------------------
{
  double scalingFactors[3];
  scalingFactors[0] = scalingFactors[1] = scalingFactors[2] = 0;

  m_points = vtkPoints::New();
  m_points->SetNumberOfPoints(m_mirrorEle);

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
    for (i = 0; i < m_mirrorEle; i++)
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
    for (i = 0; i < m_mirrorEle; i++)
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
void SplotchPipeCamera::setScaling()
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

//------------------------------------------------------------------------------
int SplotchPipeCamera::createPipe()
//------------------------------------------------------------------------------
{
  return 0;
}
//------------------------------------------------------------------------------
int SplotchPipeCamera::getCamera(SplotchCamera *splCamera)
//------------------------------------------------------------------------------
{
  setMirror();

  vtkCameraData(splCamera);

  this->destroyVTK();
  return 0;
}
