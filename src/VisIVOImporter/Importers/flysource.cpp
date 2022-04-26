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
#include "flysource.h"

#include "visivoutils.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

//---------------------------------------------------------------------
int FlySource::readHeader()
//---------------------------------------------------------------------

{
  m_fieldNames.clear();
  m_nCols = 6;

  std::string fileType, variableName, dataField, str;

  double dimension[3];
  int numberOfFiles = 0;
  std::vector<double> times;

  variableName = "";
  dataField = "";
  numberOfFiles = 0;
  dimension[0] = dimension[1] = dimension[2] = 0;
  m_files.clear();
  times.clear();

  if (m_pointsFileName.c_str() != "")
  {
    char cFileType[256];

    std::ifstream inFile;

    //! Get File type
    inFile.open(m_pointsFileName.c_str(), std::ios::binary);

    if (!inFile)
      return 1;

    inFile.read(cFileType, 7);
    inFile.close();
    cFileType[7] = '\0';

    fileType = cFileType;

    inFile.open(m_pointsFileName.c_str());

    if (fileType == "flyDesc")
    {
      inFile >> cFileType;

      char tmp[256];

      inFile >> tmp;
      variableName = tmp;

      inFile >> tmp; //! read the data type line
      dataField = tmp;

      if (dataField == "Float" || dataField == "float" || dataField == "f")
        m_type = 'f';
      else
        m_type = 'd';

      inFile >> tmp; //! read the time line

      inFile >> m_nRows; //! Read total number of points

      inFile >> dimension[0] >> dimension[1] >> dimension[2]; //! Read dimensions

      inFile >> m_endian; //! Read the endian type

      numberOfFiles = 0;

      while (!inFile.eof()) //! Read in all filenames and time steps
      {
        double tmpTime;
        inFile >> tmpTime;

        times.push_back(tmpTime);
        std::string path = getDir(m_pointsFileName.c_str()); //! direcory of filename descriptor
        inFile >> tmp;
        str = tmp; //! read file name
        trim(str);
        std::string pathDir;
        size_t found;

        found = str.find("/");
        if (found != std::string::npos)
          pathDir = str;
        else
          pathDir = path + str; //! FLY filename
                                //// std::clog<<pathDir<<endl;

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
    }

    else
    {
      inFile.close();

      m_files.push_back(m_pointsFileName.c_str());
    }
    /**********************************************************************/
    /**********************************************************************/
    /**********************************************************************/

    //       // std::clog<<m_nRows<<std::endl;
    //       // std::clog<<m_files.size()<<std::endl;
    m_fieldNames.push_back("X");
    m_fieldNames.push_back("Y");
    m_fieldNames.push_back("Z");
    m_fieldNames.push_back("Vx");
    m_fieldNames.push_back("Vy");
    m_fieldNames.push_back("Vz");

    return 0;
  }
  return 0;
}
//---------------------------------------------------------------------
int FlySource::readData()
//---------------------------------------------------------------------
{
  int j = 0;
  int i = 0;
  int k = 0;
  int nLoad = 9999999 / 3;
  unsigned long long int sum = 0;
  unsigned long long int res = 3 * m_nRows;
  unsigned long long int resvel = 3 * m_nRows;
  std::ios::off_type posWrite = 0;
  long posRead = 0;

  std::string systemEndianism;
  bool needSwap = false;
#ifdef VSBIGENDIAN
  systemEndianism = "big";
#else
  systemEndianism = "little";
#endif

  float *fArray = NULL;
  double *dArray = NULL;

  float *dataX = NULL;
  float *dataY = NULL;
  float *dataZ = NULL;
  std::string localFilename;

  std::ofstream outFile;
  std::ifstream inFile;
  std::string newName;
  int idx = m_pointsBinaryName.rfind('.');
  newName = m_pointsBinaryName.erase(idx, idx + 4);
  size_t found;

  for (int k = 0; k < m_files.size(); k++)
  {
    localFilename = m_files[k];

    std::string fileName = getName(m_files[k]);
    // std::clog<<newName<<std::endl;
    m_pointsBinaryName = newName;
    found = m_pointsBinaryName.find(".bin");
    if (found == std::string::npos)
      m_pointsBinaryName = newName + ".bin";

    //  std::clog<<m_pointsBinaryName<<std::endl;

    outFile.open(m_pointsBinaryName.c_str(), std::ofstream::binary);

    nLoad = 9999999 / 3;
    // add by ube
    if (m_files[k].substr(0, 7) == "http://" || m_files[k].substr(0, 7) == "sftp://" || m_files[k].substr(0, 6) == "ftp://")
    {
      bool remoteOp;
      std::string downloadedPath, remoteFile;
      remoteFile = m_files[k];
      downloadedPath = getDir(m_pointsBinaryName);
      downloadedPath.append(getName(m_files[k]));
      remoteOp = remoteDownloadFiles(m_files[k], m_login, downloadedPath);
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

    if (m_type == "d" || m_type == "double")
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
      dataX = new float[nLoad / 3];
    }
    catch (std::bad_alloc e)
    {
      return 1;
    }
    try
    {
      dataY = new float[nLoad / 3];
    }
    catch (std::bad_alloc e)
    {
      return 1;
    }
    try
    {
      dataZ = new float[nLoad / 3];
    }
    catch (std::bad_alloc e)
    {
      return 1;
    }

    sum = 0;
    res = 3 * m_nRows;
    posWrite = 0;
    posRead = 0;
    while (res != 0)
    {
      if (nLoad > res)
        nLoad = res;

      //      inFile.seekg(posRead);

      if (m_type == "d" || m_type == "double")
        inFile.read((char *)(dArray), nLoad * sizeof(double));
      else
        inFile.read((char *)(fArray), nLoad * sizeof(float));

      //      posRead=inFile.tellg();
      // std::clog<<"posRead="<<posRead<<std::endl;

      if ((m_endian == "b" || m_endian == "big") && systemEndianism == "little")
        needSwap = true;
      if ((m_endian == "l" || m_endian == "little") && systemEndianism == "big")
        needSwap = true;
      if (needSwap)
      {
        for (j = 0; j < nLoad; j++)
        {
          if (m_type == "d" || m_type == "double")
            dArray[j] = doubleSwap((char *)(&dArray[j]));
          else
            fArray[j] = floatSwap((char *)(&fArray[j]));
        }
      }

      if (m_type == "d" || m_type == "double")
      {
        for (i = 0; i < (nLoad / 3); i++)
        {

          dataX[i] = (float)dArray[i * 3];
          dataY[i] = (float)dArray[i * 3 + 1];
          dataZ[i] = (float)dArray[i * 3 + 2];
        }
      }

      else
      {
        for (i = 0; i < (nLoad / 3); i++)
        {

          dataX[i] = fArray[i * 3];
          dataY[i] = fArray[i * 3 + 1];
          dataZ[i] = fArray[i * 3 + 2];
        }
      }

      for (j = 0; j < 3; j++)
      {
        posWrite = ((sum * (sizeof(float))) + (sizeof(float)) * ((unsigned long long int)j * m_nRows)); /*+(sizeof(float)*(m_nRows)*/
        // QUI c'e' un problema!!!
        if (posWrite < maxInt())
          outFile.seekp(posWrite);
        else
        {
          std::ios::off_type indexOffset = 0;
          outFile.seekp(indexOffset, std::ios::beg);
          long long int totNumberToSkip = posWrite;
          int nSkip = maxInt() * sizeof(float);
          while (totNumberToSkip != 0)
          {
            if (nSkip > totNumberToSkip)
              nSkip = (int)totNumberToSkip;
            indexOffset = (std::ios::off_type)nSkip;
            outFile.seekp(indexOffset, std::ios::cur);
            totNumberToSkip = totNumberToSkip - nSkip;
          }
        }
        // std::clog<<"posWrite="<<posWrite<<std::endl;
        if (j == 0)
          outFile.write((char *)(dataX), sizeof(float) * (nLoad / 3));
        else if (j == 1)
          outFile.write((char *)(dataY), sizeof(float) * (nLoad / 3));
        else if (j == 2)
          outFile.write((char *)(dataZ), sizeof(float) * (nLoad / 3));
      }
      res = res - nLoad;
      sum = sum + (nLoad / 3);
      //  std::clog <<"res="<<res<<std::endl;
      // std::clog <<"sum="<<sum<<std::endl;
      posWrite = outFile.tellp();
      //   std::clog<<"posWritefinal="<<posWrite<<std::endl;
    }

    sum = 0;
    nLoad = 9999999 / 3;
    resvel = 3 * m_nRows;
    while (resvel != 0)
    {

      if (nLoad > resvel)
        nLoad = resvel;

      //      inFile.seekg(posRead);

      if (m_type == "d" || m_type == "double")
        inFile.read((char *)(dArray), nLoad * sizeof(double));
      else
        inFile.read((char *)(fArray), nLoad * sizeof(float));

      //      posRead=inFile.tellg();
      //  std::clog<<"posRead="<<posRead<<std::endl;

      if ((m_endian == "b" || m_endian == "big") && systemEndianism == "little")
        needSwap = true;
      if ((m_endian == "l" || m_endian == "little") && systemEndianism == "big")
        needSwap = true;
      if (needSwap)
      {
        for (j = 0; j < nLoad; j++)
        {
          if (m_type == "d" || m_type == "double")
            dArray[j] = doubleSwap((char *)(&dArray[j]));
          else
            fArray[j] = floatSwap((char *)(&fArray[j]));
        }
      }

      if (m_type == "d" || m_type == "double")
      {
        for (i = 0; i < (nLoad / 3); i++)
        {

          dataX[i] = (float)dArray[i * 3];
          dataY[i] = (float)dArray[i * 3 + 1];
          dataZ[i] = (float)dArray[i * 3 + 2];
        }
      }

      else
      {
        for (i = 0; i < (nLoad / 3); i++)
        {

          dataX[i] = fArray[i * 3];
          dataY[i] = fArray[i * 3 + 1];
          dataZ[i] = fArray[i * 3 + 2];
        }
      }

      for (j = 3; j < 6; j++)
      {
        posWrite = ((sum * (sizeof(float))) + ((sizeof(float)) * ((unsigned long long int)j * m_nRows)) /*+(sizeof(float)*(m_nRows)*/);
        if (posWrite < maxInt())
          outFile.seekp(posWrite);
        else
        {
          std::ios::off_type indexOffset = 0;
          outFile.seekp(indexOffset, std::ios::beg);
          long long int totNumberToSkip = posWrite;
          int nSkip = maxInt() * sizeof(float);
          while (totNumberToSkip != 0)
          {
            if (nSkip > totNumberToSkip)
              nSkip = (int)totNumberToSkip;
            indexOffset = (std::ios::off_type)nSkip;
            outFile.seekp(indexOffset, std::ios::cur);
            totNumberToSkip = totNumberToSkip - nSkip;
          }
        }
        //   std::clog<<"posWrite="<<posWrite<<std::endl;
        if (j == 3)
          outFile.write((char *)(dataX), sizeof(float) * (nLoad / 3));
        else if (j == 4)
          outFile.write((char *)(dataY), sizeof(float) * (nLoad / 3));
        else if (j == 5)
          outFile.write((char *)(dataZ), sizeof(float) * (nLoad / 3));
      }
      //   std::clog <<"resvel="<<resvel<<std::endl;

      resvel = resvel - nLoad;
      // std::clog <<"sum="<<sum<<std::endl;
      sum = sum + (nLoad / 3);
      //   std::clog <<"resvel="<<resvel<<std::endl;
      //  std::clog <<"sum="<<sum<<std::endl;
      posWrite = outFile.tellp();
      //   std::clog<<"posWritefinal="<<posWrite<<std::endl;
    }

    if (fArray)
    {
      delete[] fArray;
    }

    if (dArray)
    {
      delete[] dArray;
    }

    if (dataX)
    {
      delete[] dataX;
    }

    if (dataY)
    {
      delete[] dataY;
    }

    if (dataZ)
    {
      delete[] dataZ;
    }
    inFile.close();

    outFile.close();

    makeHeader(m_nRows, m_pointsBinaryName, m_fieldNames, m_cellSize, m_cellComp, m_volumeOrTable);
    if (m_files[k].substr(0, 7) == "http://" || m_files[k].substr(0, 7) == "sftp://" || m_files[k].substr(0, 6) == "ftp://")
      remove(localFilename.c_str());
  }

  return 0;
}
