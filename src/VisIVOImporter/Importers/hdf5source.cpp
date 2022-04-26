/***************************************************************************
 *   Copyright (C) 2008 by Gabriella Caniglia 				   *
 *  gabriella.caniglia@oact.inaf.it 					   *
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

#include "hdf5source.h"
#include "visivoutils.h"
#include <iostream>
#include <fstream>
#include <sstream>
std::vector<std::string> commonString;
static herr_t file_info(hid_t loc_id, const char *name, const H5O_info_t *info, void *op_data)
{
    switch (info->type)
    {
    case H5O_TYPE_GROUP:
    {
        std::string gName(name);
        std::cout << "Object with name " << gName << " is a group" << std::endl;
        break;
    }
    case H5O_TYPE_DATASET:
    {
        std::string dName(name);
        std::cout << " Object with name " << dName << " is a dataset" << std::endl;
        commonString.push_back(dName);
        break;
    }
    default:
        std::cout << " Unable to identify an object" << std::endl;
    }
    return 0;
}

//---------------------------------------------------------------------
void HDF5Source::readVolume()
//---------------------------------------------------------------------
{
    std::ofstream outFile;
    size_t found = m_pointsBinaryName.find(".bin");
    if (found == std::string::npos)
        m_pointsBinaryName = m_pointsBinaryName + ".bin";

    outFile.open(m_pointsBinaryName.c_str(), std::ofstream::binary);

    int nOfEle;
    if (m_hyperslabStruct[0].count[2] <= MAX_LOAD)
        nOfEle = (int)m_hyperslabStruct[0].count[2];
    else
        nOfEle = MAX_LOAD;

    float *farray;
    farray = new float[nOfEle];

    for (int k = 0; k < m_nOfDatasets; k++)
    {
#ifdef LIGHT

#ifdef WIN32
        hid_t sourceObj = H5Dopen(m_sourceId, m_hyperslabStruct[k].datasetName.c_str());
#else
        // hid_t sourceObj = H5Dopen(m_sourceId,m_hyperslabStruct[k].datasetName.c_str());
        hid_t sourceObj = H5Dopen(m_sourceId, m_hyperslabStruct[k].datasetName.c_str(), H5P_DEFAULT);
#endif

#else

#ifdef WIN32
        hid_t sourceObj = H5Dopen(m_sourceId, m_hyperslabStruct[k].datasetName.c_str());
#else

        //  hid_t sourceObj = H5Dopen(m_sourceId,m_hyperslabStruct[k].datasetName.c_str(),H5P_DEFAULT);
        hid_t sourceObj = H5Dopen(m_sourceId, m_hyperslabStruct[k].datasetName.c_str());

#endif
#endif
        hid_t sourceSpace = H5Dget_space(sourceObj);

        hsize_t offset[3];
        hsize_t count[3];
        // read/write plane by plane
        for (int i = 0; i < m_hyperslabStruct[k].count[0]; i++)
            for (int j = 0; j < m_hyperslabStruct[k].count[1]; j++)
            {

                offset[0] = m_hyperslabStruct[k].offset[0] + i;
                offset[1] = m_hyperslabStruct[k].offset[1] + j;
                count[0] = 1;
                count[1] = 1;
                unsigned long long int totEle = m_hyperslabStruct[k].count[2];
                int readEle = nOfEle;
                int gcount = 0;
                while (totEle > 0)
                {

                    offset[2] = m_hyperslabStruct[k].offset[2] + gcount * readEle;
                    count[2] = readEle; // nOfEle elementi lungo la terza dimensione
                    hsize_t s_count[1];
                    s_count[0] = readEle;
                    int srank = 1;
                    hid_t memoryspace = H5Screate_simple(srank, s_count, s_count);
                    herr_t status = H5Sselect_hyperslab(sourceSpace, H5S_SELECT_SET, offset, NULL, count, NULL);
                    if (status != 0)
                    {
                        std::cerr << "Invalid hyperslab selection. Importer Aborted" << std::endl;
                        delete[] farray;
                        return;
                    }
                    status = H5Dread(sourceObj, H5T_NATIVE_FLOAT, memoryspace, sourceSpace, H5P_DEFAULT, farray);
                    if (status != 0)
                    {
                        std::cerr << "Invalid hyperslab read. Importer Aborted" << std::endl;
                        delete[] farray;
                        return;
                    }
                    outFile.write((char *)(farray), sizeof(float) * (readEle));
                    if (totEle == readEle)
                        totEle = 0;
                    else
                        totEle = -readEle; //**QUI verifica shorthand
                    if (totEle < readEle)
                        readEle = totEle;
                    gcount++;
                } // while
            }     // for(j
        H5Dclose(sourceObj);

    } // for(k
    outFile.close();
    delete[] farray;
}
//---------------------------------------------------------------------
void HDF5Source::readTable()
//---------------------------------------------------------------------
{
    std::ofstream outFile;
    size_t found = m_pointsBinaryName.find(".bin");
    if (found == std::string::npos)
        m_pointsBinaryName = m_pointsBinaryName + ".bin";

    outFile.open(m_pointsBinaryName.c_str(), std::ofstream::binary);
    if (!outFile.is_open())
    {
        std::cerr << "HDF5 Importer: unable to open file " << m_pointsBinaryName
                  << std::endl
                  << "Operation Aborted" << std::endl;
        return;
    }

    int nOfEle;
    if (m_maxNumberOfRows <= MAX_LOAD)
        nOfEle = (int)m_maxNumberOfRows;
    else
        nOfEle = MAX_LOAD;

    float *farray;
    farray = new float[nOfEle];
    int rank;

    for (int k = 0; k < m_nOfDatasets; k++)
    {
        // std::cout<<"Dataset : "<<m_hyperslabStruct[k].datasetName.c_str()<<std::endl;
#ifdef LIGHT

#ifdef WIN32
        hid_t sourceObj = H5Dopen(m_sourceId, m_hyperslabStruct[k].datasetName.c_str());

#else
        //  hid_t sourceObj = H5Dopen(m_sourceId,m_hyperslabStruct[k].datasetName.c_str());
        hid_t sourceObj = H5Dopen(m_sourceId, m_hyperslabStruct[k].datasetName.c_str(), H5P_DEFAULT);
#endif

#else

#ifdef WIN32
        hid_t sourceObj = H5Dopen(m_sourceId, m_hyperslabStruct[k].datasetName.c_str());

#else
        // hid_t sourceObj = H5Dopen(m_sourceId,m_hyperslabStruct[k].datasetName.c_str(),H5P_DEFAULT);
        hid_t sourceObj = H5Dopen(m_sourceId, m_hyperslabStruct[k].datasetName.c_str());
#endif
#endif
        hid_t sourceSpace = H5Dget_space(sourceObj);
        int rank = H5Sget_simple_extent_ndims(sourceSpace); // rank of dataset

        hsize_t *offset = (hsize_t *)calloc(rank, sizeof(hsize_t));
        hsize_t *count = (hsize_t *)calloc(rank, sizeof(hsize_t));
        //    for(int i=0;i<rank-1;i++)
        for (int i = 0; i < rank; i++)
            count[i] = 1;
        // read/write plane by plane
        bool endReached = false;
        for (int i = 0; i < rank; i++)
            offset[i] = m_hyperslabStruct[k].offset[rank - i - 1];

        while (!endReached)
        {
            int readEle = 0;
            if (m_hyperslabStruct[k].count[rank - 1] > nOfEle)
                readEle = nOfEle;
            else
                readEle = m_hyperslabStruct[k].count[rank - 1];
            // unsigned long long int totEle=m_hyperslabStruct[k].count[rank-1]-m_hyperslabStruct[k].offset[rank-1];
            unsigned long long int totEle = m_maxNumberOfRows;
            // std::cout<<"totEle : "<<totEle<<std::endl;
            while (totEle > 0)
            {
                // count[rank-1]=readEle;  //nOfEle elementi lungo la terza dimensione
                count[0] = readEle;
                hsize_t s_count[1];
                s_count[0] = readEle;
                // std::cout<<readEle<<std::endl;
                int srank = 1;
                // hid_t memoryspace = H5Screate_simple (srank, s_count, s_count);
                hid_t memoryspace = H5Screate_simple(srank, count, count);
                herr_t status = H5Sselect_hyperslab(sourceSpace, H5S_SELECT_SET, offset, NULL, count, NULL);
                // std::clog<<offset[0]<<" "<<offset[1]<<" "<<offset[2]<<" <== Offset"<<std::endl;
                // std::clog<<count[0]<<" "<<count[1]<<" "<<count[2]<<" <== Count"<<std::endl;
                if (status != 0)
                {
                    std::cerr << "Invalid hyperslab selection. Importer Aborted" << std::endl;
                    delete[] farray;
                    return;
                }
                status = H5Dread(sourceObj, H5T_NATIVE_FLOAT, memoryspace, sourceSpace, H5P_DEFAULT, farray);
                if (status != 0)
                {
                    std::cerr << "Invalid hyperslab read. Importer Aborted" << std::endl;
                    delete[] farray;
                    return;
                }
                outFile.write((char *)(farray), sizeof(float) * (readEle));
                //	  std::clog<<readEle<<" "<<totEle<<" "<<offset[rank-1]<<std::endl;
                if (totEle == readEle)
                    totEle = 0;
                else
                    totEle = totEle - readEle;
                //	  std::clog<<readEle<<" "<<totEle<<" "<<offset[rank-1]<<std::endl;
                //	  offset[rank-1]=offset[rank-1]+readEle;
                //	  std::clog<<readEle<<" "<<totEle<<" "<<offset[rank-1]<<std::endl;
                if (totEle < readEle)
                    readEle = totEle;
                //	  std::clog<<readEle<<" "<<totEle<<" "<<offset[rank-1]<<std::endl;
            }                                                             // while(totEle
            if (m_hyperslabStruct[k].count[rank - 1] < m_maxNumberOfRows) // pad array if necessary
            {
                totEle = m_maxNumberOfRows - m_hyperslabStruct[k].count[rank - 1];
                readEle = 0;
                if (totEle > nOfEle)
                    readEle = nOfEle;
                else
                    readEle = totEle;
                while (totEle > 0)
                {
                    for (int i = 0; i < readEle; i++)
                        farray[i] = MISSING_VALUE;
                    outFile.write((char *)(farray), sizeof(float) * (readEle));
                    if (totEle == readEle)
                        totEle = 0;
                    else
                        totEle = totEle - readEle;
                    if (totEle < readEle)
                        readEle = totEle;
                }
            }
            /* advance next rows on hyperslab
             //      offset[rank-1]=m_hyperslabStruct[k].offset[rank-1]; //reset starting hyperslab in the column
             if(rank==1) endReached=true;
             for (int i=rank-2;i>=0;i--)
             {
             offset[i]++;
             if(i==0 && offset[i]==m_hyperslabStruct[k].offset[i]+m_hyperslabStruct[k].count[i])
             {
             endReached=true;
             break;
             }
             if(offset[i]> (m_hyperslabStruct[k].count[i]+m_hyperslabStruct[k].offset[i])-1)
             offset[i]=m_hyperslabStruct[k].offset[i];
             else break;
             }
             */
            if (rank == 1)
                endReached = true;
            for (int i = 1; i < rank; i++)
            {
                offset[i]++;
                // std::cout<<"offset "<<offset[i]<<" "<<m_hyperslabStruct[k].offset[0]+m_hyperslabStruct[k].count[0]<<std::endl;
                if (i == rank - 1 && offset[i] == m_hyperslabStruct[k].offset[0] + m_hyperslabStruct[k].count[0])
                {
                    endReached = true;
                    break;
                }
                if (offset[i] > (m_hyperslabStruct[k].count[rank - i - 1] + m_hyperslabStruct[k].offset[rank - i - 1]) - 1)
                    offset[i] = m_hyperslabStruct[k].offset[rank - i - 1];
                else
                    break;
            }

        } // while !endReached
        H5Dclose(sourceObj);
    } // for(k
    outFile.close();
    delete[] farray;
}
//---------------------------------------------------------------------
bool HDF5Source::checkhyperslab()
//---------------------------------------------------------------------
// Note: if volume is set, each dataset MUST have rank=3 and each dataset MUST have
// the same dimension. They are volumetric fields.
// Otherwhise datasets having more than rank=1 are considered al multicolumn.
// Each column will be reported in the VBT.
{
    int volumeDef[3];
    int *rank;
    rank = new int[m_nOfDatasets];
    int totalNumberOfRanks = 0;
    int maxNumberOfRows = 0;
    hid_t *sourceObj;
    hid_t *sourceSpace;
    sourceObj = new hid_t[m_nOfDatasets];
    sourceSpace = new hid_t[m_nOfDatasets];
    hyperdef element;
    m_maxNumberOfRows = 0;

    for (int k = 0; k < m_nOfDatasets; k++)
    {
        // Open a dataset in the file

#ifdef LIGHT

#ifdef WIN32
        sourceObj[k] = H5Dopen(m_sourceId, m_vDatasetList[k].c_str());
#else
        // sourceObj[k] = H5Dopen(m_sourceId,m_vDatasetList[k].c_str());
        sourceObj[k] = H5Dopen(m_sourceId, m_vDatasetList[k].c_str(), H5P_DEFAULT);
#endif

#else
#ifdef WIN32
        sourceObj[k] = H5Dopen(m_sourceId, m_vDatasetList[k].c_str());

#else
        //   sourceObj[k] = H5Dopen(m_sourceId,m_vDatasetList[k].c_str(),H5P_DEFAULT);
        sourceObj[k] = H5Dopen(m_sourceId, m_vDatasetList[k].c_str());

#endif
#endif

        sourceSpace[k] = H5Dget_space(sourceObj[k]);
        rank[k] = H5Sget_simple_extent_ndims(sourceSpace[k]); // rank of dataset
        if (m_volumeOrTable == "volume" && rank[k] != 3)
        {
            std::cerr << "HDF5 Importer: volume option is given but dataset " << m_vDatasetList[k]
                      << " has rank= " << rank[k] << " . The rank must be 3."
                      << std::endl
                      << "Operation Aborted" << std::endl;
            return false;
        }
        if (rank[k] > 1000)
        {
            std::cerr << "HDF5 Importer: dataset " << m_vDatasetList[k]
                      << " has rank= " << rank[k] << " . The rank must be lower tha 1000."
                      << std::endl
                      << "Operation Aborted" << std::endl;
            return false;
        }

        hsize_t *sMaxdims = new hsize_t[rank[k]];
        hsize_t *sDims = new hsize_t[rank[k]];
        hsize_t tmpsDims;

        // sDims actual dimension of array, sMaxdims is the maximum size (in our case is not significant)
        H5Sget_simple_extent_dims(sourceSpace[k], sDims, sMaxdims);
        element.datasetName = m_vDatasetList[k];

        if (m_volumeOrTable != "volume")
        {
            for (int i = 0, j = rank[k] - 1; i < j; i++, j--)
            {
                tmpsDims = sDims[i];
                sDims[i] = sDims[j];
                sDims[j] = tmpsDims;
            }

            // std::cout<<sDims[0]<<" "<<sDims[1]<<std::endl;
        }

        for (int j = 0; j < rank[k]; j++)
            element.offset[j] = 0;
        for (int j = 0; j < rank[k]; j++)
            element.count[j] = sDims[j];

        // check for hyperslab modification

        for (int i = 0; i < m_hyperslab.size(); i++)
        {
            std::stringstream sstmp(m_hyperslab[i]);
            std::string name;
            sstmp >> name; // dataset name
            // check for dataset
            if (name != element.datasetName)
                continue;
            int countextract = 0;
            while (!sstmp.eof()) // dataset hyperslab limits are given
            {
                std::string tmp;
                sstmp >> tmp;
                for (int j = 0; j < rank[k]; j++)
                {
                    size_t pos = tmp.find_first_of(',');
                    std::stringstream ss1;
                    if (j < 2)
                        ss1 << tmp.substr(0, pos);
                    else
                        ss1 << tmp;
                    if (countextract == 0)
                    {
                        if (m_volumeOrTable == "volume")
                            ss1 >> element.offset[j];
                        else
                            ss1 >> element.offset[rank[k] - 1 - j];
                    }
                    if (countextract == 1)
                    {
                        if (m_volumeOrTable == "volume")
                            ss1 >> element.count[j];
                        else
                            ss1 >> element.count[rank[k] - 1 - j];
                    }
                    if (pos == std::string::npos)
                        break;
                    tmp.erase(0, pos + 1);
                }
                countextract++;
            } // while(!sstmp.eof())
        }     // for (int i=0....

        if (element.count[0] == 0)
        {
            std::cerr << "HDF5 Importer: dataset " << m_vDatasetList[k]
                      << " has invalid count number. The count number must be greater than 0."
                      << std::endl
                      << "Operation Aborted" << std::endl;
            return false;
        }

        // adjust hyperslab if necessary
        for (int i = 0; i < rank[k]; i++)
            if (element.count[i] > sDims[i] - element.offset[i])
            {
                element.count[i] = sDims[i] - element.offset[i];
                std::cerr << "Warning: count elements in rank " << i << " of hyperslab  "
                          << m_vDatasetList[k].c_str() << " is reduced to " << element.count[i]
                          << std::endl;
            }
        m_hyperslabStruct.push_back(element);

        if (m_volumeOrTable == "volume")
            m_fieldNames.push_back(element.datasetName);
        else if (rank[k] == 1)
            m_fieldNames.push_back(element.datasetName);
        else
        {
            int *offsetName;
            offsetName = new int[rank[k]];
            for (int i = 0; i < rank[k]; i++)
                offsetName[i] = element.offset[i];
            bool endReached = false;
            while (!endReached)
            {
                std::stringstream nameCol;
                nameCol << element.datasetName;
                nameCol << "_";
                for (int ii = 0; ii < rank[k] - 1; ii++)
                {
                    nameCol << offsetName[ii];
                    if (ii < rank[k] - 2)
                        nameCol << "_";
                }
                std::string stm = nameCol.str();
                // std::clog<<stm<<std::endl;
                m_fieldNames.push_back(stm);
                for (int i = rank[k] - 2; i >= 0; i--)
                {
                    offsetName[i]++;
                    if (i == 0 && offsetName[i] == element.offset[i] + element.count[i])
                        endReached = true;
                    if (offsetName[i] > (element.count[i] + element.offset[i]) - 1)
                        offsetName[i] = element.offset[i];
                    else
                        break;
                }
            }
        }

        // m_maxNumberOfRows=element.count[rank[k]-1];
        m_maxNumberOfRows = element.count[rank[k] - 1] - element.offset[rank[k] - 1];
        if (k > 0 && m_volumeOrTable == "volume") // hyperslab must be all equals (rank==3)
            for (int j = 0; j < 3; j++)
                if (element.count[j] != m_hyperslabStruct[0].count[j])
                {
                    std::cerr << "HDF5 Importer: dataset " << m_vDatasetList[k]
                              << " has hyperslab  extension on rank " << j
                              << " equal to " << element.count[j] - element.offset[j] << std::endl;
                    std::cerr << "This is different from dataset " << m_vDatasetList[0]
                              << " that is "
                              << m_hyperslabStruct[0].count[j] - m_hyperslabStruct[0].offset[j] << std::endl
                              << "Operation Aborted" << std::endl;
                    return false;
                }
        H5Dclose(sourceObj[k]);
    } // for (int k=0; k<m_nOfDatasets; k++)
    // Note in case of volume with more than one hyperslab , the first
    // hyperslab set the volume dimension. Strongly suggested to give the same
    // hyperlab extension
    // in case of volume check hyperslab with --compx-y-z if given, and num Of rows
    if (m_volumeOrTable == "volume")
    {
        m_maxNumberOfRows = 1;
        for (int i = 0; i < 3; i++)
        {
            m_cellComp[i] = m_hyperslabStruct[0].count[i];
            m_maxNumberOfRows = m_maxNumberOfRows * (m_hyperslabStruct[0].count[i]);
        }
    }

    return true;
}
//---------------------------------------------------------------------
int HDF5Source::readHeader()
//---------------------------------------------------------------------
{
    //! Open HDF5 file:
    m_invalidFile = false;
    m_sourceId = H5Fopen(m_pointsFileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT); //! Open the HDF5 file
    if (m_sourceId < 0)
    {
        std::cerr << "HDF5 Error. Invalid hdf5 filename. File not opened. Importer aborted." << std::endl;
        return 2;
    }
    // H5L_info_t li;
    // char oname[] = "/";
    // H5Lget_info(m_sourceId, oname, &li, H5P_DEFAULT);

    // execute test on valid datasets
    std::stringstream ss1(m_datasetList);
    bool noDataSetGiven = true;
    while (!ss1.eof())
    {
        std::string strtmp;
        ss1 >> strtmp;
        if (strtmp == "")
            continue;
        noDataSetGiven = false;
        htri_t tf = H5Lexists(m_sourceId, strtmp.c_str(), H5P_DEFAULT);
        if (tf > 0)
            m_vDatasetList.push_back(strtmp);
        if (tf == 0)
            std::cerr << "Invalid dataset " << strtmp << std::endl;
        if (tf < 0)
        {
            std::cerr << "Error hdf5 file searching for dataset"
                      << strtmp << std::endl
                      << "Operation aborted" << std::endl;
            return 3;
        }
    }
    if (noDataSetGiven)
    {
        H5Ovisit(m_sourceId, H5_INDEX_NAME, H5_ITER_NATIVE, file_info, NULL, NULL);
        for (int i = 0; i < commonString.size(); i++)
            m_vDatasetList.push_back(commonString[i]);
    }
    m_nOfDatasets = m_vDatasetList.size();
    if (m_nOfDatasets == 0)
    {
        std::cerr << "HDF5 Error. Invalid datasets. Importer aborted." << std::endl;
        return 2;
    }

    if (checkhyperslab())
        makeHeader(m_maxNumberOfRows, m_pointsBinaryName, m_fieldNames, m_cellSize, m_cellComp, m_volumeOrTable);
    else
    {
        m_invalidFile = true;
        return 1;
    }
    return 0;
}

//---------------------------------------------------------------------
int HDF5Source::readData()
//---------------------------------------------------------------------
{
    if (m_invalidFile)
        return 1;
    if (m_volumeOrTable == "volume")
        readVolume();
    else
        readTable();
    H5Fclose(m_sourceId);
    return 0;
    /*
     int nLoad=9999999/3;
     //TEST: hyperslab per una lettura per colonna
     hsize_t offset[1];
     hsize_t count[1];
     float dataarray[m_maxNumberOfRows];
     count[0]=10;
     offset[0]=2;
     hsize_t s_count[1];
     s_count[0]=10;

     herr_t status=H5Sselect_hyperslab(sourceSpace[0], H5S_SELECT_SET, offset, NULL,count, NULL);

     H5Dread(sourceObj[0], H5T_NATIVE_FLOAT, memoryspace, sourceSpace[0], H5P_DEFAULT, dataarray);

     */
}
