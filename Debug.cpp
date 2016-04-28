#include "Debug.h"

Debug::Debug() : std::ostream() {

}

Debug &operator<<(Debug &debug, const char s[]) {
  #ifdef DEBUG
  std::cerr << s;
  #endif // DEBUG
  return debug;
}

Debug &operator<<(Debug &debug, std::string &s) {
  #ifdef DEBUG
  std::cerr << s;
  #endif // DEBUG
  return debug;
}

Debug &operator<<(Debug &debug, char c) {
  #ifdef DEBUG
  std::cerr << c;
  #endif // DEBUG
  return debug;
}

Debug &operator<<(Debug &debug, int n) {
  #ifdef DEBUG
  std::cerr << n;
  #endif // DEBUG
  return debug;
}

