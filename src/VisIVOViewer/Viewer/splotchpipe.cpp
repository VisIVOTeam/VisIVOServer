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

#include "splotchpipe.h"
#include <cstdlib>
#include <cstring>

#include "visivoutils.h"

#include <sstream>
#include <algorithm>

int splotchMain(VisIVOServerOptions opt);

//---------------------------------------------------------------------
SplotchPipe::SplotchPipe(VisIVOServerOptions options)
//---------------------------------------------------------------------
{
  m_visOpt = options;
}
//---------------------------------
SplotchPipe::~SplotchPipe()
//---------------------------------
{
}

//------------------------------------------------------------------------------
int SplotchPipe::createPipe()
//------------------------------------------------------------------------------
{
  int i = 0;
  int j = 0;

  std::ifstream inFile;
  inFile.open(m_visOpt.path.c_str());
  if (!inFile.is_open())
  {
    std::cerr << "Input splotch.par file does not exist" << std::endl;
    return 0;
  }
  //! open binary file. m_visOpt is the structure (parameter of the constructor) that contain alla data to be visualized
  // std::clog<<m_visOpt.path.c_str()<<std::endl;

  inFile.close();
  splotchMain(m_visOpt);

  return 0;
}
