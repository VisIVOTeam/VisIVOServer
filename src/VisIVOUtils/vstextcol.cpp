/***************************************************************************
 *   Copyright (C) 2014 by Fabio Vitello                                   *
 *   fabio.vitello@oact.inaf.it                                            *
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
#include <iostream>
#include <fstream>
#include <sstream>
#include "parametersparser.h"
#include "vstextcol.h"
#include "visivoutils.h"
#include <boost/algorithm/string.hpp>

//---------------------------------------------------------------------
VSTextCol::VSTextCol()
//---------------------------------------------------------------------
{
    m_outFile = "column.ascii";
}
//---------------------------------------------------------------------
void VSTextCol::printHelp()
//---------------------------------------------------------------------
{
    std::cout << "Starting from an ascii file file, extract the value of a column as string" << std::endl
              << std::endl;
    ;

    std::cout << "Usage: VisIVOUtils --op textcol --file <table.ascii> --colname <column_name> [--help]" << std::endl;

    std::cout << "Example: VisIVOUtils --op textcol --file table.ascii --colname X " << std::endl
              << std::endl;

    std::cout << "Note: " << std::endl;

    std::cout << "--out output filename. Default filename column.ascii" << std::endl;
    std::cout << "--help produce this output " << std::endl;
    std::cout << "--file Input ASCII file " << std::endl;

    return;
}

//---------------------------------------------------------------------
bool VSTextCol::execute()
//---------------------------------------------------------------------
{

    if (!(getParameterAsString("file").empty() || getParameterAsString("file") == "unknown"))
    {
        std::stringstream sstmp;
        sstmp.str(getParameterAsString("file"));
        sstmp >> m_inFile;
    }
    else
    {
        std::cerr << "Error. The \"file\" option is not given." << std::endl;
        return false;
    }

    if (!(getParameterAsString("colname").empty() || getParameterAsString("colname") == "unknown"))
    {
        std::stringstream sstmp;
        sstmp.str(getParameterAsString("colname"));
        sstmp >> m_colName;
    }
    else
    {
        std::cerr << "Error. The \"colname\" option is not given." << std::endl;
        return false;
    }

    if (isParameterPresent("out"))
    {
        std::stringstream sstmp;
        sstmp.str(getParameterAsString("out"));
        sstmp >> m_outFile;
    }

    extractColumn();

    return true;
}

//---------------------------------------------------------------------
bool VSTextCol::extractColumn()
//---------------------------------------------------------------------
{

    // std::cout<<"INPUT FILE: "<<m_inFile<<std::endl;
    // std::cout<<"COL NAME: "<<m_colName<<std::endl;

    std::string line;
    std::string data;

    std::ifstream inFile;
    inFile.open(m_inFile.c_str());
    std::getline(inFile, line);

    // std::cout<<"***** "<<trim(line)<<" ******"<<std::endl;

    std::stringstream ss;
    ss << line;
    int pos = 0;
    while (!ss.eof()) //! fill m_fieldNames: name of columns
    {
        ss >> data;
        /*
            std::cout<<"----------------------------"<<std::endl;
            std::cout<<"*"<<data<<"*"<<std::endl;
            std::cout<<"*"<<m_colName<<"*"<<std::endl;
            std::cout<<"++++++++++++++++++++++++++++"<<std::endl;
        */

        if (data.compare(m_colName) == 0)
        {
            //            std::cout<<"DATA: "<<data<<" @pos: "<<pos<< std::endl;

            break;
        }
        pos++;
    }

    std::ofstream outFile;
    outFile.open(m_outFile.c_str());

    std::string colValue;
    int cnt;
    while (!inFile.eof()) //! read file content
    {
        cnt = 0;
        std::getline(inFile, line);

        std::stringstream ss_1;

        ss_1 << line;
        while (!ss_1.eof())
        {
            ss_1 >> colValue;

            if (cnt == pos)
            {
                break;
            }
            cnt++;
        }
        outFile << colValue << std::endl;
        // std::cout<<"-.-...-.>>>>>>>>>>>>>>>>>>>"<<colValue<<"<<<<<<<<<<<<<<<<<<<<<<<"<<std::endl;
    }
    inFile.close();
    outFile.close();

    return true;
}
