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


#ifndef HECUBASOURCE_H
#define HECUBASOURCE_H

#include "abstractsource.h"

#include <vector>
#include <string>
#include "StorageDict.h"
#include <StorageNumpy.h>
#include <StorageObject.h>
#include <StorageStream.h>
#include <KeyClass.h>
#include <ValueClass.h>

class headerObject:public StorageObject{
public:
HECUBA_ATTRS (
     double, time,
     int32_t, nbodies,
     int32_t, ndim,
     int32_t, nsph,
     int32_t, ndark,
     int32_t, nstar,
     int32_t, pad,
    )
};
class particleObject:public StorageObject{
    public:
    HECUBA_ATTRS (
        StorageNumpy, particle
        )
};
using Key = KeyClass<int32_t>;
using Value = ValueClass<particleObject>;
class particleDict : public StorageDict<Key,Value,particleDict>{
} ;
class HecubaSource : public AbstractSource
   
{
  public: 
    int readHeader();
    int readData();
        
  private:
    void writeGasParticles(StorageNumpy s);
    void writeDakParticles(StorageNumpy s);
    void writeStarParticles(StorageNumpy s);
    std::vector <std::string> m_fieldsNames;   
    unsigned int      npart_total[6];
    
    int nsph;
    int ndark;
    int nstar;
    char m_dataType, m_Endian;
  
};

#endif
