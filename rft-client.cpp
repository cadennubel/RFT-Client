//
// Created by Phillip Romig on 7/16/24.
//
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <array>
#include <cstring>

#include "timerC.h"
#include "unreliableTransport.h"
#include "logging.h"


#define WINDOW_SIZE 10
int main(int argc, char* argv[]) {

    // Defaults
    uint16_t portNum = 12345;
    std::string hostname = "isengard.mines.edu";
    std::string inputFilename = "";

    int opt;
    try {
        int signedTemp;
        while ((opt = getopt(argc, argv, "f:h:p:d:")) != -1) {
            switch (opt) {
                case 'p':
                    signedTemp = std::stoi(optarg);
                    if ((signedTemp < 12000) or (signedTemp > 13000)) {
                        std::cerr  << "To deal with isengard's firewall, port numbers must be between 12,000 and 13,000."  << std::endl;
                         exit(EXIT_FAILURE);
                    }
                    portNum = signedTemp;
                    break;
 
                case 'h':
                    hostname = optarg;
                    break;
                case 'd':
                    LOG_LEVEL = std::stoi(optarg);
                    if ((LOG_LEVEL < 0) or (LOG_LEVEL > 6)) {
                        std::cerr  << "Debug level must be between 0 and 6"  << std::endl;
                        std::cout << "Usage: " << argv[0] << " -f filename [-h hostname] [-p port] [-d debug_level]" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'f':
                    inputFilename = optarg;
                    break;
                case '?':
                default:
                    std::cout << "Usage: " << argv[0] << " -f filename [-h hostname] [-p port] [-d debug_level]" << std::endl;
                    break;
            }
        }
    } catch (std::exception &e) {
        FATAL << "Invalid command line arguments: " << e.what() << ENDL;
        std::cout << "Usage: " << argv[0] << " -f filename [-h hostname] [-p port] [-d debug_level]" << std::endl;
        exit(1);
    }

    if (inputFilename == "") {
        FATAL << "Invalid command line arguments: -f filename is required" << ENDL;
         exit(EXIT_FAILURE);
    }
    TRACE << "Command line arguments parsed." << ENDL;
    TRACE << "\tServername: " << hostname << ENDL;
    TRACE << "\tPort number: " << portNum << ENDL;
    TRACE << "\tDebug Level: " << LOG_LEVEL << ENDL;
    TRACE << "\tOutput file name: " << inputFilename << ENDL;


    // **************************************************
    // Open the input file.
    // **************************************************


    try {

       // *******************************************************************
       // * Initialize your timer, datagram buffer, transport layer etc.
       // *******************************************************************
       bool finished(false);
        while (!finished) {
            
            // ***********************************************************************
            // * Is there space in the window? If so, read data from file and send it
            // ***********************************************************************
        
            // ***********************************************************************
            // * Are there acknowledgments in from the server. If so, process them.
            // ***********************************************************************
            
            // ***********************************************************************
            // * Have we has the timer gone off?
            // ***********************************************************************
 
        }

        
    } catch (std::exception &e) {
        FATAL<< "Error: " << e.what() << ENDL;
        exit(1);
    }

    return 0;
}
