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
#include <queue>

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
		randvals = NULL;
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

class process {
public:
	// process attributes - input
	int AT;				// arrival time
	int TC;				// total CPU time
	int CB;				// CPU burst
	int IO;				// IO burst
	int PRIO;			// priority

	// process execution info - output
	int FT;				// finishing time
	int TT;				// turnaround time
	int IT;				// IO time
	int CW;				// CPU waiting time

	// constructor
	process() {};
	process(int arrival_time, int total_CPU_time, int CPU_burst, int IO_burst, int priority) {
		AT = arrival_time;
		TC = total_CPU_time;
		CB = CPU_burst;
		IO = IO_burst;
		PRIO = priority;
	};

	// destructor
	~process() {};
};

class event {
public:
	// event attributes
	int TS;				// time stamp
	process *PROC;		// process
	transition_t TRANS;	// transition

	// constructor
	event() {};
	event(int timestamp, process *p, transition_t transition) {
		TS = timestamp;
		PROC = p;
		TRANS = transition;
	};

	// destructor
	~event() {};
};

class scheduler {
public:
	virtual void add_to_queue(process *p) = 0;
	virtual process *get_next_process() = 0;

	// destructor
	virtual  ~scheduler() {};
};

class FCFS : public scheduler {
public:
	void add_to_queue(process *p);
	process *get_next_process();
};

class LCFS : public scheduler {
public:
	void add_to_queue(process *p);
	process *get_next_process();
};

class SRTF : public scheduler {
public:
	void add_to_queue(process *p);
	process *get_next_process();
};

class RR : public scheduler {
public:
	void add_to_queue(process *p);
	process *get_next_process();
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

queue<event *> event_queue;

int main(int argc, char *argv[]) {
	// read command - get scheduler type
	int c;
	string scheduler_type = "";
	bool verbose = false;
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

	// read files
	char *inputfile = argv[optind];
	char *randfile = argv[optind + 1];
	myrandom r(randfile);

	return 0;
}
