#include <iostream>
#include "AuroraDB/auroradb.cpp"


int main(int argc, char** argv) {
  AuroraDB db;

  db.setLock("123456);
  db.cmdArgs(argc, argv);
  db.InteractiveMode();

  return 0;
}
