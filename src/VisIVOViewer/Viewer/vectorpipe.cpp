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

#include "vectorpipe.h"

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
#include "vtkArrowSource.h"
#include "vtkLineSource.h"

//---------------------------------------------------------------------
VectorPipe::VectorPipe(VisIVOServerOptions options)
//---------------------------------------------------------------------
{
  m_visOpt = options;
  constructVTK();
  m_Glyph3D = vtkGlyph3D::New();
  m_PolyDataMapper = vtkPolyDataMapper::New();
  m_actor = vtkActor::New();
}
//---------------------------------
VectorPipe::~VectorPipe()
//---------------------------------
{
  destroyVTK();

  if (m_Glyph3D != 0)
    m_Glyph3D->Delete();
  if (m_PolyDataMapper != 0)
    m_PolyDataMapper->Delete();
  if (m_actor != 0)
    m_actor->Delete();
}
//---------------------------------
void VectorPipe::destroyAll()
//---------------------------------
{
  if (m_Glyph3D != 0)
    m_Glyph3D->Delete();
  if (m_PolyDataMapper != 0)
    m_PolyDataMapper->Delete();
  if (m_actor != 0)
    m_actor->Delete();
}

//-----------------------------------------------------------------------------------
int VectorPipe::createPipe()
//------------------------------------------------------------------------------------
{
  int i = 0;
  int j = 0;

  std::ifstream inFile;

  const int nArrays = 7;
  vtkFloatArray *arrays[nArrays];
  for (int i = 0; i < nArrays; i++)
  {
    arrays[i] = vtkFloatArray::New();
    arrays[i]->SetNumberOfTuples(m_visOpt.nRows);
  }
  float tmp;
  int xIndex, yIndex, zIndex, vxIndex, vyIndex, vzIndex;
  int colorIndex = -1;
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
      if (p->first == m_visOpt.colorScalar)
        colorIndex = p->second;
    }
    for (i = 0; i < m_visOpt.nRows; i++)
    {
      float vxValue, vyValue, vzValue;
      vxValue = m_visOpt.tableData[vxIndex][i];
      vyValue = m_visOpt.tableData[vyIndex][i];
      vzValue = m_visOpt.tableData[vzIndex][i];
      arrays[0]->SetValue(i, m_visOpt.tableData[xIndex][i]); //! these are the row that fill vect app. point
      arrays[1]->SetValue(i, m_visOpt.tableData[yIndex][i]);
      arrays[2]->SetValue(i, m_visOpt.tableData[zIndex][i]);
      arrays[3]->SetValue(i, vxValue); //! these are the rows that fill vect values
      arrays[4]->SetValue(i, vyValue);
      arrays[5]->SetValue(i, vzValue);
      if (colorIndex >= 0)
        arrays[6]->SetValue(i, m_visOpt.tableData[colorIndex][i]);
      else
      {
        float modValue;
        modValue = sqrt((vxValue * vxValue + vyValue * vyValue + vzValue * vzValue));
        arrays[6]->SetValue(i, modValue);
      }
    }
  }
  else
  {
    inFile.open(m_visOpt.path.c_str(), ios::binary); //! open binary file. m_visOpt is the structure (parameter of the constructor) that contain alla data to be visualized
    if (!inFile.is_open())
      return -1;

    float tmpAxis[1];

    for (i = 0; i < m_visOpt.nRows; i++)
    {
      inFile.seekg((m_visOpt.x * m_visOpt.nRows + i) * sizeof(float));
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      arrays[0]->SetValue(i, tmpAxis[0]);

      inFile.seekg((m_visOpt.y * m_visOpt.nRows + i) * sizeof(float));
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      arrays[1]->SetValue(i, tmpAxis[0]);

      inFile.seekg((m_visOpt.z * m_visOpt.nRows + i) * sizeof(float));
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      arrays[2]->SetValue(i, tmpAxis[0]);

      float vxValue, vyValue, vzValue;

      inFile.seekg((m_visOpt.vx * m_visOpt.nRows + i) * sizeof(float));
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      arrays[3]->SetValue(i, tmpAxis[0]);
      vxValue = tmpAxis[0];

      inFile.seekg((m_visOpt.vy * m_visOpt.nRows + i) * sizeof(float));
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      arrays[4]->SetValue(i, tmpAxis[0]);
      vyValue = tmpAxis[0];

      inFile.seekg((m_visOpt.vz * m_visOpt.nRows + i) * sizeof(float));
      inFile.read((char *)(tmpAxis), sizeof(float));
      if (m_visOpt.needSwap)
        tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
      arrays[5]->SetValue(i, tmpAxis[0]);
      vzValue = tmpAxis[0];

      if (colorIndex >= 0)
      {
        inFile.seekg((m_visOpt.nColorScalar * m_visOpt.nRows + i) * sizeof(float));
        inFile.read((char *)(tmpAxis), sizeof(float));
        if (m_visOpt.needSwap)
          tmpAxis[0] = floatSwap((char *)(&tmpAxis[0]));
        arrays[6]->SetValue(i, tmpAxis[0]);
      }
      else
      {
        float modValue;
        modValue = sqrt((vxValue * vxValue + vyValue * vyValue + vzValue * vzValue));
        arrays[6]->SetValue(i, modValue);
      }
    }
  } // else
  double b[2];
  arrays[6]->GetRange(b);
  m_modMin = (float)b[0];
  m_modMax = (float)b[1];

  vtkPolyData *pd = vtkPolyData::New();
  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(m_visOpt.nRows);
  vtkCellArray *verts = vtkCellArray::New();
  verts->SetNumberOfCells(m_visOpt.nRows);
  vtkFloatArray *vectors = vtkFloatArray::New();
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(m_visOpt.nRows);

  pd->SetPoints(points);
  pd->SetVerts(verts);
  pd->GetPointData()->SetVectors(vectors);
  pd->GetPointData()->SetScalars(arrays[6]);
  for (int i = 0; i < m_visOpt.nRows; i++)
  {
    double x = arrays[0]->GetTuple1(i);
    double y = arrays[1]->GetTuple1(i);
    double z = arrays[2]->GetTuple1(i);
    double vx = arrays[3]->GetTuple1(i);
    double vy = arrays[4]->GetTuple1(i);
    double vz = arrays[5]->GetTuple1(i);

    points->SetPoint(i, x, y, z);
    //    #IMPORTANTE
    verts->InsertNextCell(1);
    verts->InsertCellPoint(i);
    vectors->SetTuple3(i, vx, vy, vz);
  }

  //#crea la forma dei glyph
  vtkLineSource *line = vtkLineSource::New();
  line->SetPoint1(0, 0, 0);
  // line->SetPoint1(1,0,0);
  line->SetPoint2(1, 0, 0);
  //#line = vtk.vtkLineSource()
  //#line.SetPoint1(0, 0, 0)
  //#line.SetPoint2(1, 0, 0)

  vtkArrowSource *arrow = vtkArrowSource::New();

  /* VTK9 migration
  m_Glyph3D->SetInput(pd);
  */
  m_Glyph3D->SetInputData(pd);

  if (!m_visOpt.vectorLine)
    m_Glyph3D->SetSourceConnection(arrow->GetOutputPort()); // arrows
  else
    m_Glyph3D->SetSourceConnection(line->GetOutputPort()); // lines

  // G3D->SetScaleFactor(0.1); //riduce la dimensione delle frecce/ 0
  if (m_visOpt.vectorScalingFactor < 0.0)
    m_Glyph3D->ScalingOff();
  else
    m_Glyph3D->SetScaleFactor(m_visOpt.vectorScalingFactor); // riduce la dimensione delle frecce/ 0

  /*#VTK_COLOR_BY_SCALE  0
  #VTK_COLOR_BY_SCALAR 1
  #VTK_COLOR_BY_VECTOR 2*/
  m_Glyph3D->SetColorMode(1); // 0 or 1 in our case is the same: because array[6] or is a scalar or is vect mag.
  /*#VTK_SCALE_BY_SCALAR 0
  #VTK_SCALE_BY_VECTOR 1
  #VTK_SCALE_BY_VECTORCOMPONENTS 2
  #VTK_DATA_SCALING_OFF 3*/
  if (m_visOpt.vectorScale == 0)
    m_Glyph3D->SetScaleMode(0);
  if (m_visOpt.vectorScale == 1)
    m_Glyph3D->SetScaleMode(1);
  if (m_visOpt.vectorScale == -1)
    m_Glyph3D->SetScaleMode(3);

  /* VTK9 migration
  m_PolyDataMapper->SetInput((vtkPolyData *) m_Glyph3D->GetOutput());
  */
  m_PolyDataMapper->SetInputData((vtkPolyData *)m_Glyph3D->GetOutput());
  m_PolyDataMapper->SetNumberOfPieces(1);
  m_PolyDataMapper->SetScalarRange(0, 0.1);
  m_PolyDataMapper->SetColorMode(1);
  m_PolyDataMapper->SetResolveCoincidentTopology(0);
  m_PolyDataMapper->SetScalarMode(0);
  /* VTK9 migration
  m_PolyDataMapper->SetImmediateModeRendering(1);
 */
  m_PolyDataMapper->SetScalarVisibility(0);
  m_PolyDataMapper->SetUseLookupTableScalarRange(0);
  vtkProperty *P = vtkProperty::New();
  P->SetColor(1, 0, 1); // default is white

  m_actor->SetMapper(m_PolyDataMapper);
  //    a->SetProperty(P);
  //    a->SetOrigin(0 , 0 , 0);
  //    a->SetPosition(0 , 0 , 0);
  //    a->SetScale(1 , 1 , 1);
  // /*   a->SetPickable(1);*/
  m_actor->SetVisibility(1);
  m_actor->GetProperty()->SetOpacity(1);

  setBoundingBox(pd);
  m_pRenderer->AddActor(m_actor);

  if (m_visOpt.color != "none")
    setLookupTable();

  m_pRenderer->SetBackground(0.0, 0.0, 0.0);
  if (m_visOpt.backColor == "yellow")
    m_pRenderer->SetBackground(1, 1, 0);
  if (m_visOpt.backColor == "red")
    m_pRenderer->SetBackground(1, 0, 0);
  if (m_visOpt.backColor == "green")
    m_pRenderer->SetBackground(0, 1, 0);
  if (m_visOpt.backColor == "blue")
    m_pRenderer->SetBackground(0, 0, 1);
  if (m_visOpt.backColor == "cyan")
    m_pRenderer->SetBackground(0, 1, 1);
  if (m_visOpt.backColor == "violet")
    m_pRenderer->SetBackground(1, 0, 1);
  if (m_visOpt.backColor == "white")
    m_pRenderer->SetBackground(1, 1, 1);

  m_pRenderWindow->AddRenderer(m_pRenderer);
  m_pRenderWindow->SetSize(1024, 731);

  m_pRenderWindow->SetWindowName("VisIVOServer View");

  if (m_visOpt.stereo)
  {
    m_pRenderWindow->StereoRenderOn();
    if (m_visOpt.stereoMode == "RedBlue")
      m_pRenderWindow->SetStereoTypeToRedBlue();
    else if (m_visOpt.stereoMode == "Anaglyph")
    {
      m_pRenderWindow->SetStereoTypeToAnaglyph();
      m_pRenderWindow->SetAnaglyphColorSaturation(m_visOpt.anaglyphsat);
      m_visOpt.anaglyphmask = trim(m_visOpt.anaglyphmask);
      std::stringstream anatmp(m_visOpt.anaglyphmask);
      int rightColorMask = 4, leftColorMask = 3;
      anatmp >> rightColorMask;
      anatmp >> leftColorMask;
      m_pRenderWindow->SetAnaglyphColorMask(rightColorMask, leftColorMask);
    }
    else
    {
      m_pRenderWindow->SetStereoTypeToCrystalEyes();

      if (m_visOpt.stereoImg == 0)
      {
        m_pRenderWindow->SetStereoTypeToRight();
      }
      else
      {
        m_pRenderWindow->SetStereoTypeToLeft();
      }
    }
    m_pRenderWindow->StereoUpdate();
  }

  // open view
  //------------------------------------------------------------------
  /**/ vtkGenericRenderWindowInteractor *inter = vtkGenericRenderWindowInteractor::New(); // generic/**/
  m_pRenderWindow->SetInteractor(inter);
  m_pRenderWindow->Render();
  //--------------------------------------------------------------------
  //  m_pRenderer->Render();

  setCamera();
  double *bounds;

  arrays[0]->GetRange(m_xRange); //! minimum and maximum value
  arrays[1]->GetRange(m_yRange); //! minimum and maximum value
  arrays[2]->GetRange(m_zRange); //! minimum and maximum value

  bounds = new double[6];
  bounds[0] = m_xRange[0];
  bounds[1] = m_xRange[1];
  bounds[2] = m_yRange[0];
  bounds[3] = m_yRange[1];
  bounds[4] = m_zRange[0];
  bounds[5] = m_zRange[1];

  if (m_visOpt.showAxes)
    setAxes(pd, bounds);
  delete[] bounds;
  // open view
  //-----------------------------------
  inter->Start();
  inter->ExitEvent();
  //-----------------------------------

  //   vtkVRMLExporter *writer= vtkVRMLExporter::New();
  //   writer->SetInput(m_pRenderWindow);
  //   writer->SetFilePointer(stdout);
  //   writer->Write();
  // write->Delete();

  if (inter != 0)
    inter->Delete();

  for (int i = 0; i < nArrays; i++)
    arrays[i]->Delete();

  pd->Delete();
  points->Delete();
  verts->Delete();
  vectors->Delete();
  line->Delete();
  arrow;
  P;

  return 0;
}

//---------------------------------------------------------------------
void VectorPipe::setLookupTable()
//---------------------------------------------------------------------
{

  double b[2];
  b[0] = m_modMin;
  b[1] = m_modMax;
  if (m_visOpt.isColorRangeFrom)
    b[0] = m_visOpt.colorRangeFrom;
  if (m_visOpt.isColorRangeTo)
    b[1] = m_visOpt.colorRangeTo;
  if (b[1] <= b[0])
    b[1] = b[0] + 0.0001;

  m_lut->SetTableRange(b);

  if (m_visOpt.uselogscale == "yes")
    m_lut->SetScaleToLog10();
  else
    m_lut->SetScaleToLinear();

  m_lut->Build();

  SelectLookTable(&m_visOpt, m_lut);

  m_PolyDataMapper->SetLookupTable(m_lut);

  m_PolyDataMapper->SetScalarVisibility(1);
  m_PolyDataMapper->UseLookupTableScalarRangeOn();

  m_actor->SetMapper(m_PolyDataMapper);
  if (m_visOpt.colorScalar == "none")
    m_visOpt.colorScalar = "Vector magnitude";
  if (m_visOpt.showLut)
    colorBar();
}
