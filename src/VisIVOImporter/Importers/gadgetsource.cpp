/***************************************************************************
 *   Copyright (C) 2008 by Gabriella Caniglia,Roberto Munzone *
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
#include "gadgetsource.h"

#include "visivoutils.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>

//---------------------------------------------------------------------
int GadgetSource::readHeader()
//---------------------------------------------------------------------

{
  char dummy[4];
  //   char checkType[4];
  int type = 0;
  unsigned int Npart = 0;
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

  std::ifstream inFile;
  inFile.open(m_pointsFileName.c_str(), std::ios::binary);

  if (!inFile)
  {
    std::cerr << "Error while opening block" << std::endl;
    return -1;
  }

  inFile.read((char *)(dummy), 4 * sizeof(char));   //!*** IMPORTANT NOT REMOVE ***//
  inFile.read((char *)(tmpType), 4 * sizeof(char)); //!*** IMPORTANT NOT REMOVE ***//

  tagType = tmpType;

  checkType.push_back(tagType);

  if (iCompare(checkType[0].c_str(), "HEAD") == 0) //! read header Type2 else Type1
  {
    inFile.seekg(4, std::ios::beg);
    inFile.read((char *)(&m_pHeaderType2), 296); //!*** COPY DATA in STRUCT m_pHeaderType2 ***//
    m_snapformat = 2;

    if (needSwap)
      swapHeaderType2();

    for (type = 0; type < 6; type++)
    {
      Npart = Npart + m_pHeaderType2.npart[type];
      // 	     std::clog<<"Npart="<<Npart<<std::endl;
    }

    if (m_pHeaderType2.sizeFirstBlock[0] != (3 * Npart * sizeof(float)))
    {
      std::cerr << "The size File is not than expected" << std::endl;
      return -1;
    }
  }
  else // Type1
  {
    inFile.seekg(0, std::ios::beg);
    inFile.read((char *)(&m_pHeaderType1), 268); //!*** COPY DATA in STRUCT m_pHeaderType2 ***//
    m_snapformat = 1;

    if (needSwap)
      swapHeaderType1();

    for (type = 0; type < 6; type++)
    {
      Npart = Npart + m_pHeaderType1.npart[type];
      // 	     std::clog<<"Npart="<<Npart<<std::endl;
    }

    if (m_pHeaderType1.sizeFirstBlock[0] != (3 * Npart * sizeof(float)))
    {
      std::cerr << "The size File is not than expected" << std::endl;
      return -1;
    }
  }

  inFile.seekg(280, std::ios::beg); //!** BEGINNING BLOCK POS **//

  numBlock = 0;

  while (!inFile.eof()) //! it is (probably ??) a check control!
  {
    inFile.seekg(8, std::ios::cur);
    inFile.read((char *)(m_sizeBlock), sizeof(int));
    if (needSwap)
      m_sizeBlock[0] = intSwap((char *)(&m_sizeBlock[0]));
    std::clog << "m_sizeBlock:" << m_sizeBlock[0] << std::endl;

    inFile.seekg(m_sizeBlock[0], std::ios::cur);
    // 	   inFile.seekg (4, std::ios::cur);
    inFile.read((char *)(m_sizeBlock), sizeof(int));
    if (needSwap)
      m_sizeBlock[0] = intSwap((char *)(&m_sizeBlock[0]));
    std::clog << "m_sizeBlock_END:" << m_sizeBlock[0] << std::endl;

    if (!inFile.eof())
      numBlock++;
  }

  inFile.close();

  return 0;
}

//---------------------------------------------------------------------
int GadgetSource::readData()
//---------------------------------------------------------------------
{ /* START */
  char dummy[4];
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

  int j = 0;
  int k = 0;
  int type = 0;
  int block = 0;
  //   int m_sizeBlock[1]; int numBlock=-1;

  std::ios::off_type pWrite = 0;
  int pToStart = 0;
  std::ios::off_type pWriteX = 0, pWriteY = 0, pWriteZ = 0;

  unsigned int i = 0;
  unsigned long int chunk = 0; //!** Number of Information read & write for cycle **//
  unsigned long int n = 0;
  unsigned long int Resto = 0;

  char tagTmp[5] = "";

  unsigned int param = 1;
  unsigned int esp = 32;
  unsigned long long int maxULI;
  unsigned long long int minPart;

  maxULI = ldexp((float)param, esp);
  minPart = maxULI;

  //!   struct header pHeader;

  std::string tag;

  //   const char point = '.';

  int idx = m_pointsBinaryName.rfind('.');

  std::string pathFileIn = m_pointsBinaryName.erase(idx, idx + 4);
  std::string pathFileOut = pathFileIn;

  std::string bin = ".bin";
  std::string X = "_X";
  std::string Y = "_Y";
  std::string Z = "_Z";

  /*=======================================================================*/

  unsigned int threeDim = 0;
  unsigned int oneDim = 0;
  unsigned int otherBlock = 0;

  /*=======================================================================*/

  /*=======================================================================*/
  /*!================== Identification TYPE & NAME BLOCK ===================*/

  std::vector<std::string> tagTypeForNameFile; //! species block nameset
  tagTypeForNameFile.push_back("GAS");
  tagTypeForNameFile.push_back("HALO");
  tagTypeForNameFile.push_back("DISK");
  tagTypeForNameFile.push_back("BULGE");
  tagTypeForNameFile.push_back("STARS");
  tagTypeForNameFile.push_back("BNDRY");

  std::vector<std::string> blockNamesToCompare; //! block fields names
  blockNamesToCompare.push_back("POS");
  blockNamesToCompare.push_back("VEL");
  blockNamesToCompare.push_back("ID");
  blockNamesToCompare.push_back("MASS");
  blockNamesToCompare.push_back("U");
  blockNamesToCompare.push_back("RHO");
  blockNamesToCompare.push_back("HSML");
  blockNamesToCompare.push_back("POT");
  blockNamesToCompare.push_back("ACCE");
  blockNamesToCompare.push_back("ENDT");
  blockNamesToCompare.push_back("TSTP");

  std::vector<std::vector<std::string>> namesFields;

  std::vector<std::string> tmpNamesFields;

  std::vector<std::string> blockNames;

  /*=======================================================================*/

  std::vector<std::string> namesFieldsType1_GAS;
  namesFieldsType1_GAS.push_back("POS_X");
  namesFieldsType1_GAS.push_back("POS_Y");
  namesFieldsType1_GAS.push_back("POS_Z");
  namesFieldsType1_GAS.push_back("VEL_X");
  namesFieldsType1_GAS.push_back("VEL_Y");
  namesFieldsType1_GAS.push_back("VEL_Z");
  namesFieldsType1_GAS.push_back("ID");
  namesFieldsType1_GAS.push_back("MASS");
  namesFieldsType1_GAS.push_back("U");
  namesFieldsType1_GAS.push_back("RHO");
  namesFieldsType1_GAS.push_back("HSML");

  std::vector<std::string> namesFieldsType1_Other;
  namesFieldsType1_Other.push_back("POS_X");
  namesFieldsType1_Other.push_back("POS_Y");
  namesFieldsType1_Other.push_back("POS_Z");
  namesFieldsType1_Other.push_back("VEL_X");
  namesFieldsType1_Other.push_back("VEL_Y");
  namesFieldsType1_Other.push_back("VEL_Z");
  namesFieldsType1_Other.push_back("ID");
  namesFieldsType1_Other.push_back("MASS");

  std::string pathHeader = "";
  int KK = 0;

  /*!=======================================================================*//*!========================== Open and Check FILE ========================*/

  std::ifstream inFile;
  inFile.open(m_pointsFileName.c_str(), std::ios::binary);

  if (!inFile)
  {
    std::cerr << "Error while opening block" << std::endl;
    return -1;
  }

  /*=======================================================================*/

  switch (m_snapformat)
  {
  case 1:
    /*!==================   TYPE 1  FILE     =================================*/
    /*!=========== Check Dimension BLOCK & SETTING variable CHUNK ============*//*!===========              for buffer of reading.            ============*/
    /*!=======================================================================*/

    for (type = 0; type < 6; type++)
    {
      if (m_pHeaderType1.npart[type] != 0 && m_pHeaderType1.npart[type] <= minPart)
        minPart = m_pHeaderType1.npart[type]; // minimum number of part for all species
    }

    if (minPart <= 2500000)
      chunk = minPart;
    else
      chunk = 2500000;

    /*=======================================================================*/
    /*!=========== Create OUTPUT file for each species =======================*/

    for (type = 0; type < 6; type++) //! for all species
    {
      if (m_pHeaderType1.npart[type] != 0) //! if there are particles
      {
        std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
        std::ofstream outFileBin;
        outFileBin.open(nameFileBinOut.c_str(), std::ios::binary /*| ios::app*/);
        outFileBin.close();
      }
    }

    /*=======================================================================*/
    /*=======================================================================*/
    /*!======================== Start Processing FILE ========================*/

    inFile.seekg(264, std::ios::beg); //** BEGINNING BLOCK POS TYPE 1**//

    for (block = 0; block < 7; block++)
    { /*2*/

      inFile.read((char *)(m_sizeBlock), sizeof(int));

      /*=========================================================================================================*/

      if (block == 0 || block == 1) // Tridimensional elements: pos, vel!
      {                             /*3*/
        for (type = 0; type < 6; type++)
        { /*4*/
          if (m_pHeaderType1.npart[type] != 0)
          { /*5*/
            pToStart = ((3 * threeDim * m_pHeaderType1.npart[type]) + (oneDim * m_pHeaderType1.npart[type]) + (otherBlock * m_pHeaderType1.npart[type]));

            float *bufferBlock = NULL;
            bufferBlock = new float[3 * chunk];

            float *buffer_X = NULL;
            buffer_X = new float[chunk];
            float *buffer_Y = NULL;
            buffer_Y = new float[chunk];
            float *buffer_Z = NULL;
            buffer_Z = new float[chunk];

            std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
            std::ofstream outFileBin;
            outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

            n = m_pHeaderType1.npart[type] / chunk;
            Resto = m_pHeaderType1.npart[type] - (chunk * n);

            for (k = 0; k < n; k++) // cycling n times (3*chunk of each) up to reach number of particles excluding Resto
            {                       /*6*/
              inFile.read((char *)(bufferBlock), 3 * chunk * sizeof(float));

              // 								if( needSwap)
              // 								{
              // 									for (j=0; j<(3*chunk); j++)
              // 									{
              // 										bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
              // 									}
              // 								}

              /*=================================================================*/
              /*!============ Buffer Block and Write out FILE [n-Cycle] ==========*/

              /*								for (i=1; i<=(chunk); i++)
                              {
                              buffer_X[i-1] = bufferBlock[3*i-3];
                            }

                              for (i=1; i<=(chunk); i++)
                              {
                              buffer_Y[i-1] = bufferBlock[3*i-2];
                            }

                              for (i=1; i<=(chunk); i++)
                              {
                              buffer_Z[i-1] = bufferBlock[3*i-1];
                            }*/
              if (needSwap)
                for (i = 0; i < chunk; i++)
                {
                  buffer_X[i] = floatSwap((char *)(&bufferBlock[3 * i]));
                  buffer_Y[i] = floatSwap((char *)(&bufferBlock[3 * i + 1]));
                  buffer_Z[i] = floatSwap((char *)(&bufferBlock[3 * i + 2]));
                }
              else
                for (i = 0; i < chunk; i++)
                {
                  buffer_X[i] = bufferBlock[3 * i];
                  buffer_Y[i] = bufferBlock[3 * i + 1];
                  buffer_Z[i] = bufferBlock[3 * i + 2];
                }

              pWriteX = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)));
              outFileBin.seekp(pWriteX);
              outFileBin.write((char *)(buffer_X), chunk * sizeof(float));

              pWriteY = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)) + (m_pHeaderType1.npart[type] * sizeof(float)));
              outFileBin.seekp(pWriteY);
              outFileBin.write((char *)(buffer_Y), chunk * sizeof(float));

              pWriteZ = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)) + (2 * m_pHeaderType1.npart[type] * sizeof(float)));
              outFileBin.seekp(pWriteZ);
              outFileBin.write((char *)(buffer_Z), chunk * sizeof(float));

              /*=======================================================================*/

            } /*6*/

            /*=================================================================*/
            /*!============ Buffer Block and Write out FILE [Resto] ============*/

            inFile.read((char *)(bufferBlock), 3 * Resto * sizeof(float));

            // 							if(needSwap)
            // 							{
            // 								for (j=0; j<(3*Resto); j++)
            // 								{
            // 									bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
            // 								}
            // 							}
            //
            // 							for (i=1; i<=(3*Resto); i++)
            // 							{
            // 								buffer_X[i-1] = bufferBlock[3*i-3];
            // 							}
            //
            // 							for (i=1; i<=(Resto); i++)
            // 							{
            // 								buffer_Y[i-1] = bufferBlock[3*i-2];
            // 							}
            //
            // 							for (i=1; i<=(Resto); i++)
            // 							{
            // 								buffer_Z[i-1] = bufferBlock[3*i-1];
            // 							}
            if (needSwap)
              for (i = 0; i < Resto; i++)
              {
                buffer_X[i] = floatSwap((char *)(&bufferBlock[3 * i]));
                buffer_Y[i] = floatSwap((char *)(&bufferBlock[3 * i + 1]));
                buffer_Z[i] = floatSwap((char *)(&bufferBlock[3 * i + 2]));
              }
            else
              for (i = 0; i < Resto; i++)
              {
                buffer_X[i] = bufferBlock[3 * i];
                buffer_Y[i] = bufferBlock[3 * i + 1];
                buffer_Z[i] = bufferBlock[3 * i + 2];
              }

            pWriteX = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)));
            outFileBin.seekp(pWriteX);
            outFileBin.write((char *)(buffer_X), Resto * sizeof(float));

            pWriteY = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)) + (m_pHeaderType1.npart[type] * sizeof(float)));
            outFileBin.seekp(pWriteY);
            outFileBin.write((char *)(buffer_Y), Resto * sizeof(float));

            pWriteZ = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)) + (2 * m_pHeaderType1.npart[type] * sizeof(float)));
            outFileBin.seekp(pWriteZ);
            outFileBin.write((char *)(buffer_Z), Resto * sizeof(float));

            /*=================================================================*/

            delete[] buffer_X;
            delete[] buffer_Y;
            delete[] buffer_Z;

            delete[] bufferBlock;
            outFileBin.close();

          } /*5*/

          else //! of if(nPart!=0)
          {
            n = 0;
            Resto = 0;
          }

        } /*4*/ //! close for on type

        pWriteX = 0;
        pWriteY = 0;
        pWriteZ = 0;
        threeDim++;
      } /*3*/

      else //! block greather than 2
      {    /*a*/
        if (block == 3)
        { /*b*/
          // 				oneDim++;
          for (type = 0; type < 6; type++)
          { /*c*/
            if (m_pHeaderType1.npart[type] != 0)
            { /*d*/
              pToStart = ((3 * threeDim * m_pHeaderType1.npart[type]) + (oneDim * m_pHeaderType1.npart[type]) + (otherBlock * m_pHeaderType1.npart[type]));

              n = m_pHeaderType1.npart[type] / chunk;
              Resto = m_pHeaderType1.npart[type] - (chunk * n);

              float *bufferBlock = NULL;
              bufferBlock = new float[chunk];

              std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
              std::ofstream outFileBin;
              outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

              for (k = 0; k < n; k++)
              { /*e*/
                inFile.read((char *)(bufferBlock), chunk * sizeof(float));

                if (needSwap)
                  for (j = 0; j < (chunk); j++)
                    bufferBlock[j] = floatSwap((char *)(&bufferBlock[j]));

                pWrite = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)));
                outFileBin.seekp(pWrite);
                outFileBin.write((char *)(bufferBlock), chunk * sizeof(float));
              } /*e*/

              inFile.read((char *)(bufferBlock), Resto * sizeof(float));

              if (needSwap)
                for (j = 0; j < Resto; j++)
                  bufferBlock[j] = floatSwap((char *)(&bufferBlock[j]));

              pWrite = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)));
              outFileBin.seekp(pWrite);
              outFileBin.write((char *)(bufferBlock), Resto * sizeof(float));

              delete[] bufferBlock;
              outFileBin.close();
            } /*d*/

            else
            {
              n = 0;
              Resto = 0;
            }

          } /*c*/
          oneDim++;
        } /*b*/

        else
        { /*a*/
          if (block == 2)
          { /*b*/
            // 				oneDim++;
            for (type = 0; type < 6; type++)
            { /*c*/
              if (m_pHeaderType1.npart[type] != 0)
              { /*d*/
                pToStart = ((3 * threeDim * m_pHeaderType1.npart[type]) + (oneDim * m_pHeaderType1.npart[type]) + (otherBlock * m_pHeaderType1.npart[type]));

                n = m_pHeaderType1.npart[type] / chunk;
                Resto = m_pHeaderType1.npart[type] - (chunk * n);

                int *bufferBlock = NULL;
                bufferBlock = new int[chunk];

                std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
                std::ofstream outFileBin;
                outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

                for (k = 0; k < n; k++)
                { /*e*/
                  inFile.read((char *)(bufferBlock), chunk * sizeof(int));

                  if (needSwap)
                    for (j = 0; j < chunk; j++)
                      bufferBlock[j] = intSwap((char *)(&bufferBlock[j]));

                  pWrite = ((pToStart * sizeof(int)) + (k * chunk * sizeof(int)));
                  outFileBin.seekp(pWrite);
                  outFileBin.write((char *)(bufferBlock), chunk * sizeof(int));
                } /*e*/

                inFile.read((char *)(bufferBlock), Resto * sizeof(int));

                if (needSwap)
                  for (j = 0; j < Resto; j++)
                    bufferBlock[j] = intSwap((char *)(&bufferBlock[j]));

                pWrite = ((pToStart * sizeof(int)) + (n * chunk * sizeof(int)));
                outFileBin.seekp(pWrite);
                outFileBin.write((char *)(bufferBlock), Resto * sizeof(int));

                delete[] bufferBlock;
                outFileBin.close();
              } /*d*/

              else
              {
                n = 0;
                Resto = 0;
              }

            } /*c*/
            oneDim++;
          } /*b*/

          else
          { /*I*/
            // 				otherBlock++;
            for (type = 0; type < 1; type++)
            { /*II*/
              if (m_pHeaderType1.npart[type] != 0)
              { /*III*/
                pToStart = ((3 * threeDim * m_pHeaderType1.npart[type]) + (oneDim * m_pHeaderType1.npart[type]) + (otherBlock * m_pHeaderType1.npart[type]));

                n = m_pHeaderType1.npart[type] / chunk;
                Resto = m_pHeaderType1.npart[type] - (chunk * n);

                float *bufferBlock = NULL;
                bufferBlock = new float[chunk];

                std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
                std::ofstream outFileBin;
                outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

                for (k = 0; k < n; k++)
                { /*IV*/
                  inFile.read((char *)(bufferBlock), chunk * sizeof(float));

                  if (needSwap)
                  {
                    for (j = 0; j < chunk; j++)
                    {
                      bufferBlock[j] = floatSwap((char *)(&bufferBlock[j]));
                    }
                  }

                  pWrite = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)));
                  outFileBin.seekp(pWrite);
                  outFileBin.write((char *)(bufferBlock), chunk * sizeof(float));
                } /*IV*/

                inFile.read((char *)(bufferBlock), Resto * sizeof(float));

                if (needSwap)
                  for (j = 0; j < Resto; j++)
                    bufferBlock[j] = floatSwap((char *)(&bufferBlock[j]));

                pWrite = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)));
                outFileBin.seekp(pWrite);
                outFileBin.write((char *)(bufferBlock), Resto * sizeof(float));

                delete[] bufferBlock;
                outFileBin.close();
              } /*III*/

              else
              {
                n = 0;
                Resto = 0;
              }

            } /*II*/
            otherBlock++;
          } /*I*/

        } /*a*/
      }

      inFile.seekg(4, std::ios::cur); //!** Beginning next block **//

    } /*2*/

    break; /*! END Snapformat 1 */

  case 2:

    /*=======================================================================*/
    /*!=========== Check Dimension BLOCK & SETTING variable CHUNK ============*/
    /*!===========              for buffer of reading.            ============*/
    /*=======================================================================*/

    for (type = 0; type < 6; type++)
    {
      if (m_pHeaderType2.npart[type] != 0 && m_pHeaderType2.npart[type] <= minPart)
      {
        minPart = m_pHeaderType2.npart[type];
      }
    }

    if (minPart <= 2500000)
    {
      chunk = minPart;
    }
    else
    {
      chunk = 2500000;
    }

    /*!=======================================================================*/
    /*=======================================================================*/
    /*!==================== Create final file for TYPE =======================*/

    for (type = 0; type < 6; type++)
    {
      if (m_pHeaderType2.npart[type] != 0)
      {
        std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
        std::ofstream outFileBin;
        outFileBin.open(nameFileBinOut.c_str(), std::ios::binary /*| ios::app*/);
        outFileBin.close();
      }
    }

    /*=======================================================================*/

    /*=======================================================================*/
    /*!======================== Start Processing FILE ========================*/

    inFile.seekg(284, std::ios::beg); //** BEGINNING BLOCK POS **//

    //   while (!inFile.eof())
    //   { /* ON WHILE */

    for (block = 0; block < (numBlock); block++)
    // 	for (block=0; block<2; block++)
    { /*2*/

      inFile.read((char *)(tagTmp), 4 * sizeof(char));
      // 		std::clog<<"tagTmp="<<tagTmp<<std::endl;

      tag = strtok(tagTmp, " ");
      // 		std::clog<<"tag="<<tag<<std::endl;

      blockNames.push_back(tag);
      // 		std::clog<<"blockNames[block]="<<blockNames[block]<<std::endl;

      inFile.seekg(8, std::ios::cur);
      inFile.read((char *)(m_sizeBlock), sizeof(int));

      /*=========================================================================================================*/

      if (iCompare(blockNames[block].c_str(), blockNamesToCompare[0].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[1].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[8].c_str()) == 0)
      { /*3*/
        for (type = 0; type < 6; type++)
        { /*4*/
          if (m_pHeaderType2.npart[type] != 0)
          { /*5*/
            pToStart = ((3 * threeDim * m_pHeaderType2.npart[type]) + (oneDim * m_pHeaderType2.npart[type]) + (otherBlock * m_pHeaderType2.npart[type]));

            // 				std::clog<<"Extraction Block "<<tagTypeForNameFile[type]<< "_" <<blockNames[block]<<std::endl;
            n = m_pHeaderType2.npart[type] / chunk;
            Resto = m_pHeaderType2.npart[type] - (chunk * n);

            float *bufferBlock = NULL;
            bufferBlock = new float[3 * chunk];

            float *buffer_X = NULL;
            buffer_X = new float[chunk];
            float *buffer_Y = NULL;
            buffer_Y = new float[chunk];
            float *buffer_Z = NULL;
            buffer_Z = new float[chunk];

            std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
            std::ofstream outFileBin;
            outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

            for (k = 0; k < n; k++)
            { /*6*/
              inFile.read((char *)(bufferBlock), 3 * chunk * sizeof(float));

              if (needSwap)
                for (i = 0; i < chunk; i++)
                {
                  buffer_X[i] = floatSwap((char *)(&bufferBlock[3 * i]));
                  buffer_Y[i] = floatSwap((char *)(&bufferBlock[3 * i + 1]));
                  buffer_Z[i] = floatSwap((char *)(&bufferBlock[3 * i + 2]));
                }
              else
                for (i = 0; i < chunk; i++)
                {
                  buffer_X[i] = bufferBlock[3 * i];
                  buffer_Y[i] = bufferBlock[3 * i + 1];
                  buffer_Z[i] = bufferBlock[3 * i + 2];
                }

              // 								if( needSwap)
              // 									for (j=0; j<(3*chunk); j++)
              // 										bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
              //
              // 								/*=================================================================*/
              /*!============ Buffer Block and Write out FILE [n-Cycle] ==========*/
              //
              // 								for (i=1; i<=(chunk); i++)
              // 								{
              // 									buffer_X[i-1] = bufferBlock[3*i-3];
              // // 							std::clog<<"buffer_X="<<buffer_X[i-1]<<" and bufferBlock=" <<bufferBlock[3*i-3]<<std::endl;
              // 								}
              //
              // 								for (i=1; i<=(chunk); i++)
              // 								{
              // 									buffer_Y[i-1] = bufferBlock[3*i-2];
              // 								}
              //
              // 								for (i=1; i<=(chunk); i++)
              // 								{
              // 									buffer_Z[i-1] = bufferBlock[3*i-1];
              // 								}

              //             for (i=1; i<=10; i++)
              //             {
              // 						std::clog<<"buffer_X="<<buffer_X[i-1]<<" and bufferBlock=" <<bufferBlock[3*i-3]<<std::endl;
              // 						std::clog<<"buffer_Y="<<buffer_Y[i-1]<<" and bufferBlock=" <<bufferBlock[3*i-2]<<std::endl;
              // 						std::clog<<"buffer_Z="<<buffer_Z[i-1]<<" and bufferBlock=" <<bufferBlock[3*i-1]<<std::endl;
              //             }

              pWriteX = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)));
              outFileBin.seekp(pWriteX);
              outFileBin.write((char *)(buffer_X), chunk * sizeof(float));

              pWriteY = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)) + (m_pHeaderType2.npart[type] * sizeof(float)));
              outFileBin.seekp(pWriteY);
              outFileBin.write((char *)(buffer_Y), chunk * sizeof(float));

              pWriteZ = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)) + (2 * m_pHeaderType2.npart[type] * sizeof(float)));
              outFileBin.seekp(pWriteZ);
              outFileBin.write((char *)(buffer_Z), chunk * sizeof(float));

              /*=======================================================================*/

            } /*6*/

            /*=================================================================*/
            /*============ Buffer Block and Write out FILE [Resto] ============*/

            inFile.read((char *)(bufferBlock), 3 * Resto * sizeof(float));
            if (needSwap)
              for (i = 0; i < Resto; i++)
              {
                buffer_X[i] = floatSwap((char *)(&bufferBlock[3 * i]));
                buffer_Y[i] = floatSwap((char *)(&bufferBlock[3 * i + 1]));
                buffer_Z[i] = floatSwap((char *)(&bufferBlock[3 * i + 2]));
              }
            else
              for (i = 0; i < Resto; i++)
              {
                buffer_X[i] = bufferBlock[3 * i];
                buffer_Y[i] = bufferBlock[3 * i + 1];
                buffer_Z[i] = bufferBlock[3 * i + 2];
              }

            /*							if( needSwap)
                          {
                          for (j=0; j<(3*Resto); j++)
                          {
                          bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
                        }
                        }

                          for (i=1; i<=(Resto); i++)
                          {
                          buffer_X[i-1] = bufferBlock[3*i-3];
                        }

                          for (i=1; i<=(Resto); i++)
                          {
                          buffer_Y[i-1] = bufferBlock[3*i-2];
                        }

                          for (i=1; i<=(Resto); i++)
                          {
                          buffer_Z[i-1] = bufferBlock[3*i-1];
                        }
            */
            pWriteX = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)));
            outFileBin.seekp(pWriteX);
            outFileBin.write((char *)(buffer_X), Resto * sizeof(float));

            pWriteY = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)) + (m_pHeaderType2.npart[type] * sizeof(float)));
            outFileBin.seekp(pWriteY);
            outFileBin.write((char *)(buffer_Y), Resto * sizeof(float));

            pWriteZ = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)) + (2 * m_pHeaderType2.npart[type] * sizeof(float)));
            outFileBin.seekp(pWriteZ);
            outFileBin.write((char *)(buffer_Z), Resto * sizeof(float));

            /*=================================================================*/

            delete[] buffer_X;
            delete[] buffer_Y;
            delete[] buffer_Z;

            delete[] bufferBlock;
            outFileBin.close();

          } /*5*/

          else
          {
            n = 0;
            Resto = 0;
          }

        } /*4*/
        pWriteX = 0;
        pWriteY = 0;
        pWriteZ = 0;
        threeDim++;
      } /*3*/

      else if (iCompare(blockNames[block].c_str(), blockNamesToCompare[3].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[7].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[10].c_str()) == 0)
      { /*b*/
        // 				oneDim++;
        for (type = 0; type < 6; type++)
        { /*c*/
          if (m_pHeaderType2.npart[type] != 0)
          { /*d*/
            pToStart = ((3 * threeDim * m_pHeaderType2.npart[type]) + (oneDim * m_pHeaderType2.npart[type]) + (otherBlock * m_pHeaderType2.npart[type]));

            // 						std::clog<<"Extraction Block "<<tagTypeForNameFile[type]<<"_" <<blockNames[block]<<std::endl;

            n = m_pHeaderType2.npart[type] / chunk;
            Resto = m_pHeaderType2.npart[type] - (chunk * n);

            float *bufferBlock = NULL;
            bufferBlock = new float[chunk];

            std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
            std::ofstream outFileBin;
            outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

            for (k = 0; k < n; k++)
            { /*e*/
              inFile.read((char *)(bufferBlock), chunk * sizeof(float));

              if (needSwap)
                for (j = 0; j < chunk; j++)
                  bufferBlock[j] = floatSwap((char *)(&bufferBlock[j]));

              pWrite = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)));
              outFileBin.seekp(pWrite);
              outFileBin.write((char *)(bufferBlock), chunk * sizeof(float));
            } /*e*/

            inFile.read((char *)(bufferBlock), Resto * sizeof(float));

            if (needSwap)
              for (j = 0; j < Resto; j++)
                bufferBlock[j] = floatSwap((char *)(&bufferBlock[j]));

            pWrite = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)));
            outFileBin.seekp(pWrite);
            outFileBin.write((char *)(bufferBlock), Resto * sizeof(float));

            delete[] bufferBlock;
            outFileBin.close();
          } /*d*/

          else
          {
            n = 0;
            Resto = 0;
          }

        } /*c*/
        oneDim++;
      } /*b*/

      else if (iCompare(blockNames[block].c_str(), blockNamesToCompare[2].c_str()) == 0)
      { /*bb*/
        // 		oneDim++;
        for (type = 0; type < 6; type++)
        { /*cc*/
          if (m_pHeaderType2.npart[type] != 0)
          { /*dd*/
            pToStart = ((3 * threeDim * m_pHeaderType2.npart[type]) + (oneDim * m_pHeaderType2.npart[type]) + (otherBlock * m_pHeaderType2.npart[type]));

            // 						std::clog<<"Extraction Block "<<tagTypeForNameFile[type]<<"_" <<blockNames[block]<<std::endl;

            n = m_pHeaderType2.npart[type] / chunk;
            Resto = m_pHeaderType2.npart[type] - (chunk * n);

            int *bufferBlock = NULL;
            bufferBlock = new int[chunk];

            std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
            std::ofstream outFileBin;
            outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

            for (k = 0; k < n; k++)
            { /*ee*/
              inFile.read((char *)(bufferBlock), chunk * sizeof(int));

              if (needSwap)
                for (j = 0; j < chunk; j++)
                  bufferBlock[j] = intSwap((char *)(&bufferBlock[j]));

              pWrite = ((pToStart * sizeof(int)) + (k * chunk * sizeof(int)));
              outFileBin.seekp(pWrite);
              outFileBin.write((char *)(bufferBlock), chunk * sizeof(int));
            } /*ee*/

            inFile.read((char *)(bufferBlock), Resto * sizeof(int));

            if (needSwap)
              for (j = 0; j < Resto; j++)
                bufferBlock[j] = intSwap((char *)(&bufferBlock[j]));

            pWrite = ((pToStart * sizeof(int)) + (n * chunk * sizeof(int)));
            outFileBin.seekp(pWrite);
            outFileBin.write((char *)(bufferBlock), Resto * sizeof(int));

            delete[] bufferBlock;
            outFileBin.close();
          } /*dd*/

          else
          {
            n = 0;
            Resto = 0;
          }

        } /*cc*/
        oneDim++;
      } /*bb*/

      else if (iCompare(blockNames[block].c_str(), blockNamesToCompare[4].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[5].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[6].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[9].c_str()) == 0)
      { /*bbb*/

        // 	otherBlock++;
        // 			inFile.seekg (m_sizeBlock[0], std::ios::cur);
        for (type = 0; type < 1; type++)
        { /*II*/
          if (m_pHeaderType2.npart[type] != 0)
          { /*III*/
            pToStart = ((3 * threeDim * m_pHeaderType2.npart[type]) + (oneDim * m_pHeaderType2.npart[type]) + (otherBlock * m_pHeaderType2.npart[type]));

            // 				std::clog<<"Extraction Block "<<tagTypeForNameFile[type]<< "_" <<blockNames[block]<<std::endl;

            n = m_pHeaderType2.npart[type] / chunk;
            Resto = m_pHeaderType2.npart[type] - (chunk * n);

            float *bufferBlock = NULL;
            bufferBlock = new float[chunk];

            std::string nameFileBinOut = pathFileOut + tagTypeForNameFile[type].c_str() + bin;
            std::ofstream outFileBin;
            outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

            for (k = 0; k < n; k++)
            { /*IV*/
              inFile.read((char *)(bufferBlock), chunk * sizeof(float));

              if (needSwap)
                for (j = 0; j < chunk; j++)
                  bufferBlock[j] = floatSwap((char *)(&bufferBlock[j]));
              pWrite = ((pToStart * sizeof(float)) + (k * chunk * sizeof(float)));
              outFileBin.seekp(pWrite);
              outFileBin.write((char *)(bufferBlock), chunk * sizeof(float));
            } /*IV*/

            inFile.read((char *)(bufferBlock), Resto * sizeof(float));

            if (needSwap)
              for (j = 0; j < Resto; j++)
                bufferBlock[j] = floatSwap((char *)(&bufferBlock[j]));

            pWrite = ((pToStart * sizeof(float)) + (n * chunk * sizeof(float)));
            outFileBin.seekp(pWrite);
            outFileBin.write((char *)(bufferBlock), Resto * sizeof(float));

            delete[] bufferBlock;
            outFileBin.close();
          } /*III*/

          else
          {
            n = 0;
            Resto = 0;
          }

        } /*II*/
        otherBlock++;
      } /*bbb*/

      else
      { /*I*/
        //     otherBlock++;
        inFile.seekg(m_sizeBlock[0], std::ios::cur);
      } /*I*/

      inFile.seekg(8, std::ios::cur); //** Beginning next block **//

    } /* OFF WHILE */

    /*=================================================================*/

    break; /* END Snapformat 2 */
  }
  /*=================================================================*/

  inFile.close();

  /*=================================================================*/
  /*!=========== Read information to write FILE.bin.head =============*/

  inFile.open(m_pointsFileName.c_str(), std::ios::binary);

  switch (m_snapformat)
  {
  case 2:

    int ii, jj, kk, yy;

    //   inFile.seekg (288, std::ios::beg);
    for (type = 0; type < 6; type++)
    { /*I*/
      if (m_pHeaderType2.npart[type] == 0)
      {
        tmpNamesFields.clear();
        tmpNamesFields.push_back(" ");
        namesFields.push_back(tmpNamesFields);
        tmpNamesFields.clear();
      }

      else if (m_pHeaderType2.npart[type] != 0)
      {
        //       m_sizeBlock[0]=0;
        inFile.seekg(280, std::ios::beg); //** START BLOCK POS **//
                                          //          std::clog<<"m_sizeBlock="<<m_sizeBlock[0]<<std::endl;

        ii = 0;
        jj = 0;
        kk = 0;

        for (block = 0; block < (numBlock); block++)
        //    for (block=0; block<2; block++)
        { /*II*/
          //         std::clog<<"blockNames"<<blockNames[block]<<std::endl;
          inFile.seekg(8, std::ios::cur);
          inFile.read((char *)(m_sizeBlock), sizeof(int));

          if (needSwap)
            m_sizeBlock[0] = intSwap((char *)(&m_sizeBlock[0]));
          //            std::clog<<"m_sizeBlock="<<m_sizeBlock[0]<<std::endl;

          inFile.seekg(m_sizeBlock[0], std::ios::cur);
          inFile.seekg(4, std::ios::cur);

          if (m_pHeaderType2.npart[type] != 0 /*&& type == 0*/ && (iCompare(blockNames[block].c_str(), blockNamesToCompare[0].c_str()) == 0 || strcmp(blockNames[block].c_str(), blockNamesToCompare[1].c_str()) == 0 || strcmp(blockNames[block].c_str(), blockNamesToCompare[8].c_str()) == 0))
          { /*III*/
            ii++;
            tmpNamesFields.push_back(blockNames[block] + X);
            tmpNamesFields.push_back(blockNames[block] + Y);
            tmpNamesFields.push_back(blockNames[block] + Z);
            // 					j++;
          } /*III*/

          else if (m_pHeaderType2.npart[type] != 0 && type == 0 && (iCompare(blockNames[block].c_str(), blockNamesToCompare[2].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[3].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[4].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[5].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[6].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[7].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[9].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[10].c_str()) == 0))
          { /*IV*/
            jj++;
            tmpNamesFields.push_back(blockNames[block]);
          } /*IV*/

          //         else
          //           if (m_pHeaderType2.npart[type] != 0 && type == 0)
          //         {/*V*/
          //           kk++;
          //           tmpNamesFields.push_back(blockNames[block]);
          //         }/*V*/

          //////////////////////////////
          else if (m_pHeaderType2.npart[type] != 0 && type != 0 && (iCompare(blockNames[block].c_str(), blockNamesToCompare[2].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[3].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[7].c_str()) == 0 || iCompare(blockNames[block].c_str(), blockNamesToCompare[10].c_str()) == 0))
          { /*IV*/
            jj++;
            tmpNamesFields.push_back(blockNames[block]);
          } /*IV*/

          //         else
          //           if (m_pHeaderType2.npart[type] != 0 && type != 0)
          //         {/*V*/
          //           kk=0;
          //         }/*V*/

          else if (m_pHeaderType2.npart[type] == 0 /*&& type != 0*/)
          { /*V*/
            ii = 0;
            jj = 0;
            kk = 0;

          } /*V*/

        } /*II*/

        namesFields.push_back(tmpNamesFields);
        tmpNamesFields.clear();
      }
    } /*I*/

    /*=================================================================*//*!===================== Write FILE.bin.head =======================*/

    for (type = 0; type < 6; type++)
    {

      if (m_pHeaderType2.npart[type] != 0)
      {

        for (KK = 0; KK < namesFields[type].size(); KK++)
        {
          //            std::clog<<"namesFields="<<namesFields[type][KK]<<std::endl;
          m_fieldsNames.push_back(namesFields[type][KK]);
        }
        pathHeader = pathFileOut + tagTypeForNameFile[type] + bin;
        //          std::clog<<"pathHeader="<<pathHeader<<std::endl;

        makeHeader((unsigned long long int)m_pHeaderType2.npart[type], pathHeader, m_fieldsNames, m_cellSize, m_cellComp, m_volumeOrTable);

        m_fieldsNames.clear();
        pathHeader = "";
      }
    }

    /*=================================================================*/

    break; /* END Snapformat 2 */

  case 1:

    /*=================================================================*/
    /*!===================== Write FILE.bin.head =======================*/

    for (type = 0; type < 6; type++)
    {
      if (m_pHeaderType1.npart[type] != 0 && type == 0)
      {
        for (KK = 0; KK < 11; KK++)
        {
          m_fieldsNames.push_back(namesFieldsType1_GAS[KK]);
        }
        pathHeader = pathFileOut + tagTypeForNameFile[type] + bin;
        // 			  std::clog<<"pathHeader="<<pathHeader<<std::endl;

        makeHeader((unsigned long long int)m_pHeaderType1.npart[type], pathHeader, m_fieldsNames, m_cellSize, m_cellComp, m_volumeOrTable);

        m_fieldsNames.clear();
        pathHeader = "";
      }
      else if (m_pHeaderType1.npart[type] != 0 && type != 0)
      {
        for (KK = 0; KK < 8; KK++)
        {
          m_fieldsNames.push_back(namesFieldsType1_Other[KK]);
        }
        pathHeader = pathFileOut + tagTypeForNameFile[type] + bin;
        // 			std::clog<<"pathHeader="<<pathHeader<<std::endl;

        makeHeader((unsigned long long int)m_pHeaderType1.npart[type], pathHeader, m_fieldsNames, m_cellSize, m_cellComp, m_volumeOrTable);

        m_fieldsNames.clear();
        pathHeader = "";
      }
    }

    /*=================================================================*/

    break; /* END Snapformat 1 */
  }
  /*=================================================================*/

  inFile.close();

  /*=================================================================*/

  // 	std::clog<<"Finish Process"<<std::endl;

  return 0;
} /* END */

//--------------------------------------
void GadgetSource::swapHeaderType2()
//--------------------------------------
{
  char first_fortran_legacy[4]; // m_snapformat=2 : you have to "jump" 4+2*4 =12 bytes and you'll find the the Block size
  m_pHeaderType2.boh[0] = intSwap((char *)(&m_pHeaderType2.boh[0]));
  m_pHeaderType2.nBlock[0] = intSwap((char *)(&m_pHeaderType2.nBlock[0]));
  m_pHeaderType2.size[0] = intSwap((char *)(&m_pHeaderType2.size[0]));

  for (int i = 0; i < 6; i++)
    m_pHeaderType2.npart[i] = intSwap((char *)(&m_pHeaderType2.npart[i]));

  for (int i = 0; i < 6; i++)
    m_pHeaderType2.mass[i] = intSwap((char *)(&m_pHeaderType2.mass[i]));

  m_pHeaderType2.time[0] = doubleSwap((char *)(&m_pHeaderType2.time[0]));
  m_pHeaderType2.redshift[0] = doubleSwap((char *)(&m_pHeaderType2.redshift[0]));
  m_pHeaderType2.flag_sfr[0] = intSwap((char *)(&m_pHeaderType2.flag_sfr[0]));
  m_pHeaderType2.flag_feedback[0] = intSwap((char *)(&m_pHeaderType2.flag_feedback[0]));

  for (int i = 0; i < 6; i++)
    m_pHeaderType2.npartTotal[i] = intSwap((char *)(&m_pHeaderType2.npartTotal[i]));

  m_pHeaderType2.foling[0] = intSwap((char *)(&m_pHeaderType2.foling[0]));
  m_pHeaderType2.num_files[0] = intSwap((char *)(&m_pHeaderType2.num_files[0]));
  m_pHeaderType2.BoxSize[0] = doubleSwap((char *)&(m_pHeaderType2.BoxSize[0]));
  m_pHeaderType2.Omega0[0] = doubleSwap((char *)(&m_pHeaderType2.Omega0[0]));
  m_pHeaderType2.OmegaLambda[0] = doubleSwap((char *)(&m_pHeaderType2.OmegaLambda[0]));
  m_pHeaderType2.HubbleParam[0] = doubleSwap((char *)(&m_pHeaderType2.HubbleParam[0]));
  m_pHeaderType2.FlagAge[0] = intSwap((char *)(&m_pHeaderType2.FlagAge[0]));
  m_pHeaderType2.FlagMetals[0] = intSwap((char *)(&m_pHeaderType2.FlagMetals[0]));

  for (int i = 0; i < 6; i++)
    m_pHeaderType2.NallWH[i] = intSwap((char *)(&m_pHeaderType2.NallWH[i]));

  m_pHeaderType2.flag_entr_ics[0] = intSwap((char *)(&m_pHeaderType2.flag_entr_ics[0]));

  char fill[256 - 6 * sizeof(int) - 6 * sizeof(double) - 2 * sizeof(double) - 2 * sizeof(int) - 6 * sizeof(int) - 2 * sizeof(int) -
            4 * sizeof(double) - 9 * sizeof(int)]; /* fills to 256 Bytes */
  m_pHeaderType2.final_boh[0] = intSwap((char *)(&m_pHeaderType2.final_boh[0]));
  m_pHeaderType2.final_nBlock[0] = intSwap((char *)(&m_pHeaderType2.final_nBlock[0]));

  char tagFirstBlock[4];
  m_pHeaderType2.first_boh[0] = intSwap((char *)(&m_pHeaderType2.first_boh[0]));
  m_pHeaderType2.first_nBlock[0] = intSwap((char *)(&m_pHeaderType2.first_nBlock[0]));
  m_pHeaderType2.sizeFirstBlock[0] = intSwap((char *)(&m_pHeaderType2.sizeFirstBlock[0]));
}

//--------------------------------------
void GadgetSource::swapHeaderType1()
//--------------------------------------
{
  m_pHeaderType1.size[0] = intSwap((char *)(&m_pHeaderType1.size[0]));

  for (int i = 0; i < 6; i++)
    m_pHeaderType1.npart[i] = intSwap((char *)(&m_pHeaderType1.npart[i]));

  for (int i = 0; i < 6; i++)
    m_pHeaderType1.mass[i] = intSwap((char *)(&m_pHeaderType1.mass[i]));

  m_pHeaderType1.time[0] = doubleSwap((char *)(&m_pHeaderType1.time[0]));
  m_pHeaderType1.redshift[0] = doubleSwap((char *)(&m_pHeaderType1.redshift[0]));
  m_pHeaderType1.flag_sfr[0] = intSwap((char *)(&m_pHeaderType1.flag_sfr[0]));
  m_pHeaderType1.flag_feedback[0] = intSwap((char *)(&m_pHeaderType1.flag_feedback[0]));

  for (int i = 0; i < 6; i++)
    m_pHeaderType1.npartTotal[i] = intSwap((char *)(&m_pHeaderType1.npartTotal[i]));

  m_pHeaderType1.foling[0] = intSwap((char *)(&m_pHeaderType1.foling[0]));
  m_pHeaderType1.num_files[0] = intSwap((char *)(&m_pHeaderType1.num_files[0]));
  m_pHeaderType1.BoxSize[0] = doubleSwap((char *)(&m_pHeaderType1.BoxSize[0]));
  m_pHeaderType1.Omega0[0] = doubleSwap((char *)(&m_pHeaderType1.Omega0[0]));
  m_pHeaderType1.OmegaLambda[0] = doubleSwap((char *)(&m_pHeaderType1.OmegaLambda[0]));
  m_pHeaderType1.HubbleParam[0] = doubleSwap((char *)(&m_pHeaderType1.HubbleParam[0]));
  m_pHeaderType1.FlagAge[0] = intSwap((char *)(&m_pHeaderType1.FlagAge[0]));
  m_pHeaderType1.FlagMetals[0] = intSwap((char *)(&m_pHeaderType1.FlagMetals[0]));

  for (int i = 0; i < 6; i++)
    m_pHeaderType1.NallWH[i] = intSwap((char *)(&m_pHeaderType1.NallWH[i]));

  m_pHeaderType1.flag_entr_ics[0] = intSwap((char *)(&m_pHeaderType1.flag_entr_ics[0]));

  char fill[256 - 6 * sizeof(int) - 6 * sizeof(double) - 2 * sizeof(double) - 2 * sizeof(int) - 6 * sizeof(int) - 2 * sizeof(int) -
            4 * sizeof(double) - 9 * sizeof(int)]; /* fills to 256 Bytes */
  m_pHeaderType1.final_boh[0] = intSwap((char *)(&m_pHeaderType1.final_boh[0]));
  m_pHeaderType1.sizeFirstBlock[0] = intSwap((char *)(&m_pHeaderType1.sizeFirstBlock[0]));
}
