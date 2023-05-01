#include "mpigadgetreader.h"

#include "visivoutils.h"

#include "mpi.h"
#include "unordered_map"
#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>

struct message{
  int typePos;
  int filePos;
};

mpiGadgetReader::mpiGadgetReader(std::string fileName, std::string pathFileOut, int numFiles, std::vector<headerType2> HeaderType2, 
std::vector<std::string> listOfBlocks, std::vector<std::string> blockNamesToCompare){
  m_fileName = fileName;
  m_numFiles = numFiles;
  m_pHeaderType2 = HeaderType2;
  m_blockNamesToCompare = blockNamesToCompare;
  m_pathFileOut = pathFileOut;
  m_listOfBlocks = listOfBlocks;
}

int mpiGadgetReader::startRead(bool needSwap){
  MPI_File inFile[m_numFiles];
  MPI_Status status;
  int proc_id;
  int num_proc;
  std::string bin = ".bin";  
  std::string tag;   
  char tagTmp[5]="";
  int m_sizeBlock[1];
  int j=0; int k=0; int type=0; int block=0;
  unsigned int i=0;
  unsigned long int chunk=0;  //!** Number of Information read & write for cycle **//
  unsigned long int n=0; unsigned long int Resto=0;
  MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
  MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
    
  unsigned int param=1; unsigned int esp=32;
  unsigned long long int maxULI; 
  unsigned long long int minPart[6];
        
  maxULI=ldexp((float)param, esp); 
  minPart[0]=maxULI;
  minPart[1]=maxULI;
  minPart[2]=maxULI;
  minPart[3]=maxULI;
  minPart[4]=maxULI;
  minPart[5]=maxULI;

  std::vector<unsigned int> dimV{0, 0, 0, 0, 0, 0};
  std::vector<unsigned int> threeDimV{0, 0, 0, 0, 0, 0};
  std::vector<unsigned int> oneDimV{0, 0, 0, 0, 0, 0};
  std::vector<unsigned int> otherBlockDimV{0, 0, 0, 0, 0, 0};
  std::vector<unsigned int> massBlockDimV{0, 0, 0, 0, 0, 0};
	
  /*=======================================================================*/ 

	
  /*=======================================================================*/
  /*!================== Identification TYPE & NAME BLOCK ===================*/

  std::vector<std::string> tagTypeForNameFile; //!species block nameset 
  tagTypeForNameFile.push_back("GAS");
  tagTypeForNameFile.push_back("HALO");
  tagTypeForNameFile.push_back("DISK");
  tagTypeForNameFile.push_back("BULGE");
  tagTypeForNameFile.push_back("STARS");
  tagTypeForNameFile.push_back("BNDRY");
  
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

  for (i = 0; i < m_blockNamesToCompare.size(); ++i){
    mapBlockNamesToFields[m_blockNamesToCompare[i]] = blockNamesToFields[i];
  }

  for (i = 0; i < m_blockNamesToCompare.size(); ++i){
    mapBlockSize[m_blockNamesToCompare[i]] = blockSize[i];
  }

  std::vector<std::string> blockNames;

      int typePosition[m_listOfBlocks.size() + 1][6];
  int fileStartPosition[6][m_numFiles + 1];

  for(int j = 0; j < 6; j++){
    typePosition[0][j] = 0;
    for(int i = 0; i < m_listOfBlocks.size(); i++){
      if(iCompare(m_listOfBlocks[i], "MASS") != 0 || (iCompare(m_listOfBlocks[i], "MASS") == 0 && m_pHeaderType2[0].mass[j] == 0)) typePosition[i + 1][j]= 
        blocksFields[mapBlockNamesToFields[m_listOfBlocks[i]]][j] * 
        mapBlockSize[m_listOfBlocks[i]];
      else
        typePosition[i + 1][j] = 0;
        
    }
  }

  //Prefix sum
  for(int j = 0; j < 6; j++){
    for(int i = 1; i < m_listOfBlocks.size(); i++){
      typePosition[i][j] += typePosition[i-1][j];
    }
  }

  //initialize pos 0 for each type
  for(int type = 0; type < 6; type++){
      fileStartPosition[type][0] = 0;
  }

  for(int file = 1; file < m_numFiles; file++){
    for(int type = 0; type < 6; type++){
      fileStartPosition[type][file] = fileStartPosition[type][file-1] + m_pHeaderType2[file-1].npart[type];
    }
  }

  for(int i = 0; i < m_numFiles; i++){
    if(m_numFiles > 1) MPI_File_open(MPI_COMM_WORLD,
      (m_fileName + std::to_string(i)).c_str(), 
      MPI_MODE_RDONLY,
      MPI_INFO_NULL, &inFile[i]);
    else MPI_File_open(MPI_COMM_WORLD,
      m_fileName.c_str(), 
      MPI_MODE_RDONLY,
      MPI_INFO_NULL, &inFile[i]);

    if (!inFile[i])
    {
      std::cerr<<"Error while opening block"<<std::endl;
      return -1;
    }
  }

  if(proc_id == 0){
    for (type=0; type<6; type++)
    {
      if (m_pHeaderType2[0].npart[type] != 0)	
      {	
        std::string nameFileBinOut =  m_pathFileOut + tagTypeForNameFile[type].c_str() + bin;
        std::ofstream outFileBin;
        outFileBin.open(nameFileBinOut.c_str(), std::ios::binary /*| ios::app*/);
        outFileBin.close();
      }
    }	
  }    
  MPI_Barrier(MPI_COMM_WORLD);
  int nTotalBlocks = m_listOfBlocks.size() * m_numFiles;
  double t1, t2; 

  //Create consumer communicator
  MPI_Group group_world;
  MPI_Group consumer_group;
  MPI_Comm consumer_comm;
  int proc_ranks[num_proc - 1];
  for(int i = 1; i < num_proc; i++) proc_ranks[i-1] = i;
  MPI_Comm_group(MPI_COMM_WORLD, &group_world); 
  MPI_Group_incl(group_world, num_proc - 1, proc_ranks, &consumer_group);
  MPI_Comm_create(MPI_COMM_WORLD, consumer_group, &consumer_comm);

  if(proc_id == 0)//producer
  {
    MPI_Status status;
    double root;
    MPI_Request r= MPI_REQUEST_NULL;
    for(int blId = 0; blId < nTotalBlocks; blId++){
      int nFile = blId/m_listOfBlocks.size() ;
      int nBlock  = blId % m_listOfBlocks.size();
      MPI_Recv(&root, 1, MPI_DOUBLE, MPI_ANY_SOURCE,
               MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
      int message[] = {nBlock, nFile};
      MPI_Isend(&message, 2, MPI_INT, status.MPI_SOURCE,50, MPI_COMM_WORLD, &r);
    }

    for(int i = 0; i < num_proc -1; i++){
      int message[] = {-1, -1};     
      MPI_Recv(&root, 1, MPI_DOUBLE, MPI_ANY_SOURCE,
               MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      MPI_Isend(&message, 2, MPI_INT, status.MPI_SOURCE,50, MPI_COMM_WORLD, &r);
    }

    for(int file = 0; file < m_numFiles; file++) MPI_File_close(&inFile[file]);    
    return proc_id;
  }
  else{
    std::ios::off_type pWrite=0; int pToStart=0;
    std::ios::off_type pWriteX=0,  pWriteY=0, pWriteZ=0;
    int nBlock;
    int nFile;
    MPI_File outFileBin[6];
    bool alreadyOpen = false;
    
    while(true){
      double test = 0;
      int message[2];
      MPI_Send(&test, 1, MPI_DOUBLE, 0, 50, MPI_COMM_WORLD);
      MPI_Recv(&message, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, NULL);
        
      nBlock = message[0];
      nFile = message[1];
      
      if(message[0] == -1){          
        for(int type = 0; type < 6; type++){
            if(m_pHeaderType2[0].npartTotal[type] != 0) MPI_File_close(&outFileBin[type]);        
        }
        for(int file = 0; file < m_numFiles; file++) MPI_File_close(&inFile[file]);

        return proc_id;
      }
        
      if(!alreadyOpen){
        for(int type = 0; type < 6; type++){
          if(m_pHeaderType2[nFile].npartTotal[type] != 0){
            std::string nameFileBinOut = m_pathFileOut + tagTypeForNameFile[type].c_str() + bin;
            MPI_File_open(consumer_comm, nameFileBinOut.c_str(), MPI_MODE_WRONLY, MPI_INFO_NULL, &outFileBin[type]);
          }    
        }
        alreadyOpen = true;
      }
      
      MPI_File_seek(inFile[nFile], 0, MPI_SEEK_SET);
      do{
        MPI_File_seek(inFile[nFile], 4, MPI_SEEK_CUR);
        MPI_File_read(inFile[nFile], (char *)(tagTmp), 4, MPI_CHAR, &status);
        tag = strtok(tagTmp, " ");
        if(iCompare(tag, m_listOfBlocks[nBlock]) == 0) break;
        MPI_File_read(inFile[nFile], m_sizeBlock, 1, MPI_INT, &status);
        if (needSwap)
          m_sizeBlock[0]=intSwap((char *)(&m_sizeBlock[0]));      
        MPI_File_seek(inFile[nFile], m_sizeBlock[0], MPI_SEEK_CUR);
        MPI_File_read(inFile[nFile], m_sizeBlock, 1, MPI_INT, &status);
      }while(status._ucount >= 4);  
        
      MPI_File_seek(inFile[nFile], 12, MPI_SEEK_CUR);
      //start processing block
      for(type=0; type < 6; type++){
        if (m_pHeaderType2[nFile].npart[type] != 0 && m_pHeaderType2[nFile].npart[type] <= 2500000)
          minPart[type] = m_pHeaderType2[nFile].npart[type];
        else
          minPart[type] = 2500000;
      }
      
      for(int type = 0; type < 6; type++)
      {             
        if(m_pHeaderType2[nFile].npart[type] != 0 && blocksFields[mapBlockNamesToFields[m_listOfBlocks[nBlock]]][type] && 
          (iCompare(m_listOfBlocks[nBlock], "MASS") != 0 || m_pHeaderType2[nFile].mass[type] == 0))
        {  
          pToStart = typePosition[nBlock][type]*m_pHeaderType2[nFile].npartTotal[type] + fileStartPosition[type][nFile];
          chunk = minPart[type];
          n=m_pHeaderType2[nFile].npart[type]/chunk;
          Resto=m_pHeaderType2[nFile].npart[type]-(chunk*n); 
              
          float *bufferBlock=NULL;
          bufferBlock = new float[mapBlockSize[m_listOfBlocks[nBlock]]*chunk];

          if(mapBlockSize[m_listOfBlocks[nBlock]] == 3)
          {
            float *buffer_X=NULL;
            buffer_X = new float[chunk];
            float *buffer_Y=NULL;
            buffer_Y = new float[chunk];
            float *buffer_Z=NULL;
            buffer_Z = new float[chunk];
                
            for (k=0; k<n; k++)
            {    
              MPI_File_read(inFile[nFile], bufferBlock, 3*chunk, MPI_FLOAT, &status);
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
              pWriteX=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)));
              MPI_File_seek(outFileBin[type], pWriteX, MPI_SEEK_SET);  
              MPI_File_write(outFileBin[type], buffer_X, chunk, MPI_FLOAT, &status);

              pWriteY=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)) + (m_pHeaderType2[0].npartTotal[type]*sizeof(float)));
              MPI_File_seek(outFileBin[type], pWriteY, MPI_SEEK_SET);
              MPI_File_write(outFileBin[type], buffer_Y, chunk, MPI_FLOAT, &status);

              pWriteZ=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)) + (2*m_pHeaderType2[0].npartTotal[type]*sizeof(float)));  
              MPI_File_seek(outFileBin[type], pWriteZ, MPI_SEEK_SET);
              MPI_File_write(outFileBin[type], buffer_Z, chunk, MPI_FLOAT, &status);
            }
            /*=================================================================*/
            /*============ Buffer Block and Write out FILE [Resto] ============*/
    
            if(Resto>0){
              MPI_File_read(inFile[nFile], bufferBlock, 3*Resto, MPI_FLOAT, &status);
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
              MPI_File_seek(outFileBin[type], pWriteX, MPI_SEEK_SET);
              MPI_File_write(outFileBin[type], buffer_X, chunk, MPI_FLOAT, &status);

              pWriteY=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)) + (m_pHeaderType2[0].npart[type]*sizeof(float)));
              MPI_File_seek(outFileBin[type], pWriteY, MPI_SEEK_SET);
              MPI_File_write(outFileBin[type], buffer_Y, chunk, MPI_FLOAT, &status);

              pWriteZ=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)) + (2*m_pHeaderType2[0].npart[type]*sizeof(float)));
              MPI_File_seek(outFileBin[type], pWriteZ, MPI_SEEK_SET);
              MPI_File_write(outFileBin[type], buffer_Z, chunk, MPI_FLOAT, &status);
            }
                    /*=================================================================*/  

            delete [] buffer_X;
            delete [] buffer_Y;
            delete [] buffer_Z;
            delete [] bufferBlock;
          }
          else
          {
            for (k=0; k<n; k++)
            {
              MPI_File_read(inFile[nFile], bufferBlock, chunk, MPI_FLOAT, &status);
              if(needSwap)
                for (j=0; j<chunk; j++)
                  bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
              
              pWrite=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)));
              MPI_File_seek(outFileBin[type], pWrite, MPI_SEEK_SET);
              MPI_File_write(outFileBin[type], bufferBlock, chunk, MPI_FLOAT, &status);            
            }

            /*=================================================================*/
            /*============ Buffer Block and Write out FILE [Resto] ============*/

            MPI_File_read(inFile[nFile], bufferBlock, Resto, MPI_FLOAT, &status);
            if( needSwap)
              for (j=0; j<Resto; j++)
                bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
                        
            pWrite=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)));
            MPI_File_seek(outFileBin[type], pWrite, MPI_SEEK_SET);
            MPI_File_write(outFileBin[type], bufferBlock, Resto, MPI_FLOAT, &status);

            delete [] bufferBlock;
          }/*d*/

        }
            
        pWriteX = 0; pWriteY = 0; pWriteZ = 0; 
      }  
    }
  }
}
