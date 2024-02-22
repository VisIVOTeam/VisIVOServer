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

#include "pointspipe.h"

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
#include"vtkGlyph3D.h"
#include "vtkScalarBarActor.h"
#include "vtkOutlineCornerFilter.h"
#include "vtkProperty.h"

#include "vtkGenericRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkAxesActor.h"

#include "vtkAppendPolyData.h"
#include "vtkPolyDataWriter.h"

//---------------------------------------------------------------------
PointsPipe::PointsPipe ( VisIVOServerOptions options)
//---------------------------------------------------------------------
{
    m_visOpt=options;
    constructVTK();
    m_glyphFilter   = ExtendedGlyph3D::New();
    m_glyph         = vtkGlyph3D::New();
    m_pConeActor    = vtkActor::New();
    m_polyData      = vtkPolyData::New();
    m_pConeMapper   = vtkPolyDataMapper::New();
}
//---------------------------------
PointsPipe::~PointsPipe()
//---------------------------------
{
    destroyVTK();
    if ( m_glyph!=0)
        m_glyph->Delete() ;
    if ( m_glyphFilter!=0)
        m_glyphFilter->Delete() ;
    if ( m_pConeMapper != 0 )
        m_pConeMapper->Delete();
    if ( m_pConeActor != 0 )
        m_pConeActor->Delete();
    if ( m_polyData!=0)
        m_polyData->Delete() ;
}
//---------------------------------
void PointsPipe::destroyAll()
//---------------------------------
{
    destroyVTK();
    if ( m_glyph!=0)
        m_glyph->Delete() ;
    if ( m_glyphFilter!=0)
        m_glyphFilter->Delete() ;
    if ( m_pConeMapper != 0 )
        m_pConeMapper->Delete();
    if ( m_pConeActor != 0 )
        m_pConeActor->Delete();
    if ( m_polyData!=0)
        m_polyData->Delete() ;
}

//-----------------------------------------------------------------------------------
int PointsPipe::createPipe ()
//------------------------------------------------------------------------------------
{
    unsigned long long int i = 0;
    unsigned long long int j = 0;
    std::ifstream inFile;
    
    vtkFloatArray *radiusArrays =vtkFloatArray::New();
   
    
    
    unsigned long long int xIndex, yIndex,zIndex;
    
    
    inFile.open(m_visOpt.path.c_str(), ios::binary); //!open binary file. m_visOpt is the structure (parameter of the constructor) that contain alla data to be visualized
    // std::clog<<m_visOpt.path.c_str()<<std::endl;
    if(!inFile.is_open())
        return -1;
    
    unsigned long long int chunk = 10000;
    if(m_visOpt.nRows < chunk)
        chunk = m_visOpt.nRows;
    float tmpAxis[chunk];
    
    std::vector<vtkSmartPointer<vtkPolyData>> polyDataList ;
    for(i=0; i <  m_visOpt.nRows; i= i + chunk)
    {
        m_polyData      = vtkPolyData::New();
        vtkSmartPointer<vtkFloatArray> xAxis = vtkFloatArray::New();
        vtkSmartPointer<vtkFloatArray> yAxis = vtkFloatArray::New();
        vtkSmartPointer<vtkFloatArray> zAxis = vtkFloatArray::New();
        
        xAxis->SetNumberOfTuples(chunk);
        yAxis->SetNumberOfTuples(chunk);
        zAxis->SetNumberOfTuples(chunk);
        
        xAxis->SetName(m_visOpt.xField.c_str());
        yAxis->SetName(m_visOpt.yField.c_str());
        zAxis->SetName(m_visOpt.zField.c_str());
        
        // Leggere i dati dal file per il chunk corrente
          if (i + chunk > m_visOpt.nRows) {
            chunk = m_visOpt.nRows - i;
          }
        inFile.seekg((m_visOpt.x*m_visOpt.nRows +i)* sizeof(float) );
        inFile.read((char *)( tmpAxis),chunk*sizeof(float));
        
        for(int c = 0; c <= chunk; c++){
            xAxis->  SetValue(c,tmpAxis[c]); //! this is the row that fill xAxis array
        }
        
        inFile.seekg((m_visOpt.y*m_visOpt.nRows+ i)* sizeof(float));
        inFile.read((char *)( tmpAxis),  chunk*sizeof(float));
        
        
        for(int c = 0; c <= chunk; c++){
            yAxis->  SetValue(c,tmpAxis[c]);
        }
        inFile.seekg((m_visOpt.z*m_visOpt.nRows+ i)* sizeof(float));
        inFile.read((char *)(tmpAxis), chunk*sizeof(float));
        
        for(int c = 0; c <= chunk; c++){
            zAxis->  SetValue(c,tmpAxis[c]);
        }
        
        
        
        // SetXYZ(xAxis,yAxis,zAxis);
        m_points=vtkPoints::New();
        m_points->SetNumberOfPoints(chunk);
        vtkCellArray *newVerts = vtkCellArray::New();
        newVerts->EstimateSize (chunk,1 );

        
        float outPoint[3];
        for(j = 0; j < chunk; j++)
        {
            outPoint[0] = xAxis->GetValue(j) ;
            outPoint[1] = yAxis->GetValue(j) ;
            outPoint[2] = zAxis->GetValue(j) ;
            
            m_points->SetPoint(j,outPoint);
            
            newVerts->InsertNextCell(1);
            newVerts->InsertCellPoint ( j );
        }
                
        m_polyData->SetPoints(m_points);
        m_polyData->SetVerts ( newVerts );
       
        polyDataList.push_back(m_polyData);
        m_points->Delete();
       // break;
    }
    
    inFile.close();
    
    // connect m_pRendererderer and m_pRendererder window and configure m_pRendererder window
    m_pRenderWindow->AddRenderer ( m_pRenderer );
    
   
    vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();

    for (auto polyData : polyDataList) {
      appendFilter->AddInputData(polyData);
    }

    appendFilter->Update();

    
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
       mapper->SetInputData(appendFilter->GetOutput()); // Otteniamo l'output dell'append
     
    // Visualizza il PolyData combinato
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    // Aggiungi l'attore alla scena
    vtkRenderer* renderer = vtkRenderer::New();
    renderer->AddActor(actor);

    // Visualizza la scena
    vtkRenderWindow* renderWindow = vtkRenderWindow::New();
    renderWindow->AddRenderer(renderer);

    vtkRenderWindowInteractor* renderWindowInteractor = vtkRenderWindowInteractor::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);

    renderWindow->Render();
    renderWindowInteractor->Start();
    
}


//---------------------------------------------------------------------
void PointsPipe::setGlyphs ( )
//---------------------------------------------------------------------
{
    int max=1000;
    
    if ( m_visOpt.nRows<max )
    {
        /* VTK9 migration
         m_glyph->SetInput (m_polyData );
         replaced
         m_glyph->SetInputData (m_polyData );
         
         */
        m_glyph->SetInputData (m_polyData );
        
        
        if (m_visOpt.scale=="yes")
            m_glyph->SetScaleFactor ( 0.04 );
        
        else
            m_glyph->SetScaleFactor ( 2.5 );
        
        m_pConeMapper->SetInputConnection( m_glyph->GetOutputPort() );
        
        
        if (m_visOpt.nGlyphs==1)
        {
            m_sphere   = vtkSphereSource::New();
            setResolution ( );
            setRadius ();
            /* VTK9 migration
             m_glyph->SetSource ( m_sphere->GetOutput() );
             replaced
             m_glyph->SetSourceData ( m_sphere->GetOutput() );
             */
            m_glyph->SetSourceData ( m_sphere->GetOutput() );
            m_sphere->Delete();
        }
        
        else if (m_visOpt.nGlyphs==2)
        {
            m_cone   = vtkConeSource::New();
            setResolution ( );
            setRadius ();
            /* VTK9 migration
             m_glyph->SetSource ( m_cone->GetOutput() );
             replaced
             m_glyph->SetSourceData ( m_cone->GetOutput() );
             */
            m_glyph->SetSourceData ( m_cone->GetOutput() );
            m_cone->Delete();
        }
        
        else if (m_visOpt.nGlyphs==3)
        {
            m_cylinder   = vtkCylinderSource::New();
            setResolution ( );
            setRadius ();
            /* VTK9 migration
             m_glyph->SetSource ( m_cylinder->GetOutput() );
             replaced
             m_glyph->SetSourceData ( m_cylinder->GetOutput() );
             */
            m_glyph->SetSourceData ( m_cylinder->GetOutput() );
            m_cylinder->Delete();
        }
        
        else if (m_visOpt.nGlyphs==4)
        {
            m_cube   = vtkCubeSource::New();
            setRadius ();
            /* VTK9 migration
             m_glyph->SetSource ( m_cube->GetOutput() );
             replaced
             m_glyph->SetSourceData ( m_cube->GetOutput() );
             */
            m_glyph->SetSourceData ( m_cube->GetOutput() );
            m_cube->Delete();
        }
        
        
    }
    return ;
}


//---------------------------------------------------------------------
void PointsPipe::setLookupTable ()
//---------------------------------------------------------------------
{
    
    double b[2];
    m_polyData->GetPointData()->SetActiveScalars(m_visOpt.colorScalar.c_str());
    
    m_polyData->GetPointData()->GetScalars(m_visOpt.colorScalar.c_str())->GetRange(b);
    
    
    
    m_lut->SetTableRange(m_polyData->GetPointData()->GetScalars()->GetRange());
    m_lut->GetTableRange(b);
    if(m_visOpt.isColorRangeFrom) b[0]=m_visOpt.colorRangeFrom;
    if(m_visOpt.isColorRangeTo) b[1]=m_visOpt.colorRangeTo;
    if(b[1]<=b[0]) b[1]=b[0]+0.0001;
    m_lut->SetTableRange(b[0],b[1]);
    
    if(m_visOpt.uselogscale=="yes")
        m_lut->SetScaleToLog10();
    else
        m_lut->SetScaleToLinear();
    
    m_lut->Build();
    
    SelectLookTable(&m_visOpt, m_lut);
    
    m_pConeMapper->SetLookupTable(m_lut);
    m_pConeMapper->SetScalarVisibility(1);
    m_pConeMapper->UseLookupTableScalarRangeOn();
    
    m_pConeActor->SetMapper(m_pConeMapper);
    
    if(m_visOpt.showLut)  colorBar();
    
}


//---------------------------------------------------------------------
void PointsPipe::setRadius ()
//---------------------------------------------------------------------
{
    if (m_visOpt.nGlyphs==1)
        m_sphere->SetRadius ( m_visOpt.radius);
    
    
    else if (m_visOpt.nGlyphs==2)
    {
        
        m_cone->SetRadius ( m_visOpt.radius );
        m_cone->SetHeight (m_visOpt.height );
    }
    
    else if (m_visOpt.nGlyphs==3)
    {
        
        m_cylinder->SetRadius (m_visOpt.radius );
        m_cylinder->SetHeight ( m_visOpt.height );
    }
    else if (m_visOpt.nGlyphs==4)
    {
        m_cube->SetXLength ( m_visOpt.radius );
        m_cube->SetYLength ( m_visOpt.height );
        m_cube->SetZLength ( 1 );
        
    }
}

//---------------------------------------------------------------------
void PointsPipe::setResolution ()
//---------------------------------------------------------------------
{
    if (m_visOpt.nGlyphs==1)
    {
        m_sphere->SetPhiResolution ( 10 );
        m_sphere->SetThetaResolution ( 20 );
    }
    
    else if (m_visOpt.nGlyphs==2)
        m_cone->SetResolution ( 10 );
    
    else if (m_visOpt.nGlyphs==3)
        m_cylinder->SetResolution ( 10);
    
    
}


//-------------------------------------------------------------------------
bool PointsPipe::SetXYZ(vtkFloatArray *xField, vtkFloatArray *yField, vtkFloatArray *zField  )
//-------------------------------------------------------------------------
{
    double scalingFactors[3];
    scalingFactors[0]=scalingFactors[1]=scalingFactors[2]=0;
    
    m_points=vtkPoints::New();
    m_points->SetNumberOfPoints(m_visOpt.nRows);
    
    
    if(xField->GetNumberOfComponents() != yField->GetNumberOfComponents())
    {
        if(zField && (xField->GetNumberOfComponents() != zField->GetNumberOfComponents() \
                      || yField->GetNumberOfComponents() != zField->GetNumberOfComponents()))
        {
            false;
        }
        false; // component mismatch, do nothing
    }
    
    
    if(m_visOpt.scale=="yes")
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
    
    unsigned long long int i = 0;
    for(i = 0; i < 3; i++)
        scalingFactorsInv[i] = ((scalingFactors && scalingFactors[i] != 0) ? 1/scalingFactors[i] : 0);
    
    // Set the points data
    if(m_visOpt.scale=="yes")
    {
        float inPoint[3];
        float outPoint[3];
        for(i = 0; i < m_visOpt.nRows; i++)
        {
            inPoint[0] = outPoint[0] = xField->GetValue(i) * scalingFactorsInv[0];
            inPoint[1] = outPoint[1] = yField->GetValue(i) * scalingFactorsInv[1];
            inPoint[2] = outPoint[2] = zField->GetValue(i) * scalingFactorsInv[2];
            
            m_points->SetPoint(i,outPoint);
        }
    }
    else
    {
        float outPoint[3];
        for(i = 0; i < m_visOpt.nRows; i++)
        {
            outPoint[0] = xField->GetValue(i) ;
            outPoint[1] = yField->GetValue(i) ;
            outPoint[2] = zField->GetValue(i) ;
            
            m_points->SetPoint(i,outPoint);
        }
    }
    return true;
}

//---------------------------------------------------------------------
void PointsPipe::setScaling ()
//---------------------------------------------------------------------
{
    m_glyphFilter->SetUseSecondScalar(true);
    m_glyphFilter->SetUseThirdScalar(true);
    
    m_glyphFilter->SetScaling(1);
    
    if( m_visOpt.heightscalar!="none" && m_visOpt.scaleGlyphs!="none" && m_visOpt.nGlyphs!=0 && m_visOpt.nGlyphs!=1)
        m_glyphFilter->SetInputScalarsSelectionY(m_visOpt.heightscalar.c_str());
    
    if( m_visOpt.radiusscalar!="none" && m_visOpt.scaleGlyphs!="none" && m_visOpt.nGlyphs!=0)
        m_glyphFilter->SetInputScalarsSelectionXZ(m_visOpt.heightscalar.c_str());
    
    
    if( m_visOpt.nGlyphs!=0)
        m_glyphFilter->SetScaleModeToScaleByScalar();
    else
        m_glyphFilter->ScalarVisibilityOff();
    
    
}

