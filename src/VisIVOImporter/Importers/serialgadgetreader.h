#ifndef SERIALGADGETREADER_H
#define SERIALGADGETREADER_H
#include "abstractreader.h"
#include "gadgetHeaders.h"

#include <vector>
#include <cstring>
#include <cstdlib>

class serialGadgetReader: public AbstractReader
{
  public:
    serialGadgetReader(std::string, std::string, int, std::vector<headerType2>, int, std::vector<std::string>);
    int startRead(bool);
  protected:
    std::vector<headerType2> m_pHeaderType2;
    int m_numBlocks;
    std::vector<std::string> m_blockNamesToCompare;
};
#endif