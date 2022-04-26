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
#ifndef VSCREATEPATH_H
#define VSCREATEPATH_H

/**
	@author
*/

#include "vsutils.h"

class VSCreatePathUT : public VSUtilOp
{
	float m_azimuthFrom;
	float m_azimuthTo;
	float m_elevationFrom;
	float m_elevationTo;
	double m_cameraFocalPointStart[3];
	double m_cameraFocalPointEnd[3];
	double m_cameraPoisitionStart[3];
	double m_cameraPoisitionEnd[3];
	double m_cameraRollStart;
	double m_cameraRollEnd;
	float m_zoomFrom;
	float m_zoomTo;
	float m_zStepFrame;
	int m_length;
	int m_framePerSecond;
	bool m_zoomend;
	std::string m_outFile;

	static const double INVALID_CAM;

public:
	VSCreatePathUT();
	~VSCreatePathUT(){};
	void printHelp();
	bool execute();
};

#endif
