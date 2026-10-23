namespace ptrace_sys
{

int attachAll (pid_t pid);
int cont(pid_t pid, int signal);
void detach(pid_t pid);
bool disable_aslr(void);
// Has the process replaced its image with another program?
bool eventIsExec(int signal, int event);
// Does this event relate to creating new processes or threads?
bool eventIsForky(int signal, int event);
// Does this event come from a newly created, traced, child?
bool eventIsNewChild(int signal, int event);
int follow_child(pid_t pid);
int follow_fork(pid_t pid);
int get_current_cpu(void);
// Get the entry address of the process' executable as loaded, or 0 if unknown
unsigned long get_entry_address(pid_t pid);
// Get the event that caused a trap.  The event is opaque to the caller.
int getEvent(pid_t pid, int status);
unsigned long getPc(int pid);
unsigned long peekWord(pid_t pid, unsigned long aligned_addr);
void pokeWord(pid_t pid, unsigned long aligned_addr, unsigned long value);
// Give a process the CPU affinity saved by save_cpu_affinity
void restore_cpu_affinity(pid_t pid);
// Save the current CPU affinity, before tie_process_to_cpu changes it
void save_cpu_affinity(void);
void singleStep(pid_t pid);
void skipInstruction(pid_t pid);
void tie_process_to_cpu(pid_t pid, int cpu);
int trace_me(void);
int wait_all(int *status);

}
