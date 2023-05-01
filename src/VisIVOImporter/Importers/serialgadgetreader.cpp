#include "serialgadgetreader.h"
#include "visivoutils.h"
#include "mpi.h"
#include "unordered_map"
#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>

serialGadgetReader::serialGadgetReader(std::string fileName, std::string pathFileOut, int numFiles, std::vector<headerType2> HeaderType2, 
int numBlocks, std::vector<std::string> blockNamesToCompare){
    m_fileName = fileName;
    m_numFiles = numFiles;
    m_pHeaderType2 = HeaderType2;
    m_blockNamesToCompare = blockNamesToCompare;
    m_pathFileOut = pathFileOut;
    m_numBlocks = numBlocks;
}

int serialGadgetReader::startRead(bool needSwap){
    int type, block, i, k, j;
    unsigned long int chunk=0;  //!** Number of Information read & write for cycle **//
    unsigned long int n=0; unsigned long int Resto=0;
    unsigned long long int minPart[6];
    char tagTmp[5]="";
    std::string tag; 
    std::vector<std::string> blockNames;
    std::ios::off_type pWrite=0; int pToStart=0;
    std::ios::off_type pWriteX=0,  pWriteY=0, pWriteZ=0;

    std::ifstream inFile[m_numFiles];

    for(int i = 0; i < m_numFiles; i++){
        if(m_numFiles > 1) inFile[i].open(m_fileName + std::to_string(i), std::ios::binary);
        else inFile[i].open(m_fileName, std::ios::binary);
        
        if (!inFile)
        {
            std::cerr<<"Error while opening block"<<std::endl;
            return -1;
        }
    }
        
    std::vector<unsigned int> dimV{0, 0, 0, 0, 0, 0};
    std::vector<unsigned int> threeDimV{0, 0, 0, 0, 0, 0};

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
    //blockNamesToCompare.push_back("TSTP");//10 move before
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
    /*!=======================================================================*/
    /*=======================================================================*/
    /*!==================== Create final file for TYPE =======================*/
        
    for (type=0; type<6; type++)
    {
        if (m_pHeaderType2[0].npart[type] != 0)	
        {	
        std::string nameFileBinOut =  m_pathFileOut + tagTypeForNameFile[type].c_str() + ".bin";
        std::ofstream outFileBin;
        outFileBin.open(nameFileBinOut.c_str(), std::ios::binary /*| ios::app*/);
        outFileBin.close();
        }
    }	
    /*=======================================================================*/      
        
    /*=======================================================================*/
    /*!======================== Start Processing FILE ========================*/
    
    for(int file = 0; file < m_numFiles; file++){
        inFile[file].seekg(284, std::ios::beg);  //** BEGINNING BLOCK POS **//
        for(type=0; type < 6; type++){
            if (m_pHeaderType2[file].npart[type] != 0 && m_pHeaderType2[file].npart[type] <= 2500000)
                minPart[type] = m_pHeaderType2[file].npart[type];
            else
                minPart[type] = 2500000;
        }    
    }
    int skipBlocks[m_numFiles];

    for (block=0; block < m_numBlocks + 1; block++){
        std::string tagCheck = "";
        for(int file = 0; file < m_numFiles; file++){
            inFile[file].read((char *)(tagTmp), 4*sizeof(char));
            if(tagCheck.length() == 0) tagCheck = strtok(tagTmp, " ");
            tag = strtok(tagTmp, " ");
            
            while(iCompare(tag, tagCheck) != 0){//Find needed block    
                inFile[file].seekg(8, std::ios::cur);
                inFile[file].read((char *)(skipBlocks+file), sizeof(int)); 
                if(skipBlocks[file] < 0) return -1;
                inFile[file].seekg (skipBlocks[file], std::ios::cur);  
                inFile[file].seekg(8, std::ios::cur);     
                inFile[file].read((char *)(tagTmp), 4*sizeof(char));
                tag = strtok(tagTmp, " ");  
            }
            if(file == 0) blockNames.push_back(tag);
            inFile[file].seekg(8, std::ios::cur);
            inFile[file].read((char *)(skipBlocks+file), sizeof(int));     
        }
        /*=========================================================================================================*/

        if (mapBlockNamesToFields.find(tag) !=  mapBlockNamesToFields.end() && iCompare(tag, "Zs") != 0)//13: must define importer for Zs
        {
            for (type=0; type<6; type++)
            {
                int readParticles = 0;
                for (int file = 0; file < m_numFiles; file++){
                    if (m_pHeaderType2[file].npart[type] != 0 && blocksFields[mapBlockNamesToFields[tag]][type] && ( iCompare(tag, "MASS") != 0|| m_pHeaderType2[file].mass[type] == 0))
                    {
                        if(m_numFiles>1) pToStart = dimV[type]*m_pHeaderType2[file].npartTotal[type] + readParticles;
                        else pToStart = dimV[type]*m_pHeaderType2[file].npart[type] + readParticles;
                        chunk = minPart[type];
                        readParticles += m_pHeaderType2[file].npart[type];
                        n=m_pHeaderType2[file].npart[type]/chunk;
                        Resto=m_pHeaderType2[file].npart[type]-(chunk*n);
                        float *bufferBlock=NULL;
                        bufferBlock = new float[mapBlockSize[tag]*chunk];
                        if(mapBlockSize[tag] == 3)
                        {
                            float *buffer_X=NULL;
                            buffer_X = new float[chunk];
                            float *buffer_Y=NULL;
                            buffer_Y = new float[chunk];
                            float *buffer_Z=NULL;
                            buffer_Z = new float[chunk];
                                
                            std::string nameFileBinOut = m_pathFileOut + tagTypeForNameFile[type].c_str() + ".bin";
                            std::ofstream outFileBin;
                            outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in /*| ios::app*/);

                            for (k=0; k<n; k++)
                            {
                                inFile[file].read((char *)(bufferBlock), 3*chunk*sizeof(float));

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
                                outFileBin.seekp(pWriteX);
                                outFileBin.write ((char *)(buffer_X), chunk*sizeof(float));

                                pWriteY=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)) + (m_pHeaderType2[0].npartTotal[type]*sizeof(float)));
                                outFileBin.seekp(pWriteY);
                                outFileBin.write ((char *)(buffer_Y), chunk*sizeof(float));

                                pWriteZ=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)) + (2*m_pHeaderType2[0].npartTotal[type]*sizeof(float)));
                                outFileBin.seekp(pWriteZ);
                                outFileBin.write ((char *)(buffer_Z), chunk*sizeof(float));

                                /*=======================================================================*/
                            }

                            /*=================================================================*/
                            /*============ Buffer Block and Write out FILE [Resto] ============*/
        
                    
                            inFile[file].read((char *)(bufferBlock), 3*Resto*sizeof(float));
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
                            outFileBin.seekp(pWriteX);
                            outFileBin.write ((char *)(buffer_X), Resto*sizeof(float));

                            pWriteY=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)) + (m_pHeaderType2[file].npartTotal[type]*sizeof(float)));
                            outFileBin.seekp(pWriteY);
                            outFileBin.write ((char *)(buffer_Y), Resto*sizeof(float));

                            pWriteZ=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)) + (2*m_pHeaderType2[file].npartTotal[type]*sizeof(float)));
                            outFileBin.seekp(pWriteZ);
                            outFileBin.write ((char *)(buffer_Z), Resto*sizeof(float));

                            /*=================================================================*/  

                            delete [] buffer_X;
                            delete [] buffer_Y;
                            delete [] buffer_Z;

                            delete [] bufferBlock;
                            outFileBin.close();
                        }

                        else
                        {
                            std::string nameFileBinOut =  m_pathFileOut + tagTypeForNameFile[type].c_str() + ".bin";
                            std::ofstream outFileBin;
                            outFileBin.open(nameFileBinOut.c_str(), std::ios::binary | std::ios::in);

                            for (k=0; k<n; k++)
                            {
                            inFile[file].read((char *)(bufferBlock), chunk*sizeof(float));
                            if( needSwap)
                                for (j=0; j<chunk; j++)
                                bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
                                
                            pWrite=((pToStart*sizeof(float)) + (k*chunk*sizeof(float)));
                            outFileBin.seekp(pWrite);
                            outFileBin.write ((char *)(bufferBlock), chunk*sizeof(float));
                            }
                            inFile[file].read((char *)(bufferBlock), Resto*sizeof(float));
                        
                            if( needSwap)
                            for (j=0; j<Resto; j++)
                                bufferBlock[j]=floatSwap((char *)(&bufferBlock[j]));
                            
                            pWrite=((pToStart*sizeof(float)) + (n*chunk*sizeof(float)));
                            outFileBin.seekp(pWrite);
                            outFileBin.write ((char *)(bufferBlock), Resto*sizeof(float));

                            delete [] bufferBlock;
                            outFileBin.close();
                        }
                    }
                    else
                    {
                        n = 0;
                        Resto = 0;
                    }
                }    
                dimV[type] += mapBlockSize[tag];
                pWriteX = 0; pWriteY = 0; pWriteZ = 0; 
            }
        }   
        else
        {        
            for(int file = 0; file < m_numFiles; file++) inFile[file].seekg (skipBlocks[file], std::ios::cur);
        }
        /*=================================================================*/
        for(int file = 0; file < m_numFiles; file++)  inFile[file].seekg (8, std::ios::cur);
    }
    for(int file = 0; file < m_numFiles; file++) inFile[file].close();
    return 1;
}