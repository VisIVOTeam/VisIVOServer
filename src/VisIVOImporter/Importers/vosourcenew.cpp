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
#include "vosourcenew.h"

#include "visivoutils.h"

#include <iostream>
#include <fstream>
#include <sstream>
const unsigned int VOSourcenew::MAX_NUMBER_INT = 250000000;
const unsigned int VOSourcenew::MAX_NUMBER_TO_REDUCE_ROW = 10000;
const unsigned int VOSourcenew::MIN_NUMBER_OF_ROW = 100;

//---------------------------------------------------------------------
VOSourcenew::VOSourcenew()
//---------------------------------------------------------------------
{
	m_fArray = NULL;
	m_notgood = false;
	m_nOfEle = 0;
	m_alreadyWritten = 0;
	m_toWrite = 0;
	m_buffer = NULL;
}
//---------------------------------------------------------------------
VOSourcenew::~VOSourcenew()
//---------------------------------------------------------------------
{
	if (m_fArray != NULL)
	{
		for (int i = 0; i < m_nOfCol; i++)
			delete[] m_fArray[i];
		delete[] m_fArray;
	}
	if (m_buffer != NULL)
		delete[] m_buffer;
}
//---------------------------------------------------------------------
bool VOSourcenew::writeData()
//---------------------------------------------------------------------
{
	m_outfile.seekp(0, std::ios::beg);
	size_t maxSkipFactor;
	if (m_nOfRow > MAX_NUMBER_INT)
		maxSkipFactor = MAX_NUMBER_INT * sizeof(float);
	else
		maxSkipFactor = m_nOfRow * sizeof(float);

	unsigned long long int totalSkip;
	int skipFactor;
	for (int i = 0; i < m_nOfCol; i++)
	{
		totalSkip = (i * m_nOfRow + m_alreadyWritten) * sizeof(float);
		if (totalSkip > maxSkipFactor)
			skipFactor = maxSkipFactor;
		else
			skipFactor = totalSkip;
		while (totalSkip > 0)
		{
			m_outfile.seekp(skipFactor, std::ios::cur);
			totalSkip -= skipFactor;
			if (totalSkip < skipFactor)
				skipFactor = totalSkip;
		}
		m_outfile.write((char *)m_fArray[i], sizeof(float) * m_toWrite);
		m_outfile.seekp(0, std::ios::beg);
	}
	m_alreadyWritten += m_toWrite;
	// std::clog<<"WRIITEN PARTIAL TABLE NUMBER ="<<m_alreadyWritten<<std::endl;
	// std::clog<<"WRIITEN TOTAL ELEMENT ="<<m_toWrite<<std::endl;
	// std::clog<<"***********"<<std::endl;

	return true;
}
//---------------------------------------------------------------------
int VOSourcenew::readData()
//---------------------------------------------------------------------
{
	if (m_notgood || m_buffer == NULL)
	{
		std::cerr << "Importer Aborted. Code 1" << std::endl;
		return 1;
	}
	bool write = false;
	bool finalwrite = false;
	int indexCol = 0;
	int indexRow = 0;
	unsigned long long int globalIndex = 0;
	unsigned long long int globalPointer = 0;
	unsigned long long int previousPointer = 0;
	std::string tmp;
	m_outfile.open(m_pointsBinaryName.c_str(), std::ofstream::binary);
	m_inputFile.open(m_pointsFileName.c_str());
	bool startTable = false;
	bool firstLoad = true;
	unsigned long long int getPointer;

	bool endReached = false;
	size_t endFound = 0;
	int countRead = 0;
	while (!m_inputFile.eof())
	{
		if (countRead > 0)
			firstLoad = false;
		countRead++;
		int subStrtStart = 0;
		globalPointer = m_inputFile.tellg();
		m_inputFile.read(m_buffer, m_length);

		if (countRead > 1 && !m_inputFile.eof())
		{
			if (m_inputFile.tellg() <= previousPointer)
			{
				std::cerr << "Importer Internal Error 1. Operation Aborted" << std::endl;
				return 6;
			}
			previousPointer = m_inputFile.tellg();
		}

		if (m_inputFile.eof())
			firstLoad = false;
		std::string st1(m_buffer);
		if (!startTable)
		{
			if (st1.find("<TABLEDATA>") == std::string::npos)
			{
				getPointer = m_inputFile.tellg();
				getPointer -= 10;
				m_inputFile.seekg(getPointer, std::ios::beg);
				continue;
			}
			else
			{
				startTable = true;
				subStrtStart = st1.find("<TABLEDATA>");
			}
		}
		if (m_inputFile.eof() && !startTable)
		{
			std::cerr << "Importer Aborted. Expected <TABLEDATA> not found" << std::endl;
			return 2;
		}

		if (m_inputFile.eof() && st1.find("</TABLEDATA>") == std::string::npos)
		{
			std::cerr << "Importer Aborted. Expected </TABLEDATA> not found" << std::endl;
			return 2;
		}

		if (st1.find("</TABLEDATA>") != std::string::npos)
		{
			endReached = true;
			endFound = st1.find("</TABLEDATA>");
		}

		if (firstLoad && st1.find("<TR>") == std::string::npos) // primo carico: NON trovi TR
		{
			if (m_inputFile.eof())
			{
				std::cerr << "Importer Aborted. Unredable VOTable. MSG 2" << std::endl;
				return 2;
			}
			getPointer = m_inputFile.tellg();
			getPointer -= 4;
			m_inputFile.seekg(getPointer, std::ios::beg);
			firstLoad = false;
			continue;
		}

		if (firstLoad && st1.find("</TR>") == std::string::npos) // primo carico: trovi TR e NON /TR
		{
			if (m_inputFile.eof())
			{
				std::cerr << "Importer Aborted. Unredable VOTable MSG 3" << std::endl;
				return 2;
			}
			getPointer = globalPointer - m_length + st1.find("<TR>") - 1;
			m_inputFile.seekg(getPointer, std::ios::beg);
			firstLoad = false;
			continue;
		}

		if (st1.find("</TR>") == std::string::npos) // NON trovi  /TR
		{
			std::cerr << "Importer Aborted. Unredable VOTable MSG 4" << std::endl;
			return 2;
		}
		else
		{
			if (!m_inputFile.eof())
			{
				globalPointer = m_inputFile.tellg();
				getPointer = globalPointer - m_length + st1.rfind("</TR>") - 1;
				m_inputFile.seekg(getPointer, std::ios::beg);
			}
			size_t found1;
			size_t pos = 0;
			size_t foundLastEndTR = st1.rfind("</TR>");
			while (true)
			{
				found1 = st1.find("<TR>", pos);
				if (found1 > foundLastEndTR)
					break;
				if (endReached && found1 >= endFound)
					break;
				if (found1 == std::string::npos)
					break;
				else
				{
					for (int i = 0; i < m_nOfCol; i++)
					{
						size_t foundTD, foundEndTD;
						foundTD = st1.find("<TD>", pos);
						foundEndTD = st1.find("</TD>", pos);
						//                std::clog<<st1.substr(foundTD+4,foundEndTD-foundTD-4)<<std::endl;
						if (is_number(st1.substr(foundTD + 4, foundEndTD - foundTD - 4)))
							m_fArray[i][indexRow] = atof(st1.substr(foundTD + 4, foundEndTD - foundTD - 4).c_str());
						else
							m_fArray[i][indexRow] = TEXT_VALUE;

						finalwrite = true;
						pos = foundEndTD + 4;
					}
					indexRow++;
					//			if(indexRow%500000 ==0) std::clog<<"indexRow= "<<indexRow<<" on "<<m_nOfEle<<std::endl;
					// 			std::clog<<"indexRow= "<<indexRow<<std::clog;
					globalIndex++;
					if (indexRow == m_nOfEle - 1)
						write = true;
					if (write)
					{
						m_toWrite = indexRow;
						if (!writeData())
						{
							std::cerr << "Importer write operation Failed." << std::endl;
							return 5;
						}
						indexRow = 0;
						write = false;
						finalwrite = false;
					}
					if (pos >= foundLastEndTR)
						break; // exit while true
				}
			}
		} // if(st1.find("</TR>"... else
	}	  // while(!m_inputFile.eof())

	if (globalIndex != m_nOfRow)
	{
		std::cerr << "Importer write operation wrong. Invalid data can occuur" << std::endl;
	}

	if (finalwrite)
	{
		m_toWrite = indexRow;
		if (!writeData())
		{
			std::cerr << "Importer write operation Failed." << std::endl;
			return 5;
		}
	}

	m_outfile.close();
	return EXIT_SUCCESS;
}

//---------------------------------------------------------------------
bool VOSourcenew::allocateArray()
//---------------------------------------------------------------------
{
	try
	{
		m_fArray = new float *[m_fieldName.size()];
	}
	catch (std::bad_alloc &e)
	{
		m_fArray = NULL;
	}

	if (m_fArray == NULL)
		return false;

	bool goodAllocation = false;
	while (!goodAllocation)
	{
		goodAllocation = true;
		for (unsigned int i = 0; i < m_nOfCol; i++)
		{
			try
			{
				m_fArray[i] = new float[m_nOfEle];
			}
			catch (std::bad_alloc &e)
			{
				m_fArray[i] = NULL;
			}

			if (m_fArray[i] == NULL)
			{
				goodAllocation = false;
				for (unsigned int j = 0; j < i; j++)
					delete[] m_fArray[j];
				if (m_nOfEle == MIN_NUMBER_OF_ROW)
				{
					delete[] m_fArray;
					m_fArray = NULL;
					return false;
				}
				m_nOfEle = m_nOfEle - MAX_NUMBER_TO_REDUCE_ROW;
				if (m_nOfEle <= MAX_NUMBER_TO_REDUCE_ROW)
					m_nOfEle = MIN_NUMBER_OF_ROW;
				break;
			}
		}
	}
	return true;
}
//---------------------------------------------------------------------
int VOSourcenew::readHeader()
//---------------------------------------------------------------------
{
	bool isaVotable = false;
	m_length = 200000000; // 200 MByte allocated
	while (true)
	{
		try
		{
			m_buffer = new char[m_length];
		}
		catch (std::bad_alloc &e)
		{
			m_buffer = NULL;
		}
		if (m_buffer == NULL)
		{
			if (m_length == MIN_NUMBER_OF_ROW)
			{
				std::cerr << "Invalid allocation. Aborted" << std::endl;
				return 1;
			}

			m_length = m_length - MAX_NUMBER_TO_REDUCE_ROW;
			if (m_length <= MIN_NUMBER_OF_ROW)
				m_length = MIN_NUMBER_OF_ROW;
		}
		else
			break;
	}
	size_t found;
	std::string inputFilename;
	m_inputFile.open(m_pointsFileName.c_str());
	std::string tmp;
	while (!m_inputFile.eof())
	{
		m_inputFile >> tmp;
		found = tmp.find("<VOTABLE");
		if (found != std::string::npos)
		{
			isaVotable = true;
			break;
		}
		if (m_inputFile.tellg() > 10000000) // probably it is not a no Votable
		{
			std::cerr << "Anomalous VOTable ?" << std::endl;
			break;
		}
	}
	if (!isaVotable)
	{
		std::cerr << "Unrecognized VOTable. Importer Aborted." << std::endl;
		m_notgood = true;
		return 1;
	}
	for (;;)
	{
		m_inputFile >> tmp;
		found = tmp.find("<FIELD");
		if (found != std::string::npos)
		{
			size_t f1, f2;
			f1 = tmp.find("name");
			while (f1 == std::string::npos)
			{
				m_inputFile >> tmp;
				f1 = tmp.find("name");
			}
			f1 = tmp.find("=");
			while (f1 == std::string::npos)
			{
				m_inputFile >> tmp;
				f1 = tmp.find("=");
			}
			f1 = tmp.find_first_of('"');
			while (f1 == std::string::npos)
			{
				m_inputFile >> tmp;
				f1 = tmp.find('"');
			}
			f2 = tmp.find_last_of('"');
			if (f2 != std::string::npos)
			{
				m_fieldName.push_back(tmp.substr(f1 + 1, f2 - f1 - 1));
			}
		}
		if (tmp.find("<TABLEDATA>") != std::string::npos)
			break;
		if (tmp.find("</VOTABLE>") != std::string::npos)
		{
			std::cerr << "Invalid VOTable. Aborted." << std::endl;
			return 1;
		}
	}

	m_nOfCol = m_fieldName.size();
	m_nOfRow = 0;
	int first = 0;
	std::string end;
	bool endReached = false;
	size_t endFound = 0;

	while (!m_inputFile.eof())
	{
		m_inputFile.read(m_buffer, m_length);
		std::string st1(m_buffer);
		if (m_inputFile.eof() && st1.find("</TABLEDATA>") == std::string::npos)
			break;
		if (st1.find("</TABLEDATA>") != std::string::npos)
		{
			endReached = true;
			endFound = st1.find("</TABLEDATA>");
		}
		if (first > 0)
		{
			end = end + st1.substr(0, 3);
			if (end.find("<TR>") != std::string::npos)
				m_nOfRow++;
		}
		size_t found1;
		size_t pos = 0;
		while (true)
		{
			found1 = st1.find("<TR>", pos);
			if (endReached && found1 >= endFound)
				break;
			if (found1 == std::string::npos)
				break;
			else
			{
				m_nOfRow++;
				pos = found1 + 4;
			}
		}
		first++;
		if (m_length <= st1.size()) // QUI check!
			end = st1.substr(m_length - 3, 3);
	}

	m_inputFile.close();
	if (m_nOfRow < MAX_NUMBER_INT)
		m_nOfEle = m_nOfRow;
	else
		m_nOfEle = MAX_NUMBER_INT;

	if (!allocateArray())
	{
		m_notgood = true;
		return 1;
	}
	m_volumeOrTable = "table";
	makeHeader(m_nOfRow, m_pointsBinaryName, m_fieldName, m_cellSize, m_cellComp, m_volumeOrTable);
	return 0;
}

bool VOSourcenew::is_number(const std::string &s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it))
		++it;
	return !s.empty() && it == s.end();
}
