#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"
#include <string.h>


int main(int argc, char* argv[]) {
    struct sigaction alarm;
    // memset(&alarm, '\0', sizeof(alarm)); //I have no idea what im doing fingers crossed, god i hate c syntax
    alarm.sa_handler = &alarmHandler;
    alarm.sa_flags = SA_RESTART;

    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    sigaction(SIGALRM, &alarm, NULL);



    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        if (smash.is_child) { return 0; }
        std::cout << smash.getPrompt();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        try {
            smash.executeCommand(cmd_line.c_str());
            smash.jobs.setBgTofgId(0);
        }
        catch (PipeFinish) {
            continue;
        }
        catch (SmashException &e) {
            std::cerr << "smash error: " << e.what() << std::endl;
        } catch (...) {
            break;
        }

    }
        //smash.setFgCommand(nullptr); //TODO: maybe we need to reset fg command after every command

    return 0;
}
