
#pragma once


/* 

Copyright (c) 2024 Wicks (Original Author of AuroraDB)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/


#include <iostream>      
#include <string>        
#include <unordered_map> 
#include <fstream>       
#include <mutex>         
#include <shared_mutex>  
#include <thread>        
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <unistd.h>      
#include <stdexcept>     
#include <sstream>       
#include <vector>        
#include <functional>    
#include <algorithm>     
#include <cstring>       
#include <ctime>         
#include <limits.h>      

// Define, the best thing in C++.
#define ERROR_MSG(string) (std::cerr << "ERROR!: " << string << std::endl)
#define ServerMessage(input, socket) (const char* message = input; send(socket, message, strlen(message), 0))

// All using statements (Makes my life easier.)
using std::cin;
using std::cout;
using std::cerr;
using std::shared_lock;
using std::string;
using std::unique_lock; 

class AuroraDB {
private:
    std::unordered_map<string, string> db;     
    std::unordered_map<string, string> buffer; 
    std::unordered_map<string, bool> tags;     
    std::unordered_map<string, std::vector<std::pair<string, string>>> tagged_users; 
    std::shared_mutex db_mutex;                
    string current_tag = "default";            

    // Inline internal function definitions
    inline std::string hash(const std::string &input) {
        std::hash<std::string> hasher;
        size_t hashed_value = hasher(input);
        return std::to_string(hashed_value);
    }

    inline std::string get_exe_path() {
        char buffer[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
        std::string exePath(buffer, count);
        return exePath.substr(0, exePath.find_last_of('/'));
    }

    inline void load(const string &file) {
        std::ifstream inputFile(file);
        if (!inputFile) {
            return;
        }

        string line;
        current_tag = "default";

        while (std::getline(inputFile, line)) {
            if (line.substr(0, 12) == "<AuroraDB::") {
                size_t start = line.find("-") + 2;
                size_t end = line.find(">", start);
                if (start != string::npos && end != string::npos) {
                    current_tag = line.substr(start, end - start);
                }
            } else if (!line.empty()) {
                std::istringstream iss(line);
                string username, password;
                if (iss >> username >> password) {
                    db[current_tag + ":" + username] = password;
                }
            }
        }
    }

    inline void save(const string &filename) {
        std::ofstream outfile(filename, std::ios::out | std::ios::trunc);
        
        if (!outfile.is_open()) {
            throw std::runtime_error("Error opening file for saving.");
        }

        std::unordered_map<string, std::vector<std::pair<string, string>>> tagged_users;
        
        for (const auto &pair : db) {
            size_t tag_separator = pair.first.find(':');
            
            if (tag_separator != string::npos) {
                string tag = pair.first.substr(0, tag_separator);
                string username = pair.first.substr(tag_separator + 1);
                tagged_users[tag].emplace_back(username, pair.second);
            }
        }

        for (const auto &tag_group : tagged_users) {
            outfile << "<AuroraDB::" << tag_group.first << "> -\n";
            
            for (const auto &user : tag_group.second) {
                outfile << user.first << " " << user.second << "\n";
            }
            
            outfile << "</AuroraDB>\n\n"; 
        }

        outfile.flush();
        outfile.close();
    }

    inline string GetCurrentTime() {
        std::time_t currentTime = std::time(nullptr);
        std::tm* localTime = std::localtime(&currentTime);

        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);
    
        return string(buffer);
    }

    inline void WriteToLog(const std::string &message) {
        std::ofstream outfile("storage/log.txt", std::ios::out | std::ios::app);

        if (!outfile.is_open()) {
            throw std::runtime_error("Error opening file for saving.");
        }

        outfile << GetCurrentTime() << " [AuroraDB] " << message << "\n";
    }

    inline void printAuroraDB() {
        printf("   _____                                   ________ __________ \n");
        printf("  /  _  \\  __ _________  ________________  \\______ \\\\______   \\\n");
        printf(" /  /_\\  \\|  |  \\_  __ \\/  _ \\_  __ \\__  \\ |    |  \\|    |  _/\n");
        printf("/    |    \\  |  /|  | \\(  <_> )  | \\/ __ \\|    `   \\    |   \\\n");
        printf("\\____|__  /____/ |__|   \\____/|__|  (____  /_______  /______  /\n");
        printf("        \\/                               \\/        \\/       \\/ \n");
    }

    inline void thread(const string &function, const string &name, const string &password) {
        std::vector<std::thread> threads;
        try {
            if (function == "get") {
                threads.push_back(std::thread([this, &name]() {
                    this->get(name);
                }));
            } else if (function == "set") {
                threads.push_back(std::thread([this, &name, &password]() {
                    this->set(name, password);
                }));
            } else if (function == "rm") {
                threads.push_back(std::thread([this, &name]() {
                this->rm(name);
            }));
            } else if (function == "compare") {
                threads.push_back(std::thread([this, &name, &password]() {
                    this->compare(name, password);
                }));
            } else {
                throw std::runtime_error("Unknown thread function");
            }

            for (auto &t : threads) {
                t.join();
            }
        } catch (const std::exception& e) {
            cerr << "Thread error: " << e.what() << "\n";
        }
    }

    inline void init() {
        Options options;
        Options* pOptions = &options;
        
        if (pOptions->storage_path.empty()) {
            options.storage_path = "storage/storage.txt";
        }
    }

public:
    typedef struct Options {
        string storage_path;
    } Options;

    AuroraDB() {
        try {
            tags["default"] = true;
            
            load("storage/storage.txt");
        } catch (const std::runtime_error &e) {
            cerr << "Error loading database: " << e.what() << "\n";
        }
    }

    ~AuroraDB() {
        try {
            save("storage/storage.txt");
        } catch (const std::runtime_error &e) {
            cerr << "Error saving database: " << e.what() << "\n";
        }
    }

     //----------------------------------------------------------------------------------------------------------------------------------------------

     void connect(const int &port) {
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;         //Use TCP
        serverAddress.sin_port = htons(port);       // Set port to 8080.
        serverAddress.sin_addr.s_addr = INADDR_ANY; // Change to accept only one IP.

        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            close(serverSocket);
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(serverSocket, 10000) < 0) { //If failed to listen, throw error.
            close(serverSocket);
            throw std::runtime_error("Failed to listen on socket");
        }

        int clientSocket = accept(serverSocket, nullptr, nullptr); //Accept the clientsocket.
        if (clientSocket < 0) { //If no clientsocket.
            close(serverSocket);
            throw std::runtime_error("Failed to accept client connection");
        }

        bool quit = false;
        while (!quit) {
            char buffer[1024] = {0};
            ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived <= 0) {
                break; // Connection closed or error.
            }

            buffer[bytesReceived] = '\0'; // Null-terminate the received data.

            std::string command, name, password;
            std::istringstream stream(buffer);
            
            if (!(stream >> command >> name >> password)) {
                ERROR_MSG("Invalid command format");
                continue;
            }

            try {
                if (command == "get") {
                    thread("get", name, password);
                } else if (command == "set") {
                    thread("set", name, password);
                } else if (command == "compare") {
                    thread("compare", name, password);
                } else if (command == "quit") {
                    quit = true;
                } else {
                    ERROR_MSG("Unknown command: " + command);
                }
            } catch (const std::exception& e) {
                ERROR_MSG(e.what());
            }
        }
        //Close sockets.
        close(clientSocket);
        close(serverSocket);
    }

    //----------------------------------------------------------------------------------------------------------------------------------------------
    void cmdArgs(int argc, char *argv[]) {
        if (argc > 1 && argc < 5) {  // If argument count is between 2 and 4 inclusive.
            std::string cmd, name, password, tag; // Declare cmd, name, password, tag.
            try {
                cmd = argv[1];                                 // Assign cmd to argv[1].
                name = (argc > 2) ? argv[2] : "";              // Assign name to argv[2] if argc > 2.
                password = (argc > 3) ? argv[3] : "";          // Assign password to argv[3] if argc > 3.
                tag = (argc > 4) ? argv[4] : "";              // Assign tag to argv[4] if argc == 4.

               if (cmd == "get" && argc > 2) {
                    std::cout << (get(name) == 0 ? "0" : "-1") << "\n";
               } else if (cmd == "set" && argc == 3) {
                    set(name, password);
                } else if (cmd == "set" && argc == 4) {
                    set(tag, name, password);
                } else if (cmd == "rm" && argc > 2) {
                    rm(name);
                } else if (cmd == "compare" && argc > 3) {
                    compare(name, password);
                } else {
                    throw std::runtime_error("Error: Must use (set (Name, Password), get (Name), rm (Name), compare (Name, Password))");
                }
            } catch (std::runtime_error &e) {
                std::cerr << "Error: Argument not recognised: " << e.what() << "\n";
            }
        } else if (argc >= 5) { // Ensure the comparison is clear for arguments exceeding limit.
            std::cerr << "Error: Arguments exceeding limit, maximum of three arguments.\n";
        } else {
            std::cout << "No command line arguments given. Total arguments: " << argc << std::endl;
        }
    }


    void addTag(const string& tag) {
        
        if (tag.empty() || std::all_of(tag.begin(), tag.end(), ::isspace)) {
            cerr << "Error: Tag cannot be empty\n"; //If empty of whitespace.
            return;
        }

    
        if (tags.find(tag) != tags.end()) { // Check if tag already exists.
            cerr << "Error: Tag '" << tag << "' already exists\n"; 
            return;
        }
        
        tags[tag] = true;  // Add the tag to "tags" map.
    
        cout << "Tag added: " << tag << "\n";
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------
    void set(const string tag, const string &username, const string &password) {
        if (tags.find(tag) == tags.end()) {
            ERROR_MSG("Tag doesn't exist, please create it using addTag()");
        }
        
        if (username.empty() || std::all_of(username.begin(), username.end(), ::isspace)) { // Prevent empty or whitespace only usernames.
            cerr << "Error: Username cannot be empty\n";
            return;
        }

        for (const auto& entry : db) { //Check for every tag.
            if (entry.first.substr(entry.first.find(':') + 1) == username) { //if entry exist under the same tag,
                cerr << "Error: Username already exists in database\n"; //Throw error.
                return;
            }
        }

        unique_lock<std::shared_mutex> lock(db_mutex);   

        WriteToLog(string("SET " + username + " " + password + " 0 ")); //Write ot log.
        
        db[tag + ":" + username] = hash(password);   
        
        cout << "Database: User added: " << username << "\n";  // Write to log.
        
    }




    void set(const string &username, const string &password) { 
        set("default", username, password); //Set the tag to default tag.
    }


    

    int get(const string &username) {
        if (username.empty()) {
            cerr << "Error: Username cannot be empty\n";
            return -1;
        } 
        shared_lock<std::shared_mutex> lock(db_mutex);
    
        // Search through all keys in the database.
        for (const auto& entry : db) {
            // Check if the entry's key ends with the given username.
            if (entry.first.substr(entry.first.find(':') + 1) == username) {
                WriteToLog(string("GET " + username + " 0 "));
                cout << "User: " << entry.first << ":" << entry.second << "\n";
                return 0;
            }
        }

        cout << "Error: User not found\n";
        return -2;
    }

     //-----------------------------------------------------------------------------------------------------------------------------------------------

    void setLock(const string& password) { 
        string input;
        while (true) {
            cout << "Verification password: ";
            cin >> input;

            if (input == password) { //Check if the password matches.
                cout << "Password confirmed, Welcome back!" << "\n"; //Print out result to user.
                break;
            } else {
                cout << "Password denied, try again."; //Print out result to user.
            }
        }
    }


    int InterfaceMode() {
        string action, username, password;
        printAuroraDB();
        cout << "Welcome to AuroraDB!" << "\n";
        while (true) {
            cout << "\n\nPlease enter which of following actions you want to do \n (1) Set user. (2) Remove user. (3) Get user. (4). Compare user. (5) Quit : "; //Welcome message.
            cin >> action;

            if (action != "5" && action != "4" && action != "3" && action != "2" && action != "1") {
                cout << "Invalid action.. \033 \n";
                return -1;
            }

           
            if (std::stoi(action) != 5) {
                cout << "Please enter the username: ";
                cin >> username;
            }
            

        
            if (std::stoi(action) != 3 && std::stoi(action) != 5) { // Only ask for password if not using "get".
                cout << "Please enter the password: ";
                cin >> password;
            }


            switch (std::stoi(action)) {
                case 1:
                    set(username, password);
                    break;
                case 2:
                    rm(username);
                    break;
                case 3:
                    get(username);
                    break;
                case 4:
                    compare(username, password);
                    break;
                case 5:
                    return 0;
            }
        }
       
    }

    //----------------------------------------------------------------------------------------------------------------------------------------------
    bool compare(const string &username, const string &password) {
        if (username.empty() || password.empty()) {
            cerr << "Error: Username and password cannot be empty\n";  //Check if the password or username is empty, return error.
            return false;
        }

        shared_lock<std::shared_mutex> lock(db_mutex);
        for (const auto& entry : db) {
         if (entry.first.substr(entry.first.find(':') + 1) == username) { // Check if the entry's key ends with the given username.
            if (entry.second == hash(password)) { // Compare the hashed password.
                cout << "Password matched for user: " << username << "\n"; //Print out result to user.
                return true;
            } else {
                cout << "No match found for user: " << username << "\n";  //Print out result to user.
                return false;
            }
        }
    }

    cout << "User not found: " << username << "\n";  //Print out result to user.
    return false; 
}

    void rm(const string &username) {
        if (username.empty()) {
            cerr << "Error: Username cannot be empty\n";
            return;
        }

        unique_lock<std::shared_mutex> lock(db_mutex); // Locks the db.
        WriteToLog(string("RM " + username + " " + " 0 "));
    
        
        bool userRemoved = false; // Bool to track if any user was removed.
    
        // Use iterator to safely remove while iterating.
        for (auto it = db.begin(); it != db.end(); ) {
            if (it->first.substr(it->first.find(':') + 1) == username) { //Find the semicolon to seperate the tag from the username.
                it = db.erase(it); //Erase the username.
                userRemoved = true;
            } else {
                ++it; //Else move on.
            }
        }
    
        if (userRemoved) {
            cout << "User removed: " << username << "\n";  //Print out result to user.
        } else {
            cout << "User not found: " << username << "\n";  //Print out result to user.
        }
    }
};


    
// Database supports multiple different solutions, both command line arguments, networking, interactive command line menu and you can write commands under here: 
// Threading is being worked on!
// Have fun, greetings Wicks.
   
