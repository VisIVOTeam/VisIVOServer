/***************************************************************************
 *   Copyright (C) 2008 by Fabio Vitello                                   *
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
#include "vsloadhistory.h"
#include "tinyxml.h"
#include "tinystr.h"

//---------------------------------------------------------------------
VSLoadHistoryUT::VSLoadHistoryUT()
//---------------------------------------------------------------------
{
    m_outFile = "VisIVO.sh";
}
//---------------------------------------------------------------------
void VSLoadHistoryUT::printHelp()
//---------------------------------------------------------------------
{
    std::cout << "Starting from a history xml file, create a bash script for re execution" << std::endl
              << std::endl;
    ;

    std::cout << "Usage: VisIVOUtils --op loadhistory --file <hist.xml> [--help]" << std::endl;

    std::cout << "Example: VisIVOUtils --op loadhistory --file hist.xml" << std::endl
              << std::endl;

    std::cout << "Note: " << std::endl;

    std::cout << "--out output filename. Default filename VisIVO.sh" << std::endl;
    std::cout << "--help produce this output " << std::endl;
    std::cout << "--file Input history file " << std::endl;

    return;
}

//---------------------------------------------------------------------
bool VSLoadHistoryUT::execute()
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
        std::cerr << "Error. The file option is not given." << std::endl;
        return false;
    }

    if (isParameterPresent("out"))
    {
        std::stringstream sstmp;
        sstmp.str(getParameterAsString("out"));
        sstmp >> m_outFile;
    }

    loadXml();

    return true;
}

//---------------------------------------------------------------------
bool VSLoadHistoryUT::loadXml()
//---------------------------------------------------------------------
{
    // std::cout<<"Load history file: "<<m_inFile<<std::endl;

    TiXmlDocument doc(m_inFile.c_str());
    bool load = doc.LoadFile();
    if (load)
    {
        TiXmlElement *pRoot, *pComm, *pParam, *pIn, *pOut;
        pRoot = doc.FirstChildElement("history");
        if (pRoot)
        {
            std::ofstream bashFile;
            bashFile.open(m_outFile.c_str());
            bashFile << "#!/bin/bash" << std::endl;

            // Parse parameters
            pComm = pRoot->FirstChildElement("command");
            while (pComm)
            {
                // std::cout << "Command: value='" << pComm->Attribute("value") << "'" << std::endl;

                bashFile << pComm->Attribute("value") << " ";

                pParam = pComm->FirstChildElement("param");
                while (pParam)
                {
                    // std::cout << "      Param: name='"<<pParam->Attribute("name") <<"' value='" << pParam->Attribute("value") << "'" << std::endl;

                    bashFile << "--" << pParam->Attribute("name") << " " << pParam->Attribute("value") << " ";
                    pParam = pParam->NextSiblingElement("param");
                }

                pIn = pComm->FirstChildElement("input");
                while (pIn)
                {
                    // std::cout << "      Input: value='" << pIn->Attribute("value") << "'" << std::endl;

                    bashFile << "--file \"" << pIn->Attribute("value") << "\" ";
                    pIn = pIn->NextSiblingElement("input");
                }

                pOut = pComm->FirstChildElement("output");
                while (pOut)
                {
                    // std::cout << "      Output: value='" << pOut->Attribute("value") << "'" << std::endl;
                    bashFile << "--out " << pOut->Attribute("value") << " ";
                    pOut = pOut->NextSiblingElement("output");
                }

                pComm = pComm->NextSiblingElement("command");
                bashFile << std::endl;
            }

            //     std::cout << std::endl;

            bashFile.close();
        }
        else
        {
            std::cout << "Cannot find 'history' node" << std::endl;
            return false;
        }
        return true;
    }
    return false;
}
