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
 **************************************************************************/

#ifndef __VisIVOServer_New072012__vstextcol__
#define __VisIVOServer_New072012__vstextcol__

#include "vsutils.h"

class VSTextCol : public VSUtilOp
{

public:
    VSTextCol();
    // ~VSLoadHistoryUT();
    void printHelp();
    bool execute();

private:
    std::string m_inFile;
    std::string m_outFile;
    std::string m_colName;
    bool extractColumn();
};

#endif /* defined(__VisIVOServer_New072012__vstextcol__) */
