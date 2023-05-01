#ifndef MPIGADGETREADER_H
#define MPIGADGETREADER_H
#include "abstractreader.h"
#include "gadgetHeaders.h"

#include <vector>
#include <cstring>
#include <cstdlib>

class mpiGadgetReader: public AbstractReader
{
  public:
    mpiGadgetReader(std::string, std::string, int, std::vector<headerType2>, std::vector<std::string>, std::vector<std::string>);
    int startRead(bool);
  protected:
    std::vector<headerType2> m_pHeaderType2;
    std::vector<std::string> m_listOfBlocks;
    std::vector<std::string> m_blockNamesToCompare;
};
#endif