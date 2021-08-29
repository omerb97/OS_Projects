#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <map>
#include "Exceptions.h"


#define  STOPPED 1
#define NOT_STOPPED 0

enum outChannel {
    OUT, ERR, OVERRIDE, APPEND
};
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

/*************************************************Commands*********************************************/
class Command {

protected:
    int pid;
    std::string cmd_line;
    int num_args;

public:
    Command(const char *cmd_line);

    int getPid() const;

    void setPid(int pid);

    int getNumArgs() const;

    const std::string &getCmdLine() const;

    virtual ~Command() = default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    std::string getTrimmedLine();

    void setCmdLine(const std::string &cmdLine);

    bool isBackground;
    std::vector<std::string> args;
};



class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line) : Command(cmd_line) {};

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line) : Command(cmd_line) {};


    virtual ~ExternalCommand() {}

    void execute() override;
};

/*************************************************Special Commands**********************************/
class PipeCommand : public Command {
    std::pair<std::string, std::string> commands;
    outChannel channel;

public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    std::pair<std::string, std::string> commands;
    outChannel channel;
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

/************************************************BuiltIn Commands**********************************/
class ChangeDirCommand : public BuiltInCommand {
public:
    ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
public:
    QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~QuitCommand() {}

    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
public:
    ChangePromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~ChangePromptCommand() {}

    void execute() override;

};


/*************************************************Alarm Classes**********************************/
class TimedCommandHandler {


public:
    class TimedCommand {
        int duration;
        time_t startTime;
        pid_t pid;
        std::string *cmd_line;

    public:
        TimedCommand(int duration, time_t start, pid_t pid, const std::string &cmdLine);
        int getDuration() const;

        std::string *getCmdLine() const;
        time_t getStart() const;
        int getPidT() const;

        std::string *getOriginalCmdLine() const;


    };

public:
    std::map<int, TimedCommand> commandList;


    TimedCommandHandler() = default;

    void addTimedCommand(int duration, pid_t pid, const std::string &cmdLine);

    TimedCommand *findTimeOut();

    int findNextAlarm();


};

/*************************************************JobList**********************************/

class JobsList {
public:
    class JobEntry {
        int jobID;
        pid_t PID; //inserted by father when son is created
        bool currentStatus;
        time_t jobInserted;
        Command *cmd;
    public:
        JobEntry(Command *cmd, int PID, bool isStopped = false);

        void setJobId(int jobId);

        void setCurrentStatus(bool currentStatus);

        void setJobInserted(time_t jobInserted);

        bool getCurrentStatus() const;

        time_t getJobInserted() const;

        int getJobId();

        pid_t getPid() const;

        Command *getCmd() const;

        bool operator<(const JobEntry &jobEntry) const;

        friend std::ostream &operator<<(std::ostream &os, const JobEntry &je);
    };

    int maxJobID;
    int bgTofgID;
    std::map<int, JobEntry *> entries;
public:
    JobsList();

    ~JobsList() = default;

    void addJob(Command *cmd, int PID, bool isStopped = false);

    void printJobsList();

    void printKilledJobs();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId); //Removes job and sets new Jobmax

    void removeJobByPId(pid_t PId);



    bool isEmpty();

    int getMaxStoppedJobID();

    int GetMaxJobId();

    void setBgTofgId(int bgTofgId);

    int getBgTofgId() const;

    void AddJobByID(Command *cmd, int PID, int jobID, bool isStopped = false);

    int getSize();
    JobEntry *getLastStoppedJob(int *jobId);

};

/*************************************************AdvancedBuiltIn Commands**********************************/
class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~JobsCommand() = default;

    void execute() override;
};

class KillCommand : public BuiltInCommand {

public:
    KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~KillCommand() = default;

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {

public:
    ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~ForegroundCommand() = default;

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {

public:
    BackgroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~BackgroundCommand() = default;

    void execute() override;
};

class CatCommand : public BuiltInCommand {
public:
    CatCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~CatCommand() = default;

    void execute() override;
};

/*************************************************Smash Class*********************************/
class SmallShell {
private:
    std::string prompt;
    std::string lastPath;



    SmallShell();

public:
    Command *fgCommand;
    JobsList jobs;
    TimedCommandHandler timedCommandHandler;
    bool is_child;
    pid_t pid;

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    const std::string &getLastPath() const;

    void setLastPath(const std::string &lastPath);

    ~SmallShell() = default;

    void executeCommand(const char *cmd_line, bool forkable = true);

    void setNewPrompt(std::string const &new_prompt);

    std::string getPrompt();


};

#endif //SMASH_COMMAND_H_
