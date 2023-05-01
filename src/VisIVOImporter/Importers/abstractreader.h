/***************************************************************************
 *   Copyright (C) 2008 by Gabriella Caniglia, Marco Comparato *
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
#ifndef ABSTRACTREADER_H
#define ABSTRACTREADER_H
#include <string>
#include <vector>

class AbstractReader
{
   static const  int MAX_INT;

  public:
    AbstractReader();
    virtual ~AbstractReader() = default;    
    virtual int startRead(bool) = 0;
  protected:

    int m_numFiles;
    std::string m_fileName;
    std::string m_pathFileOut;
};

#endif