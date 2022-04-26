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
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "parametersparser.h"
#include "vscreatepath.h"
#include "vscreateslices.h"
#include "vscreategenericslices.h"
#include "vsloadhistory.h"

int main(int argc, char *argv[])
{
  std::string filename;
  std::stringstream commandParametersSStream;
  std::map<std::string, std::string> appParameters;
  std::map<std::string, std::string>::iterator iter;

  for (int i = 1; i < argc; i++)
  {
    commandParametersSStream << argv[i] << " ";
  }

  ParametersParser myparser(commandParametersSStream.str());

  appParameters = myparser.getParameters();

  iter = appParameters.find("op");
  if (iter == appParameters.end())
  {
    iter = appParameters.find("help");
    if (iter == appParameters.end())
      std::cerr << "No operation is requested" << std::endl;
    std::clog << "VisIVOUtils Version 2.1.1 July 2th 2013 " << std::endl
              << std::endl;
    std::cerr << "Syntax: VisIVOUtils --op utility  [PARAMETERS] [--help]" << std::endl;
    std::cerr << "valid utilities: createpath, orthoslices, genericslices, loadhistory " << std::endl;
    return EXIT_SUCCESS;
  }
  std::stringstream sstreamOp(iter->second);
  int idOp = -1;

  if (sstreamOp.str() == "createpath")
    idOp = 1;
  if (sstreamOp.str() == "orthoslices")
    idOp = 2;
  if (sstreamOp.str() == "genericslices")
    idOp = 3;
  if (sstreamOp.str() == "loadhistory")
    idOp = 4;

  switch (idOp)
  {
  /*** Create Path **/
  case 1:
  {
    iter = appParameters.find("help");
    if (iter != appParameters.end())
    {
      VSCreatePathUT op;
      op.printHelp();
      return 1;
    }

    VSCreatePathUT op;
    op.setParameters(appParameters);
    op.execute();
    break;
  }
  /***END Create Path OP **/
  /*** Create OrthoSlices **/
  case 2:
  {
    iter = appParameters.find("help");
    if (iter != appParameters.end())
    {
      VSCreateSlicesUT op;
      op.printHelp();
      return 1;
    }

    VSCreateSlicesUT op;
    op.setParameters(appParameters);
    op.execute();
    break;
  }
  /***END Create Slice OP **/
  /*** Create GenericSlices **/
  case 3:
  {
    iter = appParameters.find("help");
    if (iter != appParameters.end())
    {
      VSCreateGenericSlicesUT op;
      op.printHelp();
      return 1;
    }

    VSCreateGenericSlicesUT op;
    op.setParameters(appParameters);
    op.execute();
    break;
  }
  case 4:
  {
    iter = appParameters.find("help");
    if (iter != appParameters.end())
    {
      VSLoadHistoryUT op;
      op.printHelp();
      return 1;
    }

    VSLoadHistoryUT op;
    op.setParameters(appParameters);
    op.execute();
    break;
  }
  /***END Create Slice OP **/
  /*** Default **/
  default:
  {

    std::cerr << "No valid operation was given" << std::endl;
    std::cerr << "Syntax: VisIVOUtils --op utility  [PARAMETERS] [--help]" << std::endl;
    std::cerr << "An operation code is expected: createpath, orthoslices, genericslices, loadhistory" << std::endl;
    return 1;
  }
    /*** END Default  OP **/
  }
  return EXIT_SUCCESS;
}

//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif
