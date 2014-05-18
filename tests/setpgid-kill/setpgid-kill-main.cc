#include <stdio.h>
#include <unistd.h>

#include <utils.hh>

int main (int argc, char** argv)
{
    printf("pid: %d\n", getpid());
    printf("pgid: %d\n", getpgid(0));

    printf("calling setpgid...\n");
    const int result = setpgid(0, 0);
    printf("setpgid result: %d\n", result);

    int pid = getpid();
    printf("pid: %d\n", pid);
    printf("pgid: %d\n", getpgid(0));

    std::string s = fmt("%d\n", pid);
    write_file(s.c_str(), s.size(), "/tmp/setpgid.pid");

    // Long sleep just to allow kcov to clean up stuff
    sleep(20);

    printf("After sleep\n");

    return 0;
}
