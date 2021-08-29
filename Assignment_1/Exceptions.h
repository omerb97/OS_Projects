//
// Created by Tomer Melnik on 2021/04/22.
//

#ifndef OS1_EXCEPTIONS_H
#define OS1_EXCEPTIONS_H

#include <exception>
#include <string>

#endif //OS1_EXCEPTIONS_H

/*************************************************SmashExceptions**********************************/


class SmashException : std::exception {
protected:
    std::string message;
public:
    SmashException() = default;

    const char *what() const noexcept override {
        return message.c_str();
    }
};

class TooManyArguments : public SmashException {

public:
    TooManyArguments(std::string cmd_name) {
        message = cmd_name + ": too many arguments";
    }

    ~TooManyArguments() = default;


};

class NotEnoughArguments : public SmashException {

public:
    NotEnoughArguments(std::string cmd_name) {
        message = cmd_name + ": not enough arguments";
    }

    ~NotEnoughArguments() = default;


};


class NoPreviousPath : public SmashException {

public:
    NoPreviousPath(std::string cmd_name) {
        message = cmd_name + ": OLDPWD not set";
    }

    ~NoPreviousPath() = default;


};

class InvalidArgs : public SmashException {

public:
    InvalidArgs(std::string cmd_name) {
        message = cmd_name + ": invalid arguments";
    }

    ~InvalidArgs() = default;


};

class InvalidArg : public SmashException {

public:
    InvalidArg(std::string cmd_name) {
        message = cmd_name + ": invalid argument";
    }

    ~InvalidArg() = default;


};


class JobDoesNotExist : public SmashException {

    int jobId;
public:
    JobDoesNotExist(std::string cmd_name, int job_id) {
        message = cmd_name + ": job-id " + std::to_string(job_id) + " does not exist";
        //this is a test comment
        jobId = job_id;
    }

    ~JobDoesNotExist() = default;


};

class JobListEmpty : public SmashException {

public:
    JobListEmpty(std::string cmd_name) {
        message = cmd_name + ": jobs list is empty";
    }

    ~JobListEmpty() = default;


};

class JobAlreadyRunning : public SmashException {


public:
    JobAlreadyRunning(std::string cmd_name, int job_id) {
        message = cmd_name + ": job-id" + std::to_string(job_id)
                  + ": is already running in the background";

    }

    ~JobAlreadyRunning() = default;


};

class NoStoppedJobs : public SmashException {

public:
    NoStoppedJobs(std::string cmd_name) {
        message = cmd_name + ": there is no stopped jobs to resume";
    }

    ~ NoStoppedJobs() = default;


};


/*************************************************FailedSysCalls**********************************/
class SysCallFailed {
public:
    SysCallFailed(std::string cmd_name) {
        perror(("smash error: " + cmd_name + " failed").c_str());
    }

    ~SysCallFailed() = default;

};

class KillFailed : public SysCallFailed {
public:
    KillFailed() : SysCallFailed("kill") {}

    ~KillFailed() = default;
};

class ForkFailed : public SysCallFailed {
public:
    ForkFailed() : SysCallFailed("fork") {}

    ~ForkFailed() = default;
};

class ChDirFailed : public SysCallFailed {
public:
    ChDirFailed() : SysCallFailed("chdir") {}

    ~ChDirFailed() = default;
};

class CatFailed : public SysCallFailed {
public:
    CatFailed() : SysCallFailed("cat") {}

    ~CatFailed() = default;
};

class ThereIsNoJob : public SmashException {


public:
    ThereIsNoJob(std::string cmd_name, int job_id) {
        message = cmd_name + ": job-id " + std::to_string(job_id) + " does not exist";

    }

    ~ThereIsNoJob() = default;


};

class AlreadyInBackground : public SmashException {

public:
    AlreadyInBackground(std::string cmd_name, int job_id) {
        message = cmd_name + ": job-id " + std::to_string(job_id)
                  + " is already running in the background";

    }

    ~AlreadyInBackground() = default;


};

class QuitExp : std::exception {
protected:
    std::string message;
public:
    QuitExp() = default;

    ~QuitExp() = default;
};


class PipeFinish : std::exception {
public:
    PipeFinish() = default;

    ~PipeFinish() = default;
};


class WaitPIDFailed : public SysCallFailed {
public:
    WaitPIDFailed() : SysCallFailed("waitpid") {}

    ~WaitPIDFailed() = default;
};

class PipeFailed : public SysCallFailed {
public:
    PipeFailed() : SysCallFailed("pipe") {}

    ~PipeFailed() = default;
};

class TimeFailed : public SysCallFailed {
public:
    TimeFailed() : SysCallFailed("time") {}

    ~TimeFailed() = default;
};