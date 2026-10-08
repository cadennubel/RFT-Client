#include <iostream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <array>
#include <cstring>
#include <chrono>

#include "timerC.h"
#include "unreliableTransport.h"
#include "logging.h"


//get file size 

std::ifstream::pos_type filesize(const char* filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

//make packet.
datagramS make_pkt(uint32_t seqnum, std::istream& input) {
    datagramS pkt{};      
    pkt.seqNum = seqnum;  
    pkt.ackNum = 0;
    int count = 0;
    char byte;
    while (count < MAX_PAYLOAD_LENGTH && input.get(byte)) {
        pkt.data[count] = byte;
        count++;
    }
    pkt.payloadLength = count;
    pkt.checksum = computeChecksum(pkt);
    return pkt;
}
enum class State { WAIT, TIMEOUT, SENDDATA, GOODPCKRCV, CORRUPTPCKT };

void gbnSend(unreliableTransportC& connection, std::istream& input, uint32_t WINDOW_SIZE, int TIMEOUT_SIZE){
    uint32_t base = 1;
    uint32_t nextseqnum = 1;
    std::array<datagramS, 10> sndpkt;
    timerC t(TIMEOUT_SIZE);
    bool done = false;
    int finalRetries = 0;   // timeouts since the end-of-file packet was sent

    while (!done || base != nextseqnum){
        datagramS rcvpkt;

        //finite state machine
        State state = State::WAIT;

        bool rcvd = connection.udt_receive(rcvpkt) > 0;

        if (rcvd && validateChecksum(rcvpkt)){
            state = State::GOODPCKRCV;
        }
        else if (rcvd && !validateChecksum(rcvpkt)){
            state = State::CORRUPTPCKT;
        }
        else if (t.timeout()){
            state = State::TIMEOUT;
        }
        else if (!done){
            state = State::SENDDATA;
        }

        switch (state){
        case State::WAIT:
            // Runs constantly while waiting; only visible at -d 6
            TRACE << "WAIT: base=" << base << " nextseqnum=" << nextseqnum << ENDL;
            break;

        case State::TIMEOUT:
            if (done && ++finalRetries > 10) {
                WARNING << "No ACK after 10 retries; assuming server received EOF and exited." << ENDL;
                return;
            }
            INFO << "TIMEOUT: resending packets " << base << " to " << nextseqnum - 1 << ENDL;
            t.start();
            for(uint32_t i = base; i <= nextseqnum - 1; i++){
                DEBUG << "  Resending seqNum " << i << ENDL;
                connection.udt_send(sndpkt[i % 10]);
            }
            break;

        case State::GOODPCKRCV:
            if (rcvpkt.ackNum >= base && rcvpkt.ackNum < nextseqnum) {
                base = rcvpkt.ackNum + 1;
                finalRetries = 0;
                DEBUG << "GOODPCKRCV: ACK " << rcvpkt.ackNum << " accepted, base now " << base
                      << ", nextseqnum " << nextseqnum << ENDL;
                if (base == nextseqnum) {
                    t.stop();
                    DEBUG << "  All outstanding packets ACKed, timer stopped" << ENDL;
                }
                else t.start();
            } else {
                DEBUG << "GOODPCKRCV: ACK " << rcvpkt.ackNum << " ignored (window is "
                      << base << " to " << nextseqnum - 1 << ")" << ENDL;
            }
            break;

        case State::CORRUPTPCKT:
            WARNING << "CORRUPTPCKT: bad checksum on ACK (ackNum field reads "
                    << rcvpkt.ackNum << "), ignoring" << ENDL;
            break;

        case State::SENDDATA:
            if (nextseqnum < base + WINDOW_SIZE){
                sndpkt[nextseqnum % 10] = make_pkt(nextseqnum, input);
                DEBUG << "SENDDATA: sending seqNum " << nextseqnum << " ("
                      << (int)sndpkt[nextseqnum % 10].payloadLength << " bytes)" << ENDL;
                connection.udt_send(sndpkt[nextseqnum % 10]);
                if (base == nextseqnum){
                    t.start();
                }

                if (sndpkt[nextseqnum % 10].payloadLength == 0){
                    done = true;
                    INFO << "SENDDATA: end of file, sent empty packet seqNum " << nextseqnum << ENDL;
                }
                nextseqnum++;
            }
            break;
        }
    }
}

int main(int argc, char* argv[]){
   
    uint16_t portNum = 12566;
    std::string hostname = "isengard.mines.edu";
    std::string inputFilename = "";
    uint32_t WINDOW_SIZE = 10;
    int TIMEOUT_SIZE = 250;
    using namespace std::chrono;
   
    int opt;
    try {
        int signedTemp;
        while ((opt = getopt(argc, argv, "f:h:p:d:w:t:")) != -1) {
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
                case 'w':
                    WINDOW_SIZE = std::stoi(optarg);
                    if(WINDOW_SIZE < 1 || WINDOW_SIZE > 10){
                        std::cerr << "Window size must be greater than 1 and less than 10" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 't':
                    TIMEOUT_SIZE = std::stoi(optarg);
                    if (TIMEOUT_SIZE < 10 || TIMEOUT_SIZE > 2000) {
                        std::cerr << "Timeout must be between 10 and 2000 ms." << std::endl;
                        exit(EXIT_FAILURE);
                    }
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
    // Use fstream binary to open file.
    // **************************************************

    std::ifstream openfile;
    openfile.open(inputFilename, std::ios::binary);
    if (!openfile.is_open()) {
        FATAL << "Unable to open input file: " << inputFilename << ENDL;
        exit(EXIT_FAILURE);
    }
    //catch the exception and call GBN send
     //start the clock using chrono
    time_point<steady_clock> start = steady_clock::now();
    try {
        unreliableTransportC connection(hostname, portNum);
        gbnSend(connection, openfile, WINDOW_SIZE, TIMEOUT_SIZE);
    } catch (std::exception &e) {
        FATAL << "Transfer failed hit exception: " << e.what() << ENDL;
        exit(EXIT_FAILURE);
    }

    openfile.close();
    //end clock and return 0
    time_point<steady_clock> end = steady_clock::now();
    auto elapsed = duration_cast<milliseconds>(end - start);
    TRACE << "File transfer complete." << ENDL;
    //get throughput
    double seconds = elapsed.count() / 1000.0;
    std::streamoff fileSize = filesize(inputFilename.c_str());
    double throughput = fileSize / seconds;
    std::cout << "Elapsed Time: " << elapsed.count() << " ms" << std::endl;
    std::cout << "Throughput: " << throughput << " bytes/s" << std::endl;
    return 0;
}
