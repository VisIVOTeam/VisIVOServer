/***************************************************************************
 *   Copyright (C) 2008 by Gabriella Caniglia, Roberto Munzone *
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


#ifndef GADGETSOURCE_H
#define GADGETSOURCE_H

#include "abstractsource.h"
#include "gadgetHeaders.h"

#include <vector>
#include <string>

class GadgetSource : public AbstractSource
   
{
  public: //! Read the headerType2 file and set the basic table parameters
    int readHeader();
    int readData();
        
  private:
    std::vector <std::string> m_fieldsNames;   
    unsigned int      npart_total[6];
    int numFiles = 1;
    int m_nRows;
    char m_dataType, m_Endian;
    
     int m_snapformat;
     char tmpType[4]; int numBlock; int m_sizeBlock[1];
    
    std::vector<std::string> checkType;
    std::string tagType;
    	
    void swapHeaderType2();
    void swapHeaderType1();
    
    std::vector<headerType2> m_pHeaderType2;
    struct headerType1 m_pHeaderType1;
    int readMultipleHeaders(int, std::string, bool);
    int checkMultipleFiles(int, std::string);
    void updateNpart2(int);
  
};
  

#endif
