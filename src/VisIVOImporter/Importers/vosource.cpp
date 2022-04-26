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

#include "vosource.h"

#include "visivoutils.h"

#include "VOTableParser.h"
#include "voFieldParam.h"

#include <iostream>
#include <fstream>
#include <sstream>

//#include "VOTableParser.h"
//---------------------------------------------------------------------
int VOSource::readData()
//---------------------------------------------------------------------
{
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //      in case of missing data we put 0 as default value
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  m_fieldNames.clear();

  string tmpStr = "";
  int tmpInt = 0;

  int i = 0;
  int j = 0;
  int k = 0;

  std::ios::off_type pos = 0;

  ofstream outfile(m_pointsBinaryName.c_str(), ofstream::binary);

  std::vector<voFieldParam> locFieldList;
  std::vector<std::string> locFieldNames;

  locFieldList.clear();
  locFieldNames.clear();

  std::vector<voFieldParam> m_fieldList;
  m_fieldList.clear();

  int m_typeFlag = 0;

  // Read Data

  int status;

  VOTableParser sPP;

  sPP.Init();

  if (sPP.Parse(const_cast<char *>(m_pointsFileName.c_str())))
    return 1;

  sPP.SetCurrentResource(0);
  sPP.SetCurrentTable(0);
  std::string m_tableDescription = sPP.GetTableDescription(0);

  m_nRows = sPP.GetResourceTRCount(0);
  m_nCols = sPP.GetResourceTableFieldsCount();

  for (i = 0; i < m_nCols; i++)
  {
    voFieldParam f;

    tmpStr = sPP.GetResourceTableFieldArraySize(i);
    f.setArraySize(const_cast<char *>(tmpStr.c_str()), &status);

    tmpStr = sPP.GetResourceTableFieldDescription(i);
    f.setDescription(const_cast<char *>(tmpStr.c_str()), &status);

    tmpStr = sPP.GetResourceTableFieldId(i);
    f.setID(const_cast<char *>(tmpStr.c_str()), &status);

    tmpStr = sPP.GetResourceTableFieldName(i);
    f.setName(const_cast<char *>(tmpStr.c_str()), &status);

    tmpStr = sPP.GetResourceTableFieldPrecision(i);
    f.setPrecision(const_cast<char *>(tmpStr.c_str()), &status);

    tmpStr = sPP.GetResourceTableFieldRef(i);
    f.setRef(const_cast<char *>(tmpStr.c_str()), &status);

    tmpStr = sPP.GetResourceTableFieldUcd(i);
    f.setUCD(const_cast<char *>(tmpStr.c_str()), &status);

    tmpStr = sPP.GetResourceTableFieldUnit(i);
    f.setUnit(const_cast<char *>(tmpStr.c_str()), &status);

    tmpStr = sPP.GetResourceTableFieldDataType(i);
    f.setDatatype(const_cast<char *>(tmpStr.c_str()), &status);

    tmpInt = sPP.GetResourceTableFieldWidth(i);
    f.setWidth(tmpInt, &status);

    locFieldList.push_back(f);
  }

  double scal = 0;
  string unit = "";
  string dataType = "";
  float *tmpArray = 0;
  int sum = 0;

  for (i = 0; i < m_nCols; i++)
  {
    // std::clog<<"i="<<i<<endl;

    unit = getUnit(locFieldList[i]);
    dataType = getDataType(locFieldList[i]);

    if (!(iCompare(dataType, "char")))
      if (iCompare(unit, "\"h:m:s\"") && iCompare(unit, "\"d:m:s\""))
        continue;
    try
    {
      tmpArray = new float[m_nRows];
    }

    catch (std::bad_alloc e)
    {
      return 1;
    }
    int nLoad = (int)(10000000 / m_nCols);

    if (m_nRows <= nLoad)
    {

      for (j = 0; j < m_nRows; j++)
      {
        sPP.SetCurrentTR(j);

        tmpStr = sPP.GetData(i);
        // Missing data and text data
        if (tmpStr.length() == 0)
        {
          tmpArray[j] = MISSING_VALUE;
          continue;
        }

        bool numeric = false;
        char *cstr;
        cstr = new char[tmpStr.size() + 1];
        strcpy(cstr, tmpStr.c_str());
        for (unsigned int k = 0; k < tmpStr.length(); k++)
        {
          numeric = true;
          if (!(isdigit(cstr[k])))
          {
            if ((k == 0 && tmpStr.compare(k, 1, "-") != 0) && (k == 0 && tmpStr.compare(k, 1, "+") != 0))
              if (tmpStr.compare(k, 1, ".") != 0)
              {
                numeric = false;
                break;
              }
            if (k > 0 && tmpStr.compare(k, 1, ".") != 0)
            {
              numeric = false;
              break;
            }
          }
        }

        if (!numeric)
        {
          tmpArray[j] = TEXT_VALUE;
          continue;
        }
        //
        if (!(iCompare(unit, "mas")))
          scal = masToRad(atof(const_cast<char *>(tmpStr.c_str())));
        else if (!(iCompare(unit, "\"h:m:s\"")))
          scal = hmsToRad(const_cast<char *>(tmpStr.c_str()));
        else if (!(iCompare(unit, "\"d:m:s\"")))
          scal = dmsToRad(const_cast<char *>(tmpStr.c_str()));
        else if (!(iCompare(unit, "deg")))
          scal = degToRad(atof(const_cast<char *>(tmpStr.c_str())));
        else
          scal = atof(const_cast<char *>(tmpStr.c_str()));

        tmpArray[j] = scal;
      }

      pos = ((sizeof(float)) * ((unsigned long long int)i * m_nRows));
      outfile.seekp(pos);
      // std::clog<<"posrows="<<pos<<endl;

      outfile.write((char *)(tmpArray), sizeof(float) * m_nRows);

      pos = outfile.tellp();
      // std::clog<<"posfinal="<<pos<<endl;
    }
    else
    {
      int num = 0;
      unsigned long long int res = m_nRows;
      pos = ((sizeof(float)) * ((unsigned long long int)i * m_nRows));

      while (res >= nLoad)
      {
        k = 0;
        for (j = num * nLoad; j < num * nLoad + nLoad; j++)
        {
          sPP.SetCurrentTR(j);

          tmpStr = sPP.GetData(i);

          if (!(iCompare(unit, "mas")))
            scal = masToRad(atof(const_cast<char *>(tmpStr.c_str())));
          else if (!(iCompare(unit, "\"h:m:s\"")))
            scal = hmsToRad(const_cast<char *>(tmpStr.c_str()));
          else if (!(iCompare(unit, "\"d:m:s\"")))
            scal = dmsToRad(const_cast<char *>(tmpStr.c_str()));
          else if (!(iCompare(unit, "deg")))
            scal = degToRad(atof(const_cast<char *>(tmpStr.c_str())));
          else
            scal = atof(const_cast<char *>(tmpStr.c_str()));

          tmpArray[k] = scal;
          ++k;
        }

        res = res - nLoad;
        // std::clog<<"res="<<res<<endl;
        sum = sum + nLoad;
        // std::clog<<"sum="<<sum<<endl;

        outfile.seekp(pos);
        // std::clog<<"posload="<<pos<<endl;

        outfile.write((char *)(tmpArray), sizeof(float) * nLoad);
        ++num;
        //  std::clog<<"num="<<num<<endl;
        pos = outfile.tellp();
        //  std::clog<<"posfinal="<<pos<<endl;
      }

      if (res < nLoad)
      {

        for (j = num * nLoad; j < num * nLoad + res; j++)
        {
          sPP.SetCurrentTR(j);

          tmpStr = sPP.GetData(i);

          if (!(iCompare(unit, "mas")))
            scal = masToRad(atof(const_cast<char *>(tmpStr.c_str())));
          else if (!(iCompare(unit, "\"h:m:s\"")))
            scal = hmsToRad(const_cast<char *>(tmpStr.c_str()));
          else if (!(iCompare(unit, "\"d:m:s\"")))
            scal = dmsToRad(const_cast<char *>(tmpStr.c_str()));
          else if (!(iCompare(unit, "deg")))
            scal = degToRad(atof(const_cast<char *>(tmpStr.c_str())));
          else
            scal = atof(const_cast<char *>(tmpStr.c_str()));

          tmpArray[k] = scal;
          ++k;
        }

        sum = sum + res;
        //  std::clog<<"sum2="<<sum<<endl;

        outfile.seekp(pos);
        //    std::clog<<"posres="<<pos<<endl;

        outfile.write((char *)(tmpArray), sizeof(float) * res);

        pos = outfile.tellp();
      }
    }

    // std::clog<<"posfinal="<<pos<<endl;
    if (tmpArray)
      delete[] tmpArray;

    char *str;

    locFieldList[i].getName(str, &status);
    m_fieldList.push_back(locFieldList[i]);
    m_fieldNames.push_back(str);
    //   std::clog <<m_visData.fieldNames[i].c_str();
    free(str);
    str = 0;
  }

  outfile.close();
  makeHeader(m_nRows, m_pointsBinaryName, m_fieldNames, m_cellSize, m_cellComp, m_volumeOrTable);

  return 0;
}

//---------------------------------------------------------------------
int VOSource::readHeader()
//---------------------------------------------------------------------
{

  return 0;
}
//----------------------------------------------------------------------------
string VOSource::getUnit(voFieldParam f)
//----------------------------------------------------------------------------
{
  char *str = NULL;
  int status = 0;
  string unita = "";
  if (f.getUnit(str, &status) == 0 && str != NULL)
  {
    unita = str;
    free(str);
    str = NULL;
  }

  return unita;
}
//----------------------------------------------------------------------------
string VOSource::getDataType(voFieldParam f)
//----------------------------------------------------------------------------
{
  char *str = NULL;
  int status = 0;
  string dataType = "";
  if (f.getDatatype(str, &status) == 0 && str != NULL)
  {
    dataType = str;
    free(str);
    str = NULL;
  }

  return dataType;
}
