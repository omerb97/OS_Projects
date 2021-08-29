#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <ctime>
#include <vector>
#include <map>
#include <csignal>
#include <fstream>
#include <cstdio>
#include <fcntl.h>


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
const std::string WHITESPACE = " \n\r\t\f\v";
#define UNINITALIZED (-1)
#define SIGCONT 18
#define STDOUT 1
#define STDERR 2
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)));
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = nullptr;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


std::pair<std::string, std::string> parsePipeAndRedirection(const char *cmd_line, outChannel *out) {

    pair<string, string> parsed;
    string temp = string(cmd_line);
    int car_index = temp.find_first_of(">|");
    // assert(car_index != (int) string::npos);
    if (temp[car_index] == '>') {
        if ((temp[car_index + 1] == '>')) {
            *out = APPEND;
        } else {
            *out = OVERRIDE;
        }
    } else if (temp[car_index] == '|') {
        if ((temp[car_index + 1] == '&')) {
            *out = ERR;
        } else {
            *out = OUT;
        }
    }
    parsed.first = temp.substr(0, car_index);
    int cdr_index = temp.find_first_not_of(">&|", car_index);
    parsed.second = _trim(temp.substr(cdr_index));

    return parsed;


}

bool is_number(const std::string &s) {
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}



SmallShell::SmallShell() {
    lastPath = "";
    prompt = "smash> ";
    is_child = false;

    pid = getpid();
    if (pid == -1) {

    }
    fgCommand = nullptr;
}



/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (cmd_s.find_first_of('|') != (int) string::npos) {
        return new PipeCommand(cmd_line);
    } else if (cmd_s.find_first_of('>') != (int) string::npos) {
        return new RedirectionCommand(cmd_line);
    } else if (firstWord.compare("chprompt") == 0) {
        return new ChangePromptCommand(cmd_line);
    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line);
    } else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line);
    } else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line);
    } else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line);
    } else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line);
    } else if (firstWord.compare("quit") == 0) {
        QuitCommand cmd = QuitCommand(cmd_line);
        cmd.execute();
    } else if (firstWord.compare("cat") == 0) {
        return new CatCommand(cmd_line);

    }//Timed Out is done at external cmd
    return new ExternalCommand(cmd_line);


}

void SmallShell::executeCommand(const char *cmd_line, bool forkable) {
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.removeFinishedJobs();
    int timed = false;
    int duration = 0;

    // for example:

    Command *cmd = CreateCommand(cmd_line);
    //TODO Might Cause problems need to check with destructor.

    if (dynamic_cast<ExternalCommand *>(cmd) == nullptr) {
        cmd->execute();
        delete cmd;
        return;
    }
    std::string original_cmd_line = cmd->getCmdLine();

    if (cmd->getNumArgs() == 0) return;
    if (cmd->args[0] == "timeout") {
        if (cmd->getNumArgs() < 3) throw InvalidArgs("timeout");
        if (!is_number(cmd->args[1])) throw InvalidArgs("timeout");
        try { //check if second arg is valid
            duration = stoi(cmd->args[1]);
            if (duration < 0) throw InvalidArgs("timeout");
        } catch (...) {
            throw InvalidArgs("timeout");
        }
        cmd->args.erase(cmd->args.begin(), cmd->args.begin() + 2);
        std::string new_cmd_line = "";
        for (auto const &it : cmd->args) {// remove first two args
            new_cmd_line += it + " ";

        }
        cmd->setCmdLine(new_cmd_line);

        timed = true;
    }
    int newPID = 0;
    if (forkable) { //IF not forkable current process thinks its the child
        newPID = fork();
    }
        if (newPID == 0) {//son
            if (forkable) {
                setpgrp();
            }
            try {

                cmd->execute();
            } catch (std::exception &e) {
                std::cerr << "smash error: " << e.what() << std::endl;
            }
            exit(0);
        }
    if (newPID != 0) {
        if (timed) {
            smash.timedCommandHandler.addTimedCommand(duration, newPID, cmd_line);
            int new_alarm = smash.timedCommandHandler.findNextAlarm();
            alarm(new_alarm);

        }

        if (_isBackgroundComamnd(cmd_line)) {
            cmd->isBackground = true;
            if (timed) cmd->setCmdLine(original_cmd_line);
            jobs.addJob(cmd, newPID, false);
            smash.fgCommand = nullptr;
            return;
        } else {
            smash.fgCommand = cmd;

            smash.fgCommand->setPid(newPID);
            int status;
            waitpid(newPID, &status, WUNTRACED);
            if (status == -1) {
                WaitPIDFailed();
                return;
            }

            smash.fgCommand = nullptr;

        }

    }

}
// Please note that you must fork smash process for some commands (e.g., external commands....)


void SmallShell::setNewPrompt(std::string const &new_prompt) {
    prompt = new_prompt + "> ";
}

std::string SmallShell::getPrompt() {
    return prompt;
}

const string &SmallShell::getLastPath() const {
    return lastPath;
}

void SmallShell::setLastPath(const string &lastPath) {
    SmallShell::lastPath = lastPath;
}




JobsList::JobEntry::JobEntry(Command *cmd, int pID, bool isStopped) {
    jobInserted = (time_t) (-1);
    PID = pID;
    if (isStopped) {
        currentStatus = STOPPED;
    } else {
        currentStatus = NOT_STOPPED;
    }
    jobID = UNINITALIZED;
    this->cmd = cmd;

}

bool JobsList::JobEntry::operator<(const JobsList::JobEntry &jobEntry) const {
    return jobID < jobEntry.jobID;

}

int JobsList::JobEntry::getJobId() {
    return jobID;
}

void JobsList::JobEntry::setJobId(int jobId) {
    jobID = jobId;
}

void JobsList::JobEntry::setCurrentStatus(bool currentStatus) {
    JobEntry::currentStatus = currentStatus;
}

void JobsList::JobEntry::setJobInserted(time_t jobInserted) {
    JobEntry::jobInserted = jobInserted;
}

bool JobsList::JobEntry::getCurrentStatus() const {
    return currentStatus;
}

time_t JobsList::JobEntry::getJobInserted() const {
    return jobInserted;
}

std::ostream &operator<<(std::ostream &os, const JobsList::JobEntry &je) {
    time_t timeDifference = difftime(time(nullptr), je.jobInserted);
    std::string stopped = "";
    if (je.currentStatus == STOPPED) stopped = " (stopped)";
    os << "[" << je.jobID << "] " << je.cmd->getCmdLine() << " : " << je.PID << " " << timeDifference << " secs"
       << stopped;
    return os;
}

pid_t JobsList::JobEntry::getPid() const {
    return PID;
}

Command *JobsList::JobEntry::getCmd() const {
    return cmd;
}


void ChangePromptCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (num_args > 1) {
        smash.setNewPrompt(args[1]);
    } else {
        smash.setNewPrompt("smash");
    }
}

Command::Command(const char *cmd_line) {

    /*  char **buffer = new char *[COMMAND_MAX_ARGS];
      for (int i = 0; i < COMMAND_MAX_ARGS; ++i) {
          buffer[i] = new char[COMMAND_ARGS_MAX_LENGTH];
      }
      */
    char *buffer[COMMAND_ARGS_MAX_LENGTH];
    num_args = _parseCommandLine(cmd_line, buffer);
    for (int j = 0; j < num_args; ++j) {
        args.push_back(buffer[j]);

    }
/*
    for (int k = 0; k < COMMAND_MAX_ARGS; ++k) {
        delete[] buffer[k];

    }
   */


    this->cmd_line = cmd_line;
    for (int i = 0; i < num_args; ++i) {
        free(buffer[i]);
    }
    isBackground = false;
}

std::string Command::getTrimmedLine() {
    return _trim(cmd_line);
}

int Command::getPid() const {
    return pid;
}

void Command::setPid(int pid) {
    Command::pid = pid;
}

void Command::setCmdLine(const string &cmdLine) {
    cmd_line = cmdLine;
}

int Command::getNumArgs() const {
    return num_args;
}

const string &Command::getCmdLine() const {
    return cmd_line;
}


JobsList::JobsList() {
    maxJobID = 0;


}

void JobsList::addJob(Command *cmd, int PID, bool isStopped) {
    JobEntry *newJob = new JobEntry(cmd, PID, isStopped);
    newJob->setJobId(maxJobID + 1);
    maxJobID++;
    newJob->setJobInserted(time(nullptr));
    std::pair<int, JobEntry *> jobPair(maxJobID, newJob);
    entries.insert(jobPair);

}

void JobsList::printJobsList() {
    for (auto f: entries) {
        std::cout << *f.second << std::endl;
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    auto it = (entries.find(jobId));
    if (it == entries.end()) return nullptr;
    return it->second;
}

int JobsList::GetMaxJobId() {
    return this->maxJobID;
}

int JobsList::getMaxStoppedJobID() {
    int maxStoppedID = 0;
    for (auto const &f: entries) {
        if (f.second->getCurrentStatus() == STOPPED && f.second->getJobId() > maxStoppedID) {
            maxStoppedID = f.second->getJobId();
        }
    }
    return maxStoppedID;
}

void JobsList::killAllJobs() {
    vector<int> toRemove;
    for (auto const &f: entries) {
        int status = kill(f.second->getPid(), SIGKILL);
        if (status == -1) {
            KillFailed();
            return;
        }
        toRemove.push_back(f.first);
    }
    for (auto const g: toRemove) {
        JobEntry *job = getJobById(g);
        removeJobById(g);
        Command *cmd = job->getCmd();
        delete job;
        delete cmd;
    }

}

void JobsList::removeJobById(int jobId) {
    int max = 0;
    if (jobId == maxJobID) {

        //auto temp = entries.find(jobId)->second;
        entries.erase(jobId);
        //delete temp; //dodgy
        for (auto const &f: entries) {
            if (f.second->getJobId() > max) {
                max = f.second->getJobId();
            }
        }
        maxJobID = max;
        return;
    } else {
        entries.erase(jobId);
    }
}

void JobsList::removeFinishedJobs() {
    SmallShell &smash = SmallShell::getInstance();
    if (smash.is_child) return;
    int status = 1;
    std::vector<int> toRemove;
    for (auto const &f: entries) {

        int pid = f.second->getPid();
        pid_t return_pid = waitpid(pid, &status, WNOHANG);
        if (status == -1) {
            WaitPIDFailed();
            return;
        }

        if (return_pid == pid) {
            toRemove.push_back(f.first);
        }

    }
    for (auto const &g: toRemove) {
        JobEntry *job = getJobById(g);
        removeJobById(g);
        Command *cmd = job->getCmd();
        delete job;
        delete cmd;

    }

}

void JobsList::removeJobByPId(int PId) {
    for (auto f: entries) {
        if (f.second->getPid() == PId) {
            if (f.second->getJobId() == maxJobID) {
                entries.erase(maxJobID);
                int max = 0;
                for (auto g: entries) {
                    if (g.second->getJobId() > max) {
                        max = g.second->getJobId();
                    }
                }
                maxJobID = max;
                return;
            }
            auto temp = f.second;
            entries.erase(temp->getPid());
            delete temp;

            return;
        }

    }
    throw JobDoesNotExist("remove by PID _bug_", PId);
}

bool JobsList::isEmpty() {
    return entries.empty();
}

void JobsList::setBgTofgId(int bgTofgId) {
    bgTofgID = bgTofgId;
}

int JobsList::getBgTofgId() const {
    return bgTofgID;
}

void JobsList::AddJobByID(Command *cmd, int PID, int jobID, bool isStopped) {
    JobEntry *newJob = new JobEntry(cmd, PID, isStopped);
    newJob->setJobId(jobID);
    newJob->setJobInserted(time(nullptr));
    std::pair<int, JobEntry *> jobPair(jobID, newJob);
    if (jobID > maxJobID) {
        maxJobID = jobID;
    }
    entries.insert(jobPair);
}

int JobsList::getSize() {
    return entries.size();
}


void JobsList::printKilledJobs() {
    for (auto f: entries) {
        std::cout << (f.second)->getPid() << ": " << ((f.second)->getCmd())->getCmdLine() << std::endl;
    }
}

void ShowPidCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    std::cout << "smash pid is " << smash.pid << std::endl;

}


void GetCurrDirCommand::execute() {
    char buffer[100];
    getcwd(buffer, 100);
    std::cout << buffer << std::endl;
}

void ChangeDirCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (num_args > 2) {
        throw TooManyArguments("cd");
    }
    if (args[1] == "-") { //Revert to previous Directory (dir is stored in smash)

        if (smash.getLastPath().empty()) {
            throw NoPreviousPath("cd");
        } else { //case of revert to last directory
            char buffer[100];

            getcwd(buffer, 100);
            std::string s(buffer);
            int status = chdir(smash.getLastPath().c_str());
            if (status == -1) {
                ChDirFailed();
                return;
            }
            smash.setLastPath(s);
        }
    } else {
        char buffer[100];

        getcwd(buffer, 100);
        std::string s(buffer);
        int status = chdir(args[1].c_str());
        if ((status == -1) && (num_args != 1)) {
            ChDirFailed();
            return;
        }
        smash.setLastPath(s);
    }

}


void ExternalCommand::execute() {
    if (num_args == 0) return;
    char wholeStringArray[cmd_line.length() + 1];
    strcpy(wholeStringArray, cmd_line.c_str());
    _removeBackgroundSign(wholeStringArray);
    char *argsv[] = {"bash", "-c", wholeStringArray, nullptr};
    execv("/bin/bash", argsv);

}


void JobsCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.printJobsList();
}

void KillCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    int sig;
    int jobId;
    if (num_args != 3) throw InvalidArgs("kill");

    try {
        sig = std::stoi(args[1].substr(1, string::npos));
        jobId = std::stoi(args[2]);
    } catch (...) {
        throw InvalidArgs("kill");
    }

    JobsList::JobEntry *job = smash.jobs.getJobById(jobId);
    if (job == nullptr) throw JobDoesNotExist("kill", jobId);
    //if (sig > 31) throw InvalidArgs("kill");
    //  if(!is_number(args[1].substr(1, string::npos))||!is_number(args[2].substr(1, string::npos))) throw InvalidArgs("kill");

    int status = kill(job->getPid(), sig);

    if (status == -1) {
        KillFailed();
        return;
    }
    if (sig == SIGSTOP) {
        job->setCurrentStatus(STOPPED);

    }
    std::cout << "signal number " << sig << " was sent to pid " << (job->getPid()) << std::endl;

}

void QuitCommand::execute() {//No Need to delete smash, should be destroyed (according to them at least)
    SmallShell &smash = SmallShell::getInstance();
    if (num_args > 1) {
        if (args[1] == "kill") {
            std::cout << "smash: sending SIGKILL signal to " << smash.jobs.getSize() << " jobs:" << std::endl;
            smash.jobs.printKilledJobs();
            smash.jobs.killAllJobs();
        }
    }

    throw QuitExp(); //Changed from exit() because default d'tors wouldn't be called.

}

void ForegroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (num_args > 2) {
        throw InvalidArgs("fg");
    }
    if (num_args == 2) {
        std::string jobStr = std::string(args[1]);
        int requestedJob = 0;
        try {
            requestedJob = stoi(jobStr);
        } catch (...) {
            throw InvalidArgs("fg");
        }
        if (smash.jobs.getJobById(requestedJob) != nullptr) {
            JobsList::JobEntry *job = smash.jobs.getJobById(requestedJob);
            int status1;
            if (job->getCurrentStatus() == STOPPED) {
                int status2 = kill(job->getPid(), SIGCONT);
                if (status2 == -1) {
                    KillFailed();
                    return;
                }
                job->setCurrentStatus(NOT_STOPPED);
            }
            smash.fgCommand = job->getCmd();
            smash.fgCommand->setPid(job->getPid());
            smash.jobs.setBgTofgId(requestedJob);
            smash.jobs.removeJobById(requestedJob);
            std::cout << job->getCmd()->getCmdLine() << " : " << job->getPid() << std::endl;
            waitpid(job->getPid(), &status1, WUNTRACED);
            if (status1 == -1) {
                WaitPIDFailed();
                return;
            }
            if (!WIFSTOPPED(status1)) {
                smash.timedCommandHandler.commandList.erase(job->getJobId());
                smash.timedCommandHandler.findNextAlarm();
            }
        } else {
            throw ThereIsNoJob("fg", requestedJob);
        }
    } else {
        if (smash.jobs.GetMaxJobId() == 0) {
            throw JobListEmpty("fg");
        }
        int requestedJob = smash.jobs.GetMaxJobId();
        JobsList::JobEntry *job = smash.jobs.getJobById(requestedJob);
        if (job->getCurrentStatus() == STOPPED) {
            int status = kill(job->getPid(), SIGCONT);
            if (status == -1) {
                KillFailed();
                return;
            }
            job->setCurrentStatus(NOT_STOPPED);
        }
        smash.fgCommand = job->getCmd();
        smash.fgCommand->setPid(job->getPid());
        smash.jobs.setBgTofgId(requestedJob);
        smash.jobs.removeJobById(requestedJob);
        std::cout << job->getCmd()->getCmdLine() << " : " << job->getPid() << std::endl;
        int status1 = waitpid(job->getPid(), nullptr, WUNTRACED);
        if (status1 == -1) {
            WaitPIDFailed();
            return;
        }
        if (!WIFSTOPPED(status1)) {
            smash.timedCommandHandler.commandList.erase(job->getJobId());
            smash.timedCommandHandler.findNextAlarm();
        }
    }
    smash.fgCommand = nullptr;

}

void BackgroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    if (num_args > 2) {
        throw InvalidArgs("bg");
    }
    if (num_args == 2) {
        std::string jobStr = std::string(args[1]);
        int requestedJob = stoi(jobStr);
        if (smash.jobs.getJobById(requestedJob) != nullptr) {
            if (smash.jobs.getJobById(requestedJob)->getCurrentStatus() != STOPPED) {
                throw AlreadyInBackground("bg", requestedJob);
            }
            JobsList::JobEntry *job = smash.jobs.getJobById(requestedJob);
            smash.jobs.setBgTofgId(0);
            job->setCurrentStatus(NOT_STOPPED);
            int status = kill(job->getPid(), SIGCONT);
            if (status == -1) {
                KillFailed();
                return;
            }
            std::cout << job->getCmd()->getCmdLine() << " : " << job->getPid() << std::endl;
        } else {
            throw ThereIsNoJob("bg", requestedJob);
        }
    } else {
        int requestedJob = smash.jobs.getMaxStoppedJobID();
        if (requestedJob == 0) {
            throw NoStoppedJobs("bg");
        }
        JobsList::JobEntry *job = smash.jobs.getJobById(requestedJob);
        smash.jobs.setBgTofgId(0);
        job->setCurrentStatus(NOT_STOPPED);
        int status = kill(job->getPid(), SIGCONT);
        if (status == -1) {
            KillFailed();
            return;
        }
        std::cout << job->getCmd()->getCmdLine() << " : " << job->getPid() << std::endl;
    }
}

PipeCommand::PipeCommand(const char *cmdLine) : Command(cmdLine) {
    commands = parsePipeAndRedirection(cmdLine, &channel);


}

void PipeCommand::execute() {
    int outchannel = STDOUT;
    int fd[2];
    int status = pipe(fd);
    SmallShell &smash = SmallShell::getInstance();
    if (smash.is_child) exit(0);

    if (status == -1) {
        PipeFailed();
        return;
    }
    int pid_1 = fork();
    if (pid_1 == -1) {
        ForkFailed();
        return;
    }
    if (pid_1 == 0) {//First Child - Writes
        setpgrp();
        smash.is_child = true;
        if (channel == ERR) {
            outchannel = STDERR;
        }

            dup2(fd[1], outchannel);
            close(fd[0]);
        close(fd[1]);

            smash.executeCommand(commands.first.c_str(), false);
            exit(0);

        }
    int pid_2 = fork();
    if (pid_2 < -1) {
        ForkFailed();
        return;
    }
    if (pid_2 == 0) {
        smash.is_child = true;
        dup2(fd[0], 0);
        close(fd[1]);
        close(fd[0]);
        smash.executeCommand(commands.second.c_str(), false);
        exit(0);
    }


    close(fd[0]);
    close(fd[1]);
    if (waitpid(pid_1, nullptr, WUNTRACED) == -1) {//Waits for previous to finish
        WaitPIDFailed();
        return;
    }
    if (waitpid(pid_2, nullptr, WUNTRACED) == -1) {//Waits for previous to finish
        WaitPIDFailed();
        return;
    }


    }


void RedirectionCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();

    if (cmd_line[(cmd_line.find_first_of(">")) + 1] == '>') { //meaning it need to append

//
//        int newPID = fork();
//        if (newPID == -1) {
//            ForkFailed();
//        }
//        if (newPID == 0) {//son
//            smash.is_child = true;
//            setpgrp();
            int savedStdOut = 1;
            try {
                int savedStdOut = dup(1);
                int openVal = open(commands.second.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0777);
                if (openVal == -1) {
                    SysCallFailed("open");
                    return;
                }
                dup2(openVal, 1);
                char cmdArray[commands.first.length() + 1];
                strcpy(cmdArray, commands.first.c_str());
                smash.executeCommand(commands.first.c_str());
                dup2(savedStdOut, 1);
                close(savedStdOut);
            }
            catch (SmashException &e) {
                dup2(savedStdOut, 1);
                close(savedStdOut);
                std::cerr << "smash error: " << e.what() << std::endl; // todo: maybe does some problems
            }
//            close(1);
        return;
//
//
//            //execv(reinterpret_cast<const char *>(cmdArray[0]), args);
//
//            close(openVal);
//            open(savedStdOut);
//            exit(0);
//        } else {
//            wait(NULL);
//        }
    } else {
//        int newPID = fork();
//        if (newPID == -1) {
//            ForkFailed();
//        }
//        if (newPID == 0) {//son
//            smash.is_child = true;
//            setpgrp();
            int savedStdOut = 1;
            try {
                savedStdOut = dup(1);
                int openVal = open(commands.second.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
                if (openVal == -1) {
                    SysCallFailed("open");
                    return;
                }
                dup2(openVal, 1);
                char cmdArray[commands.first.length() + 1];
                strcpy(cmdArray, commands.first.c_str());
                smash.executeCommand(commands.first.c_str());
                dup2(savedStdOut, 1);
                close(savedStdOut);
            }
            catch (SmashException &e) {
                dup2(savedStdOut, 1);
                close(savedStdOut);
                std::cerr << "smash error: " << e.what() << std::endl; // todo: maybe does some problems
            }
//            close(1);
//            int openVal = open(commands.second.c_str(), O_WRONLY | O_CREAT, 777);
//            char cmdArray[commands.first.length() + 1];
//            strcpy(cmdArray, commands.first.c_str());
//            char *args[] = {cmdArray, NULL};
//            // execv(reinterpret_cast<const char *>(cmdArray[0]), args);
//            smash.executeCommand(commands.first.c_str());
//            exit(0);
//        } else {
//            wait(NULL);
//        }
    }
    return;
}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {
    commands = parsePipeAndRedirection(cmd_line, &channel);
}

void CatCommand::execute() {
    if (args.size() == 1) {
        throw NotEnoughArguments("cat");
    }
    for (int i = 1; i < args.size(); i++) {
        int openVal = open(args[i].c_str(), O_RDONLY);
        if (openVal == -1) {
            perror("smash error: open failed");
            return;
        } else {
            char buffer;
            int status;
            int returnVal;
            while ((status = read(openVal, &buffer, 1)) == 1) {
                returnVal = write(1, &buffer, 1);
                if (returnVal == -1) {
                    perror("smash error: write failed");
                    return;
                }
            }
            if (status == -1) {
                perror("smash error: read failed");
                return;
            }
            returnVal = close(openVal);
            if (returnVal == -1) {
                perror("smash error: close failed");
                return;
            }
        }
    }

}


int TimedCommandHandler::TimedCommand::getDuration() const {
    return duration;
}


time_t TimedCommandHandler::TimedCommand::getStart() const {
    return startTime;
}





TimedCommandHandler::TimedCommand::TimedCommand(int duration, time_t start, pid_t pid, const string &cmdLine)
        : duration(duration), startTime(start), pid(pid) {
    cmd_line = new std::string(cmdLine);


}

int TimedCommandHandler::TimedCommand::getPidT() const {
    return pid;
}

string *TimedCommandHandler::TimedCommand::getCmdLine() const {
    return cmd_line;
}


void TimedCommandHandler::addTimedCommand(int duration, pid_t pid, const std::string &cmdLine) {

    time_t start;
    time(&start);

    TimedCommand *tc = new TimedCommand(duration, start, pid, cmdLine);
    commandList.insert(pair<int, TimedCommand>(pid, *tc));
}

int TimedCommandHandler::findNextAlarm() {
    if (commandList.empty()) return -1;

    time_t currentTime;
    time(&currentTime);
    if (currentTime == -1) {
        TimeFailed();
        return -1;
    }

    int next =
            commandList.begin()->second.getDuration() - difftime(currentTime, commandList.begin()->second.getStart());

    for (auto cmd: commandList) {
        int cmd_time = cmd.second.getDuration() - difftime(currentTime, cmd.second.getStart());

        if (cmd_time > 0 && cmd_time < next) {
            next = cmd_time;
        }
    }
    return next;


}

TimedCommandHandler::TimedCommand *TimedCommandHandler::findTimeOut() {
    if (commandList.empty()) return nullptr;

    time_t currentTime;
    time(&currentTime);
    if (currentTime == -1) TimeFailed();

    for (auto cmd = commandList.begin(); cmd != commandList.end(); cmd++) {
        if ((cmd->second.getDuration() - difftime(currentTime, cmd->second.getStart())) == 0) {
            return &(cmd->second);
        }
    }
    return nullptr;
}



