#ifndef GHEADER_H_
#define GHEADER_H_

struct headerType2
{
  char     first_fortran_legacy[4]; //!snapformat=2 : you have to "jump" 4+2*4 =12 bytes and you'll find the the Block size
  int      boh[1];
  int	   nBlock[1];	
  int      size[1];
  unsigned int      npart[6];
  double   mass[6];
  double   time[1];
  double   redshift[1];
  int      flag_sfr[1];
  int      flag_feedback[1];
  int      npartTotal[6];
  int      foling[1];
  int      num_files[1];
  double   BoxSize[1];
  double   Omega0[1];
  double   OmegaLambda[1];
  double   HubbleParam[1];
  int      FlagAge[1];
  int      FlagMetals[1];
  int      NallWH[6];
  int      flag_entr_ics[1]; 
  char     fill[256- 6*sizeof(int)- 6*sizeof(double)- 2*sizeof(double)- 2*sizeof(int)- 6*sizeof(int)- 2*sizeof(int)- 
      4*sizeof(double)- 9*sizeof(int)]; /*! fills to 256 Bytes */
  int      final_boh[1];
  int	   final_nBlock[1];
  
  char     tagFirstBlock[4];
  int      first_boh[1];
  int      first_nBlock[1];
  int      sizeFirstBlock[1];   
};

struct headerType1
{
	//char     first_fortran_legacy[4]; //snapformat=2 : you have to "jump" 4+2*4 =12 bytes and you'll find the the Block size
	//int      boh[2];
	int      size[1];
	unsigned int      npart[6];
	double   mass[6];
	double   time[1];
	double   redshift[1];
	int      flag_sfr[1];
	int      flag_feedback[1];
	int      npartTotal[6];
	int      foling[1];
	int      num_files[1];
	double   BoxSize[1];
	double   Omega0[1];
	double   OmegaLambda[1];
	double   HubbleParam[1];
	int      FlagAge[1];
	int      FlagMetals[1];
	int      NallWH[6];
	int      flag_entr_ics[1]; 
	char     fill[256- 6*sizeof(int)- 6*sizeof(double)- 2*sizeof(double)- 2*sizeof(int)- 6*sizeof(int)- 2*sizeof(int)- 
			4*sizeof(double)- 9*sizeof(int)]; /* fills to 256 Bytes */
  int      final_boh[1];
	int      sizeFirstBlock[1];
};
#endif