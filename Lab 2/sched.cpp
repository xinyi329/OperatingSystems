/* Lab 2 - OS Spring 2019		*/
/* Xinyi Liu (xl2700)			*/
/* Compiler Version: gcc-4.9.2	*/

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <cstring>
#include <string>
#include <list>
#include <vector>

using namespace std;

class myrandom {
private:
	int *randvals;
	int size;
	int ofs;
public:
	// get a random number
	int get_random(int burst) {
		ofs = ofs % size;
		int num = 1 + (randvals[ofs] % burst);
		ofs++;
		return num;
	};

	// for tests
	void print() {
		cout << "size\t" << size << "\n";
		for(int i = 0; i < size; i++) {
			cout << randvals[i] << "\n"; 
		}
	}

	// constructor
	myrandom() {};
	myrandom(char *path) {
		ifstream file;
		file.open(path);
		if(!file.is_open()) {
			exit(0);
		}
		string temp;
		getline(file, temp);
		size = stoi(temp);
		randvals = new int[size];
		for(int i = 0; i < size; i++) {
			getline(file, temp);
			randvals[i] = stoi(temp);
		}
		ofs = 0;
	};

	// destructor
	~myrandom() {
		delete[] randvals;
		randvals = nullptr;
	};
};

typedef enum {
	CREATED,
	READY,
	RUNNING,
	BLOCKED,
	DONE
} state_t;

typedef enum {
	TRANS_TO_READY,
	TRANS_TO_RUNNING,
	TRANS_TO_BLOCKED,
	TRANS_TO_PREEMPT,
	TRANS_TO_DONE
} transition_t;

struct process {
	// process attributes - input
	int PID;			// process ID
	int AT;				// arrival time
	int TC;				// total CPU time
	int CB;				// CPU burst
	int IO;				// IO burst
	int PRIO;			// priority

	// process progress - dynamic
	int dPRIO;			// dynamic priority
	state_t STATE;		// current state
	int STATE_TS;		// last state time stamp
	int STATE_T;		// time in previous state
	int RCT;			// remaining CPU time
	int CCB;			// current CPU burst
	int CIO;			// current IO burst

	process(int process_id, int arrival_time, int total_CPU_time, int CPU_burst, int IO_burst, int priority) {
		PID = process_id;
		AT = arrival_time;
		TC = total_CPU_time;
		CB = CPU_burst;
		IO = IO_burst;
		PRIO = priority;
		dPRIO = priority - 1;
		STATE = CREATED;
		STATE_TS = arrival_time;
		STATE_T = 0;
		RCT = total_CPU_time;
		CCB = 0;
		CIO = 0;
	};
};

struct process_stat {
	// process attributes
	int PID;			// process ID
	int AT;				// arrival time
	int TC;				// total CPU time
	int CB;				// CPU burst
	int IO;				// IO burst
	int PRIO;			// priority

	// process execution info
	int FT;				// finishing time
	int TT;				// turnaround time
	int IT;				// IO time
	int CW;				// CPU waiting time

	process_stat(int process_id, int arrival_time, int total_CPU_time, int CPU_burst, int IO_burst, int priority) {
		PID = process_id;
		AT = arrival_time;
		TC = total_CPU_time;
		CB = CPU_burst;
		IO = IO_burst;
		PRIO = priority;
		FT = 0;
		TT = 0;
		IT = 0;
		CW = 0;
	};
};

struct event {
	// event attributes
	int TS;				// time stamp
	process *PROC;		// process
	state_t PREV;		// previous state
	state_t NEXT;		// next state
	transition_t TRANS;	// transition
	int RCT;
	int CCB;

	event(int timestamp, process *p, state_t previous_state, state_t next_state, transition_t transition) {
		TS = timestamp;
		PROC = p;
		PREV = previous_state;
		NEXT = next_state;
		TRANS = transition;
		RCT = p->RCT;
		CCB = p->CCB;
	};
};

class event_queue {
public:
	list<event *> q;	// use list rather than queue for iteration use

	event *get_event() {
		if(q.empty()) {
			return nullptr;
		}
		else {
			event *evt = q.front();
			q.pop_front();
			return evt;
		}
	};

	void put_event(event *evt) {
		if(q.empty()) {
			q.push_back(evt);
		}
		else {
			list<event *>::iterator iter;
			for(iter = q.begin(); iter != q.end(); iter++) {
				if((*iter)->TS > evt->TS) {
					q.insert(iter, evt);
					break;
				}
			}
			if(iter == q.end()) {
				q.push_back(evt);
			}
		}
	};

	/*
	void rm_event(event *evt) {
		for(list<event *>::iterator iter = q.begin(); iter != q.end(); iter++) {
			if(evt == (*iter)) {
				q.erase(iter);
				break;
			}
		}
	};
	*/

	int get_next_event_time() {
		if(q.empty()) {
			return -1;
		}
		else {
			return q.front()->TS;
		}
	};

	// for tests
	void print() {
		for(list<event *>::iterator iter = q.begin(); iter != q.end(); iter++) {
			cout << (*iter)->PROC->PID << "\t" << (*iter)->PROC->AT << "\t" << (*iter)->PROC->TC << "\t" << (*iter)->PROC->CB << "\t" << (*iter)->PROC->IO << "\n";
		}
	};

	// constructor
	event_queue() {};

	// destructor
	~event_queue() {};
};

class scheduler {
public:
	string type;

	bool preempt;

	virtual void add_to_queue(process *p) = 0;

	virtual process *get_next_process() = 0;

	// constructor
	scheduler() {};

	// destructor
	virtual ~scheduler() {};
};

class FCFS : public scheduler {
private:
	list<process *> run_queue;

public:
	void add_to_queue(process *p) {
		p->STATE = READY;
		run_queue.push_back(p);
	};

	process *get_next_process()  {
		if(run_queue.empty()) {
			return nullptr;
		}
		else {
			process *p = run_queue.front();
			run_queue.pop_front();
			return p;
		}
	};

	FCFS() {
		type = "FCFS";
		preempt = false;
	};
};

class LCFS : public scheduler {
private:
	list<process *> run_queue;

public:
	void add_to_queue(process *p) {
		p->STATE = READY;
		run_queue.push_back(p);
	};

	process *get_next_process()  {
		if(run_queue.empty()) {
			return nullptr;
		}
		else {
			process *p = run_queue.back();
			run_queue.pop_back();
			return p;
		}
	};

	LCFS() {
		type = "LCFS";
		preempt = false;
	};
};

class SRTF : public scheduler {
private:
	list<process *> run_queue;

public:
	void add_to_queue(process *p) {
		p->STATE = READY;
		list<process *>::iterator iter;
		for(iter = run_queue.begin(); iter != run_queue.end(); iter++) {
			if((*iter)->RCT > p->RCT) {
				run_queue.insert(iter, p);
				break;
			}
		}
		if(iter == run_queue.end()) {
			run_queue.push_back(p);
		}
	};

	process *get_next_process()  {
		if(run_queue.empty()) {
			return nullptr;
		}
		else {
			process *p = run_queue.front();
			run_queue.pop_front();
			return p;
		}
	};

	SRTF() {
		type = "SRTF";
		preempt = false;
	};
};

class RR : public scheduler {
private:
	list<process *> run_queue;

public:
	void add_to_queue(process *p) {
		p->STATE = READY;
		run_queue.push_back(p);
	};

	process *get_next_process()  {
		if(run_queue.empty()) {
			return nullptr;
		}
		else {
			process *p = run_queue.front();
			run_queue.pop_front();
			return p;
		}
	};

	RR() {
		type = "RR";
		preempt = false;
	};
};

class PRIO : public scheduler {
public:
	void add_to_queue(process *p);
	process *get_next_process();
};

class PREPRIO : public scheduler {
public:
	void add_to_queue(process *p);
	process *get_next_process();
};

int FT = 0;				// finishing time
int CPU_UTIL = 0;		// CPU utilization
int IO_UTIL = 0;		// IO utilization
int PREV_IO_FT = 0;		// last finishing time of IO usage
vector<process_stat> stat;

void simulation(scheduler *s, event_queue *eq, int quantum, myrandom &r) {
	event *e;
	int current_time = 0;
	bool call_scheduler = false;
	process *current_running_process = nullptr;

	while(e = eq->get_event()) {
		process *p = e->PROC;
		current_time = e->TS;
		p->STATE_T = current_time - p->STATE_TS;

		switch(e->TRANS) {
			case TRANS_TO_READY: {
				// must come from BLOCKED or from PREEMPT
				// deal with preemption
				if(s->preempt && current_running_process) {
					int timestamp = 0;
					list<event *>::iterator iter;
					for(iter = eq->q.begin(); iter != eq->q.end(); iter++) {
						if((*iter)->PROC->PID == current_running_process->PID) {
							timestamp = (*iter)->TS;
							break;
						}
					}
					if(timestamp > 0 && timestamp != current_time) {
						current_running_process->RCT = (*iter)->RCT - (current_time - current_running_process->STATE_TS);
						current_running_process->CCB = (*iter)->CCB - (current_time - current_running_process->STATE_TS);
					}
					eq->q.erase(iter);
					event *temp = new event(current_time, current_running_process, RUNNING, READY, TRANS_TO_PREEMPT);
					eq->put_event(temp);
				}
				// must add to run_queue
				p->STATE_TS = current_time;
				// p->STATE = READY;
				s->add_to_queue(p);
				call_scheduler = true;
				break;
			}
			case TRANS_TO_RUNNING: {
				p->STATE = RUNNING;
				p->STATE_TS = current_time;
				if(p->CCB == 0) {
					p->CCB = r.get_random(p->CB);
					if(p->CCB > p->RCT) {
						p->CCB = p->RCT;
					}
				}
				stat[p->PID].CW += p->STATE_T;
				// create event for either PREEMPT or BLOCKED
				// RUNNING -> PREEMPT
				int temp_RCT = p->RCT;
				int temp_CCB = p->CCB;
				if(quantum < p->CCB) {
					// DONE
					if(p->RCT <= quantum) {
						int timestamp = current_time + p->RCT;
						p->RCT = 0;
						event *temp = new event(timestamp, p, RUNNING, DONE, TRANS_TO_DONE);
						temp->RCT = temp_RCT;
						temp->CCB = temp_CCB;
						eq->put_event(temp);
					}
					// PREEMPT
					else {
						int timestamp = current_time + quantum;
						p->RCT -= quantum;
						p->CCB -= quantum;
						event *temp = new event(timestamp, p, RUNNING, READY, TRANS_TO_PREEMPT);
						temp->RCT = temp_RCT;
						temp->CCB = temp_CCB;
						eq->put_event(temp);
					}
				}
				// RUNNING -> BLOCKED
				else {
					// DONE
					if(p->RCT <= p->CCB) {
						int timestamp = current_time + p->RCT;
						p->RCT = 0;
						event *temp = new event(timestamp, p, RUNNING, DONE, TRANS_TO_DONE);
						temp->RCT = temp_RCT;
						temp->CCB = temp_CCB;
						eq->put_event(temp);
					}
					// BLOCKED
					else {
						int timestamp = current_time + p->CCB;
						p->RCT -= p->CCB;
						p->CCB = 0;
						event *temp = new event(timestamp, p, RUNNING, BLOCKED, TRANS_TO_BLOCKED);
						temp->RCT = temp_RCT;
						temp->CCB = temp_CCB;
						eq->put_event(temp);
					}
				}
				break;
			}
			case TRANS_TO_BLOCKED: {
				p->STATE = BLOCKED;
				p->STATE_TS = current_time;
				p->CIO = r.get_random(p->IO);
				CPU_UTIL += p->STATE_T;
				// IO free
				if(current_time > PREV_IO_FT) {
					IO_UTIL += p->CIO;
					PREV_IO_FT = current_time + p->CIO;
				}
				// IO busy, only adding non-intersection to IO_UTIL
				else if(current_time + p->CIO > PREV_IO_FT) {
					IO_UTIL += (current_time + p->CIO - PREV_IO_FT);
					PREV_IO_FT = current_time + p->CIO;
				}
				stat[p->PID].IT += p->CIO;
				// create an event for when process becomes READY again
				// BLOCKED -> READY
				int timestamp = current_time + p->CIO;
				event *temp = new event(timestamp, p, BLOCKED, READY, TRANS_TO_READY);
				eq->put_event(temp);
				call_scheduler = true;
				current_running_process = nullptr;
				break;
			}
			case TRANS_TO_PREEMPT: {
				// p->STATE = READY;
				p->STATE_TS = current_time;
				CPU_UTIL += p->STATE_T;
				// add to run_queue (no event is generated)
				// PREEMPT -> RUNNING
				s->add_to_queue(p);
				call_scheduler = true;
				current_running_process = nullptr;
				break;
			}
			case TRANS_TO_DONE: {
				CPU_UTIL += p->STATE_T;
				stat[p->PID].FT = current_time;
				call_scheduler = true;
				current_running_process = nullptr;
				break;
			}
			default:
				break;
		}

		delete e;
		e = nullptr;

		if(call_scheduler) {
			if(eq->get_next_event_time() == current_time) {
				// process next event from event_queue
				continue;
			}

			// reset the global flag
			call_scheduler = false;

			if(current_running_process == nullptr) {
				current_running_process = s->get_next_process();
				if(current_running_process == nullptr) {
					continue;
				}
				// create event to make this process runnable for same time
				event *temp = new event(current_time, current_running_process, READY, RUNNING, TRANS_TO_RUNNING);
				eq->put_event(temp);
			}
		}
	}

	FT = current_time;
}

void print_scheduler(string type) {
	cout << type << "\n";
	double TT = 0.0;
	double CW = 0.0;
	for(int i = 0; i < stat.size(); i++) {
		printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
				stat[i].PID,
				stat[i].AT, stat[i].TC, stat[i].CB, stat[i].IO, stat[i].PRIO,
				stat[i].FT,
				stat[i].FT - stat[i].AT,
				stat[i].IT,
				stat[i].CW);
		TT += stat[i].FT - stat[i].AT;
		CW += stat[i].CW;
	}
	double CPU_UTIL_P = 100.0 * CPU_UTIL / FT;
	double IO_UTIL_P = 100.0 * IO_UTIL / FT;
	double TT_AVG = TT / stat.size();
	double CW_AVG = CW / stat.size();
	double throughput = 100.0 * stat.size() / FT;
	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
			FT,
			CPU_UTIL_P,
			IO_UTIL_P,
			TT_AVG,
			CW_AVG,
			throughput);
}

int main(int argc, char *argv[]) {
	string scheduler_type = "";
	bool verbose = false;

	// read command - get output/scheduler type
	int c;
	while(1) {
		c = getopt(argc, argv, "s:vte");
		if(c == -1)
			break;
		switch(c) {
			case 's':
				scheduler_type = optarg;
				break;
			case 'v':
				verbose = true;
				break;
			case 't':
				break;
			case 'e':
				break;
			default:
				break;
		}
	}

	event_queue *eq = new event_queue();

	// read files
	char *inputfile = argv[optind];
	char *randfile = argv[optind + 1];
	int maxprio = 4;
	int quantum = 10000;
	myrandom r(randfile);
	ifstream input;
	input.open(inputfile);
	if(!input.is_open()) {
		exit(0);
	}
	int process_id = 0;
	while(!input.eof()) {
		int arrival_time;
		input >> arrival_time;
		if(input.eof()) {
			break;
		}
		int total_CPU_time;
		input >> total_CPU_time;
		int CPU_burst;
		input >> CPU_burst;
		int IO_burst;
		input >> IO_burst;
		int priority = r.get_random(maxprio);
		process_stat ps(process_id, arrival_time, total_CPU_time, CPU_burst, IO_burst, priority);
		stat.push_back(ps);
		process *p = new process(process_id, arrival_time, total_CPU_time, CPU_burst, IO_burst, priority);
		event *e = new event(arrival_time, p, CREATED, READY, TRANS_TO_READY);
		eq->put_event(e);
		process_id++;
	}

	scheduler *s = new RR();
	quantum = stoi(optarg);

	simulation(s, eq, quantum, r);
	print_scheduler(s->type);

	return 0;
}
