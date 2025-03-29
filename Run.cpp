#include <csignal>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <future>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <cstdint>
#include <chrono>
#include <vector>
#include <thread>
#include <iomanip>
#include <map>
#include <pthread.h>
#include "src/color.h"
#include "src/rng.h"

using time_point = std::chrono::steady_clock::time_point;

const int KB{1024}, MB{1048576};
const int CODE_LENGTH{12};
const int SUCCESS{0};
const int AC{1};
const int WA{2};
const int TIME_OUT{4};
const int RUNTIME_ERROR{8};
const int MEMORY_OUT{16};

const std::map<int, Color> RESULT{
    {AC, Color(0x7A, 0xFF, 0x77)},
    {TIME_OUT, Color(0x9F, 0xE2, 0xFF)},
    {RUNTIME_ERROR, Color(0xAE, 0x9F, 0xFF)},
    {MEMORY_OUT, Color(0x99, 0xE8, 0xE6)},
    {WA, Color(0xFF, 0x41, 0x41)}
};

namespace fs = std::filesystem;

struct UserInfo {
    UserInfo(
        std::string needCompile = "true", 
        std::string compileCommand = "make", 
        std::string executeCommand = "./Solution/Sol"
    ) : needCompile(needCompile), compileCommand(compileCommand), executeCommand(executeCommand) {}
    std::string needCompile;
    std::string compileCommand;
    std::string executeCommand;
};

class CompileCommandNotFound: public std::exception {
public:
    const char* what() const noexcept {
        return "Need argument: compile command";
    }
};

class ExecuteCommandNotFound: public std::exception {
public:
    const char* what() const noexcept {
        return "Need argument: compile command";
    }
};

void RunCode(
    int timeLimit, int testCase, 
    std::string executeCommand, 
    std::promise<int> timeCost, std::promise<int> memoryCost,
    std::promise<int> status, std::promise<int> pid
) {
    std::string file = 
        executeCommand + " < ./TestCase/" + std::to_string(testCase) + ".in " + 
        "1> ./TestCase/sol" + std::to_string(testCase) + ".out " + 
        "2> ./TestCase/err" + std::to_string(testCase) + ".err";
    
    time_point start = std::chrono::steady_clock::now();
    
    int execStatus{0};
    int processId = fork();
    if (processId == 0) {
        // Set Memory Limit
        struct rlimit maxMemory;
        maxMemory.rlim_cur = 256 * MB; // 256 MB
        maxMemory.rlim_max = 256 * MB;
        setrlimit(RLIMIT_AS, &maxMemory);
        int ret{system(file.c_str())}; // Double fork
        exit(ret);
    }
    struct rusage current_usage{0};
    getrusage(RUSAGE_SELF, &current_usage);
    struct rusage child_usage{0};
    pid.set_value(processId);
    waitpid(processId, &execStatus, 0);
    getrusage(RUSAGE_CHILDREN, &child_usage);

    time_point end = std::chrono::steady_clock::now();

    int64_t t {std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count()};

#ifdef __linux__
    int memoryUsage = child_usage.ru_maxrss - current_usage.ru_maxrss;
#elif defined(__APPLE__) || defined(__MACH__)
    int memoryUsage = (child_usage.ru_maxrss - current_usage.ru_maxrss) / KB;
#endif

    memoryCost.set_value(memoryUsage);
    timeCost.set_value(t);

    if(execStatus != 0) {
        status.set_value(RUNTIME_ERROR);
        return;
    }

    if(t > timeLimit) {
        status.set_value(TIME_OUT);
        return;
    }

    status.set_value(SUCCESS);
    return;
}

int RunTestCase(int testCase, int timeLimit, std::vector<int> &costTime, std::vector<int> &costMemory, const std::string &executeCommand) {
    std::string fileNum = std::to_string(testCase);

    std::promise<int> pid;
    std::future<int> futPid {pid.get_future()};
    std::promise<int> timeCost;
    std::future<int> futTimeCost {timeCost.get_future()};
    std::promise<int> memoryCost;
    std::future<int> futMemoryCost {memoryCost.get_future()};
    std::promise<int> status;
    std::future<int> futStatus {status.get_future()};

    std::thread run{
        RunCode, timeLimit, testCase, executeCommand, 
        std::move(timeCost), std::move(memoryCost), std::move(status), std::move(pid)
    };

    time_point start = std::chrono::steady_clock::now();

    std::future_status ready;
    do {
        time_point now = std::chrono::steady_clock::now();
        int64_t time_cost = 
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count();
        if(time_cost > timeLimit + 100)
            break;
        ready = futTimeCost.wait_for(std::chrono::milliseconds(10));
    } while(ready != std::future_status::ready);
    
    if(ready != std::future_status::ready) {
        kill(futPid.get(), 9);
        run.join();
        costTime[testCase] = timeLimit + 50;
        costMemory[testCase] = 0;
        return TIME_OUT;
    } else {
        costTime[testCase] = futTimeCost.get();
        costMemory[testCase] = futMemoryCost.get();
        run.join();
        return futStatus.get();
    }
}

int Judge(int testCase){
    std::ifstream userOutput("./TestCase/sol" + std::to_string(testCase) + ".out");
    std::ifstream question("./TestCase/" + std::to_string(testCase) + ".in");
    std::ifstream answer("./TestCase/" + std::to_string(testCase) + ".out");

    std::string tp,userAns,systemAns;
    while(userOutput >> tp) 
        userAns += tp;
    while(answer>>tp) 
        systemAns += tp;
    // std::ofstream clearOutput("./TestCase/sol" + std::to_string(testCase) + ".out");
    return (systemAns == userAns ? AC : WA);
}

std::string Encode() {
    int sum{int(std::chrono::steady_clock::now().time_since_epoch().count())};

    random_number_generater rng(sum);
    std::string ret;

    sum = 0;
    for(int i = 0; i < CODE_LENGTH - 1; ++i) {
        int a = rng(100);
        sum += a;
        if(a < 10) ret += "0";
        ret += std::to_string(a);
    }

    sum %= 100;
    if(sum == 20) {
        ret += "00";
    } else if(sum < 20) {
        int a = 20 - sum;
        if(a < 10) ret += "0";
        ret += std::to_string(a);
    } else {
        int a = 120 - sum;
        ret += std::to_string(a);
    }

    return ret;
}

double FindComputerSpeed() {
    time_point start = std::chrono::steady_clock::now();

    int ans = 1;
    const int MOD = 37;
    const int TIME = 100000000;
    for(int i = 1; i <= TIME; ++i) {
        ans = (ans * i) % MOD;
    }
    if(ans != 0) throw std::runtime_error("Didn't fix the time limit");
    
    time_point end = std::chrono::steady_clock::now();

    return double(std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count()) / 1000.0;
}

double FixTimeLimit(int timeLimit) {
    std::cout << "Fixing the time limit..." << "\n";
    
    const int TEST_NUM = 5;
    double average = 0;
    for(int i = 0; i < TEST_NUM; ++i) {
        average += FindComputerSpeed();
    }
    average /= TEST_NUM;

    const double MY_TIME_COST = 0.3495;
    double multiplier = average / MY_TIME_COST;

    std::cout << "Your Computer run " << std::setprecision(2) << 1.0 / multiplier 
         << " time \nas fast as the judge." << "\n\n";

    return multiplier;
}

void ReadProblemInfo(int &testCases, int &timeLimit, std::string &problemID) {
    std::ifstream log("./TestCase/log.txt");
    std::string recycle;
    log >> recycle >> testCases;
    log >> recycle >> timeLimit;
    log >> recycle >> problemID;
}

bool CompileSolution(std::string compileCommand) {
    fs::current_path(fs::current_path() / "Solution");
    int compileStatus = std::system(compileCommand.c_str());
    fs::current_path("..");
    if(compileStatus != 0) {
        std::cout << "Compilation failed.\n";
        std::ifstream CE("Result/CE");
        std::string line;
        while(std::getline(CE, line)) {
            std::cout << line << "\n";
        }
        std::cout << std::flush;
        return false;
    }
    return true;
}

void ShowTotalResult(bool allCorrect, int statusFlag, std::ostream &output) {
    if(allCorrect) {
        std::cout << RESULT.at(AC);
        std::ifstream AC("Result/AC");
        std::string line;
        while(getline(AC, line)) {
            std::cout << line << "\n";
            output << line << "\n";
        }
        std::cout << RESET " " << std::flush;
    } else if(statusFlag & TIME_OUT) {
        std::cout << RESULT.at(TIME_OUT);
        std::ifstream TLE("Result/TLE");
        std::string line;
        while(std::getline(TLE, line)) {
            std::cout << line << "\n";
            output << line << "\n";
        }
        std::cout << RESET " " << std::flush;
    } else if(statusFlag & MEMORY_OUT) {
        std::cout << RESULT.at(MEMORY_OUT);
        std::ifstream MLE("Result/MLE");
        std::string line;
        while(std::getline(MLE, line)) {
            std::cout << line << "\n";
            output << line << "\n";
        }
        std::cout << RESET " " << std::flush;
    } else if(statusFlag & RUNTIME_ERROR) {
        std::cout << RESULT.at(RUNTIME_ERROR);
        std::ifstream RE("Result/RE");
        std::string line;
        while(std::getline(RE, line)) {
            std::cout << line << "\n";
            output << line << "\n";
        }
        std::cout << RESET " " << std::flush;
    } else {
        std::cout << RESULT.at(WA);
        std::ifstream WA("Result/WA");
        std::string line;
        while(getline(WA, line)) {
            std::cout << line << "\n";
            output << line << "\n";
        }
        std::cout << RESET " " << std::flush;
    }
}

bool CheckMLE(int testCase) {
    std::ifstream fi("./TestCase/err" + std::to_string(testCase) + ".err");
    std::string word;
    while(fi >> word) if(word == "std::bad_alloc") return true;
    return false;
}

void ShowIndividualResult(
    int testCases, 
    std::vector<int> &outputStatus, 
    std::vector<int> &costTime, std::vector<int> &costMemory,
    double multiplier, 
    std::ostream &output
) {
    std::map<int, std::string> ret;
    ret[WA] = "WA";
    ret[AC] = "AC";
    ret[TIME_OUT] = "TLE";
    ret[RUNTIME_ERROR] = "RE";
    ret[MEMORY_OUT] = "MLE";
    std::cout << "For each testcase : " << "\n\n";
    output << "For each testcase : " << "\n\n";

    for(int i = 1; i <= testCases; ++i) {
        costTime[i] /= multiplier;
    }

    for(int i = 1; i <= testCases; ++i) {
        std::cout << std::right << std::setw(3) << i << ". " << std::flush;
        std::cout << RESULT.at(outputStatus[i]) << std::flush;
        std::cout << std::setw(4) << ret[outputStatus[i]] << RESET "  " << std::flush;
        std::cout << "Execution time : " << std::right << std::setw(4) << costTime[i] << " ms" 
            << "  Memory : " << std::right << std::setw(8) << costMemory[i] << " KB"
            << std::endl;
        output << std::right << std::setw(3) << i << ". " << std::flush;
        output << std::setw(4) << ret[outputStatus[i]] << "  " << std::flush;
        output << "Execution time : " << std::right << std::setw(8) << costTime[i] << " ms" 
            << "  Memory : " << std::right << std::setw(4) << costMemory[i] << " KB"
            << std::endl;
    }
}

void RunSolution(UserInfo userInfo) {
    std::string problemID;
    int testCases, timeLimit;
    double multiplier;

    ReadProblemInfo(testCases, timeLimit, problemID);

    std::ofstream output("output.info");

    std::cout << "Problem ID : " << problemID << "\n";
    std::cout << "There're " << testCases << " testcases." << "\n" << std::endl;

    output << "Problem ID : " << problemID << "\n";
    output << "There're " << testCases << " testcases." << "\n";

    if (userInfo.needCompile == "true")
        if(!CompileSolution(userInfo.compileCommand)) return;

    multiplier = FixTimeLimit(timeLimit);
    timeLimit *= multiplier;

    std::cout << "Running TestCase..." << "\n";

    std::vector<int> costTime(testCases + 1);
    std::vector<int> costMemory(testCases + 1);
    std::vector<int> outputStatus(testCases + 1);
    for(int i = 1; i <= testCases; ++i) {
        int st = RunTestCase(i, timeLimit, costTime, costMemory, userInfo.executeCommand);
        outputStatus[i] = st;
        std::cout << i << " " << std::flush;
    }

    int correct = 0;
    bool allCorrect = true;
    int statusFlag = 0;

    for(int i = 1; i <= testCases; ++i) {
        if(outputStatus[i] == SUCCESS) {
            outputStatus[i] = Judge(i);
        }
        if(outputStatus[i] == RUNTIME_ERROR && CheckMLE(i)) {
            outputStatus[i] = MEMORY_OUT;
        }
        correct += (outputStatus[i] == AC);
        if(outputStatus[i] != AC) {
            allCorrect = false;
        }
        statusFlag |= outputStatus[i];
    }

    std::cout << "\n";
    
    ShowTotalResult(allCorrect, statusFlag, output);
    ShowIndividualResult(testCases, outputStatus, costTime, costMemory, multiplier, output);

    std::cout << "\nTotal score : " << std::fixed << std::setprecision(2) << (double)correct / testCases * 100 << std::endl;
    output << "\nTotal score : " << std::fixed << std::setprecision(2) << (double)correct / testCases * 100 << std::endl;

    std::cout << std::endl;
    output << std::endl;

    if(allCorrect) {
        std::string code = Encode();
        std::cout<<"AC code : " << code << std::endl;
        output << "AC code : " << code << std::endl;
    }
}

void PrintUsage() {
    std::cout << "Usage: ./Run <need compile> <compile command> <execute command>" << "\n";
    std::cout << "Example: ./Run" << "\n";
    std::cout << "         ./Run true make ./Solution/Sol" << "\n";
    std::cout << "         ./Run false \"python3 ./Solution/Sol.py\"" << "\n";
    std::cout << std::flush;
}

UserInfo GetUserInfo(int argc, char *argv[]) {
    UserInfo userInfo;
    if (argc >= 2) {
        if (std::string(argv[1]) == "true") {
            if (argc >= 3)
                userInfo.compileCommand = argv[2];
            else {
                PrintUsage();
                throw CompileCommandNotFound();
            }
            if (argc >= 4) {
                userInfo.executeCommand = argv[3];
            }
        }
        else {
            userInfo.needCompile = "false";
            if (argc >= 3) {
                userInfo.executeCommand = argv[2];
            } else {
                PrintUsage();
                throw ExecuteCommandNotFound();
            }
        }
    }
    return userInfo;
}

// * ./Run <need compile> <compile command> <execute command>
int main(int argc, char *argv[]) {
    UserInfo userInfo{GetUserInfo(argc, argv)};
    RunSolution(userInfo);
}