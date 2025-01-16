#include <iostream>
#include "AuroraDB/auroradb.cpp"


int main(int argc, char** argv) {
  
  AuroraDB db; //Calls AuroraDB class.
  
  db.addElement("admin"); //Sets up an admin element.
  db.set("admin", "Admin", "pAssw0rd"); //Adds a "Admin" user to the admin tag with the password "pAssw0rd".
  
  db.setLock("123456"); //Sets up a lock with a password that needs to be verified before anything else.
  
  db.cmdArgs(argc, argv); //Takes in cmd args.
  
  db.InterfaceMode(); //Starts an interactive menu.

  return 0;
}
