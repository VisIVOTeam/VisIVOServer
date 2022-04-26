

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

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>

#include "optionssetter.h"
#include "vtkGraphicsFactory.h"
//#include "vtkImagingFactory.h"

int main(int argc, char *argv[])
{
  vtkGraphicsFactory::SetOffScreenOnlyMode(1);
  vtkGraphicsFactory::SetUseMesaClasses(1);
  /*vtk9 migration
  //vtkImagingFactory::SetUseMesaClasses( 1 );
*/
  OptionsSetter *pOptSett = new OptionsSetter;

  if (argc < 2)
  {
    pOptSett->showHelp();
    return 1;
  }

  std::vector<std::string> args;
  //   copy program arguments into vector
  int i;

  for (i = 1; i < argc; i++)
    args.push_back(argv[i]);
  /*  for (i=0;i<args.size();i++)
    std::clog << " " << args[i];
    std::clog <<" args.size()="<<args.size()<<std::endl;
   */
  if (pOptSett->parseOption(args) != 0)
  {
    std::cerr << "Invalid arguments. Operation Aborted." << std::endl;
    return -1;
  }

  VisIVOServerOptions opt = pOptSett->returnOptions();

  pOptSett->readData();
  pOptSett->images();

  pOptSett->writeHistory();

  if (pOptSett != 0)
    delete pOptSett;

  return 0;
}
