#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "pointssmoothpipe.h"

#include "visivoutils.h"
#include "luteditor.h"

#include "extendedglyph3d.h"

#include <sstream>
#include <algorithm>
#include "vtkGenericRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkProperty.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"
//#include "vtkVolumeRayCastMapper.h"
//#include "vtkVolumeRayCastCompositeFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkSmartPointer.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkContourFilter.h"
#include "vtkGaussianSplatter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkAxes.h"
#include "vtkTubeFilter.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkLODActor.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLImageDataReader.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkRenderer.h"
//---------------------------------------------------------------------
PointsSmoothPipe::PointsSmoothPipe(VisIVOServerOptions options)
//---------------------------------------------------------------------
{
  m_visOpt = options;
  constructVTK();
  m_pActor = vtkActor::New();
  m_pPolyData = vtkPolyData::New();
  m_pMapper = vtkPolyDataMapper::New();
}
//---------------------------------
PointsSmoothPipe::~PointsSmoothPipe()
//---------------------------------
{
  destroyVTK();
  if (m_pMapper != 0)
    m_pMapper->Delete();
  if (m_pActor != 0)
    m_pActor->Delete();
  if (m_pPolyData != 0)
    m_pPolyData->Delete();
}
//---------------------------------
void PointsSmoothPipe::destroyAll()
//---------------------------------
{
  if (m_pMapper != 0)
    m_pMapper->Delete();
  if (m_pActor != 0)
    m_pActor->Delete();
  if (m_pPolyData != 0)
    m_pPolyData->Delete();
}
//-----------------------------------------------------------------------------------
int PointsSmoothPipe::createPipe()
//------------------------------------------------------------------------------------
{

  std::ifstream inFile;

  vtkFloatArray *xAxis = vtkFloatArray::New();
  vtkFloatArray *yAxis = vtkFloatArray::New();
  vtkFloatArray *zAxis = vtkFloatArray::New();

  xAxis->SetNumberOfTuples(m_visOpt.nRows);
  yAxis->SetNumberOfTuples(m_visOpt.nRows);
  zAxis->SetNumberOfTuples(m_visOpt.nRows);

  xAxis->SetName(m_visOpt.xField.c_str());
  yAxis->SetName(m_visOpt.yField.c_str());
  zAxis->SetName(m_visOpt.zField.c_str());

  int xIndex, yIndex, zIndex;

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
    }
    for (int i = 0; i < m_visOpt.nRows; i++)
    {
      xAxis->SetValue(i, m_visOpt.tableData[xIndex][i]); //! this is the row that fill xAxis array
      yAxis->SetValue(i, m_visOpt.tableData[yIndex][i]);
      zAxis->SetValue(i, m_visOpt.tableData[zIndex][i]);
    }
  }
  else
  {
    inFile.open(m_visOpt.path.c_str(), ios::binary); //! open binary file. m_visOpt is the structure (parameter of the constructor) that contain alla data to be visualized
    // std::clog<<m_visOpt.path.c_str()<<std::endl;
    if (!inFile.is_open())
      return -1;

    float tmpAxis[1];

    for (int i = 0; i < m_visOpt.nRows; i++)
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

  m_pPolyData->SetPoints(m_points);

  m_points->Delete();

  struct stat stFileInfo;
  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  vtkSmartPointer<vtkPiecewiseFunction> opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
  vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
  vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
  vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
  std::string OutputFilenameImage = getDir(m_visOpt.imageName) + getName(m_visOpt.path) + ".vti";

  m_pMapper = vtkPolyDataMapper::New();
  /* vtk9 migration
  m_pMapper->SetInput(m_pPolyData);
  */
  m_pMapper->SetInputData(m_pPolyData);
  m_pActor->SetMapper(m_pMapper);

  m_pActor->GetProperty()->SetRepresentationToPoints();
  m_pActor->SetVisibility(1);
  m_pActor->GetProperty()->SetPointSize(1);
  m_pActor->GetProperty()->SetOpacity(0.3);
  m_pActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  float opacity = 0.2;

  m_pActor->GetProperty()->SetOpacity(opacity);
  double bounds[6];
  m_pActor->GetMapper()->GetBounds(bounds);
  if (stat(OutputFilenameImage.c_str(), &stFileInfo))
  { // The vtkImage file cannot be loaded from file so we create it and write to file
    double bounds[6];
    int ngridX = 256, ngridY = 256, ngridZ = 256;
    // construct a Splatting pipeline
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
    float splatterRadius = 0.015;
    /* vtk9 migration
    splatter->SetInput(m_pPolyData);
    */
    splatter->SetInputData(m_pPolyData);
    splatter->SetSampleDimensions(ngridX, ngridY, ngridZ);
    splatter->SetRadius(splatterRadius);
    splatter->ScalarWarpingOff();
    splatter->SetExponentFactor(-20.0);
    vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();

    /* VTK9 migration
    writer->SetInput(splatter->GetOutput());
    */
    writer->SetInputData(splatter->GetOutput());
    writer->SetFileName(OutputFilenameImage.c_str());
    writer->Write();
    /*
    VTK9 migration
    volumeMapper->SetInput(splatter->GetOutput());
    */
    volumeMapper->SetInputData(splatter->GetOutput());
    vtkSmartPointer<vtkXMLImageDataWriter> writer2 = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    /*VTK9 migration
    writer2->SetInput(splatter->GetOutput());
    */
    writer2->SetInputData(splatter->GetOutput());
    writer2->SetFileName(OutputFilenameImage.c_str());
    writer2->Write();
  }
  else
  { // The vtkImage file can be loaded from file
    vtkSmartPointer<vtkXMLImageDataReader> imageReader = vtkSmartPointer<vtkXMLImageDataReader>::New();
    imageReader->SetFileName(OutputFilenameImage.c_str());
    imageReader->Update();
    /*VTK9 migration
    volumeMapper->SetInput(imageReader->GetOutput());
    */
    volumeMapper->SetInputData(imageReader->GetOutput());
    //        surface->SetInput(imageReader->GetOutput());
  }
  volumeMapper->SetImageSampleDistance(0.5);
  volume->SetMapper(volumeMapper);
  // A smart Color map ...8  points...
  colorTransferFunction->AddRGBPoint(0, 0, 0, 0);
  colorTransferFunction->AddRGBPoint(0.396415, 1, 0, 0);
  colorTransferFunction->AddRGBPoint(0.531872, 1, 0, 0);
  colorTransferFunction->AddRGBPoint(0.697211, 0.901961, 0, 0);
  colorTransferFunction->AddRGBPoint(0.76494, 0.901961, 0.831891, 0);
  colorTransferFunction->AddRGBPoint(0.824701, 0.901961, 0.831891, 0);
  colorTransferFunction->AddRGBPoint(0.888446, 0.901961, 0.901961, 0);
  colorTransferFunction->AddRGBPoint(1, 1, 1, 1);
  double step = 1.0 / 256;
  double opValue = 0;
  for (double i = 0; i <= 1; i += step)
  {
    opValue = (tanh(m_visOpt.opacityTF[0] * i - m_visOpt.opacityTF[1]) + 1) / m_visOpt.opacityTF[2];
    opacityTransferFunction->AddPoint(i, opValue);
  }
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volume->SetProperty(volumeProperty);

  m_pRenderer->AddVolume(volume);
  m_pRenderer->AddActor(m_pActor);
  // END:  some  Volume rendering staff ... just to improve visualization...

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
  m_pRenderWindow->AddRenderer(m_pRenderer);

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
  /**/
  vtkGenericRenderWindowInteractor *inter = vtkGenericRenderWindowInteractor::New(); // generic/**/
  m_pRenderWindow->SetInteractor(inter);
  m_pRenderWindow->Render();
  //--------------------------------------------------------------------

  setCamera();
  double *pbounds;
  pbounds = new double[6];
  pbounds[0] = m_xRange[0];
  pbounds[1] = m_xRange[1];
  pbounds[2] = m_yRange[0];
  pbounds[3] = m_yRange[1];
  pbounds[4] = m_zRange[0];
  pbounds[5] = m_zRange[1];

  if (m_visOpt.showAxes)
    setAxes(m_pPolyData, pbounds);
  delete[] pbounds;
  // open view
  //-----------------------------------
  inter->Start();
  inter->ExitEvent();
  //-----------------------------------

  if (inter != 0)
    inter->Delete();
  return 0;
}

//-------------------------------------------------------------------------
bool PointsSmoothPipe::SetXYZ(vtkFloatArray *xField, vtkFloatArray *yField, vtkFloatArray *zField)
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
