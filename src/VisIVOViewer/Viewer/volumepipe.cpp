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
#include "volumepipe.h"

#include <cstdlib>
#include <cstring>
#include <sstream>

#include <vtkGenericRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkImageData.h>
#include <vtkImageMathematics.h>
#include <vtkImageCast.h>
/* VTK9 migration

#include <vtkVolumeRayCastCompositeFunction.h>
#include <vtkVolumeRayCastMapper.h>
*/

#include "vtkFixedPointVolumeRayCastMapper.h"
#include <vtkVolume.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkVolumeProperty.h>
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkLookupTable.h"

#include "luteditor.h"

#include "visivoutils.h"

#include "vtkContourFilter.h"
//---------------------------------------------------------------------
VolumePipe::VolumePipe(VisIVOServerOptions options)
//---------------------------------------------------------------------
{
  m_visOpt = options;
  constructVTK();
  m_colorTransferFunction = vtkColorTransferFunction::New();
  m_imageData = vtkImageData::New();
  m_math2 = vtkImageMathematics::New();
  m_math = vtkImageMathematics::New();
  m_charData = vtkImageCast::New();
  m_opacityTransferFunction = vtkPiecewiseFunction::New();
  m_volumeProperty = vtkVolumeProperty::New();
  /*vtk9 migration
  m_rayCastCompositFunction = vtkVolumeRayCastCompositeFunction::New();
  */
  m_rayCastMapper = vtkFixedPointVolumeRayCastMapper::New();
  m_volume = vtkVolume::New();
}
//---------------------------------
VolumePipe::~VolumePipe()
//---------------------------------
{
  destroyVTK();
  m_colorTransferFunction->Delete();
  m_imageData->Delete();
  m_math->Delete();
  m_math2->Delete();
  m_charData->Delete();
  m_opacityTransferFunction->Delete();
  m_volumeProperty->Delete();
  /*vtk9 migration
 m_rayCastCompositFunction->Delete();
 */
  m_rayCastMapper->Delete();
  m_volume->Delete();
}
//---------------------------------
void VolumePipe::destroyAll()
//---------------------------------
{
  m_colorTransferFunction->Delete();
  m_imageData->Delete();
  m_math->Delete();
  m_math2->Delete();
  m_charData->Delete();
  m_opacityTransferFunction->Delete();
  m_volumeProperty->Delete();
  /*vtk9 migration
 m_rayCastCompositFunction->Delete();
 */
  m_rayCastMapper->Delete();
  m_volume->Delete();
}
//------------------------------------------
int VolumePipe::createPipe()
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
      if (p->first == m_visOpt.vRenderingField)
        vIndex = p->second;

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

  /* VTK9 migration
    m_imageData->SetScalarTypeToFloat();
    replaced with
    m_imageData->AllocateScalars(VTK_FLOAT, 3);
  */
  m_imageData->AllocateScalars(VTK_FLOAT, 3);

  m_imageData->SetExtent(1, (int)m_visOpt.comp[0], 1, (int)m_visOpt.comp[1], 1, (int)m_visOpt.comp[2]);
  m_imageData->SetSpacing(m_visOpt.size[0], m_visOpt.size[1], m_visOpt.size[2]);
  m_imageData->GetPointData()->SetScalars(volumeField);

  // m_imageData->SetDimensions((int*)m_visOpt.comp);
  volumeField->GetRange(m_range);
  // std::clog<<m_range[0]<<" "<<m_range[1]<<std::endl;
  volumeField->Delete();

  double max = m_range[1];
  double min = m_range[0];

  /* VTK9 migration
  m_math->SetInput(m_imageData);
  replaced with
  m_math->SetInputData(m_imageData);
*/
  m_math->SetInputData(m_imageData);

  m_math->SetOperationToAddConstant();
  m_math->SetConstantC(-min);
  float defaultRange = 255.0;

  //   if(m_visOpt.isColorRangeFrom) min=m_visOpt.colorRangeFrom;
  //   if(m_visOpt.isColorRangeTo) max=m_visOpt.colorRangeTo;

  double norm = 255.0 / (max - min);

  m_math2->SetOperationToMultiplyByK();
  m_math2->SetConstantK(norm);
  m_math2->SetInputConnection(m_math->GetOutputPort());

  m_charData->SetOutputScalarTypeToUnsignedChar();
  m_charData->SetInputConnection(m_math2->GetOutputPort());

  m_visOpt.colorScalar = m_visOpt.vRenderingField;
  m_charData->Update();
  m_charData->GetOutput()->GetScalarRange(m_range);

  /*  m_opacityTransferFunction->AddPoint(20, 0.0);
    m_opacityTransferFunction->AddPoint(255 ,0.2);*/
  // cout << "RED rows:" << m_visOpt.extPalR.size() << "\n";

  m_opacityTransferFunction->RemoveAllPoints();
  if (m_visOpt.extPalR.size() > 0)
  {
    for (int i = 0; i < m_visOpt.extPalR.size(); i++)
    {

      m_opacityTransferFunction->AddPoint(m_visOpt.extPalId[i], m_visOpt.extPalA[i]);
    }
  }
  else
  {
    m_opacityTransferFunction->AddPoint(0, 0.0);
    m_opacityTransferFunction->AddPoint(255, 0.2);
  }
  setLookupTable();

  // The property describes how the data will look

  m_volumeProperty->SetColor(m_colorTransferFunction);
  m_volumeProperty->SetScalarOpacity(m_opacityTransferFunction);
  if (m_visOpt.vShadow)
    m_volumeProperty->ShadeOn();
  m_volumeProperty->SetInterpolationTypeToLinear();

  /*VTK 9 migration
  m_rayCastMapper->SetVolumeRayCastFunction(m_rayCastCompositFunction);
  */
  m_rayCastMapper->SetInputConnection(m_charData->GetOutputPort());

  m_volume->SetMapper(m_rayCastMapper);
  m_volume->SetProperty(m_volumeProperty);

  setBoundingBox(m_imageData);
  m_pRenderer->AddVolume(m_volume);
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

  /*
    if(m_visOpt.stereo)
    {
        m_pRenderWindow->StereoRenderOn();
       if(m_visOpt.stereoMode=="RedBlue")
    m_pRenderWindow->SetStereoTypeToRedBlue();
       else if(m_visOpt.stereoMode=="Anaglyph")
       {
    m_pRenderWindow->SetStereoTypeToAnaglyph();
    m_pRenderWindow->SetAnaglyphColorSaturation(m_visOpt.anaglyphsat);
    m_visOpt.anaglyphmask=trim(m_visOpt.anaglyphmask);
    std::stringstream anatmp(m_visOpt.anaglyphmask);
    int rightColorMask=4, leftColorMask=3;
    anatmp>>rightColorMask;
    anatmp>>leftColorMask;
    m_pRenderWindow->SetAnaglyphColorMask(rightColorMask,leftColorMask);
       }
       else
       {
         m_pRenderWindow->SetStereoTypeToCrystalEyes();

         if(m_visOpt.stereoImg==0)
          {
      m_pRenderWindow->SetStereoTypeToRight();
          }else
          {
      m_pRenderWindow->SetStereoTypeToLeft();
          }
       }
        m_pRenderWindow->StereoUpdate();
    }
    */

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
    setAxes(m_imageData, bounds);
  delete[] bounds;
  // open view
  //-----------------------------------
  inter->Start();
  inter->ExitEvent();
  //-----------------------------------

  if (inter != 0)
    inter->Delete();
  return 0;
}
//------------------------------------------
bool VolumePipe::setLookupTable()
//------------------------------------------
{

  // Create transfer mapping scalar value to opacity

  m_lut->SetTableRange(m_range);
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

  if (m_visOpt.uselogscale == "yes")
    m_lut->SetScaleToLog10();
  else
    m_lut->SetScaleToLinear();

  m_lut->Build();

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
    // std::clog<<color[0]<<" "<<color[1]<<" "<<color[2]<<" "<<std::endl;
    m_colorTransferFunction->AddRGBPoint(i, color[0], color[1], color[2]);
  }

  m_lut->Build();
  m_visOpt.colorScalar = m_visOpt.vRenderingField;
  if (m_visOpt.showLut)
    colorBar();

  return true;
}
