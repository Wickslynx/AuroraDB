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

// Define, the best thing in C++.
#define error(string) (std::cout << "ERROR!: " << string << "\n")
#define ServerMessage(input, socket) (const char* message = input; send(socket, message, strlen(message), 0);)

// All using statements (Makes my life easier.)
using std::cin;
using std::cout;
using std::shared_lock;
using std::string;
using std::unique_lock; 

class AuroraDB {
private:
    std::unordered_map<string, string> db;     // Starts unordered map.
    std::unordered_map<string, string> buffer; // Starts a buffer to load data into.
    std::shared_mutex db_mutex;                // Starts a mutex that's called "db_mutex".

public:
    AuroraDB() {
        try {
            load("storage.txt"); // Runs loading method.
            // connect(8080); //Uncomment to set networking to default start mode.
        } catch (const std::runtime_error &e) {
            std::cerr << "Error loading database: " << e.what() << "\n"; // If error, send error message.
        }
    }

    ~AuroraDB() {
        try {
            save("storage.txt");
        } catch (const std::runtime_error &e) {
            std::cerr << "Error saving database: " << e.what() << "\n"; // If error, send error message.
        }
    }
    //---------------------------------------------------------------------------------------------------------------------------------------------
    void load(const string &file) {
        std::ifstream inputFile(file); // Opens file in "read" mode.
        if (!inputFile) {
            throw std::runtime_error("Error opening file for loading."); // If error, send error message.
        }

        string username, password; // Declares username and password.
        while (inputFile >> username >> password) {  // While able to read data.
            db[username] = password; // Add user to database.
        }
    }

    void save(const string &filename) {
        std::ofstream outfile(filename, std::ios::out); // Open file in "out" mode.
        if (!outfile.is_open()) {
            throw std::runtime_error("Error opening file for saving."); // If error, send error message.
        }
        for (const auto &pair : db) {
            outfile << pair.first << " " << pair.second << "\n"; // For items in db, pair item into file. (pair.first:pair.second)
        }
    }
    //----------------------------------------------------------------------------------------------------------------------------------------------

    void connect(const int &port) {
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        bool quit = false;
        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;         //Use TCP
        serverAddress.sin_port = htons(port);      // Set port to 8080.
        serverAddress.sin_addr.s_addr = INADDR_ANY;         // Change to accept only one IP.

        bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
        listen(serverSocket, 10000);

        int clientSocket = accept(serverSocket, nullptr, nullptr);

        while (!quit) {

            char buffer[1024] = {0};
            recv(clientSocket, buffer, sizeof(buffer), 0);

            std::string command, name, password;
            std::istringstream stream(buffer);
            try {
                stream >> command >> name >> password;
            } catch (const std::runtime_error &e) {
                throw std::runtime_error(e.what());
            }
            if (buffer[0] == '\0') {
                throw std::runtime_error("String was empty.");
            } else if (command == "get") {
                thread("get", name, password);
            } else if (command == "set") {
                thread("set", name, password);
            } else if (command == "compare") {
                thread("compare", name, password); // Send message to server, Used for debugging.
            } else if (command == "quit") {
                break;
            } else {
                std::cout << "Error: " << buffer << "\n";
            }
        }

        close(serverSocket);
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
                    cout << get(name) << "\n"; // If cmd is get and two arguments have been given, print the get method.
                } else if (cmd == "set" && argc > 3) {
                    set(name, password); // Else if cmd is set and three arguments have been given, add user to database.
                } else if (cmd == "rm" && argc > 2) {
                    rm(name); // Else if cmd is rm and two arguments have been given, remove user from database.
                } else if (cmd == "compare" && argc > 3) {
                    compare(name, password); // Else if cmd is compare and three arguments have been given, compare user.
                } else { // Else, throw error.
                    throw std::runtime_error("Error: Must use (set (Name, Password), get (Name), rm (Name), compare (Name, Password))");
                }
            } catch (std::runtime_error &e) {
                std::cout << "Error: Argument not recognised: " << e.what() << "\n"; // If error, throw runtime_error.
            }
        } else if (argc > 4) {
            cout << "Error: Arguments exceeding limit, maximum of three arguments."; // Arguments exceeding limit, cout error.
        } else {
            cout << "No command line arguments given. " << argc << std::endl; // No arguments given, cout warning and continue.
        }
    }
    //-----------------------------------------------------------------------------------------------------------------------------------------------
    void set(const string &username, const string &password) {
        unique_lock<std::shared_mutex> lock(db_mutex);                                      // Uses std::unique_lock, locks the mutex. (One user can edit at a time.) !Experimental!
        db[username] = password;                                                            // Adds user to database.
        cout << "Database: User added: " << username << ", Password: " << password << "\n"; // Cout message.
    }

    string get(const string &username) {
        shared_lock<std::shared_mutex> lock(db_mutex); // Uses std::shared_lock, locks the mutex. (Multiple users can read, one can edit.) !Experimental!
        if (db.find(username) != db.end()) {  // Find user, if found return username and password.
            if (username != "Wicks" && username != "Elliot") {
                cout << "User: " << username << ":" << db[username] << "\n";
                return "User";
            } else if (username == "Wicks") {
                cout << "User: " << username << ":" << db[username] << "  !Owner!." << "\n";
                return "Owner";
            } else if (username == "Elliot") {
                cout << "User: " << username << ":" << db[username] << "  !Admin!." << "\n";
                return "Admin";
            } else {
                rm(username);
                return "Ban";
            }
        } else {
            return "Error: User not found!"; // Else, returns error.
        }
    }

    void thread(const string &function, const string &name, const string &password) {
        std::vector<std::thread> threads; // Starts threading vector.
        if (function == "get") {
            threads.emplace_back(&Database::get, this, name); // If function get, start thread. (calls get in a thread)
        } else if (function == "set") {
            threads.emplace_back(&Database::set, this, name, password); // If function set, start thread. (calls set in a thread)
        } else if (function == "rm") {
            threads.emplace_back(&Database::rm, this, name); // If function rm, start thread. (calls rm in a thread.)
        } else if (function == "compare") {
            threads.emplace_back(&Database::compare, this, name, password); // If function compare, start thread. (calls compare in a thread.)
        } else {
            throw std::runtime_error("Oooops, something went wrong, try again."); // If an error occurs, throw an error.
        }

        for (auto &t : threads) {
            t.join(); // Join threads together.
        }
    }

    //----------------------------------------------------------------------------------------------------------------------------------------------
    bool compare(const string &username, const string &password) {
        shared_lock<std::shared_mutex> lock(db_mutex); // Uses shared_lock, multiple threads can read at the same time.
        auto it = db.find(username);                   // Declare variable.
        if (it != db.end() && it->second == password) {  // If user is found.
            cout << "Password matched for user: " << username << "\n"; // Print success message.
            return true; // Return true.
        } else {
            cout << "No match found for user: " << username << "\n"; // Print no match.
            return false; // Return false.
        }
    }

    void rm(const string &username) {
        unique_lock<std::shared_mutex> lock(db_mutex); // Locks the db.
        db.erase(username);                            // Erases user from database.
        cout << "User removed: " << username << "\n";  // Outputs message.
    }
};

int main(int argc, char* argv[]) {
    Aurora db;
    db.cmdArgs(argc, argv);
    // Database supports multiple different solutions, both command line arguments, networking and you can write commands under here: (Modified version has interactive menu.)
    // Threading is being worked on, you see different code parts that support it.
    // Have fun, greetings Wicks.
    return 0;

    
}
