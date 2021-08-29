#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    std::cout << "smash: got ctrl-Z" << std::endl;
    SmallShell &smash = SmallShell::getInstance();
    if (smash.fgCommand != NULL) {
        if (smash.jobs.getBgTofgId() != 0) {
            smash.jobs.removeJobById(smash.jobs.getBgTofgId());



            smash.jobs.AddJobByID(smash.fgCommand, smash.fgCommand->getPid(), smash.jobs.getBgTofgId(),
                                  STOPPED);
            smash.jobs.setBgTofgId(0);
        } else {

            smash.jobs.addJob(smash.fgCommand, smash.fgCommand->getPid(), STOPPED);
        }

        if (kill(smash.fgCommand->getPid(), SIGSTOP) == -1) {
            KillFailed();
            return;
        }
        std::cout << "smash: process " << smash.fgCommand->getPid() << " was stopped" << std::endl;

    }
    smash.fgCommand = NULL;
}

void ctrlCHandler(int sig_num) {
    std::cout << "smash: got ctrl-C" << std::endl;
    SmallShell &smash = SmallShell::getInstance();
    if (smash.fgCommand != nullptr) {
        if (kill(smash.fgCommand->getPid(), SIGKILL) == -1) {
            KillFailed();
            return;
        }
        std::cout << "smash: process " << smash.fgCommand->getPid() << " was killed" << std::endl;
    }
    smash.fgCommand = nullptr; //todo: maybe this needs to be in the if
}

void alarmHandler(int sig_num) {
    std::cout << "smash: got an alarm" << std::endl;
    SmallShell &smash = SmallShell::getInstance();
    smash.jobs.removeFinishedJobs();
    TimedCommandHandler::TimedCommand *tc = smash.timedCommandHandler.findTimeOut();
    if (tc == NULL) return;  //if empty do nothing, should never get here though
    int ping = kill(tc->getPidT(), 0); //checks if process is not finished
    if (ping == -1) return; //if child is done do nothing
    int check = kill(tc->getPidT(), SIGKILL);
    if (check == -1) {
        KillFailed();
        return;
    }
    std::cout << "smash: " << *tc->getCmdLine() << std::endl;
    smash.timedCommandHandler.commandList.erase(tc->getPidT());
    if (smash.timedCommandHandler.commandList.empty()) return; //if no more timed commands are present

    int nextAlarm = smash.timedCommandHandler.findNextAlarm();
    if (nextAlarm < 0) return;
    alarm(nextAlarm);
    std::cout << smash.getPrompt();

}

