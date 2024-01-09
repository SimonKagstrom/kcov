#ifndef _NOCOVER_H_
#define _NOCOVER_H_

#include <string>
namespace kcov
{

bool shouldCover(std::string& line, std::string& file_name);

}

#endif
