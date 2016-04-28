#ifndef SIK1_DEBUG_H
#define SIK1_DEBUG_H

#include <iostream>

class Debug : public std::ostream {
public:
  Debug();
};

Debug &operator<<(Debug &debug, const char *s);
Debug &operator<<(Debug &debug, std::string &s);
Debug &operator<<(Debug &debug, char c);
Debug &operator<<(Debug &debug, int n);


#endif //SIK1_DEBUG_H
