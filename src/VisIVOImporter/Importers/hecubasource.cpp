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
#include <cstdlib>
#include <cstring>
#include <StorageDict.h>

#include <StorageNumpy.h>
#include <StorageObject.h>
#include <KeyClass.h>
#include <ValueClass.h>
#include "hecubasource.h"
#include "visivoutils.h"
#include <iostream>
#include <fstream>
#include <sstream>

//---------------------------------------------------------------------
int HecubaSource::readData()
//---------------------------------------------------------------------
{
    particleObject pTest;
    pTest.getByAlias("particles");

    StorageNumpy s = pTest.particle;
    double* p = (double*)s.data;

    int meta1 = s.metas[0];
    int meta2 = s.metas[1];
	std::clog << meta1 << " " << meta2;
	writeGasParticles(s);
	return 0;
}

void HecubaSource::writeGasParticles(StorageNumpy s){

  	int idx = m_pointsBinaryName.rfind('.');
	std::string pathFileIn = m_pointsBinaryName.erase(idx, idx+4);
	std::string pathFileOut = pathFileIn;
	std::ofstream outfile((pathFileOut + "GAS" + ".bin").c_str(),std::ofstream::binary );
	std::vector<std::string> types; //!species block nameset 
  	types.push_back("MASS");
  	types.push_back("POS_X");
  	types.push_back("POS_Y");
  	types.push_back("POS_Z");
  	types.push_back("VEL_X");
  	types.push_back("VEL_Y");
  	types.push_back("VEL_Z");
  	types.push_back("RHO");
  	types.push_back("TEMP");
  	types.push_back("EPS");
  	types.push_back("METALS");
  	types.push_back("PHI");
	float *buffer=NULL;
    double* p = (double*)s.data;
    const int meta1 = s.metas[0];
    int meta2 = s.metas[1];
	buffer = new float[meta1];
	
	for(int t = 0; t < meta2; t++) {
		for(int i = 0; i < meta1; i++) {
			buffer[i] = static_cast<float>(p[i*meta2 + t]);
		}
		outfile.write((char *)(buffer), sizeof(float)*meta1);
	}
	outfile.close();

	std::string pathHeader = pathFileOut+"GAS"+ ".bin";
	makeHeader(meta1, pathHeader, types, m_cellSize,m_cellComp,m_volumeOrTable);
	return;
	
}

void HecubaSource::writeDarkParticles(StorageNumpy s){

  	int idx = m_pointsBinaryName.rfind('.');
	std::string pathFileIn = m_pointsBinaryName.erase(idx, idx+4);
	std::string pathFileOut = pathFileIn;
	std::ofstream outfile((pathFileOut + "DARK" + ".bin").c_str(),std::ofstream::binary );
	std::vector<std::string> types; //!species block nameset 
  	types.push_back("MASS");
  	types.push_back("POS_X");
  	types.push_back("POS_Y");
  	types.push_back("POS_Z");
  	types.push_back("VEL_X");
  	types.push_back("VEL_Y");
  	types.push_back("VEL_Z");
  	types.push_back("EPS");
  	types.push_back("PHI");
	float *buffer=NULL;
    double* p = (double*)s.data;
    const int meta1 = s.metas[0];
    int meta2 = s.metas[1];
	buffer = new float[meta1];
	
	for(int t = 0; t < meta2; t++) {
		for(int i = 0; i < meta1; i++) {
			buffer[i] = static_cast<float>(p[i*meta2 + t]);
		}
		outfile.write((char *)(buffer), sizeof(float)*meta1);
	}
	outfile.close();

	std::string pathHeader = pathFileOut+"DARK"+ ".bin";
	makeHeader(meta1, pathHeader, types, m_cellSize,m_cellComp,m_volumeOrTable);
	return;
	
}

void HecubaSource::writeStarParticles(StorageNumpy s){

  	int idx = m_pointsBinaryName.rfind('.');
	std::string pathFileIn = m_pointsBinaryName.erase(idx, idx+4);
	std::string pathFileOut = pathFileIn;
	std::ofstream outfile((pathFileOut + "STAR" + ".bin").c_str(),std::ofstream::binary );
	std::vector<std::string> types; //!species block nameset 
  	types.push_back("MASS");
  	types.push_back("POS_X");
  	types.push_back("POS_Y");
  	types.push_back("POS_Z");
  	types.push_back("VEL_X");
  	types.push_back("VEL_Y");
  	types.push_back("VEL_Z");
  	types.push_back("METALS");
  	types.push_back("TFORM");
  	types.push_back("EPS");
  	types.push_back("PHI");
	float *buffer=NULL;
    double* p = (double*)s.data;
    const int meta1 = s.metas[0];
    int meta2 = s.metas[1];
	buffer = new float[meta1];
	
	for(int t = 0; t < meta2; t++) {
		for(int i = 0; i < meta1; i++) {
			buffer[i] = static_cast<float>(p[i*meta2 + t]);
		}
		outfile.write((char *)(buffer), sizeof(float)*meta1);
	}
	outfile.close();

	std::string pathHeader = pathFileOut+"STAR"+ ".bin";
	makeHeader(meta1, pathHeader, types, m_cellSize,m_cellComp,m_volumeOrTable);
	return;
	
}

//---------------------------------------------------------------------
  int HecubaSource::readHeader()
//---------------------------------------------------------------------
{
    
  	headerObject header;
    header.getByAlias("header");

    nsph = header.nsph;
    ndark = header.ndark;
    nstar = header.nstar;

	std::clog << " nsph " << header.nsph << " ndark" << header.ndark << " nstar " << header.nstar<< std::endl;
    
	return 0;  
}