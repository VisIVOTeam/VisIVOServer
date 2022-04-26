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
#include "slicerpipe.h"

#include <cstdlib>
#include <cstring>
#include <sstream>

#include <vtkGenericRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkColorTransferFunction.h>
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkLookupTable.h"
#include "vtkStructuredPoints.h"

#include "vtkOutlineFilter.h"
#include "vtkPlane.h"
#include "vtkScalarBarActor.h"
#include "vtkImageMapper.h"

#include "vtkCutter.h"

#include "luteditor.h"

#include "visivoutils.h"

#include "vtkContourFilter.h"
//---------------------------------------------------------------------
SlicerPipe::SlicerPipe(VisIVOServerOptions options)
//---------------------------------------------------------------------
{
  m_visOpt = options;
  constructVTK();
  m_colorTransferFunction = vtkColorTransferFunction::New();
}
//---------------------------------
SlicerPipe::~SlicerPipe()
//---------------------------------
{
  destroyVTK();
  m_colorTransferFunction->Delete();
}
//---------------------------------
void SlicerPipe::destroyAll()
//---------------------------------
{
  m_colorTransferFunction->Delete();
}
//------------------------------------------
int SlicerPipe::createPipe()
//------------------------------------------
{
  m_range[0] = 0;
  m_range[1] = 0;

  float tmp[1];
  std::ifstream inFile;
  vtkFloatArray *volumeField = vtkFloatArray::New();
  volumeField->SetNumberOfTuples(m_visOpt.nRows);
  int vIndex;

  if (m_visOpt.dataRead && m_visOpt.goodAllocation)
  {
    std::map<std::string, int>::iterator p;
    for (p = m_visOpt.columns.begin(); p != m_visOpt.columns.end(); p++)
    {
      // std::cerr<<p->first<<" "<<p->second<<"  "<<m_visOpt.sliceField<<std::endl;
      if (p->first == m_visOpt.sliceField)
        vIndex = p->second;
    }
    for (int i = 0; i < m_visOpt.nRows; i++)
      volumeField->SetValue(i, m_visOpt.tableData[vIndex][i]);
  }
  else
  {

    inFile.open(m_visOpt.path.c_str(), ios::binary);
    // std::clog<<m_visOpt.path.c_str()<<std::endl;

    if (!inFile)
      return -1;
    inFile.seekg((m_visOpt.nVRenderingField * m_visOpt.nRows) * sizeof(float));

    for (int i = 0; i < m_visOpt.nRows; i++)
    {
      inFile.read((char *)(tmp), sizeof(float));

      if (m_visOpt.needSwap)
        tmp[0] = floatSwap((char *)(&tmp[0]));
      /*       if(tmp[0] <0 || tmp[0] > 255){
        std::cout << tmp[0]<<std::endl;
        }  */
      volumeField->SetValue(i, tmp[0]);
    }
    inFile.close();
  } // else
  volumeField->SetName(m_visOpt.vRenderingField.c_str());
  volumeField->GetRange(m_range);

  vtkStructuredPoints *structPoints = vtkStructuredPoints::New();
  structPoints->SetDimensions(m_visOpt.comp[0], m_visOpt.comp[1], m_visOpt.comp[2]);
  structPoints->GetPointData()->SetScalars(volumeField);

  // vtkStructuredPointsReader *SPR = vtkStructuredPointsReader::New();
  //     SPR->SetFileName("/home/ube/bovolo.vtk");  // PARAMETRO: file da aprire

  vtkOutlineFilter *OF = vtkOutlineFilter::New();
  /* VTK9 migration
     OF->SetInput(structPoints);
     replaced
     OF->SetInputData(structPoints);

  */
  OF->SetInputData(structPoints);
  // vtkOutlineFilter *OF = vtkOutlineFilter::New();
  //    OF->SetInput((vtkDataSet *) SPR->GetOutput());

  vtkPolyDataMapper *PDM = vtkPolyDataMapper::New();
  /* VTK9 migration
     PDM->SetInput((vtkPolyData *) OF->GetOutput());
     replaced
     PDM->SetInputData((vtkPolyData *) OF->GetOutput());

  */
  PDM->SetInputData((vtkPolyData *)OF->GetOutput());
  PDM->SetNumberOfPieces(1);
  PDM->SetScalarRange(0, 1);
  PDM->SetColorMode(1);
  PDM->SetResolveCoincidentTopology(0);
  PDM->SetScalarMode(0);
  /* VTK 9 migration
  removed
  PDM->SetImmediateModeRendering(0);
  */
  PDM->SetScalarVisibility(1);
  PDM->SetUseLookupTableScalarRange(0);

  vtkActor *A = vtkActor::New();
  A->SetMapper(PDM);
  A->SetOrigin(0, 0, 0);
  A->SetPosition(0, 0, 0);
  A->SetScale(1, 1, 1);
  A->SetPickable(1);
  A->SetVisibility(1);

  //-----------------------------------------
  // colortable e pipeline per visualizzarla
  //-----------------------------------------
  vtkLookupTable *LT = vtkLookupTable::New();
  //    LT->SetAlphaRange(1 , 1);
  //    LT->SetHueRange(0 , 0.66667);
  //    LT->SetNumberOfTableValues(256);
  //    LT->SetSaturationRange(1 , 1);
  //    LT->SetValueRange(1 , 1);
  //    LT->SetRamp(1);
  //    LT->SetScale(0);
  //    LT->SetVectorMode(1);
  SelectLookTable(&m_visOpt, LT);

  vtkImageMapper *IM = vtkImageMapper::New();

  vtkScalarBarActor *SBA = vtkScalarBarActor::New();
  SBA->SetMapper(IM);
  SBA->SetLookupTable(LT);
  SBA->SetOrientationToHorizontal();
  SBA->SetPosition(0.1, 0);    // posizione dell'angolo inferiore sinistro [0..1]
  SBA->SetPosition2(0.8, 0.1); // posizione dell'angolo superiore destro   [0..1]

  //-----------------------------------------
  // pipeline di estrazione e visualizzazione della slice
  //-----------------------------------------

  vtkPlane *P = vtkPlane::New();
  if (m_visOpt.nSlicePlane >= 0)
  {
    if (m_visOpt.nSlicePlane == 1) // y Plane
    {
      P->SetNormal(0, 1, 0); // PARAMETRO:  normale   del piano di taglio
      m_visOpt.azimuth = 0;
      m_visOpt.elevation = 90;
    }
    if (m_visOpt.nSlicePlane == 2) // z Plane
    {
      P->SetNormal(0, 0, 1); // PARAMETRO:  normale   del piano di taglio
      m_visOpt.azimuth = 0.001;
      m_visOpt.elevation = 0;
    }
    if (m_visOpt.nSlicePlane == 0) // x Plane
    {
      P->SetNormal(1, 0, 0); // PARAMETRO:  normale   del piano di taglio
      m_visOpt.azimuth = 90;
      m_visOpt.elevation = 0;
    }
    P->SetOrigin(m_visOpt.slicePosition, m_visOpt.slicePosition, m_visOpt.slicePosition); // PARAMETRO:  posizione del piano di taglio
  }
  else if (m_visOpt.slicePlaneNormal != "none" || m_visOpt.slicePlanePoint != "none")
  { // not ortogonal plane
    P->SetNormal(m_visOpt.slicePlaneNormalValue[0], m_visOpt.slicePlaneNormalValue[1], m_visOpt.slicePlaneNormalValue[2]);
    P->SetOrigin(m_visOpt.slicePlanePointValue[0], m_visOpt.slicePlanePointValue[1], m_visOpt.slicePlanePointValue[2]);
  }
  // vtkIndent indent;
  // std::ofstream planeof("plane.txt");
  // P->PrintSelf(planeof,indent);
  // planeof.close();

  vtkCutter *C = vtkCutter::New();
  C->SetCutFunction(P);
  /* VTK9 migration
  C->SetInput(structPoints);
  replaced
  C->SetInputData(structPoints);

*/
  C->SetInputData(structPoints);
  //   C->SetInput((vtkDataSet *) SPR->GetOutput());
  // C->SetValue(0 , 0);
  // C->SetSortBy(0);
  C->SetGenerateCutScalars(1);

  vtkPolyDataMapper *PDM2 = vtkPolyDataMapper::New();
  /* VTK9 migration
     PDM2->SetInput((vtkPolyData *) C->GetOutput());
     replaced
     PDM2->SetInputData((vtkPolyData *) C->GetOutput());

  */
  PDM2->SetInputData((vtkPolyData *)C->GetOutput());
  PDM2->SetLookupTable(LT);
  double b[2];
  b[0] = m_range[0];
  b[1] = m_range[1];
  if (m_visOpt.isColorRangeFrom)
    b[0] = m_visOpt.colorRangeFrom;
  if (m_visOpt.isColorRangeTo)
    b[1] = m_visOpt.colorRangeTo;
  PDM2->SetScalarRange(b[0], b[1]); // PARAMETRO:  range di applicazione della ColorMap
  PDM2->SetColorMode(1);
  PDM2->SetResolveCoincidentTopology(0);
  PDM2->SetScalarMode(0);
  /*VTK9 migration
  removed
  PDM2->SetImmediateModeRendering(0);
  */
  PDM2->SetScalarVisibility(1);
  PDM2->SetUseLookupTableScalarRange(0);

  vtkActor *A2 = vtkActor::New();
  A2->SetMapper(PDM2);
  /*
  vtkIndent indent;
  std::ofstream planeof("A2.txt");
  A2->PrintSelf(planeof,indent);
  planeof.close();*/

  // -----------------------------------
  // Insert all actors into the renderer
  // -----------------------------------

  m_pRenderer->AddActor(A2);
  if (m_visOpt.showBox)
    m_pRenderer->AddActor(A); // PARAMETRO: bounding-box  visibility
  if (m_visOpt.showLut)
    m_pRenderer->AddActor(SBA); // PARAMETRO: scalarbar visibility

  m_pRenderWindow->AddRenderer(m_pRenderer);

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

  if (m_visOpt.imageSize == "small")
    m_pRenderWindow->SetSize(512, 365);
  else if (m_visOpt.imageSize == "large")
    m_pRenderWindow->SetSize(1024, 731);
  else
    m_pRenderWindow->SetSize(792, 566);

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
  vtkGenericRenderWindowInteractor *inter = vtkGenericRenderWindowInteractor::New();
  m_pRenderWindow->SetInteractor(inter);
  m_pRenderWindow->Render();
  //--------------------------------------------------------------------
  setCamera();
  double *bounds;
  bounds = new double[6];
  bounds[0] = 0;
  bounds[1] = m_visOpt.comp[0];
  bounds[2] = 0;
  bounds[3] = m_visOpt.comp[1];
  bounds[4] = 0;
  bounds[5] = m_visOpt.comp[2];
  m_visOpt.xField = "X";
  m_visOpt.yField = "Y";
  m_visOpt.zField = "Z";

  if (m_visOpt.showAxes)
    setAxes(structPoints, bounds);
  delete[] bounds;
  // open view
  //-----------------------------------
  inter->Start();
  inter->ExitEvent();
  //-----------------------------------

  if (inter != 0)
    inter->Delete();

  A2->Delete();
  PDM2->Delete();
  C->Delete();
  P->Delete();
  SBA->Delete();
  IM->Delete();
  LT->Delete();
  A->Delete();
  PDM->Delete();
  OF->Delete();
  volumeField->Delete();
  return 0;
}
//------------------------------------------
bool SlicerPipe::setLookupTable()
//------------------------------------------
{
  // Create transfer mapping scalar value to opacity

  //  m_lut->SetTableRange(m_range);
  double b[2];
  b[0] = m_range[0];
  b[1] = m_range[1];
  if (m_visOpt.isColorRangeFrom)
    b[0] = m_visOpt.colorRangeFrom;
  if (m_visOpt.isColorRangeTo)
    b[1] = m_visOpt.colorRangeTo;
  if (b[1] <= b[0])
    b[1] = b[0] + 0.0001;

  m_lut->SetTableRange(b[0], b[1]);

  SelectLookTable(&m_visOpt, m_lut);

  int numOfColors = m_lut->GetNumberOfTableValues();

  double step = (m_range[1] - m_range[0]) / numOfColors;

  if (!step)
    return false;

  m_colorTransferFunction->SetColorSpaceToRGB();
  m_colorTransferFunction->RemoveAllPoints();

  for (double i = m_range[0]; i <= m_range[1]; i += step)
  {
    double color[3];
    double alpha;

    m_lut->GetColor(i, color);
    /*    std::clog<<color[0]<<" "<<color[1]<<" "<<color[2]<<" "<<std::endl;*/
    m_colorTransferFunction->AddRGBPoint(i, color[0], color[1], color[2]);
  }

  m_lut->Build();
  m_visOpt.colorScalar = m_visOpt.sliceField;
  colorBar();

  return true;
}
