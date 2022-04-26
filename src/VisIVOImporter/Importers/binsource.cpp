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
//#include "VisIVOImporterConfigure.h"
#include "binsource.h"
#include "visivoutils.h"
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>
#include <sstream>

//---------------------------------------------------------------------
int BinSource::readHeader()
//---------------------------------------------------------------------
{

  int i = 0;
  std::string headerFileName;
  if (m_binaryHeader == "noheader")
    headerFileName = m_pointsFileName + ".head";
  else
    headerFileName = m_binaryHeader;

  std::ifstream inHeader;
  inHeader.open(headerFileName.c_str());

  if (!inHeader)
    return 1;

  const unsigned long int BUFFER_SIZE = 73;
  char buffer[BUFFER_SIZE];

  const int BUFFER_FULL = BUFFER_SIZE - 1;
  std::streamsize sSize = BUFFER_FULL;

  inHeader >> m_dataType;
  inHeader >> m_nCols;
  // Modified Ube
  std::string tmp = "";
  getline(inHeader, tmp); //! to remove the carrige return character  (\r) from the last line
  getline(inHeader, tmp);
  std::stringstream sstmp(tmp);
  sstmp >> m_nRows;

  if (!(sstmp.eof()))
  {
    //    m_isVolume=true;

    sstmp >> m_cellComp[0];
    sstmp >> m_cellComp[1];
    sstmp >> m_cellComp[2];

    sstmp >> m_cellSize[0];
    sstmp >> m_cellSize[1];
    sstmp >> m_cellSize[2];
  }

  inHeader >> m_endian;

  //! to throw away the cr after the number if rows
  inHeader.getline(buffer, BUFFER_SIZE);

  // reads the field names
  m_fieldNames.clear();
  std::string name = "";

  for (i = 0; i < m_nCols && !(inHeader.eof()); ++i)
  {
    inHeader >> name;
    m_fieldNames.push_back(name);
  }

  name = "";

  //! if the field names are less the number if cols
  //! set the number of cols to the number of field names.
  //! Check if it causes any problem

  if (m_fieldNames.size() != m_nCols || m_nCols == 0)
  {
    m_nCols = 0;
    m_nRows = 0;
    m_fieldNames.clear();
    inHeader.close();
    return 1;
  }

  inHeader.close();

  return 0;
}

//---------------------------------------------------------------------
int BinSource::readData()
//---------------------------------------------------------------------
{

  int i = 0;
  int j = 0;
  int nLoad = (int)(500000000 / m_nCols);

  double *dArray = NULL;
  long double *ldArray = NULL;
  int *iArray = NULL;
  long int *liArray = NULL;
  long long int *lliArray = NULL;

  float *fArray = NULL;
  //  long posRead=0;
  //  long posWrite=0;
  //  std::ios::off_type posRead=0;
  //  std::ios::off_type posWrite=0;
  unsigned long long int res = m_nCols * m_nRows;
  /*  std::clog<<"nLoadint="<<nLoad<<endl;
    std::clog<<"resinit="<<res<<endl;*/

  std::ifstream inFile;

  std::ofstream outFile(m_pointsBinaryName.c_str(), std::ofstream::binary);

  inFile.open(m_pointsFileName.c_str(), std::ios::binary);

  if (!inFile)
  {
    return 1;
  }

  try
  {
    fArray = new float[nLoad];
  }
  catch (std::bad_alloc e)
  {
    return 1;
  }

  if (m_dataType != "f" && m_dataType != "float")
  {
    try
    {
      if (m_dataType == "d" || m_dataType == "double")
        dArray = new double[nLoad];
      else if (m_dataType == "ld" || m_dataType == "long double")
        ldArray = new long double[nLoad];
      else if (m_dataType == "i" || m_dataType == "int")
        iArray = new int[nLoad];
      else if (m_dataType == "li" || m_dataType == "long int")
        liArray = new long int[nLoad];
      else if (m_dataType == "lli" || m_dataType == "long long int")
        lliArray = new long long int[nLoad];
      else
      {
        std::cerr << "Invalid type format " << m_dataType << std::endl;
        return 1;
      }
    }

    catch (std::bad_alloc e)
    {
      return 1;
    }
  }

  while (res != 0)
  {
    if (nLoad > res)
      nLoad = res;

    //    inFile.seekg(posRead);
    if (m_dataType == "d" || m_dataType == "double")
      inFile.read((char *)(dArray), nLoad * sizeof(double));
    else if (m_dataType == "ld" || m_dataType == "long double")
      inFile.read((char *)(ldArray), nLoad * sizeof(long double));
    else if (m_dataType == "i" || m_dataType == "int")
      inFile.read((char *)(iArray), nLoad * sizeof(int));
    else if (m_dataType == "li" || m_dataType == "long int")
      inFile.read((char *)(liArray), nLoad * sizeof(long int));
    else if (m_dataType == "lli" || m_dataType == "long long int")
      inFile.read((char *)(lliArray), nLoad * sizeof(long long int));
    else
      inFile.read((char *)(fArray), nLoad * sizeof(float));

    //    posRead=inFile.tellg();
    // std::clog<<"posRead="<<posRead<<endl;
    std::string systemEndianism;
    bool needSwap = false;
#ifdef VSBIGENDIAN
    systemEndianism = "big";
#else
    systemEndianism = "little";
#endif
    if ((m_endian == "b" || m_endian == "big") && systemEndianism == "little")
      needSwap = true;
    if ((m_endian == "l" || m_endian == "little") && systemEndianism == "big")
      needSwap = true;
    if (needSwap)
    {
      for (j = 0; j < nLoad; j++)
      {
        if (m_dataType == "d" || m_dataType == "double")
          fArray[j] = (float)(doubleSwap((char *)(&dArray[j])));
        else if (m_dataType == "ld" || m_dataType == "long double")
          fArray[j] = (float)(longdoubleSwap((char *)(&ldArray[j])));
        else if (m_dataType == "i" || m_dataType == "int")
          fArray[j] = (float)(intSwap((char *)(&iArray[j])));
        else if (m_dataType == "li" || m_dataType == "long int")
          fArray[j] = (float)(longintSwap((char *)(&liArray[j])));
        else if (m_dataType == "lli" || m_dataType == "long long int")
          fArray[j] = (float)(longlongintSwap((char *)(&lliArray[j])));
        else
          fArray[j] = floatSwap((char *)(&fArray[j]));
      }
    }
    else
    {
      if (m_dataType == "d" || m_dataType == "double")
      {
        for (i = 0; i < nLoad; i++)
          fArray[i] = (float)dArray[i];
      }
      else if (m_dataType == "ld" || m_dataType == "long double")
      {
        for (i = 0; i < nLoad; i++)
          fArray[i] = (float)ldArray[i];
      }
      else if (m_dataType == "i" || m_dataType == "int")
      {
        for (i = 0; i < nLoad; i++)
          fArray[i] = (float)iArray[i];
      }
      else if (m_dataType == "li" || m_dataType == "long int")
      {
        for (i = 0; i < nLoad; i++)
          fArray[i] = (float)liArray[i];
      }
      else if (m_dataType == "lli" || m_dataType == "long long int")
      {
        for (i = 0; i < nLoad; i++)
          fArray[i] = (float)lliArray[i];
      }
    }
    //    outFile.seekp(posWrite);
    outFile.write((char *)(fArray), sizeof(float) * nLoad);
    //    posWrite=outFile.tellp();
    // std::clog<<"posWrite="<<posWrite<<endl;
    res = res - nLoad;
    // std::clog<<"res="<<res<<endl;
  }

  if (fArray)
    delete[] fArray;
  if (dArray)
    delete[] dArray;
  if (ldArray)
    delete[] ldArray;
  if (iArray)
    delete[] iArray;
  if (liArray)
    delete[] liArray;
  if (lliArray)
    delete[] lliArray;

  inFile.close();

  makeHeader(m_nRows, m_pointsBinaryName, m_fieldNames, m_cellSize, m_cellComp, m_volumeOrTable);

  return 0;
}
