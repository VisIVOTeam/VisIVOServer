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

#include "fitsimagesource.h"
#include "stdio.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "visivoutils.h"

extern "C"
{
#include "fitsio.h"
}

//---------------------------------------------------------------------
int FitsImageSource::readHeader()
//---------------------------------------------------------------------
{
    return 0;
}

//---------------------------------------------------------------------
int FitsImageSource::readData()
//---------------------------------------------------------------------
//! CHECK ENDINAISM PLEASE!!!!
// fits_read_colnull(pFile, typecode, i+1 , firstrow,firstelem, nLoad, readColData, nullArray, &anynul, &status);
//  pFile Fits table pointer; typecode Dtata Type in the Column; i+1: number of Column (1= first column),
// firstrow= row to be read in the column, firstelem First element in the array where data are stored
//  (readColData), nLoad number of lement to be read, readColData, array where data are stored, nullArry: missing
// element=1
{

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
    if (m_fitshdunum == -1)
        m_fitshdunum = 1;

    unsigned long long int res = 0;

    long firstrow = 1;
    long firstelem = 1;
    long nelements;
    char *nullArray = 0;
    int err_ret = 0;
    int fitsCol;
    char *ttype;
    ttype = new char[FLEN_VALUE];
    status = 0;
    m_nCols = 0;
    err_ret = fits_open_file(&pFile, m_pointsFileName.c_str(), READONLY, &status); //! open FITS input file
    if (err_ret != 0)
    {
        std::cerr << "Invalid open of fitstable file. Importer aborted" << std::endl;
        return -1;
    }
    fits_get_num_hdus(pFile, &hdunum, &status);
    if (hdunum < m_fitshdunum)
    {
        std::cerr << "Invalid fits hdu number " << m_fitshdunum << " The fits table contains only " << hdunum << " hdus block" << std::endl;
        return -1;
    }
    fits_get_hdu_num(pFile, &hdunum1);
    fits_get_hdu_type(pFile, &hdutype, &status);

    fits_movabs_hdu(pFile, m_fitshdunum, &hdutype, &status);
    fits_get_hdu_num(pFile, &hdunum1);
    status = 0;
    hdutype = 0;
    fits_get_hdu_type(pFile, &hdutype, &status);
    if (hdutype != 0)
    {
        std::cerr << "Invalid fits hdu type " << hdutype << " The fits hdu is not an image." << std::endl;
        return -1;
    }
    int naxis = 0;
    fits_get_img_dim(pFile, &naxis, &status);
    if (naxis < 2)
    {
        std::cerr << "Only " << naxis << " axes. The fits hdu is not an image." << std::endl;
        return -1;
    }
    if (m_file == "volume" && naxis < 3)
    {
        std::cerr << "Only " << naxis << " axes. The fits hdu is not a volume." << std::endl;
        return -1;
    }
    long dimAx[naxis];
    fits_get_img_size(pFile, naxis, dimAx, &status);

    //    std::clog<<dimAx[0]<<" "<<dimAx[1]<<" "<<dimAx[2]<<std::endl;
    int volDim;
    long *firstpix;
    firstpix = new long[naxis];
    for (int i = 0; i < naxis; i++)
        firstpix[i] = 1;
    int totPlane = 1;

    if (m_file == "volume")
    {
        m_nRows = int(dimAx[0] * dimAx[1] * dimAx[2]);
        m_cellComp[0] = dimAx[0];
        m_cellComp[1] = dimAx[1];
        m_cellComp[2] = dimAx[2];
        volDim = 3;
        std::stringstream temp1;
        temp1 << m_pointsFileName;
        if (naxis > 3)
            temp1 << "_" << 0;
        m_fieldNames.push_back(temp1.str());
        if (naxis > 3)
        {
            for (int i = 3; i < naxis; i++)
                totPlane *= dimAx[i];
            for (int i = 1; i < totPlane; i++)
            {
                std::stringstream temp1;
                temp1 << m_pointsFileName << "_" << i;
                m_fieldNames.push_back(temp1.str());
            }
        }
    }
    else
    {
        m_nRows = int(dimAx[0] * dimAx[1]);
        m_cellComp[0] = dimAx[0];
        m_cellComp[1] = dimAx[1];
        m_cellComp[2] = 1;
        volDim = 2;
        std::stringstream temp1;
        temp1 << m_pointsFileName;
        if (naxis > 2)
            temp1 << "_" << 0;
        m_fieldNames.push_back(temp1.str());
        if (naxis > 2)
        {

            for (int i = 2; i < naxis; i++)
                totPlane *= dimAx[i];
            for (int i = 1; i < totPlane; i++)
            {
                std::stringstream temp1;
                temp1 << m_pointsFileName << "_" << i;
                m_fieldNames.push_back(temp1.str());
            }
        }
    }

    for (int i = 2; i < naxis; i++)
        totPlane *= dimAx[i];
    int nLoad = MAX_LARGE_LOAD;
    res = dimAx[0] * dimAx[1];
    if (nLoad > res)
        nLoad = res;
    else
    {
        std::cerr << "Very Large planar image number of pixel " << res << " exceed the maximum planar image " << MAX_LARGE_LOAD;
        std::cerr << "Importer Aborted." << std::endl;
        return -1;
    }
    std::ofstream outfile(m_pointsBinaryName.c_str(), std::ofstream::binary); //! open out binary file
    float *DataArrayScal;
    DataArrayScal = new float[nLoad];
    long npixels;
    bool newPlane = true;
    // read image or volume by planes
    while (newPlane)
    {
        fits_read_pix(pFile, TFLOAT, firstpix, res, NULL, DataArrayScal,
                      NULL, &status);
        outfile.write((char *)(DataArrayScal), sizeof(float) * res);
        newPlane = false;
        for (int j = 2; j < naxis; j++)
        {
            firstpix[j]++;
            if (firstpix[j] > dimAx[j])
            {
                firstpix[j] = 1;
                if (j == naxis - 1)
                    newPlane = false;
            }
            else
            {
                newPlane = true;
                break;
            }
        }
    }

    //! close fits file
    fits_close_file(pFile, &status);
    outfile.close();

    m_volumeOrTable = "volume";
    makeHeader(m_nRows, m_pointsBinaryName, m_fieldNames, m_cellSize, m_cellComp, m_volumeOrTable);

    return 0;
}
