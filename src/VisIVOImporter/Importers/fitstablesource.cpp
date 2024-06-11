/***************************************************************************
 *   Copyright (C) 2008 by Gabriella Caniglia *
 *  UBE 03/10/11 *
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

#include "fitstablesource.h"

#include "stdio.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "visivoutils.h"


//----------------------------------------------------------------------------
int FitsTableSource::readHeader()
//----------------------------------------------------------------------------

{
    return 0;
}
//----------------------------------------------------------------------------
int FitsTableSource::readData()
//----------------------------------------------------------------------------
//!CHECK ENDINAISM PLEASE!!!!
//fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nLoad, readColData, nullArray, &anynul, &status);
// pFile Fits table pointer; typecode Dtata Type in the Column; i+1: number of Column (1= first column),
//firstrow= row to be read in the column, firstelem First element in the array where data are stored
// (readColData), nLoad number of lement to be read, readColData, array where data are stored, nullArry: missing
//element=1
{
    std::ofstream outfile(m_pointsBinaryName.c_str(),std::ofstream::binary );  //!open out binary file
    
    
    int i = 0;
    int j = 0;
    
    long nRows;
    char *val = 0;
    char value[1000];
    value[0] = '\0';
    char nullstr[1] = {'\0'};
    int anynul = 1;
    val = value;
    int hdunum = 0;
    int hdunum1 = 0;
    int hdutype = 0;
    int typecode;
    long repeat, width;
    if(m_fitshdunum==-1) m_fitshdunum=2;
    
    unsigned long long int res=0;
    
    long firstrow = 1;
    long firstelem = 1;
    long nelements;
    float **DataArrayScal;
    char *nullArray = 0;
    int err_ret=0;
    int fitsCol;
    char *ttype;
    ttype= new char[FLEN_VALUE];
    status=0;
    m_nCols=0;
    err_ret=fits_open_file(&pFile, m_pointsFileName.c_str(), READONLY, &status); //!open FITS input file
    if(err_ret!=0)
    {
        std::cerr<<"Invalid open of fitstable file. Importer aborted"<<std::endl;
        return -1;
    }
    fits_get_num_hdus(pFile, &hdunum, &status);
    if(hdunum<m_fitshdunum)
    {
        std::cerr<<"Invalid fits hdu number "<<m_fitshdunum<<" The fits table contains only "<<hdunum<<" hdus block"<<std::endl;
        return -1;
    }
    fits_get_hdu_num(pFile,  &hdunum1);
    fits_get_hdu_type(pFile, &hdutype, &status);
    
    fits_movabs_hdu(pFile, m_fitshdunum, &hdutype, &status);
    fits_get_hdu_num(pFile,  &hdunum1);
    status=0;
    hdutype=0;
    fits_get_hdu_type(pFile,&hdutype,&status );
    if(hdutype==0)
    {
        std::cerr<<"Invalid fits hdu type "<<hdutype<<" The fits hdu is not a table."<<std::endl;
        return -1;
    }
    
    
    fits_get_num_rows(pFile, &nRows, &status);
    fits_get_num_cols(pFile, &fitsCol, &status);
    status = 0;
    
    m_nRows=int(nRows);
    int nLoad=MAX_LOAD;
    
    char templ[2] = {'*', '\0'};
    
    //!get col names into vecotr m_fields
    char **colname;
    colname=new char*[fitsCol];
    int is=0;
    for(int i = 1; i <= fitsCol; i++)
    {
        
        colname[i-1]=new char[128];
        fits_get_colname(pFile, CASESEN, templ, colname[i-1], &is, &status);
        //        std::clog<<colname[i-1]<<std::endl;
    }
    is=0;
    for(int i = 1; i <= fitsCol; i++)
    {
        status=0;
        fits_get_coltype(pFile, i, &typecode, &repeat, &width, &status);
        
        m_nCols+=repeat;
        
        
        std::string nomeColonna = colname[i-1];
        
        if(repeat>1)
        {
            for(int j=0;j<repeat;j++)
            {
                std::stringstream temp1;
                temp1<<nomeColonna<<"_"<<j;
                m_fieldNames.push_back(temp1.str());
            }
        } else
            m_fieldNames.push_back(nomeColonna);
    }
    
    nelements = (long)m_nRows;
    status = 0;
    
    //  int nSelScalFields = scalFields.size();
    int nSelScalFields = 0;
    int numeroScal = -1;
    std::string unit = "";
    std::string type = "";
    std::string format = "";
    
    char **sData = 0;
    long pos=0;
    long totRead;
    int vbtColNum=0;
    
    for(i = 0; i < fitsCol; i++)
    {
        nLoad=MAX_LOAD;
        totRead=0;
        type = GetColType(i,&typecode,&repeat);
        unit = GetColUnit(i);
        //    std::clog<<"type "<<type<<" unit"<<unit<<std::endl;
        
        if(m_nRows*repeat<=nLoad)
            nLoad=m_nRows*repeat;
        long nChanc=nLoad/repeat;
        try
        {
            DataArrayScal = new float*[repeat];
        }
        catch (std::bad_alloc & e)
        {
            return 1;
        }
        for (int k=0;k<repeat;k++)
        {
            try
            {
                DataArrayScal[k] = new float[nChanc];
            }
            catch (std::bad_alloc & e)
            {
                return 1;
            }
        }
        
        
        
        firstelem=1;
        firstrow=1;
        res=m_nRows*repeat;
        
        while(res!=0)
        {
            if(nLoad>res)
                nLoad=res;
            
            nelements=nChanc*repeat;
            if(nelements>res)
                nelements=res;
            nChanc=nelements/repeat;
            try
            {
                nullArray = new char[nChanc*repeat];
            }
            catch (std::bad_alloc & e )
            {
                return 1;
            }
            
            for(int ii = 0; ii < nChanc*repeat; ii++)
                nullArray[ii] = 0;
            
            
            if(!(iCompare(type, "Type: String"))) //!string file reading
            {
                
                float *readColData;
                try
                {
                    readColData = new float[nelements];
                }
                catch (std::bad_alloc & e)
                {
                    return 1;
                }
                
                for(j = 0; j <nelements; j++)
                    readColData[j] = TEXT_VALUE;  //!here there is the
                
                
                for(j = 0; j < nChanc; j++)
                {
                    for(int kk=0;kk<repeat;kk++)
                    {
                        DataArrayScal[kk][j]=readColData[kk*nChanc+j];
                    }
                }
                delete [] readColData;
                
                
                
            }
            
            else
            {
                switch(typecode)
                {
                    case 11:
                    {
                        unsigned char *readColData;
                        try
                        {
                            readColData = new unsigned char[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                        fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nelements, readColData, nullArray, &anynul, &status);
                        for(j = 0; j <nelements; j++)
                            if(nullArray[j] == 1)
                                readColData[j] = MISSING_VALUE;  //!here there is the
                        
                        delete [] nullArray;
                        
                        for(j = 0; j < nChanc; j++)
                        {
                            for(int kk=0;kk<repeat;kk++)
                            {
                                DataArrayScal[kk][j]=readColData[kk*nChanc+j];
                            }
                        }
                        delete [] readColData;
                        
                        
                    }
                        break;
                    case 21:
                    {
                        short *readColData;
                        try
                        {
                            readColData = new short[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                        fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nelements, readColData, nullArray, &anynul, &status);
                        for(j = 0; j <nelements; j++)
                            if(nullArray[j] == 1)
                                readColData[j] = MISSING_VALUE;  //!here there is the
                        
                        delete [] nullArray;
                        
                        for(j = 0; j < nChanc; j++)
                        {
                            for(int kk=0;kk<repeat;kk++)
                            {
                                DataArrayScal[kk][j]=readColData[kk*nChanc+j];
                            }
                        }
                        delete [] readColData;
                    }
                        break;
                    case 30:
                    {
                        unsigned int *readColData;
                        try
                        {
                            readColData = new unsigned int[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                    }
                        break;
                    case 31:
                    {
                        int *readColData;
                        try
                        {
                            readColData = new int[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                        fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nelements, readColData, nullArray, &anynul, &status);
                        for(j = 0; j <nelements; j++)
                            if(nullArray[j] == 1)
                                readColData[j] = MISSING_VALUE;  //!here there is the
                        
                        delete [] nullArray;
                        
                        for(j = 0; j < nChanc; j++)
                        {
                            for(int kk=0;kk<repeat;kk++)
                            {
                                DataArrayScal[kk][j]=readColData[kk*nChanc+j];
                            }
                        }
                        delete [] readColData;
                    }
                        break;
                    case 40:
                    {
                        unsigned long int *readColData;
                        try
                        {
                            readColData = new unsigned long int[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                    }
                        break;
                    case 41:
                    {
                        long int *readColData;
                        try
                        {
                            readColData = new long int[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                        fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nelements, readColData, nullArray, &anynul, &status);
                        for(j = 0; j <nelements; j++)
                            if(nullArray[j] == 1)
                                readColData[j] = MISSING_VALUE;  //!here there is the
                        
                        delete [] nullArray;
                        
                        for(j = 0; j < nChanc; j++)
                        {
                            for(int kk=0;kk<repeat;kk++)
                            {
                                DataArrayScal[kk][j]=readColData[kk*nChanc+j];
                            }
                        }
                        delete [] readColData;
                    }
                        break;
                    case 42:
                    {
                        float *readColData;
                        try
                        {
                            readColData = new float[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                        fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nelements, readColData, nullArray, &anynul, &status);
                        for(j = 0; j <nelements; j++)
                            if(nullArray[j] == 1)
                                readColData[j] = MISSING_VALUE;  //!here there is the
                        
                        delete [] nullArray;
                        
                        for(j = 0; j < nChanc; j++)
                        {
                            for(int kk=0;kk<repeat;kk++)
                            {
                                DataArrayScal[kk][j]=readColData[kk*nChanc+j];
                            }
                        }
                        delete [] readColData;
                    }
                        break;
                    case 81:
                    {
                        long long int *readColData;
                        try
                        {
                            readColData = new long long int[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                        fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nelements, readColData, nullArray, &anynul, &status);
                        for(j = 0; j <nelements; j++)
                            if(nullArray[j] == 1)
                                readColData[j] = MISSING_VALUE;  //!here there is the
                        
                        delete [] nullArray;
                        
                        for(j = 0; j < nChanc; j++)
                        {
                            for(int kk=0;kk<repeat;kk++)
                            {
                                DataArrayScal[kk][j]=readColData[kk*nChanc+j];
                            }
                        }
                        delete [] readColData;
                    }
                        break;
                    case 82:
                    {
                        double *readColData;
                        try
                        {
                            readColData = new double[nelements];
                        }
                        catch (std::bad_alloc & e)
                        {
                            return 1;
                        }
                        fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nelements, readColData, nullArray, &anynul, &status);
                        for(j = 0; j <nelements; j++)
                            if(nullArray[j] == 1)
                                readColData[j] = MISSING_VALUE;  //!here there is the
                        
                        delete [] nullArray;
                        int counter=0;
                        for(int kk=0;kk<repeat;kk++)
                        {
                            for(j = 0; j < nChanc; j++)
                            {
                                DataArrayScal[kk][j]=readColData[j*repeat+kk];
                            }
                        }
                        delete [] readColData;
                    }
                        break;
                    default:
                        std::cerr<<"Invalid "<<type<<" in column '"<<colname[i]<<"' ... putting MISSING_VALUE "<<std::endl;
                        for(j = 0; j < nLoad; j++)
                            for(j = 0; j < nChanc; j++)
                            {
                                for(int kk=0;kk<repeat;kk++)
                                {
                                    DataArrayScal[kk][j]=MISSING_VALUE;
                                }
                            }
                        break;
                }//switch
                
            }//else
            
            
            
            /*           if(!(iCompare(unit, "mas")))
             masToRad(DataArrayScal, nelements);
             else if(!(iCompare(unit, "\"h:m:s\"")))
             hmsToRad(sData, DataArrayScal, nelements);
             else if(!(iCompare(unit, "\"d:m:s\"")))
             dmsToRad(sData, DataArrayScal, nelements);
             else if(!(iCompare(unit, "deg")))
             degToRad(DataArrayScal, nelements);
             */
            
            if(repeat==1)
            {
                outfile.write((char*)(DataArrayScal[0]), sizeof(float)*nChanc);
            }
            else
            {
                for(int jj=0;jj<repeat;jj++)
                {
                    outfile.seekp(((vbtColNum+jj)*m_nRows+totRead*jj)*sizeof(float),std::ios::beg);
                    outfile.write((char*)(DataArrayScal[jj]), sizeof(float)*nChanc);
                }
            }
            
            for(int jj=0;jj<repeat;jj++)
                delete [] DataArrayScal[jj];
            delete [] DataArrayScal;
            
            res-=nChanc*repeat;
            totRead+=nelements/repeat;
            firstrow+=nChanc;
            
            
        } //while(res!=0)
        
        vbtColNum+=repeat;
        
    }// for i=0
    
    
    //!close fits file
    fits_close_file(pFile, &status);
    outfile.close();
    
    makeHeader(m_nRows,m_pointsBinaryName,m_fieldNames,m_cellSize,m_cellComp,m_volumeOrTable);
    
    
    return 0;
}

//----------------------------------------------------------------------------
long FitsTableSource::GetNumRows(int ntab)
//----------------------------------------------------------------------------
{
    Ntable = ntab;
    fits_movabs_hdu(pFile, Ntable, &hdutype, &status);
	
    long nm_nRows;
    fits_get_num_rows(pFile, &nm_nRows, &status);
    
    return nm_nRows;
}
//----------------------------------------------------------------------------
int FitsTableSource::GetNumColumns(int ntab)
//----------------------------------------------------------------------------
{
    Ntable = ntab;
	
	//! Move to the selected table (starting from 1, which is dummy)
	
    fits_movabs_hdu(pFile, Ntable, &hdutype, &status);
	
    int nm_nCols;
    fits_get_num_cols(pFile, &nm_nCols, &status);
	
    return nm_nCols;
}

//----------------------------------------------------------------------------
int FitsTableSource::GetHDUType()
//----------------------------------------------------------------------------
{
    int type;
    fits_get_hdu_type(pFile, &type, &status);
	
    return type;
}
//----------------------------------------------------------------------------
std::string FitsTableSource::GetColUnit(int ncol)
//----------------------------------------------------------------------------
{
    std::string fieldunit = "";
    char value[81]; //! temp
    status = 0;
    
    std::stringstream skey;
    std::string key;
    skey << "TUNIT" << (ncol + 1);
    key = skey.str();
    char *keyword = const_cast<char *>(key.c_str());
	
    fits_read_key(pFile, TSTRING, keyword, value, 0, &status);
    
    if(status == 0)
    {
        fieldunit = value;
        return fieldunit;
    }
    
    char err[31];
    fits_get_errstatus(status, err);
    status = 0;
	
    return "";
}

//----------------------------------------------------------------------------
std::string FitsTableSource::GetColFormat(int ncol)
//----------------------------------------------------------------------------
{
    std::string fieldFormat = "";
    char value[81]; //! temp
    status = 0;
    
    std::stringstream skey;
    std::string key;
    skey << "TFORM" << (ncol + 1);
    key = skey.str();
    char *keyword = const_cast<char *>(key.c_str());
    
    fits_read_key(pFile, TSTRING, keyword, value, 0, &status);
    
    if(status == 0)
    {
        fieldFormat = value;
        return fieldFormat;
    }
    
    return "";
}

//----------------------------------------------------------------------------
std::string FitsTableSource::GetColName(int ncol)
//----------------------------------------------------------------------------
{
    if(m_fields.size() > 0)
        return m_fields[ncol];
    
    char *templ = "*";
    char colname[70];
    fits_get_colname(pFile, casesen, templ, colname, &ncol, &status);
    std::string fieldname = colname;
    
    return fieldname;
}
//----------------------------------------------------------------------------
std::string FitsTableSource::GetColType(int ncol, int *typecode,long *repeat)
//----------------------------------------------------------------------------
{
    
    long width;
    std::string fieldtype = "";
    status = 0;
    fits_get_coltype(pFile, ncol+1, typecode, repeat, &width, &status);
    
    switch(*typecode)
    {
        case 1: fieldtype = "Type: Bit"; break;
        case 11: fieldtype = "Type: Byte"; break;
        case 12: fieldtype = "Type: SByte"; break;
        case 14: fieldtype = "Type: Logical"; break;
        case 16: fieldtype = "Type: String"; break;
        case 20: fieldtype = "Type: UShort"; break;
        case 21: fieldtype = "Type: Short"; break;
        case 30: fieldtype = "Type: UInt"; break;
        case 31: fieldtype = "Type: Int"; break;
        case 40: fieldtype = "Type: ULong"; break;
        case 41: fieldtype = "Type: Long"; break;
        case 42: fieldtype = "Type: Float";  break;
        case 81:  fieldtype = "Type: LongLong"; break;
        case 82:  fieldtype = "Type: Double"; break;
        case 83: fieldtype = "Type: Complex"; break;
        case 163: fieldtype = "Type: Double Complex"; break;
        default:  fieldtype = "Type: Unknown";
    }
    
    
    return fieldtype;
}

//----------------------------------------------------------------------------
void FitsTableSource::SetScalFields(int col)
//----------------------------------------------------------------------------
{
    scalFields.clear();
    
    for(int i = 0; i < col; i++)
        scalFields.push_back(i);
    
    return;
}