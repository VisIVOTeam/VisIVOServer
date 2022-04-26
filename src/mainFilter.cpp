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
#include <vector>
#include <map>
#include "parametersparser.h"
#include "startFilter.h"
#ifdef VSMPI
#pragma message "MPI-PARALLEL compilation"
#include "mpi.h"
#else
#pragma message "SERIAL compilation"
#endif

int main(int argc, char *argv[])
{
#ifdef VSMPI
	std::cout << "VSMPI" << std::endl;
#endif
	int size = 1, rank = 0;
	std::string paramFilename;

	std::stringstream commandParametersSStream;
	std::map<std::string, std::string> appParameters;
	std::map<std::string, std::string>::iterator iter;

	bool paramFileGiven = false;

#ifdef VSMPI

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#endif

	if (argc == 2) // The parameter File is given!
	{
		paramFileGiven = true;
		std::string argStr;
		argStr.assign(argv[1]);
		if (argStr == "--op" || argStr == "-op")
			paramFileGiven = false;
		if (argStr == "-help" || argStr == "--help")
			paramFileGiven = false;
		if (paramFileGiven)
			paramFilename = argStr;
	}

	if (paramFileGiven) // fill  appParameters with parameter file content
	{
		ParametersParser myparser(paramFilename, 1);
		appParameters = myparser.getParameters(); //! local copy of the options given
	}
	else
	{
		bool fileIsAtTheEnd = true;
		for (int i = 1; i < argc; i++)
		{
			std::string argStr;
			argStr.assign(argv[i]);
			if (argStr == "--file" || argStr == "-file")
			{
				fileIsAtTheEnd = false;
				break;
			}
			if (argStr == "--op" || argStr == "-op")
			{
				std::string argStrop;
				if (i + 1 >= argc)
				{
					std::cerr << "Invalid options" << std::endl;
					exit(1);
				}
				argStrop.assign(argv[i + 1]);
				if (argStrop == "interpolate" || argStrop == "AHFstep" ||
					argStrop == "merge" || argStrop == "visualop" || argStrop == "append")
				{
					fileIsAtTheEnd = false;
					break;
				}
			}
		}

		if (fileIsAtTheEnd)
		{
			for (int i = 1; i < argc - 1; i++)
				commandParametersSStream << argv[i] << " ";
			commandParametersSStream << "--file " << argv[argc - 1] << " ";
		}
		else
			for (int i = 1; i < argc; i++)
				commandParametersSStream << argv[i] << " ";

		ParametersParser myparser(commandParametersSStream.str());
		appParameters = myparser.getParameters();
	} // if paramFileGiven ... else
	int MpiSize = size;
	iter = appParameters.find("mpisize");
	if (iter != appParameters.end())
		MpiSize = atoi(iter->second.c_str());
	if (MpiSize > size)
		MpiSize = size;

// this part implements the multi processes with MPI. It is the map that contains the info for multiprocess
// set size ==> size=1 means serial
///////
#ifdef VSMPI
	int *ranks;
	ranks = new int[MpiSize];
	for (int i = 0; i < MpiSize; i++)
		ranks[i] = i;

	// create a new communicator
	MPI_Group origGroup, newGroup;
	MPI_Comm NEW_COMM;
	MPI_Comm_group(MPI_COMM_WORLD, &origGroup);
	MPI_Group_incl(origGroup, MpiSize, ranks, &newGroup);
	MPI_Comm_create(MPI_COMM_WORLD, newGroup, &NEW_COMM);
	startFilter startFilter(appParameters, NEW_COMM); // test of MPI

	//////////

	// startFilter startFilter(appParameters); //default argument MPI_COMM_WORLD assumed in case of MPI
	//#ifdef VSMPI
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

#endif
	startFilter startFilter(appParameters);
	return EXIT_SUCCESS;
}

//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif
