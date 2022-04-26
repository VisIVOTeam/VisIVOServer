/***************************************************************************
 *   Copyright (C) 2008 by Ugo Becciani *
 *  ugo.becciani@oact.inaf.it *
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
#include <cstdlib>
#include <cstring>
#include "xmlsource.h"
#include "visivoutils.h"

#include "asciisource.h"
#include "csvsource.h"
#include "binsource.h"
#include "flysource.h"
#include "vosourcenew.h"
#include "gadgetsource.h"
#include "xmlsource.h"
#include "fitstablesource.h"
#include "fitsimagesource.h"
#include "hdf5source.h"
#include "rawgridsource.h"
#include "rawpointssource.h"

#include "VOTableParser.h"
//#include "voFieldParam.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <time.h>

const unsigned int XmlSource::MAX_NUMBER_TO_REDUCE_ROW = 10000;
const unsigned int XmlSource::MIN_NUMBER_OF_ROW = 100;
const unsigned int XmlSource::MAXINT = 250000000;

//---------------------------------------------------------------------
XmlSource::~XmlSource()
//---------------------------------------------------------------------
{
	if (m_fArray)
		delete[] m_fArray;
	if (m_fReadArray)
		delete[] m_fReadArray;
}

//---------------------------------------------------------------------
bool XmlSource::downloadFiles()
//---------------------------------------------------------------------
//! Metodo per scaricare TUTTI i file remoti in locale
{
	char *href;
	std::string fileName;
	std::string localFileName, workingDir = getDir(m_pointsBinaryName);
	int status = 0;
	int refFile = 0;
	int colNameId = -1;
	std::set<std::string> fileNameSet; //! set of file to be downloaded
	std::set<std::string>::iterator iter;
	/*  CURL *curl;
	  CURLcode result;
	  static char errorBuffer[CURL_ERROR_SIZE];
	   FILE * buffer;*/
	bool remoteOp;

	if (m_numIdHref < 0)
		return false;

	for (int i = 0; i < m_nRows; i++) //! scarica i file da remoto di ogni riga della tabella
	{
		m_sPP.SetCurrentTR(i);
		std::clog << " 1." << m_sPP.GetData(m_numIdHref) << std::endl;
		fileNameSet.insert(m_sPP.GetData(m_numIdHref));
	}
	std::clog << " 2." << fileNameSet.size() << std::endl;
	for (iter = fileNameSet.begin(); iter != fileNameSet.end(); iter++)
	{
		fileName = *iter;
		size_t pos = fileName.find_last_of("/") + 1;
		localFileName = getDir(m_pointsBinaryName);
		//     localFileName=workingDir;
		localFileName = localFileName.append(fileName.substr(pos));
		localFileName.append("_VisIVOImporter_");
		std::stringstream seq;
		seq << refFile;
		refFile++;
		std::string num;
		seq >> num;
		localFileName.append(num);
		if (fileName.substr(0, 7) == "http://" || fileName.substr(0, 7) == "sftp://" || fileName.substr(0, 6) == "ftp://")
		{
			if (!remoteDownloadFiles(fileName, m_login, localFileName))
				return false;
		}
		else
		{
			std::ifstream fileInput(fileName.c_str(), std::ios::in | std::ios::binary);
			if (!fileInput)
			{
				std::cerr << "Cannot open or access  local file" << fileName << std::endl;
				return false;
			}
			std::ofstream fileOutput(localFileName.c_str(), std::ios::out | std::ios::binary);
			fileOutput << fileInput.rdbuf();
			fileOutput.close();
			fileInput.close();
		}
		m_outfileMap.insert(make_pair(localFileName, fileName));
	}
	/*//     fileName="ftp://astrct.oact.inaf.it/ic.tar";
	//     fileName="http://www.oact.inaf.it/pma/prova.pro";
	//     fileName="http://www.oact.inaf.it/preprints/preprint/lanza19.pdf";
	//     fileName="scp ube@astrct.oact.inaf.it:/home/ube/ic.tar";
	//     std::clog<<"1. fileName="<<fileName<<std::endl;
		 curl = curl_easy_init();
		 if (curl)
		 {
		  buffer=fopen(localFileName.c_str(),"wb");

		   //! Now set up all of the curl options
		   curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
		   curl_easy_setopt(curl, CURLOPT_URL,fileName.c_str());
		   curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	//       curl_easy_setopt(curl, CURLOPT_USERPWD, "username:password");
		   if(m_login !="nouser") curl_easy_setopt(curl, CURLOPT_USERPWD, m_login.c_str());
	//       curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	//       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);  // QUI NON SO!!
		   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		   curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

		   //! Attempt to retrieve the remote page
		   result = curl_easy_perform(curl);
		   fclose(buffer);
		   if(result != CURLE_OK)  //!check if LOCAL file and copy it!!
		   {
		  std::ifstream fileInput(fileName.c_str(), std::ios::in | std::ios::binary);
		  if(!fileInput)
		  {
				//! Always cleanup
				curl_easy_cleanup(curl);
					std::cerr << "Error: [" << result << "] - " << errorBuffer;
			std::cerr<<"Cannot open or access  file"<<fileName<<std::endl;
				return false;
		  }
		  std::ofstream fileOutput(localFileName.c_str(), std::ios::out | std::ios::binary);
		  fileOutput << fileInput.rdbuf();
		  fileOutput.close();
		  fileInput.close();
		  }
		   //! Always cleanup
		   curl_easy_cleanup(curl);
		m_outfileMap.insert(make_pair(localFileName,fileName));
		}*/
	/*  }
	  std::cout<<"Downloaded file in local files "<<std::endl;*/
	return true;
}
//---------------------------------------------------------------------
bool XmlSource::readSpecifiedFormat()
//---------------------------------------------------------------------

{
	char *tmpChar;
	int status;

	std::map<std::string, std::string>::iterator it;

	if (m_numIdFormat != -1 &&
		m_numIdHref != -1 &&
		m_numIdEndianism != -1 &&
		m_numIdField != -1 &&
		m_numIdOffset != -1 &&
		m_numIdStride != -1 &&
		m_numIdRank != -1 &&
		m_numIdArraysize != -1 &&
		m_numIdType != -1 &&
		m_numIdPrecision != -1)
		return false; //! complete binary description is given

	for (int j = 0; j < m_nRows; j++)
	{
		AbstractSource *xmlSource;
		m_sPP.SetCurrentTR(j);							 //! fissa la riga
		std::string type = m_sPP.GetData(m_numIdFormat); //! getta la colonna

		if (type == "ascii" || type == "ASCII")
			xmlSource = new AsciiSource();

		else if (type == "fitstable" || type == "FITSTABLE")
			xmlSource = new FitsTableSource();

		else if (type == "fitsimage" || type == "FITSIMAGE")
			xmlSource = new FitsImageSource();

		else if (type == "csv" || type == "CSV")
			xmlSource = new CSVSource();

		else if (type == "votable" || type == "VOTABLE")
			xmlSource = new VOSourcenew();

		else if (type == "gadget" || type == "GADGET")
			xmlSource = new GadgetSource();

		else if (type == "hdf5" || type == "HDF5")
			xmlSource = new HDF5Source();

		else
			continue;

		std::string href = m_sPP.GetData(m_numIdHref); //! getta la colonna
		it = m_outfileMap.find(href);
		if (it != m_outfileMap.end())
		{
			std::cerr << "Invalid href filename " << href << std::endl;
			continue;
		}
		std::string outFileName = it->second + ".bin";
		// DISCUTERE CON GABRY
		/*   xmlSource->setPointsFileName(it->second.c_str(),outFileName.c_str());
			xmlSource->readHeader();
			xmlSource->readData();*/
		// DISCUTERE CON GABRY

		delete xmlSource;
	}
	return true;
}
//---------------------------------------------------------------------

//---------------------------------------------------------------------
int XmlSource::readHeader()
//---------------------------------------------------------------------
{

	m_fieldNames.clear();
	m_tvoXmlList.clear();
	std::string name, value, href;

	int status;

	m_sPP.Init();

	if (m_sPP.Parse(const_cast<char *>(m_pointsFileName.c_str())))
		return 1;
	m_sPP.SetCurrentResource(0);
	m_sPP.SetCurrentTable(0);
	std::string m_tableDescription = m_sPP.GetTableDescription(0);
	m_nCols = m_sPP.GetResourceTableFieldsCount();
	m_nRows = m_sPP.GetResourceTRCount(0);

	std::string tmpStr = "";
	int tmpInt = 0;
	char *tmpChar;
	std::map<std::string, unsigned long long int>::iterator it;
	std::map<std::string, std::string>::iterator itgeo;

	for (int i = 0; i < m_nCols; i++)
	{
		voFieldParam f;

		tmpStr = m_sPP.GetResourceTableFieldArraySize(i);
		f.setArraySize(const_cast<char *>(tmpStr.c_str()), &status);

		tmpStr = m_sPP.GetResourceTableFieldDescription(i);
		f.setDescription(const_cast<char *>(tmpStr.c_str()), &status);

		tmpStr = m_sPP.GetResourceTableFieldId(i);
		f.setID(const_cast<char *>(tmpStr.c_str()), &status);

		tmpStr = m_sPP.GetResourceTableFieldName(i);
		f.setName(const_cast<char *>(tmpStr.c_str()), &status);

		tmpStr = m_sPP.GetResourceTableFieldPrecision(i);
		f.setPrecision(const_cast<char *>(tmpStr.c_str()), &status);

		tmpStr = m_sPP.GetResourceTableFieldRef(i);
		f.setRef(const_cast<char *>(tmpStr.c_str()), &status);

		tmpStr = m_sPP.GetResourceTableFieldUcd(i);
		f.setUCD(const_cast<char *>(tmpStr.c_str()), &status);

		tmpStr = m_sPP.GetResourceTableFieldUnit(i);
		f.setUnit(const_cast<char *>(tmpStr.c_str()), &status);

		tmpStr = m_sPP.GetResourceTableFieldDataType(i);
		f.setDatatype(const_cast<char *>(tmpStr.c_str()), &status);

		tmpInt = m_sPP.GetResourceTableFieldWidth(i);
		f.setWidth(tmpInt, &status);

		m_tvoXmlList.push_back(f);
	}

	m_numIdSpecies = -1;
	m_numIdOffset = -1;
	m_numIdHref = -1;
	m_numIdField = -1;
	m_numIdPrecision = -1;
	m_numIdType = -1;
	m_numIdStride = -1;
	m_numIdFormat = -1;
	m_numIdEndianism = -1;
	m_numIdRank = -1;
	m_numIdArraysize = -1;

	for (int i = 0; i < m_tvoXmlList.size(); i++) // QUI da vedere
	{
		m_tvoXmlList[i].getName(tmpChar, &status);
		tmpStr.assign(tmpChar);

		//         std::clog<<"i= "<<i<<" "<<tmpStr<<std::endl;
		if (tmpStr == "Species")
			m_numIdSpecies = i;
		if (tmpStr == "Arraysize")
			m_numIdArraysize = i;
		if (tmpStr == "Field")
			m_numIdField = i;
		if (tmpStr == "Precision")
			m_numIdPrecision = i;
		if (tmpStr == "Type")
			m_numIdType = i;
		if (tmpStr == "Stride")
			m_numIdStride = i;
		if (tmpStr == "Offset")
			m_numIdOffset = i;
		if (tmpStr == "Format")
			m_numIdFormat = i;
		if (tmpStr == "href")
			m_numIdHref = i;
		if (tmpStr == "Endianism")
			m_numIdEndianism = i;
		if (tmpStr == "Rank")
			m_numIdRank = i;
	}
	if (!downloadFiles())
	{
		std::cerr << "Error: one or more files were not valid or downloaded" << std::endl;
		return 1;
	}

	for (int j = 0; j < m_nRows; j++)
	{
		m_sPP.SetCurrentTR(j);							 //! fissa la riga
		std::string key = m_sPP.GetData(m_numIdSpecies); //! getta la colonna
		it = m_speciesMapele.find(key);
		itgeo = m_speciesMapgeo.find(key);
		std::stringstream sstrValue(m_sPP.GetData(m_numIdArraysize));
		unsigned long long int value;
		sstrValue >> value;
		bool geometry = false;
		int count = 0;
		while (!(sstrValue.eof()))
		{
			unsigned long long int tmpval;
			geometry = true;
			sstrValue >> tmpval;
			value = value * tmpval;
			count++;
		}
		if (count > 2)
		{
			std::cerr << "xmlsource: ignored row containing more than 3D geometry: " << m_sPP.GetData(m_numIdArraysize) << std::endl;
			continue;
		}
		if (it != m_speciesMapele.end() && it->second != value)
		{
			std::cerr << "VisIVOImporter: xml document contain different size for the same specie: " << it->first << std::endl;
			return 1;
		}
		if (geometry && itgeo != m_speciesMapgeo.end() && itgeo->second != m_sPP.GetData(m_numIdArraysize))
		{
			std::cerr << "VisIVOImporter: xml document contain different size for the same specie: " << it->first << std::endl;
			return 1;
		}
		if (it == m_speciesMapele.end())
			m_speciesMapele.insert(make_pair(key, value));
		if (itgeo == m_speciesMapgeo.end())
			m_speciesMapgeo.insert(make_pair(key, m_sPP.GetData(m_numIdArraysize)));
	}

	return 0;
}
//---------------------------------------------------------------------
bool XmlSource::allocateArray()
//---------------------------------------------------------------------
{
	bool goodAllocation = false;
	std::stringstream ssprecision(m_sPP.GetData(m_numIdPrecision));
	int numOfBytes;
	ssprecision >> numOfBytes;
	m_fArray = NULL;
	m_fReadArray = NULL;
	while (!goodAllocation)
	{
		try
		{
			m_fArray = new float[m_maxEle];
		}
		catch (std::bad_alloc e)
		{
			m_fArray = NULL;
		}
		try
		{
			m_fReadArray = new char[m_maxEle * numOfBytes];
		}
		catch (std::bad_alloc e)
		{
			m_fReadArray = NULL;
			;
		}

		if (m_fArray == NULL || m_fReadArray == NULL)
		{
			if (m_fArray)
				delete m_fArray;
			if (m_fReadArray)
				delete m_fReadArray;
			if (m_maxEle <= MIN_NUMBER_OF_ROW)
				return false;
			m_maxEle = m_maxEle - MAX_NUMBER_TO_REDUCE_ROW;
			if (m_maxEle < MIN_NUMBER_OF_ROW)
				m_maxEle = MIN_NUMBER_OF_ROW;
		}
		else
			goodAllocation = true;
	}
	return goodAllocation;
}

//---------------------------------------------------------------------
void XmlSource::writeListFiles()
//---------------------------------------------------------------------
{
	std::string listFilename = "./downloaded_VisIVO_filelist";
	std::ofstream fileListOutput(listFilename.c_str(), std::ios::out | std::ios::app);
	std::map<std::string, std::string>::iterator it;

	time_t rawtime;
	time(&rawtime);
	for (it = m_listFileMap.begin(); it != m_listFileMap.end(); it++)
		fileListOutput << it->first << "     " << it->second << " Date: " << ctime(&rawtime) << std::endl;
	fileListOutput.close();
	return;
}
//---------------------------------------------------------------------
int XmlSource::readData()
//---------------------------------------------------------------------
{

	if (readSpecifiedFormat())
		return 0;

	m_listFileMap.clear();
	std::vector<std::string> colNameStr;
	std::string systemEndian;
#ifdef VSBIGNENDIAN
	systemEndian = "big";
#else
	systemEndian = "little";
#endif
	bool needSwap = false;

	//! READ customized and/or specified format
	m_maxEle = -1;
	unsigned long long int maxEle = 0;
	for (m_it = m_speciesMapele.begin(); m_it != m_speciesMapele.end(); m_it++)
	{
		//	std::clog<<"FIRST"<<m_it->first<<"  SECOND"<<m_it->second<<std::endl;
		if (maxEle < m_it->second)
			maxEle = m_it->second;
	}
	if (maxEle <= MAXINT)
		m_maxEle = maxEle;
	else
		m_maxEle = MAXINT;
	for (m_it = m_speciesMapele.begin(); m_it != m_speciesMapele.end(); m_it++)
	{
		colNameStr.clear();
		size_t pos = m_pointsBinaryName.find_last_of(".");

		std::string localFileName = m_pointsBinaryName.substr(0, pos);

		localFileName = localFileName + "_" + m_it->first + ".bin";
		std::ofstream filebinOutput(localFileName.c_str(), std::ios::out | std::ios::binary);
		m_listFileMap.insert(make_pair(m_it->first, localFileName));
		std::string headName = localFileName + ".head";
		std::ofstream fileheadOutput(headName.c_str(), std::ios::out);
		unsigned long long int rowForSpecies = m_it->second;
		for (int j = 0; j < m_nRows; j++)
		{
			m_sPP.SetCurrentTR(j); //! fissa la riga
			std::string key = m_sPP.GetData(m_numIdSpecies);
			std::stringstream strRank(m_sPP.GetData(m_numIdRank));
			std::string endianType = m_sPP.GetData(m_numIdEndianism);

			size_t found = endianType.find("b");
			if (found == std::string::npos)
				found = endianType.find("B");

			if (found != std::string::npos)
				endianType = "big";
			else
				endianType = "little";

			if (endianType != systemEndian)
				needSwap = true;

			int rank;
			strRank >> rank;
			int sizeOfEle = 0;
			if (key == m_it->first)
			{
				if (rank > 1)
				{
					int rankCount = rank;
					std::string FiledName = m_sPP.GetData(m_numIdField);
					while (rankCount > 0)
					{
						std::stringstream ssColName;
						ssColName << FiledName << "__" << rank - rankCount + 1;
						//					std::clog<<ssColName.str()<<" "<<FiledName<<std::endl;
						colNameStr.push_back(ssColName.str());
						rankCount--;
					}
				}
				else
					colNameStr.push_back(m_sPP.GetData(m_numIdField));

				unsigned long long int totRows = m_it->second;
				unsigned long long int nOfEle = totRows;
				std::stringstream ssprecision(m_sPP.GetData(m_numIdPrecision));
				ssprecision >> sizeOfEle;

				if (m_sPP.GetData(m_numIdType) == "float" || m_sPP.GetData(m_numIdType) == "Float" || m_sPP.GetData(m_numIdType) == "FLOAT")
				{
					if (sizeOfEle != sizeof(float))
					{
						std::cerr << "No match in size of float in Row " << j << std::endl;
						continue; //! next rows
					}
					m_inputType = fl;
				}
				if (m_sPP.GetData(m_numIdType) == "double" || m_sPP.GetData(m_numIdType) == "Double" || m_sPP.GetData(m_numIdType) == "DOUBLE")
				{
					if (sizeOfEle != sizeof(double))
					{
						std::cerr << "No match in size of double in Row " << j << std::endl;
						continue; //! next rows
					}
					m_inputType = dou;
				}
				if (m_sPP.GetData(m_numIdType) == "long double" || m_sPP.GetData(m_numIdType) == "long Double" || m_sPP.GetData(m_numIdType) == "long DOUBLE")
				{
					if (sizeOfEle != sizeof(long double))
					{
						std::cerr << "No match in size of long double in Row " << j << std::endl;
						continue; //! next rows
					}
					m_inputType = ldou;
				}
				if (m_sPP.GetData(m_numIdType) == "int" || m_sPP.GetData(m_numIdType) == "Int" || m_sPP.GetData(m_numIdType) == "INT")
				{
					if (sizeOfEle != sizeof(int))
					{
						std::cerr << "No match in size of int in Row " << j << std::endl;
						continue; //! next rows
					}
					m_inputType = i;
				}
				if (m_sPP.GetData(m_numIdType) == "long int" || m_sPP.GetData(m_numIdType) == "long Int" || m_sPP.GetData(m_numIdType) == "long INT")
				{
					if (sizeOfEle != sizeof(long int))
					{
						std::cerr << "No match in size of long int in Row " << j << std::endl;
						continue; //! next rows
					}
					m_inputType = li;
				}
				if (m_sPP.GetData(m_numIdType) == "long long int" || m_sPP.GetData(m_numIdType) == "long long Int" || m_sPP.GetData(m_numIdType) == "long long INT")
				{
					if (sizeOfEle != sizeof(long long int))
					{
						std::cerr << "No match in size of long long int in Row " << j << std::endl;
						continue; //! next rows
					}
					m_inputType = lli;
				}

				if (!allocateArray())
				{
					std::cerr << "xmlsource. Allocation Memory failed" << std::endl;
					return 1;
				}

				std::string file = m_sPP.GetData(m_numIdHref);
				std::string ifile;
				std::map<std::string, std::string>::iterator it;
				for (it = m_outfileMap.begin(); it != m_outfileMap.end(); it++)
				{
					if (it->second == file)
					{
						ifile = it->first;
						break;
					}
				}
				std::ifstream filebinInput(ifile.c_str(), std::ios::in | std::ios::binary);
				if (!filebinInput)
				{
					std::cerr << "Cannot open binary file " << ifile << std::endl;
					return -2;
				}
				std::stringstream offset(m_sPP.GetData(m_numIdOffset));
				unsigned long long int offsetValue;
				offset >> offsetValue;
				unsigned long long int totOffset = offsetValue;
				std::ios::off_type indexOffset;
				int nSkip;
				int nSaveSkip;
				if (totOffset > MAXINT)
					nSkip = MAXINT;
				else
					nSkip = totOffset;
				while (totOffset != 0)
				{
					indexOffset = (std::ios::off_type)nSkip; // QUI verificare
					filebinInput.seekg(indexOffset, std::ios::cur);
					totOffset = totOffset - nSkip;
					if (totOffset < nSkip)
						nSkip = totOffset;
				}

				std::stringstream stride(m_sPP.GetData(m_numIdStride));
				int strideValue;
				int skipChar = 0;
				stride >> strideValue;
				int maxEle = m_maxEle;
				int countRank = 0;
				while (rank > 0)
				{
					unsigned long long int totChar = totRows * (sizeOfEle + strideValue) - strideValue;
					int eleToRead = maxEle * sizeOfEle;
					eleToRead = eleToRead - eleToRead % (sizeOfEle + strideValue);
					if (totChar < maxEle * sizeOfEle)
						eleToRead = totChar;
					while (totChar != 0)
					{
						filebinInput.read((char *)&m_fReadArray[0], eleToRead);
						if (strideValue == 0)
							filebinOutput.write((char *)&m_fReadArray[0], eleToRead);
						else
						{
							int indexfArray = 0;
							char *chindex = m_fReadArray;
							int incrChar = 0;
							while (incrChar < eleToRead)
							{
								if (m_inputType == fl)
								{
									float *wrvalue;
									wrvalue = (float *)chindex;
									if (needSwap)
										m_fArray[indexfArray] = floatSwap((char *)(wrvalue));
									else
										m_fArray[indexfArray] = *wrvalue;
								}
								if (m_inputType == dou)
								{
									double *wrvalue;
									wrvalue = (double *)chindex;
									if (needSwap)
										m_fArray[indexfArray] = (float)doubleSwap((char *)(wrvalue));
									else
										m_fArray[indexfArray] = (float)*wrvalue;
								}
								if (m_inputType == ldou)
								{
									long double *wrvalue;
									wrvalue = (long double *)chindex;
									if (needSwap)
										m_fArray[indexfArray] = (float)longdoubleSwap((char *)(wrvalue));
									else
										m_fArray[indexfArray] = (float)*wrvalue;
								}
								if (m_inputType == i)
								{
									int *wrvalue;
									wrvalue = (int *)chindex;
									if (needSwap)
										m_fArray[indexfArray] = (float)intSwap((char *)(wrvalue));
									else
										m_fArray[indexfArray] = (float)*wrvalue;
								}
								if (m_inputType == li)
								{
									long int *wrvalue;
									wrvalue = (long int *)chindex;
									if (needSwap)
										m_fArray[indexfArray] = (float)longintSwap((char *)(wrvalue));
									else
										m_fArray[indexfArray] = (float)*wrvalue;
								}
								if (m_inputType == lli)
								{
									long long int *wrvalue;
									wrvalue = (long long int *)chindex;
									if (needSwap)
										m_fArray[indexfArray] = (float)longlongintSwap((char *)(wrvalue));
									else
										m_fArray[indexfArray] = (float)*wrvalue;
								}
								indexfArray++;
								incrChar = incrChar + sizeOfEle + strideValue;
								chindex = chindex + sizeOfEle + strideValue;
							}
							filebinOutput.write((char *)&m_fArray[0], indexfArray * sizeof(float));
						}
						totChar = totChar - eleToRead;
						if (totChar < eleToRead)
							eleToRead = totChar;
					}
					rank--;
					if (rank > 0)
					{
						countRank++;
						filebinInput.seekg(0, ios::beg);
						totOffset = offsetValue + countRank * sizeOfEle;
						if (totOffset > MAXINT)
							nSkip = MAXINT;
						else
							nSkip = totOffset;
						while (totOffset != 0)
						{
							indexOffset = (std::ios::off_type)nSkip; // QUI verificare
							filebinInput.seekg(indexOffset, std::ios::cur);
							totOffset = totOffset - nSkip;
							if (totOffset < nSkip)
								nSkip = totOffset;
						}
					}
				}
				delete[] m_fReadArray;
				delete[] m_fArray;
				filebinInput.close();
			}
		}
		filebinOutput.close();
		fileheadOutput << "float" << std::endl;
		fileheadOutput << colNameStr.size() << std::endl;
		std::map<std::string, std::string>::iterator itgeo;
		itgeo = m_speciesMapgeo.find(m_it->first);
		if (itgeo == m_speciesMapgeo.end())
		{
			std::cerr << "xmlsource internal error on itgeo " << m_it->first << std::endl;
			return 1;
		}
		std::stringstream ssrowForSpecies(itgeo->second);
		unsigned long long int arrayValue[3];
		arrayValue[0] = 0;
		arrayValue[1] = 0;
		arrayValue[2] = 0;
		bool geo = false;
		ssrowForSpecies >> arrayValue[0]; // Fare check x 3 valori!!
		unsigned long long int totalRows = arrayValue[0];
		if (!(ssrowForSpecies.eof()))
		{
			geo = true;
			ssrowForSpecies >> arrayValue[1];
			if (!(ssrowForSpecies.eof()))
				ssrowForSpecies >> arrayValue[2];
			else
				arrayValue[2] = 1;
			totalRows = arrayValue[0] * arrayValue[1] * arrayValue[2];
		}
		fileheadOutput << totalRows;
		if (geo)
		{
			fileheadOutput << " " << arrayValue[0];
			fileheadOutput << " " << arrayValue[1];
			fileheadOutput << " " << arrayValue[2];
			fileheadOutput << " 1 1 1";
		}
		fileheadOutput << std::endl;
#ifdef VSBIGENDIAN
		std::string endianism = "big";

#else
		std::string endianism = "little";
#endif
		fileheadOutput << endianism << std::endl;
		for (int i = 0; i < colNameStr.size(); i++)
			fileheadOutput << colNameStr[i] << std::endl;

		fileheadOutput.close();
	}

	//  writeListFiles();

	std::map<std::string, std::string>::iterator it;
	for (it = m_outfileMap.begin(); it != m_outfileMap.end(); it++)
	{
		//	std::clog<<it->first.c_str()<<std::endl;
		remove(it->first.c_str());
	}
	return 0;
}
