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
#include "mpigadgetreader.h"
#include "serialgadgetreader.h"

#include "visivoutils.h"
#include "mpi.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <filesystem>

bool isNumeric(std::string const &str)
{
  auto it = str.begin();
  while (it != str.end() && std::isdigit(*it)) {
    it++;
  }
  return !str.empty() && it == str.end();
}

//---------------------------------------------------------------------
int GadgetSource::readHeader()
//---------------------------------------------------------------------

{
  char dummy[4]; 
//   char checkType[4];
  MPI_Init(NULL, NULL);
  int type=0; unsigned int Npart=0;
  std::string systemEndianism;
  numBlock=0;
  bool needSwap=false;
#ifdef VSBIGENDIAN
  systemEndianism="big";
#else
  systemEndianism="little";
#endif
  if((m_endian=="b" || m_endian=="big") && systemEndianism=="little")
    needSwap=true;
  if((m_endian=="l" || m_endian=="little") && systemEndianism=="big")
    needSwap=true;
	
  std::string fileName = m_pointsFileName.c_str();
  std::ifstream inFile;

  inFile.open(fileName, std::ios::binary);
  if (!inFile)
  {
    std::cerr<<"Error while opening File"<<std::endl;
    return -1;
  }
  inFile.read((char *)(dummy), 4*sizeof(char)); //!*** IMPORTANT NOT REMOVE ***//
  inFile.read((char *)(tmpType), 4*sizeof(char));   //!*** IMPORTANT NOT REMOVE ***//

  tagType=tmpType;
  checkType.push_back(tagType);
  if (iCompare (checkType[0].c_str(), "HEAD") == 0)  //!read header Type2 else Type1
  {
    headerType2 curHead;
    //std::clog<<"Header1"<<std::endl;
    inFile.seekg(4, std::ios::beg);
    
    inFile.read((char *)(&curHead), 304);   //!*** COPY DATA in STRUCT m_pHeaderType2 ***//
    m_snapformat = 2;
    if (needSwap)
      swapHeaderType2();
    
    int nFiles = curHead.num_files[0];
    if(nFiles > 1){
      fileName = m_pointsFileName.c_str();
      std::filesystem::path p(fileName);
      if(p.extension().generic_string().size() > 0 && isNumeric(p.extension().generic_string().substr(1))){
        for(int j = 0; j < p.extension().generic_string().substr(1).size(); j++){
          fileName.pop_back();
        }
        
        if(checkMultipleFiles(nFiles, fileName)){
          numFiles = nFiles;
          if (readMultipleHeaders(nFiles, fileName, needSwap) == 0){
            
            inFile.close();
            MPI_Finalize();
            return -1;
          }
          else return 0;
        }
        else {
          inFile.close();         
          MPI_Finalize();
          return -1;
        }
      } 
      else{
        inFile.close();
        std::cerr << "Expecting multiple files" << std::endl;
      }
    }
    m_pHeaderType2.push_back(curHead);
    numFiles = 1;
  }
  else  //Type1 
  {
    inFile.seekg(0, std::ios::beg);	
    inFile.read((char *)(&m_pHeaderType1), 268);
    m_snapformat = 1;
    
    if (needSwap)
      swapHeaderType1();
    
    if (m_pHeaderType1.sizeFirstBlock[0] != (3*Npart*sizeof(float)))
    {
      std::cerr<<"The size File is not than expected"<<std::endl;
      return -1;
    }
      
  }
  inFile.close();
  return 0;
}

int GadgetSource::readMultipleHeaders(int nFiles, std::string fileName, bool needSwap)
{
  for(int i = 0; i < nFiles; i++){ 
    std::ifstream inFile; 
    headerType2 curHead;
    inFile.open(fileName + std::to_string(i), std::ios::binary);
    
    inFile.seekg(4, std::ios::beg);
    
    inFile.read((char *)(&curHead), 304);   //!*** COPY DATA in STRUCT m_pHeaderType2 ***//
    if (needSwap)
      swapHeaderType2();
    m_pHeaderType2.push_back(curHead);

    inFile.close();
  }
  return 1;
}

int GadgetSource::checkMultipleFiles(int nFiles, std::string fName){  
  std::ifstream inFile;
  
  for(int i = 0; i < nFiles; i++){
    inFile.open(fName + std::to_string(i), std::ios::binary);
    if (!inFile)
    {
      std::cerr<<"Error while opening multiple files, read only input file"<<std::endl;
      return 0;  
    }
    inFile.close();
    
  }
  
  return 1; 
}

int GadgetSource::readData()
{
  char dummy[4]; 
  std::string systemEndianism;
  bool needSwap=false;
#ifdef VSBIGENDIAN
  systemEndianism="big";
#else
  systemEndianism="little";
#endif
  if((m_endian=="b" || m_endian=="big") && systemEndianism=="little")
    needSwap=true;
  if((m_endian=="l" || m_endian=="little") && systemEndianism=="big")
    needSwap=true;

  int type=0;
  unsigned int i=0;

  char tagTmp[5]="";
  std::string tag; 
//   const char point = '.';
  int idx = m_pointsBinaryName.rfind('.');

  std::string pathFileIn = m_pointsBinaryName.erase(idx, idx+4);
  std::string pathFileOut = pathFileIn;
   
  std::string bin = ".bin";
  std::string X = "_X"; std::string Y = "_Y"; std::string Z = "_Z";

  /*=======================================================================*/
  /*!================== Identification TYPE & NAME BLOCK ===================*/

  std::vector<std::string> tagTypeForNameFile; //!species block nameset 
  tagTypeForNameFile.push_back("GAS");
  tagTypeForNameFile.push_back("HALO");
  tagTypeForNameFile.push_back("DISK");
  tagTypeForNameFile.push_back("BULGE");
  tagTypeForNameFile.push_back("STARS");
  tagTypeForNameFile.push_back("BNDRY");

  std::vector<std::string> blockNamesToCompare; //!block fields names
  blockNamesToCompare.push_back("POS"); //0
  blockNamesToCompare.push_back("VEL"); //1
  blockNamesToCompare.push_back("ID");  //2
  blockNamesToCompare.push_back("MASS");//3
  blockNamesToCompare.push_back("U");   //4
  blockNamesToCompare.push_back("TEMP");//5
  blockNamesToCompare.push_back("RHO"); //6
  blockNamesToCompare.push_back("NE");  //7
  blockNamesToCompare.push_back("NH");  //8
  blockNamesToCompare.push_back("HSML");//9
  blockNamesToCompare.push_back("SFR"); //10
  blockNamesToCompare.push_back("AGE"); //11
  blockNamesToCompare.push_back("Z");   //12
  blockNamesToCompare.push_back("Zs");  //13
  blockNamesToCompare.push_back("iM");  //14
  blockNamesToCompare.push_back("ZAGE");//15
  blockNamesToCompare.push_back("ZALV");//16
  blockNamesToCompare.push_back("CLDX");//17
  blockNamesToCompare.push_back("TSTP");//18
  blockNamesToCompare.push_back("POT"); //19
  blockNamesToCompare.push_back("ACCE");//20
  blockNamesToCompare.push_back("ENDT");//21
  //blockNamesToCompare.push_back("TSTP");//10
  blockNamesToCompare.push_back("IDU"); //22
  blockNamesToCompare.push_back("HOTT");//23
  blockNamesToCompare.push_back("MHOT");//24
  blockNamesToCompare.push_back("MCLD");//25
  blockNamesToCompare.push_back("EHOT");//26
  blockNamesToCompare.push_back("MSF"); //27
  blockNamesToCompare.push_back("MFST");//28
  blockNamesToCompare.push_back("NMF"); //29
  blockNamesToCompare.push_back("EOUT");//30
  blockNamesToCompare.push_back("EREC");//31
  blockNamesToCompare.push_back("EOLD");//32
  blockNamesToCompare.push_back("TDYN");//33
  blockNamesToCompare.push_back("SFRo");//34
  blockNamesToCompare.push_back("CLCK");//35
  blockNamesToCompare.push_back("Egy0");//36
  blockNamesToCompare.push_back("GRAD");//37
  blockNamesToCompare.push_back("BHMA");//38
  blockNamesToCompare.push_back("BHMD");//39
  blockNamesToCompare.push_back("BHPC");//40
  blockNamesToCompare.push_back("ACRB");//41
  
  std::vector<std::vector<bool>> blocksFields = 
  { 
    {1,1,1,1,1,1}, // 0: POS, VEL, ID, MASS, IDU, TSTP, POT, ACCE
    {1,0,0,0,0,0}, // 1: U, TEMP, RHO, NE, NH, HSML, SFR, CLDX, ENDT, HOTT, MHOT, MCLD, EHOT, MSF, MFST, NMF, EOUT, EREC, EOLD, TDYN, SFRo, CLCK, Egy0, GRAD
    {0,0,0,0,1,1}, // 2: AGE
    {1,0,0,0,1,0}, // 3: Z, Zs, ZAGE, ZALV
    {0,0,0,0,1,0}, // 4: iM
    {0,0,0,0,0,1}  // 5: BHMA, BHMD, BHPC, ACRB
  };

  std::vector<int> blockNamesToFields 
  {
    0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 3, 3, 4, 3, 3, 1, 0, 0, 0,
    1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 5
  };

  std::vector<int> blockSize 
  {
    3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1
  };

  std::unordered_map<std::string,int>mapBlockNamesToFields;
  std::unordered_map<std::string,int>mapBlockSize;

  for (i = 0; i < blockNamesToCompare.size(); ++i){
    mapBlockNamesToFields[blockNamesToCompare[i]] = blockNamesToFields[i];
  }

  for (i = 0; i < blockNamesToCompare.size(); ++i){
    mapBlockSize[blockNamesToCompare[i]] = blockSize[i];
  }

  std::vector<std::vector<std::string> > namesFields;
  std::vector<std::string> tmpNamesFields;
  std::string pathHeader = "";  int KK=0;
  
  /*!=======================================================================*/	/*!========================== Open and Check FILE ========================*/

  MPI_File inFile;
  std::string fileName;

  std::filesystem::path p(m_pointsFileName.c_str());
  fileName = m_pointsFileName.c_str();
  
  if(numFiles > 1 && p.extension().generic_string().size() > 0 && isNumeric(p.extension().generic_string().substr(1)) ){
    for(int i = 0; i < p.extension().generic_string().substr(1).size(); i++){
      fileName.pop_back();
    }
  }

  if(numFiles > 1) MPI_File_open(MPI_COMM_WORLD,
    (fileName + std::to_string(0)).c_str(), 
    MPI_MODE_RDONLY,
    MPI_INFO_NULL, &inFile);
  else MPI_File_open(MPI_COMM_WORLD,
                     fileName.c_str(), 
                     MPI_MODE_RDONLY,
                     MPI_INFO_NULL, &inFile);

  if (!inFile)
  {
    std::cerr<<"Error while opening block"<<std::endl;
    return -1;
  }

  //create list of blocks to read
  MPI_Status status;
  std::vector<std::string> listOfBlocks;
  MPI_File_seek(inFile, 280, MPI_SEEK_SET);

  do{ 
    MPI_File_seek(inFile, 4, MPI_SEEK_CUR);
    MPI_File_read(inFile, (char *)(tagTmp), 4, MPI_CHAR, &status);
    tag = strtok(tagTmp, " ");
    if(std::find(blockNamesToCompare.begin(), blockNamesToCompare.end(), tag) != blockNamesToCompare.end() && iCompare(tag, "Zs") != 0)
      listOfBlocks.push_back(tag);
    MPI_File_read(inFile, &m_sizeBlock, 1, MPI_INT, &status);  
    if (needSwap)
      m_sizeBlock[0]=intSwap((char *)(&m_sizeBlock[0]));
    MPI_File_seek(inFile, m_sizeBlock[0], MPI_SEEK_CUR);
    MPI_File_read(inFile, &m_sizeBlock, 1, MPI_INT, &status); 
  }while(status._ucount >= 4);
  MPI_File_close(&inFile);

  // Get the number of processes
  int num_proc;
  MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

  AbstractReader* reader;
  if(num_proc > 2) reader = new mpiGadgetReader(
    fileName, pathFileOut, numFiles, m_pHeaderType2, listOfBlocks, blockNamesToCompare);
  else reader = new serialGadgetReader(fileName, pathFileOut, numFiles, m_pHeaderType2, listOfBlocks.size(), blockNamesToCompare);
  
  if(num_proc <= 2) reader->startRead(needSwap);
  else if(reader->startRead(needSwap)!= 0) {
    MPI_Finalize();
    return -1;
  }
   
  /*=================================================================*/
  /*!=========== Read information to write FILE.bin.head =============*/
  
  for(type = 0; type < 6; type++){      
    if (m_pHeaderType2[0].npart[type] == 0){       
      tmpNamesFields.clear();
      tmpNamesFields.push_back(" ");
      namesFields.push_back(tmpNamesFields);
      tmpNamesFields.clear();
    }
    else if (m_pHeaderType2[0].npart[type] != 0)
    {
      for(int i = 0; i < listOfBlocks.size(); i++)
      {
        if(mapBlockSize[listOfBlocks[i]] == 3 && blocksFields[mapBlockNamesToFields[listOfBlocks[i]]][type]){
          tmpNamesFields.push_back(listOfBlocks[i] + X);
          tmpNamesFields.push_back(listOfBlocks[i] + Y);
          tmpNamesFields.push_back(listOfBlocks[i] + Z);
        }
        else if(mapBlockSize[listOfBlocks[i]] == 1 && blocksFields[mapBlockNamesToFields[listOfBlocks[i]]][type] && (iCompare(listOfBlocks[i], "MASS") != 0|| m_pHeaderType2[0].mass[type] == 0))
          tmpNamesFields.push_back(listOfBlocks[i]);
      }  
      namesFields.push_back(tmpNamesFields);
      tmpNamesFields.clear();
    }
  }  
  /*=================================================================*/		
  /*!===================== Write FILE.bin.head =======================*/

  for (type=0; type<6; type++)
  {
    if(m_pHeaderType2[0].npartTotal[type] != 0)
    {
      for(int KK = 0; KK < namesFields[type].size(); KK++)
      {
        m_fieldsNames.push_back(namesFields[type][KK]);
      }
      pathHeader = pathFileOut + tagTypeForNameFile[type] + bin;
      makeHeader((unsigned long long int)m_pHeaderType2[0].npartTotal[type], pathHeader, m_fieldsNames,m_cellSize,m_cellComp,m_volumeOrTable);
      m_fieldsNames.clear();
      pathHeader="";
    }
  }  
  MPI_Finalize();
  return 0;
}

//--------------------------------------
void GadgetSource::swapHeaderType2()
//--------------------------------------
{
  char first_fortran_legacy[4]; //m_snapformat=2 : you have to "jump" 4+2*4 =12 bytes and you'll find the the Block size
  m_pHeaderType2[0].boh[0] = intSwap((char*)(&m_pHeaderType2[0].boh[0]));
  m_pHeaderType2[0].nBlock[0] = intSwap((char*)(&m_pHeaderType2[0].nBlock[0]));	
  m_pHeaderType2[0].size[0] = intSwap((char*)(&m_pHeaderType2[0].size[0]));
	  
  for (int i=0; i<6; i++)
    m_pHeaderType2[0].npart[i] = intSwap((char*)(&m_pHeaderType2[0].npart[i]));
	  
  for (int i=0; i<6; i++)
    m_pHeaderType2[0].mass[i] = intSwap((char*)(&m_pHeaderType2[0].mass[i]));
	  
  m_pHeaderType2[0].time[0] = doubleSwap((char*)(&m_pHeaderType2[0].time[0]));
  m_pHeaderType2[0].redshift[0] = doubleSwap((char*)(&m_pHeaderType2[0].redshift[0]));
  m_pHeaderType2[0].flag_sfr[0] = intSwap((char*)(&m_pHeaderType2[0].flag_sfr[0]));
  m_pHeaderType2[0].flag_feedback[0] = intSwap((char*)(&m_pHeaderType2[0].flag_feedback[0]));
	  
  for (int i=0; i<6; i++)
    m_pHeaderType2[0].npartTotal[i] = intSwap((char*)(&m_pHeaderType2[0].npartTotal[i]));
   
  m_pHeaderType2[0].foling[0] = intSwap((char*)(&m_pHeaderType2[0].foling[0]));
  m_pHeaderType2[0].num_files[0] = intSwap((char*)(&m_pHeaderType2[0].num_files[0]));
  m_pHeaderType2[0].BoxSize[0] = doubleSwap((char*)&(m_pHeaderType2[0].BoxSize[0]));
  m_pHeaderType2[0].Omega0[0] = doubleSwap((char*)(&m_pHeaderType2[0].Omega0[0]));
  m_pHeaderType2[0].OmegaLambda[0] = doubleSwap((char*)(&m_pHeaderType2[0].OmegaLambda[0]));
  m_pHeaderType2[0].HubbleParam[0] = doubleSwap((char*)(&m_pHeaderType2[0].HubbleParam[0]));
  m_pHeaderType2[0].FlagAge[0] = intSwap((char*)(&m_pHeaderType2[0].FlagAge[0]));
  m_pHeaderType2[0].FlagMetals[0] = intSwap((char*)(&m_pHeaderType2[0].FlagMetals[0]));
	  
  for (int i=0; i<6; i++)
    m_pHeaderType2[0].NallWH[i] = intSwap((char*)(&m_pHeaderType2[0].NallWH[i]));
   
  m_pHeaderType2[0].flag_entr_ics[0] = intSwap((char*)(&m_pHeaderType2[0].flag_entr_ics[0]));
   
  char fill[256- 6*sizeof(int)- 6*sizeof(double)- 2*sizeof(double)- 2*sizeof(int)- 6*sizeof(int)- 2*sizeof(int)- 
      4*sizeof(double)- 9*sizeof(int)]; /* fills to 256 Bytes */
  m_pHeaderType2[0].final_boh[0] = intSwap((char*)(&m_pHeaderType2[0].final_boh[0]));
  m_pHeaderType2[0].final_nBlock[0] = intSwap((char*)(&m_pHeaderType2[0].final_nBlock[0]));
  
  char tagFirstBlock[4];
  m_pHeaderType2[0].first_boh[0] = intSwap((char*)(&m_pHeaderType2[0].first_boh[0]));
  m_pHeaderType2[0].first_nBlock[0] = intSwap((char*)(&m_pHeaderType2[0].first_nBlock[0]));
  m_pHeaderType2[0].sizeFirstBlock[0] = intSwap((char*)(&m_pHeaderType2[0].sizeFirstBlock[0]));
}

  //--------------------------------------
  void GadgetSource::swapHeaderType1()
  //--------------------------------------
{	
  m_pHeaderType1.size[0] = intSwap((char*)(&m_pHeaderType1.size[0]));
	  
  for (int i=0; i<6; i++)
    m_pHeaderType1.npart[i] = intSwap((char*)(&m_pHeaderType1.npart[i]));
	  
  for (int i=0; i<6; i++)
    m_pHeaderType1.mass[i] = intSwap((char*)(&m_pHeaderType1.mass[i]));
	  
  m_pHeaderType1.time[0] = doubleSwap((char*)(&m_pHeaderType1.time[0]));
  m_pHeaderType1.redshift[0] = doubleSwap((char*)(&m_pHeaderType1.redshift[0]));
  m_pHeaderType1.flag_sfr[0] = intSwap((char*)(&m_pHeaderType1.flag_sfr[0]));
  m_pHeaderType1.flag_feedback[0] = intSwap((char*)(&m_pHeaderType1.flag_feedback[0]));
	  
  for (int i=0; i<6; i++)
    m_pHeaderType1.npartTotal[i] = intSwap((char*)(&m_pHeaderType1.npartTotal[i]));
   
  m_pHeaderType1.foling[0] = intSwap((char*)(&m_pHeaderType1.foling[0]));
  m_pHeaderType1.num_files[0] = intSwap((char*)(&m_pHeaderType1.num_files[0]));
  m_pHeaderType1.BoxSize[0] = doubleSwap((char*)(&m_pHeaderType1.BoxSize[0]));
  m_pHeaderType1.Omega0[0] = doubleSwap((char*)(&m_pHeaderType1.Omega0[0]));
  m_pHeaderType1.OmegaLambda[0] = doubleSwap((char*)(&m_pHeaderType1.OmegaLambda[0]));
  m_pHeaderType1.HubbleParam[0] = doubleSwap((char*)(&m_pHeaderType1.HubbleParam[0]));
  m_pHeaderType1.FlagAge[0] = intSwap((char*)(&m_pHeaderType1.FlagAge[0]));
  m_pHeaderType1.FlagMetals[0] = intSwap((char*)(&m_pHeaderType1.FlagMetals[0]));
	  
  for (int i=0; i<6; i++)
    m_pHeaderType1.NallWH[i] = intSwap((char*)(&m_pHeaderType1.NallWH[i]));
   
  m_pHeaderType1.flag_entr_ics[0] = intSwap((char*)(&m_pHeaderType1.flag_entr_ics[0]));
   
  char fill[256- 6*sizeof(int)- 6*sizeof(double)- 2*sizeof(double)- 2*sizeof(int)- 6*sizeof(int)- 2*sizeof(int)- 
      4*sizeof(double)- 9*sizeof(int)]; /* fills to 256 Bytes */
  //m_pHeaderType1.final_boh[0] = intSwap((char*)(&m_pHeaderType1.final_boh[0]));
  m_pHeaderType1.sizeFirstBlock[0] = intSwap((char*)(&m_pHeaderType1.sizeFirstBlock[0]));
   
}
