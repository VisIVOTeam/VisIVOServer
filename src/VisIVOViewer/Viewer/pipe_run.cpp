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

#include <sstream>
#include <cstdlib>
#include <cstring>

#include "pipe.h"
#include "visivoutils.h"

#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkLookupTable.h"
#include "vtkPolyData.h"

#include "vtkOutlineCornerFilter.h"
#include "vtkProperty.h"
#include "vtkScalarBarActor.h"

#include "vtkCubeAxesActor2D.h"
#include "vtkDataSet.h"

const double Pipe::INVALID_CAM = -123456789.31;

// //---------------------------------------------------------------------
// Pipe::Pipe ( VisIVOServerOptions options)
// //---------------------------------------------------------------------
// {
//
//   m_visOpt=options;
//   constructVTK();
//
// }
// //---------------------------------
// Pipe::~Pipe()
// //---------------------------------
// {
//   destroyVTK();
// }
//---------------------------------
void Pipe::constructVTK()
//---------------------------------
{

	m_pRenderer = vtkRenderer::New();
	m_lut = vtkLookupTable::New();
	m_pRenderWindow = vtkRenderWindow::New();
}
//------------------------
int Pipe::createPipe()
//------------------------
{
	return 0;
}
//------------------------
int Pipe::getCamera(SplotchCamera *splCamera)
//------------------------
{
	return 0;
}

//------------------------
bool Pipe::readData()
//------------------------
{
	return true;
}
//---------------------------------
void Pipe::destroyVTK()
//---------------------------------
{

	if (m_pRenderer != 0)
		m_pRenderer->Delete();
	if (m_pRenderWindow != 0)
		m_pRenderWindow->Delete();
	if (m_lut != 0)
		m_lut->Delete();
}
//------------------------------------
void Pipe::saveImageAsPng(int num)
//------------------------------------
{
	if (m_visOpt.numImage == 4 && m_visOpt.azimuth == 0 && m_visOpt.elevation == 0 && m_visOpt.zoom == 1 && !m_visOpt.cycle)
		return;

	else
	{

		int magnification = 1;
		vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
		std::string path;
		std::string numero;
		std::stringstream ss;
		ss << num;
		ss >> numero;
		std::string fileName;
		w2i->SetInput(m_pRenderWindow);
		w2i->SetMagnification(magnification);
		w2i->Update();
		// std::clog <<"out "<< m_visOpt.imageName.c_str()<<std::endl;
		if (m_visOpt.imageName != "VisIVOServerImage")
		{
			if (m_visOpt.imageName.find("/") != std::string::npos)
			{

				path = getDir(m_visOpt.imageName);
				if (path.at(0) != '/')
					path = "./" + path;

				m_visOpt.imageName = getName(m_visOpt.imageName);
			}

			else
				path = "./";
		}

		else

			path = "./";

		if (m_visOpt.noDefault == "yes")
		{
			size_t found = m_visOpt.imageName.find(".png");

			if (found == std::string::npos)
				fileName = path + m_visOpt.imageName + ".png";
			else
				fileName = path + m_visOpt.imageName;

			//			fileName=path+m_visOpt.imageName+".png";
		}
		else
		{
			size_t found = m_visOpt.imageName.find(".png");

			if (found == std::string::npos)
				fileName = path + m_visOpt.imageName + numero + ".png";
			else
				fileName = path + m_visOpt.imageName.substr(0, found) + numero + ".png";
		}
		// std::clog <<"fielname "<< fileName.c_str()<<std::endl;

		vtkPNGWriter *w = vtkPNGWriter::New();
		w->SetInput(w2i->GetOutput());
		w->SetFileName(fileName.c_str());

		w->Write();

		if (w2i != 0)
			w2i->Delete();

		if (w != 0)
			w->Delete();
	}

	this->destroyVTK();
	return;
}

//---------------------------------------------------------------------
void Pipe::setCamera(SplotchCamera *splCamera)
//---------------------------------------------------------------------
{

	m_camera = m_pRenderer->GetActiveCamera();

	//	m_camera->SetClippingRange(0.01,1.0e+13);

	if (m_visOpt.setCameraPos)
		if (m_visOpt.cameraPos[0] != INVALID_CAM && m_visOpt.cameraPos[1] != INVALID_CAM && m_visOpt.cameraPos[2] != INVALID_CAM)
			m_camera->SetPosition(m_visOpt.cameraPos[0], m_visOpt.cameraPos[1], m_visOpt.cameraPos[2]);
		else if (m_visOpt.cameraPosPrev[0] != INVALID_CAM && m_visOpt.cameraPosPrev[1] != INVALID_CAM &&
				 m_visOpt.cameraPosPrev[2] != INVALID_CAM)
			m_camera->SetPosition(m_visOpt.cameraPosPrev[0], m_visOpt.cameraPosPrev[1], m_visOpt.cameraPosPrev[2]);

	if (m_visOpt.setCameraFocalPoint)
		if (m_visOpt.cameraFocalPoint[0] != INVALID_CAM && m_visOpt.cameraFocalPoint[1] != INVALID_CAM && m_visOpt.cameraFocalPoint[2] != INVALID_CAM)
			m_camera->SetFocalPoint(m_visOpt.cameraFocalPoint[0], m_visOpt.cameraFocalPoint[1], m_visOpt.cameraFocalPoint[2]);
		else if (m_visOpt.cameraFocalPointPrev[0] != INVALID_CAM && m_visOpt.cameraFocalPointPrev[1] != INVALID_CAM &&
				 m_visOpt.cameraFocalPointPrev[2] != INVALID_CAM)
			m_camera->SetFocalPoint(m_visOpt.cameraFocalPointPrev[0], m_visOpt.cameraFocalPointPrev[1], m_visOpt.cameraFocalPointPrev[2]);

	if (m_visOpt.setCameraRoll)
		if (m_visOpt.cameraRoll != INVALID_CAM)
			m_camera->SetRoll(m_visOpt.cameraRoll);
		else if (m_visOpt.cameraRollPrev[0] != INVALID_CAM)
			m_camera->SetRoll(m_visOpt.cameraRollPrev[0]);

	if (m_visOpt.fovIsGiven)
		m_camera->SetViewAngle(m_visOpt.fov);

	if (m_visOpt.numImage == 0) // QUI con Splotch è sbagliato
	{
		m_camera->Azimuth(0);
		m_camera->Elevation(0);
		m_camera->Zoom(1);
	}
	else if (m_visOpt.numImage == 1)
	{
		m_camera->Azimuth(90);
		m_camera->Elevation(0);
		m_camera->Zoom(1);
	}
	else if (m_visOpt.numImage == 2)
	{
		m_camera->Azimuth(0);
		m_camera->Elevation(89.90);
		m_camera->Zoom(1);
	}
	else if (m_visOpt.numImage == 3)
	{
		m_camera->Azimuth(45);
		m_camera->Elevation(45);
		m_camera->Zoom(1);
	}
	else if (m_visOpt.numImage == 4)
	{
		if (m_visOpt.elevation == 90.0)
			m_visOpt.elevation = 89.90;
		if (m_visOpt.elevation == -90.0)
			m_visOpt.elevation = -89.90;
		if (m_visOpt.elevation > 90)
		{
			std::cerr << "Invalid elevation " << m_visOpt.elevation << std::endl;
			m_visOpt.elevation = 89.90;
		}
		if (m_visOpt.elevation < -90)
		{
			std::cerr << "Invalid elevation " << m_visOpt.elevation << std::endl;
			m_visOpt.elevation = -89.90;
		}

		m_camera->Azimuth(m_visOpt.azimuth);
		m_camera->Elevation(m_visOpt.elevation);
		m_camera->Zoom(m_visOpt.zoom);
	}
	// m_camera->SetPosition(32+1*10,32+5*10,32+70);
	//  m_camera->SetViewUp(1,0,0);
	// m_camera->SetViewAngle(160);
	// vtkIndent indent;
	// std::ofstream planeof("cam.txt");
	// m_camera->PrintSelf(planeof,indent);
	// planeof.close();

	if (splCamera != NULL)
	{
		vtkIndent indent;
		/*		std::ofstream cameraof("camera.txt");
				m_camera->PrintSelf(cameraof,indent);*/
		double a, b, c;
		m_camera->GetPosition(a, b, c);
		splCamera->position[0] = a;
		splCamera->position[1] = b;
		splCamera->position[2] = c;
		m_camera->GetFocalPoint(a, b, c);
		splCamera->lookat[0] = a;
		splCamera->lookat[1] = b;
		splCamera->lookat[2] = c;
		splCamera->roll = m_camera->GetRoll();
		/*		splCamera->sky[0]=a;
				splCamera->sky[1]=b;
				splCamera->sky[2]=c;*/
		splCamera->fov = m_camera->GetViewAngle();
		m_camera->GetClippingRange(a, b);
		splCamera->clip[0] = a;
		splCamera->clip[1] = b;

		/*		m_camera->SetPosition(80,80,280);
				m_camera->Roll(180);
		//		m_camera->SetFocalPoint(-10,-10,-10);
				m_camera->PrintSelf(cameraof,indent);*/
		//		cameraof.close();
	}

	return;
}
//---------------------------------------------------------------------
void Pipe::setBoundingBox(vtkDataObject *data)
//---------------------------------------------------------------------
{
	vtkOutlineCornerFilter *corner = vtkOutlineCornerFilter::New();
	corner->SetInput(data);
	corner->ReleaseDataFlagOn();

	vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
	outlineMapper->SetInput(corner->GetOutput());

	vtkProperty *outlineProperty = vtkProperty::New();
	outlineProperty->SetColor(1, 1, 1); // Set the color to white
	outlineProperty->SetAmbient(1);
	if (m_visOpt.showBox)
		outlineProperty->SetOpacity(0.999);
	else
		outlineProperty->SetOpacity(0.0);
	outlineProperty->SetRepresentationToWireframe();
	outlineProperty->SetInterpolationToFlat();

	vtkActor *outlineActor = vtkActor::New();
	outlineActor->SetMapper(outlineMapper);
	outlineActor->SetProperty(outlineProperty);
	outlineActor->SetPickable(false);
	outlineActor->SetVisibility(true);

	m_pRenderer->AddActor(outlineActor);

	if (outlineActor != 0)
		outlineActor->Delete();
	if (outlineMapper != 0)
		outlineMapper->Delete();
	if (outlineProperty != 0)
		outlineProperty->Delete();
	if (corner != 0)
		corner->Delete();
}

//---------------------------------------------------------------------
void Pipe::colorBar()
//---------------------------------------------------------------------
{
	vtkScalarBarActor *scalarBar = vtkScalarBarActor::New();
	scalarBar->SetTitle(m_visOpt.colorScalar.c_str());
	scalarBar->SetLabelFormat("%.3g");
	scalarBar->SetOrientationToHorizontal();
	scalarBar->SetPosition(0.1, 0);
	scalarBar->SetPosition2(0.8, 0.1);
	scalarBar->SetLookupTable(m_lut);
	scalarBar->SetVisibility(1);

	m_pRenderer->AddActor(scalarBar);

	if (scalarBar != 0)
		scalarBar->Delete();
}
//---------------------------------------------------------------------
void Pipe::setAxes(vtkDataSet *data, double *bounds)
//---------------------------------------------------------------------
{
	vtkCubeAxesActor2D *axesActor = vtkCubeAxesActor2D::New();

	axesActor->SetInput(data);
	axesActor->UseRangesOn();

	axesActor->SetBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
	axesActor->SetRanges(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);

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
