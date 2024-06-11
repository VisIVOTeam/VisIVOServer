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
#include "changasource.h"

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
#include <fcntl.h>
#include <omp.h>
#include <unistd.h>
#include <rpc/xdr.h>
#include <rpc/rpc.h>

//---------------------------------------------------------------------
int ChangaSource::readHeader()
//---------------------------------------------------------------------

{
  std::clog<< "Francesco Miceli" << std::endl;
  char dummy[4]; 
//   char checkType[4];
  //int provided;
  //MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided);
  int type=0; unsigned int Npart=0;
  std::string systemEndianism;
  //umBlock=0;
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
  //inFile.read((char *)(dummy), 4*sizeof(char)); //!*** IMPORTANT NOT REMOVE ***//
  //inFile.read((char *)(tmpType), 4*sizeof(char));   //!*** IMPORTANT NOT REMOVE ***//

  //tagType=tmpType;
  //checkType.push_back(tagType);
  
  header curHead;
  inFile.seekg(0, std::ios::beg);
  int pad;



  std::clog<< "Francesco Miceli denuvo" << std::endl;
  fpread = fopen( m_pointsFileName.c_str(), "r" );
  fread(&curHead,28,1,fpread);
  rewind(fpread);

  xdrstdio_create(&xdrread, fpread, XDR_DECODE);

  std::clog<< "Francesco Miceli diamine" << std::endl;
  if(xdr_double(&xdrread, &((struct header *)&curHead)->time) != TRUE){
  std::clog << "time " << curHead.time << std::endl;
    return 1;
  }
  
  if(xdr_int(&xdrread, &((struct header *)&curHead)->nbodies) != TRUE)
    return 1;
  if(xdr_int(&xdrread, &((struct header *)&curHead)->ndim) != TRUE)
    return 1;
  if(xdr_int(&xdrread, &((struct header *)&curHead)->nsph) != TRUE)
    return 1;
  if(xdr_int(&xdrread, &((struct header *)&curHead)->ndark) != TRUE)
    return 1;
  if(xdr_int(&xdrread, &((struct header *)&curHead)->nstar) != TRUE)
    return 1;
  if(xdr_int(&xdrread, &pad) != TRUE)
    return 1;

  std::clog<< "Francesco Miceli diamine" << std::endl;
  //m_snapformat = 2;
  std::clog << "time " << curHead.time << std::endl <<
              "nbodies " << curHead.nbodies << std::endl <<
              "ndim " << curHead.ndim<< std::endl<<
              "nsph " << curHead.nsph << std::endl<<
              "ndark " << curHead.ndark << std::endl<<
              "nstar " << curHead.nstar << std::endl<<
              "pad " << curHead.pad << std::endl;
  nsph = curHead.nsph;
  
  return 0;
}

int ChangaSource::readData()
{
  int idx = m_pointsBinaryName.rfind('.');
	std::string pathFileIn = m_pointsBinaryName.erase(idx, idx+4);
	std::string pathFileOut = pathFileIn;
  //gas_particle* gasParticles = (gas_particle*) malloc(sizeof(gas_particle)*nsph);
  std::ofstream outfile((pathFileOut + "GAS" + ".bin").c_str(),std::ofstream::binary );
  float* gasParticles = (float*) malloc(sizeof(float)*12*nsph);
  std::clog << sizeof(gas_particle)*nsph << std::endl;
  if (gasParticles == NULL) {
    std::clog << "Malloc Error" << std::endl;
    return 1;
  }
  float* tmp = gasParticles;
  std::clog << sizeof(gas_particle) << std::endl;
  for (int i=0; i<nsph; ++i) {
 // std::clog << i << std::endl;
    xdr_vector(&xdrread,(char *) tmp, (12),
                sizeof(float),(xdrproc_t) xdr_float);
    tmp+=12;
  }

  std::vector<std::string> gasBlocks; //!block fields names
  gasBlocks.push_back("MASS"); //0
  gasBlocks.push_back("POS_X"); //1
  gasBlocks.push_back("POS_Y");  //2
  gasBlocks.push_back("POS_Z");//3
  gasBlocks.push_back("VEL_X");   //4
  gasBlocks.push_back("VEL_y");//5
  gasBlocks.push_back("VEL_Z"); //6
  gasBlocks.push_back("RHO");  //7
  gasBlocks.push_back("TEMP");  //8
  gasBlocks.push_back("EPS");//9
  gasBlocks.push_back("METALS"); //10
  gasBlocks.push_back("PHI"); //11

  float* bufferBlock = (float*) malloc(sizeof(float)*nsph);
  for(int elem = 0; elem < 12; elem ++){
    for (int part=0; part<nsph; part++) {
      bufferBlock[part] = gasParticles[part*12 + elem];
      if(elem == 1) std::clog << bufferBlock[part] << std::endl;
    }
		outfile.write((char *)(bufferBlock), sizeof(float)*nsph);
  }

	outfile.close();

	std::string pathHeader = pathFileOut+"GAS"+ ".bin";
	makeHeader(nsph, pathHeader, gasBlocks, m_cellSize,m_cellComp,m_volumeOrTable);

  return 1;
}
  /*
  char dummy[4]; 
  int inFile[numFiles];
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
  unsigned int j=0;

  char tagTmp[5]="";
  std::string tag; 
//   const char point = '.';
  int idx = m_pointsBinaryName.rfind('.');

  std::string pathFileIn = m_pointsBinaryName.erase(idx, idx+4);
  std::string pathFileOut = pathFileIn;
   
  std::string bin = ".bin";
  std::string X = "_X"; std::string Y = "_Y"; std::string Z = "_Z";
  */
  /*=======================================================================*/
  /*!================== Identification TYPE & NAME BLOCK ===================*/
  /*
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
  */
  /*!=======================================================================*/	/*!========================== Open and Check FILE ========================*/
  /*
  std::string fileName;
  std::filesystem::path p(m_pointsFileName.c_str());
  fileName = m_pointsFileName.c_str();
  
  if(numFiles > 1 && p.extension().generic_string().size() > 0 && isNumeric(p.extension().generic_string().substr(1)) ){
    for(int i = 0; i < p.extension().generic_string().substr(1).size(); i++){
      fileName.pop_back();
    }
  }

  //create list of blocks to read
  std::ifstream fStream;
  fStream.open(fileName + std::to_string(0));
  fStream.seekg(280, std::ios::beg);
    
  std::vector<std::string> listOfBlocks;
  while(fStream.peek() != EOF){
    fStream.seekg(4, std::ios::cur);
    fStream.read((char *)(tagTmp), 4*sizeof(char));   
    tag = strtok(tagTmp, " ");
    if(std::find(blockNamesToCompare.begin(), blockNamesToCompare.end(), tag) != blockNamesToCompare.end() && iCompare(tag, "Zs") != 0){
      if(m_fields.size() == 0 || std::find(m_fields.begin(), m_fields.end(), tag) != m_fields.end())
        listOfBlocks.push_back(tag);
    }
    fStream.read((char *)(m_sizeBlock), sizeof(int));  
    if (needSwap)
      m_sizeBlock[0]=intSwap((char *)(&m_sizeBlock[0]));
    fStream.seekg (m_sizeBlock[0], std::ios::cur);
    fStream.read((char *)(m_sizeBlock), sizeof(int));
  }
  fStream.close();

  // Get the number of processes
  int num_proc, proc_id;
  MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
  MPI_Comm_rank (MPI_COMM_WORLD, &proc_id);


  std::vector<std::string> blockNames;

      int typePosition[listOfBlocks.size() + 1][6];
  long long fileStartPosition[6][numFiles + 1];

  for(int j = 0; j < 6; j++){
    typePosition[0][j] = 0;
    for(int i = 0; i < listOfBlocks.size(); i++){
      if(iCompare(listOfBlocks[i], "MASS") != 0 || (iCompare(listOfBlocks[i], "MASS") == 0 && m_pHeaderType2[0].mass[j] == 0)) typePosition[i + 1][j]= 
        blocksFields[mapBlockNamesToFields[listOfBlocks[i]]][j] * 
        mapBlockSize[listOfBlocks[i]];
      else
        typePosition[i + 1][j] = 0;
        
    }
  }

  //Prefix sum
  for(int j = 0; j < 6; j++){
    for(int i = 1; i < listOfBlocks.size(); i++){
      typePosition[i][j] += typePosition[i-1][j];
    }
  }

  //initialize pos 0 for each type
  for(int type = 0; type < 6; type++){
      fileStartPosition[type][0] = 0;
  }

  for(int file = 1; file < numFiles; file++){
    for(int type = 0; type < 6; type++){
      fileStartPosition[type][file] = fileStartPosition[type][file-1] + (unsigned long long)m_pHeaderType2[file-1].npart[type];
    }
  }
  
  int nTotalBlocks = listOfBlocks.size() * numFiles;
  double t1, t2; 
  
  bool alreadyOpen = false;
  int totBlocks = listOfBlocks.size();

  int outFileBin[6];
  for (type=0; type<6; type++)
  {
    if (m_pHeaderType2[0].npart[type] != 0)	
    {	
      std::string nameFileBinOut =  pathFileOut + tagTypeForNameFile[type].c_str() + bin;
      outFileBin[type] = creat(nameFileBinOut.c_str(), S_IRWXU);
    }
  }	

  for(int i = 0; i < numFiles; i++){
    if(numFiles > 1) inFile[i] = open((fileName + std::to_string(i)).c_str(), std::ios::binary);
    else inFile[i] = open(fileName.c_str(), std::ios::binary);
  }

  unsigned int param=1; unsigned int esp=32;
  unsigned long long int maxULI;
  #pragma omp parallel for collapse(2)
  for(int nBlock = proc_id; nBlock < totBlocks; nBlock+=num_proc){
    for(int nFile = 0; nFile < numFiles; nFile++){
      std::string tag;   
      char tagTmp[5]="";
      long long unsigned int offset = 0;
      int sizeBlock[1];
      sizeBlock[0] = 0;
      unsigned long int chunk=0; unsigned long int n=0; unsigned long int Resto=0;
      int c=0;
      while(iCompare(tag, listOfBlocks[nBlock]) != 0){
        offset += 4;
        pread(inFile[nFile], (char *)(tagTmp), 4*sizeof(char), offset);
        offset += 4;
        tag = strtok(tagTmp, " ");
        if(iCompare(tag, listOfBlocks[nBlock]) == 0) break;
        pread(inFile[nFile], (char *)(sizeBlock), sizeof(int), offset);
        offset += (4 + sizeBlock[0] + 4);
        if (needSwap)
          m_sizeBlock[0]=intSwap((char *)(&m_sizeBlock[0])); 
      }
      offset += 12;
      off64_t pWrite=0; long long unsigned int pToStart=0;
      off64_t pWriteX=0,  pWriteY=0, pWriteZ=0;
      //start processing block
      unsigned long long int minPart[6];
            
      maxULI=ldexp((float)param, esp); 
      minPart[0]=maxULI;
      minPart[1]=maxULI;
      minPart[2]=maxULI;
      minPart[3]=maxULI;
      minPart[4]=maxULI;
      minPart[5]=maxULI;
      for(int type=0; type < 6; type++){
        if (m_pHeaderType2[nFile].npart[type] != 0 && m_pHeaderType2[nFile].npart[type] <= 2500000)
          minPart[type] = m_pHeaderType2[nFile].npart[type];
        else
          minPart[type] = 2500000;
      }
      
      for(int type = 0; type < 6; type++)
      {             
        if(m_pHeaderType2[nFile].npart[type] != 0 && blocksFields[mapBlockNamesToFields[listOfBlocks[nBlock]]][type] && 
          (iCompare(listOfBlocks[nBlock], "MASS") != 0 || m_pHeaderType2[nFile].mass[type] == 0))
        {    
          pToStart = typePosition[nBlock][type]*(unsigned long long)m_pHeaderType2[nFile].npartTotal[type] + fileStartPosition[type][nFile];
          chunk = minPart[type];
          n=m_pHeaderType2[nFile].npart[type]/chunk;
          
          Resto=m_pHeaderType2[nFile].npart[type]-(chunk*n); 
              
          float *bufferBlock=NULL;
          bufferBlock = new float[mapBlockSize[listOfBlocks[nBlock]]*chunk];

          if(mapBlockSize[listOfBlocks[nBlock]] == 3)
          {           
            float *buffer_X=NULL;
            buffer_X = new float[chunk];
            float *buffer_Y=NULL;
            buffer_Y = new float[chunk];
            float *buffer_Z=NULL;
            buffer_Z = new float[chunk];
                
            for (int k = 0; k < n; k++)
            {    
              pread(inFile[nFile], (char *)(bufferBlock), 3*chunk*sizeof(float), offset);
              offset += 3*chunk*sizeof(float);
              if(needSwap)
                for (i=0; i<chunk; i++)
                {	
                  buffer_X[i] = floatSwap((char *)(&bufferBlock[3*i]));
                  buffer_Y[i] = floatSwap((char *)(&bufferBlock[3*i+1]));
                  buffer_Z[i] = floatSwap((char *)(&bufferBlock[3*i+2]));
                }
              else
                for (i=0; i<chunk; i++)
                {	
                  buffer_X[i] = bufferBlock[3*i];
                  buffer_Y[i] = bufferBlock[3*i+1];
                  buffer_Z[i] = bufferBlock[3*i+2];
                }
              
              pWriteX=(((unsigned long long)pToStart*sizeof(float)) + (k*chunk*sizeof(float)));
              pwrite(outFileBin[type], (char *)(buffer_X), chunk*sizeof(float), pWriteX);
              pWriteY=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)) + ((unsigned long long)m_pHeaderType2[0].npartTotal[type]*sizeof(float)));
              pwrite(outFileBin[type], (char *)(buffer_Y), chunk*sizeof(float), pWriteY);
              pWriteZ=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)) + (2*(unsigned long long)m_pHeaderType2[0].npartTotal[type]*sizeof(float)));  
              pwrite(outFileBin[type], (char *)(buffer_Z), chunk*sizeof(float), pWriteZ);
            }
            */
            /*=================================================================*/
            /*============ Buffer Block and Write out FILE [Resto] ============*/
    /*
            if(Resto>0){
              pread(inFile[nFile], (char *) bufferBlock, 3*Resto*sizeof(float), offset);
              offset += 3*Resto*sizeof(float);
              if(needSwap)
                for (i=0; i<Resto; i++)
                {	
                  buffer_X[i] = floatSwap((char *)(&bufferBlock[3*i]));
                  buffer_Y[i] = floatSwap((char *)(&bufferBlock[3*i+1]));
                  buffer_Z[i] = floatSwap((char *)(&bufferBlock[3*i+2]));
                }
              else
                for (i=0; i<Resto; i++)
                {	
                  buffer_X[i] = bufferBlock[3*i];
                  buffer_Y[i] = bufferBlock[3*i+1];
                  buffer_Z[i] = bufferBlock[3*i+2];
                }

              pWriteX=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)));
              pwrite(outFileBin[type], (char *)(buffer_X), Resto*sizeof(float), pWriteX);
              pWriteY=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)) + (m_pHeaderType2[0].npartTotal[type]*sizeof(float)));
              pwrite(outFileBin[type], (char *)(buffer_Y), Resto*sizeof(float), pWriteY);
              pWriteZ=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)) + (2*m_pHeaderType2[0].npartTotal[type]*sizeof(float)));
              pwrite(outFileBin[type], (char *)(buffer_Z), Resto*sizeof(float), pWriteZ);
            }
            */
                    /*=================================================================*/  
/*
            delete [] buffer_X;
            delete [] buffer_Y;
            delete [] buffer_Z;
            delete [] bufferBlock;
          }
          else
          {
            for (int k = 0; k < n; k++)
            {
              pread(inFile[nFile], (char *)(bufferBlock), chunk*sizeof(float), offset);
              offset += chunk*sizeof(float);
              if(needSwap)
                for (j=0; j<chunk; j++)
                  bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
              
              pWrite=((pToStart*sizeof(float)) + (k*chunk*sizeof(float))); 
              pwrite(outFileBin[type], (char *)(bufferBlock), chunk*sizeof(float), pWrite);         
            }
*/
            /*=================================================================*/
            /*============ Buffer Block and Write out FILE [Resto] ============*/
/*
            if(Resto > 0){  
              pread(inFile[nFile], (char *) bufferBlock, Resto*sizeof(float), offset);
              offset += Resto*sizeof(float);
              if( needSwap)
                for (j=0; j<Resto; j++)
                  bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
            
              pWrite=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)));
              pwrite(outFileBin[type], (char *)(bufferBlock), Resto*sizeof(float), pWrite);
            }
            delete [] bufferBlock;
          }
        }
        pWriteX = 0; pWriteY = 0; pWriteZ = 0; 
      } 
    }
  }

  for(int nFile = 0; nFile < 6; nFile++)close(inFile[nFile]);
  for(type = 0; type < 6; type++)close(outFileBin[type]);
*/   
  /*=================================================================*/
  /*!=========== Read information to write FILE.bin.head =============*/
/*
  if(proc_id==0){
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
*/  
  /*=================================================================*/		
  /*!===================== Write FILE.bin.head =======================*/
/*
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
  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
  */
