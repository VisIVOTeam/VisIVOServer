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
#include "rawgridsource.h"
#include "visivoutils.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

//---------------------------------------------------------------------
int RawGridSource::readHeader()
//---------------------------------------------------------------------
{
  m_fieldNames.clear();

  std::string fileType, variableName, dataField, str;

  double dimension[3];
  int numberOfFiles = 0;
  std::vector<double> times;
  int dim = 3;
  variableName = "";
  dataField = "";
  m_nRows = 0;
  numberOfFiles = 0;
  dimension[0] = dimension[1] = dimension[2] = 0;
  m_files.clear();
  times.clear();

  if (m_pointsFileName.c_str() != "")
  {
    char cFileType[256];

    std::ifstream inFile;

    //!     Get File type
    inFile.open(m_pointsFileName.c_str(), std::ios::binary);

    if (!inFile)
      return 1;

    inFile.read(cFileType, 12);
    inFile.close();
    cFileType[12] = '\0';

    fileType = cFileType;

    inFile.open(m_pointsFileName.c_str());

    if (fileType == "rawGridsDesc")
    {
      inFile >> cFileType;

      char tmp[256];

      inFile >> tmp;
      variableName = tmp;

      inFile >> dim; //! read the data dimension

      inFile >> tmp; //! read the data type line
      dataField = tmp;

      if (dataField.c_str() == "Double" || dataField.c_str() == "double" || dataField.c_str() == "d")
        m_dataType = "double";
      else
        m_dataType = "float";

      m_nRows = 1;

      for (int i = 0; i < dim; i++) //! Read dimensions
      {
        inFile >> m_cellComp[i];
        m_nRows = m_nRows * m_cellComp[i];
      }

      inFile >> tmp; //! read the time line

      inFile >> m_endian; //! Read the endian type

      numberOfFiles = 0;

      while (!inFile.eof()) //! Read in all filenames and time steps
      {
        double tmpTime;
        inFile >> tmpTime;

        times.push_back(tmpTime);
        std::string path = getDir(m_pointsFileName.c_str());
        inFile >> tmp;
        str = tmp; //! read file name
        trim(str);
        // std::clog<<str<<std::endl;
        std::string pathDir = path + str;
        // std::clog<<pathDir<<std::endl;
        if (numberOfFiles == 0)
        {
          if (str.substr(0, 7) == "http://" || str.substr(0, 7) == "sftp://" || str.substr(0, 6) == "ftp://")
            m_files.push_back(str);
          else
            m_files.push_back(pathDir);
          ++numberOfFiles;
        }
        else
        {
          if (str.substr(0, 7) == "http://" || str.substr(0, 7) == "sftp://" || str.substr(0, 6) == "ftp://")
          {
            if (iCompare(m_files[m_files.size() - 1].c_str(), str.c_str()))
            {
              m_files.push_back(str);
              ++numberOfFiles;
            }
          }
          else
          {
            //	      std::clog<<"pippo"<<std::endl;
            if (iCompare(m_files[m_files.size() - 1].c_str(), pathDir.c_str()))
            {
              m_files.push_back(pathDir);
              ++numberOfFiles;
            }
          }
        }
      }
      inFile.close();

      // std::clog<<m_files.size()<<std::endl;
      // std::clog<<m_nRows<<std::endl;

      m_fieldNames.push_back(variableName);
    }

    return 0;
  }
  return 0;
}
//---------------------------------------------------------------------
int RawGridSource::readData()
//---------------------------------------------------------------------
{
  int j = 0;
  int i = 0;
  int k = 0;
  int nLoad = 9999999 / 3;
  int sum = 0;
  unsigned long long int resvel = m_nRows;
  long posWrite = 0;
  long posRead = 0;
  std::string systemEndianism;
  std::string localFilename;
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

  float *fArray = NULL;
  double *dArray = NULL;

  float *data = NULL;
  std::ofstream outFile;
  std::ifstream inFile;

  int idx = m_pointsBinaryName.rfind('.');
  std::string newName = m_pointsBinaryName.erase(idx, idx + 4);

  if (m_dataType == "d" || m_dataType == "double")
  {
    try
    {
      dArray = new double[nLoad];
    }
    catch (std::bad_alloc e)
    {
      return 1;
    }
  }
  else
  {
    try
    {
      fArray = new float[nLoad];
    }
    catch (std::bad_alloc e)
    {
      return 1;
    }
  }
  try
  {

    data = new float[nLoad];
  }
  catch (std::bad_alloc e)
  {
    return 1;
  }

  for (int j = 0; j < m_files.size(); j++)
  {
    sum = 0;
    resvel = m_nRows;
    posWrite = 0;
    posRead = 0;
    localFilename = m_files[j];

    std::string fileName = getName(m_files[j]);
    // std::clog<<newName<<std::endl;
    m_pointsBinaryName = newName + fileName + ".bin";
    // std::clog<<m_pointsBinaryName<<std::endl;
    outFile.open(m_pointsBinaryName.c_str(), std::ofstream::binary);
    // add by ube
    if (m_files[j].substr(0, 7) == "http://" || m_files[j].substr(0, 7) == "sftp://" || m_files[j].substr(0, 6) == "ftp://")
    {
      bool remoteOp;
      std::string downloadedPath, remoteFile;
      remoteFile = m_files[j];
      downloadedPath = getDir(m_pointsBinaryName);
      downloadedPath.append(getName(m_files[j]));
      remoteOp = remoteDownloadFiles(m_files[j], m_login, downloadedPath);
      if (!remoteOp)
      {
        std::cout << "Remote download failed" << std::endl;
        return -1;
      }
      localFilename = downloadedPath;
    }

    //

    inFile.open(localFilename.c_str(), std::ios::binary);
    if (!inFile)
      return 1;

    while (resvel != 0)
    {

      if (nLoad > resvel)
        nLoad = resvel;

      /*      inFile.seekg(posRead);
            std::clog<<"posRead="<<posRead<<std::endl;*/

      if (m_dataType == "double")
        inFile.read((char *)(dArray), nLoad * sizeof(double));
      else
        inFile.read((char *)(fArray), nLoad * sizeof(float));

      //       posRead=inFile.tellg();
      //       std::clog<<"posRead="<<posRead<<std::endl;

      if (needSwap)
      {
        for (j = 0; j < nLoad; j++)
        {
          if (m_dataType == "double")
            dArray[j] = doubleSwap((char *)(&dArray[j]));
          else
            fArray[j] = floatSwap((char *)(&fArray[j]));
        }
      }

      if (m_dataType == "double")
        for (i = 0; i < (nLoad); i++)
          data[i] = (float)dArray[i];

      if (m_dataType == "double")
        outFile.write((char *)(data), sizeof(float) * (nLoad));
      else
        outFile.write((char *)(fArray), sizeof(float) * (nLoad));

      resvel = resvel - nLoad;
      sum = sum + (nLoad);
    }

    outFile.close();
    inFile.close();
    makeHeader(m_nRows, m_pointsBinaryName, m_fieldNames, m_cellSize, m_cellComp, m_volumeOrTable);
    if (m_files[k].substr(0, 7) == "http://" || m_files[k].substr(0, 7) == "sftp://" || m_files[k].substr(0, 6) == "ftp://")
      remove(localFilename.c_str());
  }
  if (fArray)
  {
    delete[] fArray;
  }

  if (dArray)
  {
    delete[] dArray;
  }

  if (data)
  {
    delete[] data;
  }

  return 0;
}
