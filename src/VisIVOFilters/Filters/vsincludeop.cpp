/***************************************************************************
 *   Copyright (C) 2008 by Ugo Becciani   *
 *   ugo.becciani@oact.inaf.it   *
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
#include <iostream>
#include <sstream>
#include <cmath>
#include "vstable.h"
#include "vsincludeop.h"
#include "vsstatisticop.h"

const unsigned int VSIncludeOp::MAX_NUMBER_TO_REDUCE_ROW = 10000;
const unsigned int VSIncludeOp::MIN_NUMBER_OF_ROW = 100;

//---------------------------------------------------------------------
VSIncludeOp::VSIncludeOp()
//---------------------------------------------------------------------
{
	m_fArray = NULL;
	m_radius = 0.0;
	m_centerCoords[0] = 0.0;
	m_centerCoords[1] = 0.0;
	m_centerCoords[2] = 0.0;
	m_outValue = 0.0;
	m_inValue = 1.0;
	m_nOfEle = 0;
	m_nOfRows = 0;
	m_nOfCols = 3;
}
//---------------------------------------------------------------------
VSIncludeOp::~VSIncludeOp()
//---------------------------------------------------------------------
{
	if (m_fArray != NULL)
	{
		for (int k = 0; k < m_nOfCols; k++)
			delete[] m_fArray[k];

		delete[] m_fArray;
	}
}
//---------------------------------------------------------------------
void VSIncludeOp::printHelp()
//---------------------------------------------------------------------
{
	std::cout << "Produce a new table or add a new field to the input table. Points inside the sphere (given with center and radius) will have the value invalue, otherwise outvalue." << std::endl
			  << std::endl;
	std::cout << "Usage: VisIVOFilters --op include  --center x_coord y_coord z_coord --radius radius [--field x_col y_col z_col]  [--append] [--history] [--historyfile filename.xml] [--out filename_out.bin] [--outcol col_name] [--outvalue outvalue] [--invalue invalue] [--help] [--file] inputFile.bin" << std::endl
			  << std::endl;

	std::cout << "Example: VisIVOFilters --op include  --center 1.0 1.0 1.0 --radius 1.0 --field X Y Z  --append --outcol INOUT [--help] --file inputFile.bin" << std::endl;

	std::cout << "Note:  " << std::endl;
	std::cout << "--center  coordinates of the sphere center." << std::endl;
	std::cout << "--radius radius of the sphere." << std::endl;
	std::cout << "--field three valid columns names. Default value are the first three columns." << std::endl;
	std::cout << "--append. No new table will be cretaed. The original table will have the new field. " << std::endl;
	std::cout << "--out Name of the new table. Default name is given. Ignored if --append is specified." << std::endl;
	std::cout << "--outcol. Column name of the new field" << std::endl;
	std::cout << "--outvalue. Value given to points outside the sphere. Devault value is 0." << std::endl;
	std::cout << "--invalue. Value given to points inside the sphere. Devault value is 1." << std::endl;
	std::cout << "--history (optional) create an XML file which contains the history of operations performed (default create hist.xml file)" << std::endl;
	std::cout << "--historyfile [filename.xml]   (optional) Change default history file name  and or directory " << std::endl;
	std::cout << "--file Input table filename." << std::endl;

	std::cout << "--help produce this output." << std::endl;

	return;
}
//---------------------------------------------------------------------
bool VSIncludeOp::allocatefArray()
//---------------------------------------------------------------------
{
	try
	{
		m_fArray = new float *[m_nOfCols];
	}
	catch (std::bad_alloc &e)
	{
		return false;
	}

	bool goodAllocation = false;
	while (!goodAllocation)
	{
		goodAllocation = true;
		for (int col = 0; col < m_nOfCols; col++)
		{
			try
			{
				m_fArray[col] = new float[m_nOfEle];
			}
			catch (std::bad_alloc &e)
			{
				m_fArray[col] = NULL;
			}
			if (m_fArray[col] == NULL)
			{
				goodAllocation = false;
				for (int k = 0; k < col; k++)
					delete[] m_fArray[k];

				if (m_nOfEle == MIN_NUMBER_OF_ROW)
				{
					delete[] m_fArray;
					m_fArray = NULL;
					return false;
				}
				if (m_nOfEle <= MAX_NUMBER_TO_REDUCE_ROW)
					m_nOfEle = MIN_NUMBER_OF_ROW;
				else
					m_nOfEle = m_nOfEle - MAX_NUMBER_TO_REDUCE_ROW;
			}
		}
	}
	return true;
}

//---------------------------------------------------------------------
bool VSIncludeOp::execute()
//---------------------------------------------------------------------

{
	if (m_tables[0]->getIsVolume())
	{
		std::cerr << "The operation include is not applied for volumes." << std::endl;
		return false;
	}
	if (m_tables[0]->getNumberOfColumns() <= 2)
	{
		std::cerr << "The table must contain 3 o more columns." << std::endl;
		return false;
	}
	unsigned int colId[3];
	if (!isParameterPresent("field"))
	{
		colId[0] = 0;
		colId[1] = 1;
		colId[2] = 2;
	}
	else
	{
		std::stringstream sstmp(getParameterAsString("field"));
		int count = 0;
		while (!sstmp.eof() && count < 3)
		{
			std::string stmp;
			sstmp >> stmp;
			if (m_tables[0]->getColId(stmp) < 0)
				continue;
			colId[count] = m_tables[0]->getColId(stmp);
			count++;
		}
		if (count != 3)
		{
			std::cerr << "Invalid field parameter." << std::endl;
			return false;
		}
	}

	if (colId[0] == colId[1] || colId[0] == colId[2] || colId[1] == colId[2])
		std::cerr << "Warning: duplicate field names: " << getParameterAsString("field") << std::endl;

	if (!isParameterPresent("center"))
	{
		std::cerr << "Missing center parameter." << std::endl;
		return false;
	}

	std::stringstream sstmp(getParameterAsString("center"));
	int count = 0;
	while (!sstmp.eof() && count < 3)
	{

		sstmp >> m_centerCoords[count];
		count++;
	}
	if (count != 3)
	{
		std::cerr << "Invalid center parameter." << std::endl;
		return false;
	}
	if (!isParameterPresent("radius"))
	{
		std::cerr << "Missing radius parameter." << std::endl;
		return false;
	}

	m_radius = getParameterAsFloat("radius");

	// CHECK BLOCK
	{
		VSStatisticOp stat;
		std::stringstream ssstat;
		for (int i = 0; i < 3; i++)
			ssstat << m_tables[0]->getColName(colId[i]) << " ";
		stat.addParameter("list", ssstat.str());
		stat.addParameter("silent", " ");
		stat.addInput(m_tables[0]);
		stat.execute();
		float min[3], max[3], avg[3], sum[3];
		for (unsigned int i = 0; i < 3; i++)
		{
			stat.getRange(i, max[i], min[i], avg[i], sum[i]);
			std::clog << min[i] << " " << max[i] << std::endl;
		}
		for (int i = 0; i < 3; i++)
			if (m_centerCoords[i] > max[i] || m_centerCoords[i] < min[i])
				std::cerr << "Warning: Coordinate " << i << " =" << m_centerCoords[i] << " outside the domain range:" << min[i] << "  " << max[i] << std::endl;
		bool inside = true;
		for (int i = 0; i < 3; i++)
		{
			if (m_centerCoords[i] - m_radius > max[i])
				inside = false;
			if (m_centerCoords[i] + m_radius < min[i])
				inside = false;
		}
		if (!inside)
			std::cerr << "Warning: the domain is outside the sphere." << std::endl;
	}

	if (isParameterPresent("outvalue"))
		m_outValue = getParameterAsFloat("outvalue");
	if (isParameterPresent("invalue"))
		m_inValue = getParameterAsFloat("invalue");

	VSTable newTable;

	std::string outColName;

	if (!isParameterPresent("outcol") || getParameterAsString("outcol") == "unknown")
		outColName = "IncludeOp";
	else
		outColName = getParameterAsString("outcol");

	std::string outName;

	if (!isParameterPresent("out") || getParameterAsString("out") == "unknown")
		outName = "VSIncludeOp.bin";
	else
		outName = getParameterAsString("out");

	VSTable *resultTable = NULL;
	unsigned int colPutId[1];
	if (!isParameterPresent("append"))
	{
		newTable.setType("float");
		newTable.setNumberOfRows(m_tables[0]->getNumberOfRows());
		newTable.setIsVolume(m_tables[0]->getIsVolume());
		newTable.setEndiannes(m_tables[0]->getEndiannes());

		newTable.addCol(outColName);

		newTable.setLocator(outName);
		newTable.writeHeader();
		resultTable = &newTable;
		m_realOutFilename.push_back(outName);
	}
	else
	{
		if (!m_tables[0]->addCol(outColName))
		{
			std::cerr << "Duplicate outcol parameter" << std::endl;
			return false;
		}
		m_tables[0]->writeHeader();
		resultTable = m_tables[0];
		m_realOutFilename.push_back(m_tables[0]->getLocator());
	}
	colPutId[0] = resultTable->getNumberOfColumns() - 1;
	m_nOfRows = m_tables[0]->getNumberOfRows();
	if (m_nOfRows <= getMaxNumberInt())
		m_nOfEle = (int)m_nOfRows;
	else
		m_nOfEle = getMaxNumberInt();

	if (!allocatefArray())
	{
		std::cerr << "Invalid memory allocation." << std::endl;
		return false;
	}
	unsigned long long int index = 0;
	while (index < m_nOfRows)
	{
		unsigned long long int toIndex = index + m_nOfEle - 1;
		if (toIndex >= m_nOfRows)
			toIndex = m_nOfRows - 1;

		m_tables[0]->getColumn(colId, m_nOfCols, index, toIndex, m_fArray);
		for (int row = 0; row < toIndex - index + 1; row++)
		{
			float distance = sqrt((m_centerCoords[0] - m_fArray[0][row]) * (m_centerCoords[0] - m_fArray[0][row]) + (m_centerCoords[1] - m_fArray[1][row]) * (m_centerCoords[1] - m_fArray[1][row]) + (m_centerCoords[2] - m_fArray[2][row]) * (m_centerCoords[2] - m_fArray[2][row]));

			if (distance < m_radius)
				m_fArray[0][row] = m_inValue;
			else
				m_fArray[0][row] = m_outValue;
		}
		resultTable->putColumn(colPutId, 1, index, toIndex, m_fArray);

		index = toIndex + 1;
	}
	return true;
}