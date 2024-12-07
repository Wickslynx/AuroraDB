#include <iostream>      // For cout, cin, and more.
#include <string>        // String library.
#include <unordered_map> // The unordered_map.
#include <fstream>       // For file usage.
#include <mutex>         // Imports standard mutex library.
#include <shared_mutex>  // Imports an add-on to the standard mutex library.
#include <thread>        // Imports multi-threading.
#include <netinet/in.h>  // For networking.
#include <sys/socket.h>  // For networking.
#include <unistd.h>      // For networking.
#include <stdexcept>     // For all error, Will be replaced and removed soon..
#include <sstream>       // For getting strings.
#include <vector>
#include <functional>
#include <algorithm>     // Added for additional string and algorithm functions

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
    std::unordered_map<string, string> db;     // Starts unordered map.
    std::unordered_map<string, string> buffer; // Starts a buffer to load data into.
    std::unordered_map<string, std::vector<std::pair<string, string>>> tagged_users; //Starts a map to load the users with the same tag into.
    std::shared_mutex db_mutex;                // Starts a mutex that's called "db_mutex".

    //----------------------------------------------------------------------------------------------------------------------------------------------
    //Internal function definitions.
    std::string hash(const std::string &input) {
        std::hash<std::string> hasher; //Declare the hasher.
        size_t hashed_value = hasher(input); //Hash the input.
        return std::to_string(hashed_value);  //return the hashed value.
    }

    //----------------------------------------------------------------------------------------------------------------------------------------------

    void load(const string &file) {
        std::ifstream inputFile(file); // Opens file in "read" mode.
        if (!inputFile) {
            throw std::runtime_error("Error opening file for loading."); // If error, send error message.
        }

        string line;
        string current_tag = "default"; // Js use default tag if no tag specified... ..

        // First, look for the tag
        if (std::getline(inputFile, line)) {
            if (line.substr(0, 12) == "<AuroraDB::") {
                // Get tag name between <AuroraDB::  and >. 
                size_t start = line.find("-") + 2;
                size_t end = line.find(">", start);
                if (start != string::npos && end != string::npos) {
                    current_tag = line.substr(start, end - start); //Set the current_tag to the tag.
                }
            } else {
                inputFile.seekg(0); //If no tag, set it to the beginning.
            }
        }

        string username, password; // Declares username and password.
            
        while (inputFile >> username >> password) {  // While able to read data.
            if (username == "</AuroraDB>") break; //check for the closing, tag: Break.

            // Sanitize input to prevent injection
            if (!username.empty() && !password.empty()) {
                db[current_tag + ":" + username] = hash(password); // Add user to database with the tag.
            }
        }
    }

    void save(const string &filename) {
        std::ofstream outfile(filename, std::ios::out); // Open file in "out" mode.
        
        if (!outfile.is_open()) {
            throw std::runtime_error("Error opening file for saving."); // If error, send error message.
        }

        std::unordered_map<string, std::vector<std::pair<string, string>>> tagged_users; // Group users by their database tag.
        
        for (const auto &pair : db) {
            size_t tag_separator = pair.first.find(':');  // Split the key into tag and username
            
            if (tag_separator != string::npos) {
                string tag = pair.first.substr(0, tag_separator);
                string username = pair.first.substr(tag_separator + 1);
                tagged_users[tag].emplace_back(username, pair.second);
            }
        }

        // Write each database tag group
        for (const auto &tag_group : tagged_users) {
            // Write tag at the beginning of the user list
            outfile << "<AuroraDB::" << tag_group.first << "> -\n";
            
            // Write users in this tag group
            for (const auto &user : tag_group.second) {
                outfile << user.first << " " << user.second << "\n";
            }
            
            // Close the tag
            outfile << "</AuroraDB>\n\n";
        }
    }

    //----------------------------------------------------------------------------------------------------------------------------------------------

    void connect(const int &port) {
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        sockaddr_in serverAddress;
        std::memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;         //Use TCP
        serverAddress.sin_port = htons(port);       // Set port to 8080.
        serverAddress.sin_addr.s_addr = INADDR_ANY; // Change to accept only one IP.

        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            close(serverSocket);
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(serverSocket, 10000) < 0) {
            close(serverSocket);
            throw std::runtime_error("Failed to listen on socket");
        }

        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            close(serverSocket);
            throw std::runtime_error("Failed to accept client connection");
        }

        bool quit = false;
        while (!quit) {
            char buffer[1024] = {0};
            ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived <= 0) {
                break; // Connection closed or error
            }

            buffer[bytesReceived] = '\0'; // Null-terminate the received data

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

        close(clientSocket);
        close(serverSocket);
    }

public:
    AuroraDB() {
        try {
            load("storage.txt"); // Runs loading method.
            // connect(8080); //Uncomment to set networking to default start mode.
        } catch (const std::runtime_error &e) {
            cerr << "Error loading database: " << e.what() << "\n";
        }
    }

    ~AuroraDB() {
        try {
            save("storage.txt");
        } catch (const std::runtime_error &e) {
            cerr << "Error saving database: " << e.what() << "\n";
        }
    }
  
    //----------------------------------------------------------------------------------------------------------------------------------------------
    void cmdArgs(int argc, char *argv[]) {
        if (argc > 1 && argc < 5) {  // If argument is under 4 and over 1.
            std::string cmd, name, password; // Declare cmd, name, password.
            try {
                cmd = argv[1];                        // Assign cmd to argv[1].
                name = (argc > 2) ? argv[2] : "";     // Assign name to argv[2].
                password = (argc > 3) ? argv[3] : ""; // Assign password to argv[3].

                if (cmd == "get" && argc > 2) {
                    cout << (get(name) == 0 ? "Get successful" : "Get failed") << "\n";
                } else if (cmd == "set" && argc > 3) {
                    set(name, password);
                } else if (cmd == "rm" && argc > 2) {
                    rm(name);
                } else if (cmd == "compare" && argc > 3) {
                    compare(name, password);
                } else {
                    throw std::runtime_error("Error: Must use (set (Name, Password), get (Name), rm (Name), compare (Name, Password))");
                }
            } catch (std::runtime_error &e) {
                cerr << "Error: Argument not recognised: " << e.what() << "\n";
            }
        } else if (argc > 4) {
            cerr << "Error: Arguments exceeding limit, maximum of three arguments.\n";
        } else {
            cout << "No command line arguments given. Total arguments: " << argc << std::endl;
        }
    }
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    void set(const string &username, const string &password) {
        // Prevent empty or whitespace-only usernames
        if (username.empty() || std::all_of(username.begin(), username.end(), ::isspace)) {
            cerr << "Error: Username cannot be empty\n";
            return;
        }

        unique_lock<std::shared_mutex> lock(db_mutex);                                      
        db[username] = hash(password);                                                            
        cout << "Database: User added: " << username << "\n"; 
    }

    int get(const string &username) {
        if (username.empty()) {
            cerr << "Error: Username cannot be empty\n";
            return -1;
        }

        shared_lock<std::shared_mutex> lock(db_mutex);
        auto it = db.find(username);
        if (it != db.end()) {
            cout << "User: " << username << ":" << it->second << "\n";
            return 0;
        } else {
            cout << "Error: User not found\n";
            return -2;
        }
    }

    void thread(const string &function, const string &name, const string &password) {
        std::vector<std::thread> threads;
        try {
            if (function == "get") {
                threads.emplace_back(&AuroraDB::get, this, name);
            } else if (function == "set") {
                threads.emplace_back(&AuroraDB::set, this, name, password);
            } else if (function == "rm") {
                threads.emplace_back(&AuroraDB::rm, this, name);
            } else if (function == "compare") {
                threads.emplace_back(&AuroraDB::compare, this, name, password);
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

    //----------------------------------------------------------------------------------------------------------------------------------------------
    bool compare(const string &username, const string &password) {
        if (username.empty() || password.empty()) {
            cerr << "Error: Username and password cannot be empty\n";
            return false;
        }

        shared_lock<std::shared_mutex> lock(db_mutex); // Uses shared_lock, multiple threads can read at the same time.
        auto it = db.find(username);                   // Declare variable.
        if (it != db.end() && it->second == hash(password)) {  // If user is found.
            cout << "Password matched for user: " << username << "\n"; // Print success message.
            return true; // Return true.
        } else {
            cout << "No match found for user: " << username << "\n"; // Print no match.
            return false; // Return false.
        }
    }

    void rm(const string &username) {
        if (username.empty()) {
            cerr << "Error: Username cannot be empty\n";
            return;
        }

        unique_lock<std::shared_mutex> lock(db_mutex); // Locks the db.
        size_t removed = db.erase(username);            // Erases user from database.
        if (removed > 0) {
            cout << "User removed: " << username << "\n";  // Outputs message.
        } else {
            cout << "User not found: " << username << "\n";
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        AuroraDB db;
        db.cmdArgs(argc, argv);
    } catch (const std::exception& e) {
        cerr << "Unhandled exception: " << e.what() << "\n";
        return 1;
    }
    
    // Database supports multiple different solutions, both command line arguments, networking and you can write commands under here: (Modified version has interactive menu.)
    // Threading is being worked on, you see different code parts that support it.
    // Have fun, greetings Wicks.
    return 0;
}
