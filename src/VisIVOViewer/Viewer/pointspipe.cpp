/***************************************************************************
 *   Copyright (C) 2008 by Gabriella Caniglia *
 *  gabriella.caniglia@oact.inaf.it *
 *                                                                     	*
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or 	*
 *   (at your option) any later version.                               	*
 *                                                                     	*
 *   This program is distributed in the hope that it will be useful,   	*
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of    	*
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     	*
 *   GNU General Public License for more details.                      	*
 *                                                                     	*
 *   You should have received a copy of the GNU General Public License 	*
 *   along with this program; if not, write to the                     	*
 *   Free Software Foundation, Inc.,                                   	*
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.         	*
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

#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"

//---------------------------------------------------------------------
PointsPipe::PointsPipe ( VisIVOServerOptions options)
//---------------------------------------------------------------------
{
    m_visOpt=options;
    constructVTK();
    m_glyphFilter   = ExtendedGlyph3D::New();
    m_glyph     	= vtkGlyph3D::New();
    m_pConeActor	= vtkActor::New();
    m_polyData  	= vtkPolyData::New();
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
    vtkSmartPointer<vtkFloatArray> xAxis = vtkSmartPointer<vtkFloatArray>::New();
    vtkSmartPointer<vtkFloatArray> yAxis = vtkSmartPointer<vtkFloatArray>::New();
    vtkSmartPointer<vtkFloatArray> zAxis = vtkSmartPointer<vtkFloatArray>::New();
    
    
    
    xAxis->SetName(m_visOpt.xField.c_str());
    yAxis->SetName(m_visOpt.yField.c_str());
    zAxis->SetName(m_visOpt.zField.c_str());
    
    vtkFloatArray *lutArrays =vtkFloatArray::New();
    lutArrays->SetNumberOfTuples(m_visOpt.nRows);
    lutArrays->SetName(m_visOpt.colorScalar.c_str());
    
    unsigned long long int xIndex, yIndex,zIndex;
    
    if(m_visOpt.dataRead && m_visOpt.goodAllocation)
    {
        std::map<std::string, int>::iterator p;
        for(p=m_visOpt.columns.begin();p!=m_visOpt.columns.end();p++)
        {
            if(p->first==m_visOpt.xField) xIndex=p->second;
            if(p->first==m_visOpt.yField) yIndex=p->second;
            if(p->first==m_visOpt.zField) zIndex=p->second;
        }
        for(i=0; i<m_visOpt.nRows;i++)
        {
            xAxis->  SetValue(i,m_visOpt.tableData[xIndex][i]); //! this is the row that fill xAxis array
            yAxis->  SetValue(i,m_visOpt.tableData[yIndex][i]);
            zAxis->  SetValue(i,m_visOpt.tableData[zIndex][i]);
        }
    }
    else if (!m_visOpt.dataRead)
    {
        double range [2];
        
        if(m_visOpt.colorScalar!="none")
        {
            inFile.open(m_visOpt.path.c_str(), ios::binary); //!open binary file. m_visOpt is the structure (parameter of the constructor) that contain alla data to be visualized
            // std::clog<<m_visOpt.path.c_str()<<std::endl;
            if(!inFile.is_open())
                return -1;
            
            
            int chunk = 10000;
            if(m_visOpt.nRows < chunk) chunk = m_visOpt.nRows;
            float tmpScalar[chunk];
            for (i=0;i<m_visOpt.nRows;i=i+chunk)
            {
                if(i + chunk> m_visOpt.nRows) chunk = m_visOpt.nRows - i;
                inFile.seekg((m_visOpt.nColorScalar * m_visOpt.nRows+i )* sizeof(float));
                inFile.read((char *)(tmpScalar),  chunk*sizeof(float));
                
                if(m_visOpt.needSwap){
                    for(int c = 0; c < chunk; c++){
                        tmpScalar[c]=floatSwap((char *)(&tmpScalar[c]));
                    }
                }
                
                for(int c = 0; c < chunk; c++){
                    lutArrays->  SetValue(i+c,tmpScalar[c]);
                }
            }
            
            lutArrays->GetRange(range);
            if(range[0]<=0)
                m_visOpt.uselogscale="none";
            std::cout<<range[0]<<" "<<range[1];
            
            inFile.close();
            
        }
        
        
        inFile.open(m_visOpt.path.c_str(), ios::binary); //!open binary file. m_visOpt is the structure (parameter of the constructor) that contain alla data to be visualized
        // std::clog<<m_visOpt.path.c_str()<<std::endl;
        if(!inFile.is_open())
            return -1;
        
        unsigned long long int chunk = 2000000;
        if(m_visOpt.nRows < chunk)
            chunk = m_visOpt.nRows;
        float tmpAxis[chunk];
        
        int z=0;
        vtkSmartPointer<vtkPoints> m_points = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> newVerts = vtkSmartPointer<vtkCellArray>::New();
        
        
        for(i=0; i <  m_visOpt.nRows; i= i + chunk)
        {
            xAxis->SetNumberOfTuples(m_visOpt.nRows);
            yAxis->SetNumberOfTuples(m_visOpt.nRows);
            zAxis->SetNumberOfTuples(m_visOpt.nRows);
            
            xAxis->SetName(m_visOpt.xField.c_str());
            yAxis->SetName(m_visOpt.yField.c_str());
            zAxis->SetName(m_visOpt.zField.c_str());
            
            // Elimina i puntatori agli array esistenti prima di allocare memoria
            xAxis->Reset();
            yAxis->Reset();
            zAxis->Reset();
            
            m_points->Reset();
            newVerts->Reset();
            
            if ( m_pRenderer!=0)
            {
                m_pRenderer->Delete() ;
                m_pRenderer      = vtkRenderer::New();
            }
            if ( m_lut!=0)
            {
                m_lut->Delete() ;
                m_lut      = vtkLookupTable::New();
            }
            if ( m_pRenderWindow!=0)
            {
                m_pRenderWindow->Delete() ;
                m_pRenderWindow      = vtkRenderWindow::New();
            }
            if ( m_polyData!=0)
            {
                m_polyData->Delete() ;
                m_polyData      = vtkPolyData::New();
            }
            
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
            m_points->SetNumberOfPoints(chunk);
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
            
            
            // m_points->Delete();
            // xAxis->Delete();
            // yAxis->Delete();
            // zAxis->Delete();
            
            
            m_polyData->GetPointData()->SetScalars(lutArrays);
            if ( m_visOpt.colorScalar!="none" && m_visOpt.color!="none")
            {
                m_polyData->GetPointData()->SetActiveScalars(m_visOpt.colorScalar.c_str());
                if(m_visOpt.isColorRangeFrom)
                    range[0]=m_visOpt.colorRangeFrom;
                if(m_visOpt.isColorRangeTo)
                    range[1]=m_visOpt.colorRangeTo;
                if(range[1]<=range[0])
                    range[1]=range[0]+0.0001;
                
                m_lut->SetTableRange(range);
                
                
                if(m_visOpt.uselogscale=="yes")
                    m_lut->SetScaleToLog10();
                else
                    m_lut->SetScaleToLinear();
                
                m_lut->Build();
                
                SelectLookTable(&m_visOpt, m_lut);
                
                m_pConeMapper->SetLookupTable(m_lut);
                m_pConeMapper->SetScalarVisibility(1);
                m_pConeMapper->UseLookupTableScalarRangeOn();
                
                m_pConeMapper->SetInputData (m_polyData );
                m_pConeActor->SetMapper ( m_pConeMapper );
                if (m_visOpt.opacity<0 )
                    m_visOpt.opacity=0;
                
                else if( m_visOpt.opacity>1)
                    m_visOpt.opacity=1;
                
                m_pConeActor->GetProperty()->SetOpacity ( m_visOpt.opacity);
                
                
                m_pRenderer->AddActor(m_pConeActor);
                
                m_pRenderWindow->AddRenderer ( m_pRenderer );
                m_pRenderWindow->SetSize ( 1024,731 );
                if(m_visOpt.showLut && z==0)
                    colorBar();
                if (z==0)
                    setBoundingBox (m_polyData  );
                
                
                vtkGenericRenderWindowInteractor* inter=vtkGenericRenderWindowInteractor::New();//generic/**/
                m_pRenderWindow->SetInteractor(inter);
                m_pRenderWindow->Render();
                inter->Start();
                inter->ExitEvent();
                
                localSaveImageAsPng(z);
                if(inter!=0)
                    inter->Delete();
                /*
                 if (z>4)
                 break;
                 else
                 */
                z++;
            }
        }
        // Pulizia delle allocazioni di memoria alla fine
        m_points->Delete();
        newVerts->Delete();
        xAxis->Delete();
        yAxis->Delete();
        zAxis->Delete();
        
        if(newVerts!=0)
            newVerts->Delete();
        radiusArrays->Delete();
        return 0;
    }
    else
    {
        
        
        inFile.open(m_visOpt.path.c_str(), ios::binary); //!open binary file. m_visOpt is the structure (parameter of the constructor) that contain alla data to be visualized
        // std::clog<<m_visOpt.path.c_str()<<std::endl;
        if(!inFile.is_open())
            return -1;
        
        int chunk = 100000;
        if(m_visOpt.nRows < chunk) chunk = m_visOpt.nRows;
        float tmpAxis[chunk];
        for(i=0; i < 100000; i= i + chunk)
        {
            if(i + chunk> m_visOpt.nRows) chunk = m_visOpt.nRows - i;
            inFile.seekg((m_visOpt.x*m_visOpt.nRows +i)* sizeof(float) );
            
            inFile.read((char *)( tmpAxis),chunk*sizeof(float));
            if(m_visOpt.needSwap){
                for(int c = 0; c < chunk; c++){
                    tmpAxis[c]=floatSwap((char *)(&tmpAxis[c]));
                }
            }
            
            for(int c = 0; c < chunk; c++){
                xAxis->  SetValue(i+c,tmpAxis[c]); //! this is the row that fill xAxis array
            }
            
            inFile.seekg((m_visOpt.y*m_visOpt.nRows+ i)* sizeof(float));
            inFile.read((char *)( tmpAxis),  chunk*sizeof(float));
            
            if(m_visOpt.needSwap){
                for(int c = 0; c < chunk; c++){
                    tmpAxis[c]=floatSwap((char *)(&tmpAxis[c]));
                }
            }
            
            for(int c = 0; c < chunk; c++){
                yAxis->  SetValue(i+c,tmpAxis[c]);
            }
            inFile.seekg((m_visOpt.z*m_visOpt.nRows+ i)* sizeof(float));
            inFile.read((char *)(tmpAxis), chunk*sizeof(float));
            if(m_visOpt.needSwap){
                for(int c = 0; c < chunk; c++){
                    tmpAxis[c]=floatSwap((char *)(&tmpAxis[c]));
                }
            }
            
            for(int c = 0; c < chunk; c++){
                zAxis->  SetValue(i+c,tmpAxis[c]);
            }
        }
        
        //saveImageAsPng(1);
    } //else
    
    xAxis->GetRange(m_xRange);  //!minimum and maximum value
    yAxis->GetRange(m_yRange);  //!minimum and maximum value
    zAxis->GetRange(m_zRange);  //!minimum and maximum value
    SetXYZ(xAxis,yAxis,zAxis);
    
    m_polyData->SetPoints(m_points);
    m_points->Delete();
    
    // connect m_pRendererderer and m_pRendererder window and configure m_pRendererder window
    m_pRenderWindow->AddRenderer ( m_pRenderer );
    
    unsigned long long int nPoints = m_polyData->GetNumberOfPoints();
    
    vtkCellArray *newVerts = vtkCellArray::New();
    newVerts->EstimateSize (nPoints,1 );
    //  newVerts->InsertNextCell ( nPoints );
    
    for (i = 0; i < nPoints; i++ )
    {
        newVerts->InsertNextCell(1);
        newVerts->InsertCellPoint ( i );
    }
    m_polyData->SetVerts ( newVerts );
    
    xAxis->Delete();
    yAxis->Delete();
    zAxis->Delete();
    
    float tmp[1];
    
    if(m_visOpt.radiusscalar!="none"&& m_visOpt.scaleGlyphs!="none"&& m_visOpt.nGlyphs!=0 )
    {
        if(m_visOpt.color=="none")
        {
            m_visOpt.color="yes";
            m_visOpt.nColorTable=22; //default is white
            
            if(m_visOpt.oneColor=="yellow") m_visOpt.nColorTable=19;
            if(m_visOpt.oneColor=="red") m_visOpt.nColorTable=24;
            if(m_visOpt.oneColor=="green") m_visOpt.nColorTable=25;
            if(m_visOpt.oneColor=="blue") m_visOpt.nColorTable=26;
            if(m_visOpt.oneColor=="cyan") m_visOpt.nColorTable=20;
            if(m_visOpt.oneColor=="violet") m_visOpt.nColorTable=21;
            if(m_visOpt.oneColor=="black") m_visOpt.nColorTable=23;
            
            m_visOpt.colorScalar=m_visOpt.radiusscalar;
            m_visOpt.showLut=false;
        }
        radiusArrays->SetNumberOfTuples(m_visOpt.nRows);
        if(m_visOpt.dataRead && m_visOpt.goodAllocation)
        {
            int iRadius;
            std::map<std::string, int>::iterator p;
            for(p=m_visOpt.columns.begin();p!=m_visOpt.columns.end();p++)
                if(p->first==m_visOpt.radiusscalar) iRadius=p->second;
            for(i=0; i<m_visOpt.nRows;i++)
                radiusArrays->  SetValue(i,m_visOpt.tableData[iRadius][i]);
        } 
        else
        {
            for (i=0;i<m_visOpt.nRows;i++)
            {
                inFile.seekg((m_visOpt.nRadius * m_visOpt.nRows+i )* sizeof(float));
                inFile.read((char *)(tmp ),  sizeof(float));
                
                if(m_visOpt.needSwap)
                    tmp[0]=floatSwap((char *)(&tmp[0]));
                
                radiusArrays->  SetValue(i,tmp[0]);
            }
        }//else
        
        radiusArrays->SetName(m_visOpt.radiusscalar.c_str());
        m_polyData->GetPointData()->SetScalars(radiusArrays);   //!adda scalar to vtkpolidata
    }
    
    
    if(m_visOpt.heightscalar!="none"&& m_visOpt.scaleGlyphs!="none"&& m_visOpt.nGlyphs!=0 && m_visOpt.nGlyphs!=1 )
    {
        vtkFloatArray *heightArrays = vtkFloatArray::New();
        heightArrays->SetNumberOfTuples(m_visOpt.nRows);
        
        if(m_visOpt.dataRead && m_visOpt.goodAllocation)
        {
            int iheight;
            std::map<std::string, int>::iterator p;
            for(p=m_visOpt.columns.begin();p!=m_visOpt.columns.end();p++)
                if(p->first==m_visOpt.heightscalar) iheight=p->second;
            for(i=0; i<m_visOpt.nRows;i++)
                heightArrays->  SetValue(i,m_visOpt.tableData[iheight][i]);
        } else
        {
            for (i=0;i<m_visOpt.nRows;i++)
            {
                inFile.seekg((m_visOpt.nHeight * m_visOpt.nRows+i )* sizeof(float));
                inFile.read((char *)(tmp ),  sizeof(float));
                
                if(m_visOpt.needSwap)
                    tmp[0]=floatSwap((char *)(&tmp[0]));
                
                heightArrays->  SetValue(i,tmp[0]);
            }
        }//else
        
        heightArrays->SetName(m_visOpt.heightscalar.c_str());
        m_polyData->GetPointData()->SetScalars(heightArrays);
        heightArrays->Delete();
    }
    
    if(m_visOpt.colorScalar!="none")
    {
        vtkFloatArray *lutArrays =vtkFloatArray::New();
        lutArrays->SetNumberOfTuples(m_visOpt.nRows);
        
        if(m_visOpt.dataRead && m_visOpt.goodAllocation)
        {
            unsigned long long int ilut;
            std::map<std::string, int>::iterator p;
            for(p=m_visOpt.columns.begin();p!=m_visOpt.columns.end();p++)
                if(p->first==m_visOpt.colorScalar) ilut=p->second;
            for(i=0; i<m_visOpt.nRows;i++)
                lutArrays->  SetValue(i,m_visOpt.tableData[ilut][i]);
        }
        else
        {
            int chunk = 10000;
            if(m_visOpt.nRows < chunk) chunk = m_visOpt.nRows;
            float tmpScalar[chunk];
            for (i=0;i<m_visOpt.nRows;i=i+chunk)
            {
                if(i + chunk> m_visOpt.nRows) chunk = m_visOpt.nRows - i;
                inFile.seekg((m_visOpt.nColorScalar * m_visOpt.nRows+i )* sizeof(float));
                inFile.read((char *)(tmpScalar),  chunk*sizeof(float));
                
                if(m_visOpt.needSwap){
                    for(int c = 0; c < chunk; c++){
                        tmpScalar[c]=floatSwap((char *)(&tmpScalar[c]));
                    }
                }
                
                for(int c = 0; c < chunk; c++){
                    lutArrays->  SetValue(i+c,tmpScalar[c]);
                }
            }
        } //else
        
        lutArrays->SetName(m_visOpt.colorScalar.c_str());
        m_polyData->GetPointData()->SetScalars(lutArrays);
        
        double range [2];
        lutArrays->GetRange(range);
        if(range[0]<=0)
            m_visOpt.uselogscale="none";
        
        lutArrays->Delete();
    }
    
    inFile.close();
    
    
    
    setBoundingBox (m_polyData  );
    
    m_pConeMapper->SetInputData (m_polyData );
    m_pConeActor->SetMapper ( m_pConeMapper );
    
    if(m_visOpt.color=="none")
    {
        vtkProperty *P = vtkProperty::New();
        P->SetColor(1, 1 ,1);  //default is white
        if(m_visOpt.oneColor=="yellow") P->SetColor(1, 1 ,0);
        if(m_visOpt.oneColor=="red") P->SetColor(1, 0 ,0);
        if(m_visOpt.oneColor=="green") P->SetColor(0, 1 ,0);
        if(m_visOpt.oneColor=="blu") P->SetColor(0, 0 ,1);
        if(m_visOpt.oneColor=="cyane") P->SetColor(0, 1 ,1);
        if(m_visOpt.oneColor=="violet") P->SetColor(1, 0 ,1);
        if(m_visOpt.oneColor=="black") P->SetColor(0, 0 ,0);
        // 110 (giallo);  100(rosso), 010 Verde, 001 Blu, 011 Cyane, 101 Violet, 111 bianco ,000 (nero)
        m_pConeActor->SetProperty(P);
        P->Delete();
    }
    
    m_pRenderer->AddActor ( m_pConeActor );
    
    if (m_visOpt.opacity<0 )
        m_visOpt.opacity=0;
    
    else if( m_visOpt.opacity>1)
        m_visOpt.opacity=1;
    
    m_pConeActor->GetProperty()->SetOpacity ( m_visOpt.opacity);
    
    if (m_visOpt.nGlyphs!=0)
        setGlyphs (  );
    
    if(m_visOpt.scaleGlyphs!="none")
        setScaling ();
    
    if ( m_visOpt.colorScalar!="none" && m_visOpt.color!="none")
        setLookupTable ( );
    
    if(m_visOpt.scaleGlyphs!="none"&& m_visOpt.nGlyphs!=0 ||( (m_visOpt.heightscalar!="none" ||  (m_visOpt.radiusscalar!="none" && m_visOpt.nGlyphs!=1))  ))
        m_glyph->ScalingOn();
    else
        m_glyph->ScalingOff();
    
    
    m_pRenderer->SetBackground ( 0.0,0.0,0.0);
    if(m_visOpt.backColor=="yellow") m_pRenderer->SetBackground (1, 1 ,0);
    if(m_visOpt.backColor=="red")m_pRenderer->SetBackground (1, 0 ,0);
    if(m_visOpt.backColor=="green") m_pRenderer->SetBackground (0, 1 ,0);
    if(m_visOpt.backColor=="blue") m_pRenderer->SetBackground (0, 0 ,1);
    if(m_visOpt.backColor=="cyan") m_pRenderer->SetBackground (0, 1 ,1);
    if(m_visOpt.backColor=="violet") m_pRenderer->SetBackground (1, 0 ,1);
    if(m_visOpt.backColor=="white") m_pRenderer->SetBackground (1, 1 ,1);
    
    if(m_visOpt.imageSize=="small")
        m_pRenderWindow->SetSize ( 512,365 );
    else if(m_visOpt.imageSize=="large")
        m_pRenderWindow->SetSize ( 1024,731 );
    else
        m_pRenderWindow->SetSize ( 792,566 );
    
    m_pRenderWindow->SetWindowName ("VisIVOServer View");
    
    
    if(m_visOpt.stereo)
    {
        m_pRenderWindow->StereoRenderOn();
        if(m_visOpt.stereoMode=="RedBlue")
        {
            m_pRenderWindow->SetStereoTypeToRedBlue();;
        }
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
    
    //open view
    //------------------------------------------------------------------
    vtkGenericRenderWindowInteractor* inter=vtkGenericRenderWindowInteractor::New();
    m_pRenderWindow->SetInteractor(inter);
    m_pRenderWindow->Render();
    //--------------------------------------------------------------------
    
    setCamera ();
    double *bounds;
    bounds=new double[6];
    bounds[0]=m_xRange[0];
    bounds[1]=m_xRange[1];
    bounds[2]=m_yRange[0];
    bounds[3]=m_yRange[1];
    bounds[4]=m_zRange[0];
    bounds[5]=m_zRange[1];
    
    if(m_visOpt.showAxes)
        setAxes (m_polyData,bounds );
    delete [] bounds;
    
    //open view
    //-----------------------------------
    inter->Start();
    inter->ExitEvent();
    //-----------------------------------
    
    if(inter!=0)
        inter->Delete();
    if(newVerts!=0)
        newVerts->Delete();
    radiusArrays->Delete();
    return 0;
}

//------------------------------------
std::string PointsPipe::localSaveImageAsPng(int num )
//------------------------------------
{
    
    
    int magnification=1;
    vtkWindowToImageFilter *w2i=vtkWindowToImageFilter::New();
    std::string path;
    std::string numero;
    std::stringstream ss;
    ss<<num;
    ss>>numero;
    std::string fileName;
    w2i->SetInputBufferTypeToRGBA();  // Enable alpha channel
    w2i->ReadFrontBufferOff();         // Read from back buffer
    
    w2i->SetInput(m_pRenderWindow);
    
    w2i->SetScale(magnification);
    
    w2i->Update();
    //std::clog <<"out "<< m_visOpt.imageName.c_str()<<std::endl;
    if (m_visOpt.imageName!="VisIVOServerImage")
    {
        if (m_visOpt.imageName.find("/")!=std::string::npos)
        {
            
            path=getDir(m_visOpt.imageName);
            if (path.at(0)!='/')
                path="./"+ path;
            
            m_visOpt.imageName=getName(m_visOpt.imageName);
        }
        
        else
            path="./";
    }
    
    else
        
        path="./";
    
    
    
    if (m_visOpt.noDefault=="yes")
    {
        size_t found=m_visOpt.imageName.find(".png");
        
        if(found== std::string::npos)
            fileName=path+m_visOpt.imageName+numero+".png";
        else
            fileName=path+m_visOpt.imageName;
        
        
        //            fileName=path+m_visOpt.imageName+".png";
    }
    else
    {
        size_t found=m_visOpt.imageName.find(".png");
        
        if(found== std::string::npos)
            fileName=path+m_visOpt.imageName+numero+"-"+std::to_string(num)+".png";
        else
            fileName=path+m_visOpt.imageName.substr(0,found)+numero+"-"+std::to_string(num)+"+"".png";
        
    }
    //std::clog <<"fielname "<< fileName.c_str()<<std::endl;
    
    vtkPNGWriter *w=vtkPNGWriter::New();
    
    
    w->SetInputData(w2i->GetOutput());
    
    w->SetFileName(fileName.c_str());
    
    w->Write();
    
    if ( w2i != 0 )
        w2i->Delete();
    
    if ( w!= 0 )
        w->Delete();
    
    return fileName;
    
    
    
    
    this->destroyVTK();
    return "";
    
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
