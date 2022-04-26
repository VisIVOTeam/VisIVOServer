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

#ifndef SPLOTCHPIPECAMERA_H
#define SPLOTCHPIPECAMERA_H
#include "vtkPolyDataMapper.h"

#include "pipe.h"
#include "optionssetter.h"

class vtkActor;
class vtkSphereSource;
class vtkConeSource;
class vtkCylinderSource;
class vtkCubeSource;
class vtkGlyph3D;
class vtkPoints;

class ExtendedGlyph3D;

// Define a new frame type: this is going to be our main frame
class SplotchPipeCamera : public Pipe
{
public:
  SplotchPipeCamera(VisIVOServerOptions options);
  ~SplotchPipeCamera();

protected:
  int createPipe();
  int getCamera(SplotchCamera *splCamera);
  int vtkCameraData(SplotchCamera *splCamera);

private:
  void setGlyphs(){};
  void setLookupTable();
  void setAxes();
  void setMirror();
  void setRadius();
  void setResolution();
  bool SetXYZ(vtkFloatArray *xField, vtkFloatArray *yField, vtkFloatArray *zField);
  void setScaling();

  vtkPolyDataMapper *m_pConeMapper;
  vtkActor *m_pConeActor;
  vtkPolyData *m_polyData;
  vtkGlyph3D *m_glyph;
  vtkSphereSource *m_sphere;
  vtkConeSource *m_cone;
  vtkCylinderSource *m_cylinder;
  vtkCubeSource *m_cube;
  vtkPoints *m_points;

  ExtendedGlyph3D *m_glyphFilter;

  double m_xRange[2], m_yRange[2], m_zRange[2];
  int m_mirrorEle;
  float m_mirror[3][1000];
};

#endif
