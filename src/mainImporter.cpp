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
#include <cstdlib>
#include <cstring>
#include <vector>
#ifdef WIN32
	#include <direct.h>
#endif
#include "commandline.h"
#include "visivoutils.h"
#include "time.h"
#include <sstream>
#include <fstream>
#ifdef WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif
int main(int argc, char*argv[])
{
  bool fitsRestore=false;
  std::string fitsOriginalName;
  std::string tmpOutName;  
  CommandLine *pComLine=new CommandLine;
  std::vector<std::string> args;
  bool usingParameterFile=false;

  if(argc==2)
  {
        std::string filename=argv[1];
	if(filename=="--help" || filename=="-help")
	{ 
		pComLine->showHelp();
        	exit(0);
	}
	usingParameterFile=true;
       std::ifstream parameterFile(filename.c_str());
       if(!parameterFile.is_open())
       {
	   std::cerr<<"Invalid Importer parameters filename"<<std::endl;
	   exit(1);
       } 
      std::string inputFile;
    
      while(!parameterFile.eof())  //!read file content
      {
        std::string tmp = "";
        getline(parameterFile, tmp); //!read row
	if(tmp.compare(0,1,"#")==0) continue; //!comment line
 	tmp = trim(tmp);
	if(tmp.size()==0) continue; //!ignore blank line
	if(tmp.find("=")==std::string::npos)
	{
		std::cerr<<"Invalid row in parameter file: "<<tmp<<std::endl;
		exit(1);
	}
	std::string key,value,argskey="--";
	key=tmp.substr(0,tmp.find("="));
	key=trim(key);
	value=tmp.substr(tmp.find("=")+1,tmp.size()-1);
	value=trim(value);
	if(key=="file")
	{
		inputFile=value;
		continue;
	}
        argskey.append(key);
	args.push_back(argskey);
	if(value!="true")  args.push_back(value);

      }
      if(inputFile.size()==0)
      {
	 std::cerr<<"Invalid row 'file' in parameter file"<<std::endl;
	 exit(1);
      }
      args.push_back(inputFile);	
      parameterFile.close();	  
  }
  
  if(!usingParameterFile)
  {
	std::clog << "narg " << argc<< std::endl;
    if(argc<3)
    { 
      pComLine->showHelp();
      exit(1);
    }

	// copy program arguments into vector
    int i;

    for (i=0;i<argc-1;i++) // known bug: bug correction on FitsTable with very long out name
    {
      std::string arg=argv[i];
      std::string arg1=argv[i+1];
      if((arg=="--fformat" || arg=="-fformat") && (arg1=="fitstable" ||arg1=="fitsimage" ))
      {
	for (i=1;i<argc;i++) 
    	{
		std::string arg2=argv[i];
		if((arg2=="--out" || arg2=="-out"))
		{
  			std::string tmp=argv[i+1];
			std::string tmpOutDir=getDir(tmp);
			tmpOutName=getName(tmp);
			chdir(tmpOutDir.c_str());
			if(tmpOutName.size()>28)
			{
				fitsRestore=true;
				fitsOriginalName=tmpOutName;
				std::stringstream fileNameOutputSStream;
				time_t rawtime;
				struct tm *timeinfo;
				char buffer [80];
				time ( &rawtime );
				timeinfo = localtime ( &rawtime );
				strftime (buffer,80,"%Y%m%d%H%M%S",timeinfo);
  				fileNameOutputSStream<<buffer<<".bin"; 
				tmpOutName=fileNameOutputSStream.str();
				strcpy(argv[i+1],tmpOutName.c_str());
				tmp=argv[i+1];
				tmp=argv[i+1];
			}
		}
	}
      }
    }

    bool nextStringContainBlank=false;
    std::string stringCollection;
    for (i=1;i<argc;i++)
    {
      if(nextStringContainBlank)
      {
	stringCollection.append(argv[i]);
	std::string tmp=argv[i+1];
	if(tmp.compare(0,1,"-")==0 || i==argc-2)
	{
	  stringCollection =trim(stringCollection);
	  args.push_back(stringCollection);
	  nextStringContainBlank=false;
	  stringCollection.erase();
	}
	stringCollection.append(" ");
	continue;
	
      }
      std::string stmp(argv[i]);
      if(stmp.compare("--datasetlist")==0 ||stmp.compare("-datasetlist") ==0 )
	nextStringContainBlank=true;
      
      args.push_back(argv[i]);
    }
  } //if(!usingParameterFile)


  if(pComLine->parseOption (args ) !=0)
  {
      return -1;
      
  }
  pComLine->loadFile();

 
  if((pComLine -> getRemoteFile() !="noremote") && !((pComLine -> getType() =="rawpoints") || (pComLine -> getType() =="xml") ) )
  { 
	std::string remoteFile;
	remoteFile=pComLine -> getRemoteFile();
	remove(remoteFile.c_str());
	if(pComLine -> getType() =="binary")
	{
		remoteFile=remoteFile.append(".head");
		remove(remoteFile.c_str());
	}
	
   }
  if(fitsRestore)
  {
	if(fitsOriginalName.find(".bin") == std::string::npos)
		fitsOriginalName=fitsOriginalName+".bin";
  	rename(tmpOutName.c_str(),fitsOriginalName.c_str());
	tmpOutName=tmpOutName+".head";
	fitsOriginalName=fitsOriginalName+".head";
  	rename(tmpOutName.c_str(),fitsOriginalName.c_str());
  }
//  std::cout<<"VisIVOImporter operation done."<<std::endl;		
  if ( pComLine)
    delete pComLine ;
  return 0;
}

