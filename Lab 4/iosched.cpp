/* Lab 4 - OS Spring 2019		*/
/* Xinyi Liu (xl2700)			*/

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
#include <limits.h>

using namespace std;

vector<string> split(string s, string p) {
	vector<string> res;
	string::size_type d;
	s += p;
	int i = 0;
	while(i < s.size()) {
		d = s.find(p, i);
		if(d < s.size()) {
			res.push_back(s.substr(i, d - i));
			i = d + p.length();
		}
		else {
			i++;
		}
	}
	return res;
}

struct req_t {
	int arrival_time;
	int track;
	int start_time;
	int end_time;

	req_t(int at, int t) {
		arrival_time = at;
		track = t;
		start_time = 0;
		end_time = 0;
	}
};

int head;

class iosched_t {
public:
	int direction;

	virtual void add_req(req_t *r) = 0;

	virtual req_t *fetch_req() = 0;

	virtual bool empty() = 0;

	// constructor
	iosched_t() {
		direction = 1;
	};

	// destructor
	virtual ~iosched_t() {};
};

class FIFO : public iosched_t {
private:
	list<req_t *> io_queue;

public:
	void add_req(req_t *r) {
		io_queue.push_back(r);
	};

	req_t *fetch_req() {
		if(io_queue.empty()) {
			return nullptr;
		}
		else {
			req_t *r = io_queue.front();
			io_queue.pop_front();
			if(r->track > head) {
				direction = 1;
			}
			else if(r->track < head) {
				direction = -1;
			}
			return r;
		}
	};

	bool empty() {
		return (io_queue.size() == 0? true : false);
	};
};

class SSTF : public iosched_t {
private:
	list<req_t *> io_queue;

public:
	void add_req(req_t *r) {
		io_queue.push_back(r);
	};

	req_t *fetch_req() {
		if(io_queue.empty()) {
			return nullptr;
		}
		else {
			list<req_t *>::iterator iter;
			list<req_t *>::iterator r_iter;
			int min_distance = INT_MAX;
			for(iter = io_queue.begin(); iter != io_queue.end(); iter++) {
				if(abs(head - (*iter)->track) < min_distance) {
					r_iter = iter;
					min_distance = abs(head - (*iter)->track);
				}
			}
			req_t *r = *r_iter;
			io_queue.erase(r_iter);
			if(r->track > head) {
				direction = 1;
			}
			else if(r->track < head) {
				direction = -1;
			}
			return r;
		}
	};

	bool empty() {
		return (io_queue.size() == 0? true : false);
	};
};

class LOOK : public iosched_t {
private:
	list<req_t *> io_queue;

public:
	void add_req(req_t *r) {
		io_queue.push_back(r);
	};

	req_t *fetch_req() {
		if(io_queue.empty()) {
			return nullptr;
		}
		else {
			list<req_t *>::iterator iter;
			list<req_t *>::iterator r_iter;
			int min_distance = INT_MAX;
			bool reverse = true;
			for(iter = io_queue.begin(); iter != io_queue.end(); iter++) {
				if((direction == 1 && (*iter)->track - head >= 0 && (*iter)->track - head < min_distance) || (direction == -1 && head - (*iter)->track >= 0 && head - (*iter)->track < min_distance)) {
					reverse = false;
					r_iter = iter;
					min_distance = abs(head - (*iter)->track);
				}
			}
			if(reverse) {
				direction = -direction;
				for(iter = io_queue.begin(); iter != io_queue.end(); iter++) {
					if((direction == 1 && (*iter)->track - head >= 0 && (*iter)->track - head < min_distance) || (direction == -1 && head - (*iter)->track >= 0 && head - (*iter)->track < min_distance)) {
						r_iter = iter;
						min_distance = abs(head - (*iter)->track);
					}
				}
			}
			req_t *r = *r_iter;
			io_queue.erase(r_iter);
			return r;
		}
	};

	bool empty() {
		return (io_queue.size() == 0? true : false);
	};
};

class CLOOK : public iosched_t {
private:
	list<req_t *> io_queue;

public:
	void add_req(req_t *r) {
		io_queue.push_back(r);
	};

	req_t *fetch_req() {
		if(io_queue.empty()) {
			return nullptr;
		}
		else {
			list<req_t *>::iterator iter;
			list<req_t *>::iterator r_iter;
			int min_distance = INT_MAX;
			bool reverse = true;
			for(iter = io_queue.begin(); iter != io_queue.end(); iter++) {
				if((*iter)->track - head >= 0 && (*iter)->track - head < min_distance) {
					reverse = false;
					r_iter = iter;
					min_distance = (*iter)->track - head;
				}
			}
			if(reverse) {
				min_distance = INT_MAX;
				for(iter = io_queue.begin(); iter != io_queue.end(); iter++) {
					if((*iter)->track < min_distance) {
						r_iter = iter;
						min_distance = (*iter)->track;
					}
				}
			}
			req_t *r = *r_iter;
			io_queue.erase(r_iter);
			if(r->track > head) {
				direction = 1;
			}
			else if(r->track < head) {
				direction = -1;
			}
			return r;
		}
	};

	bool empty() {
		return (io_queue.size() == 0? true : false);
	};
};

class FLOOK : public iosched_t {
private:
	list<req_t *> active_queue;
	list<req_t *> add_queue;

public:
	void add_req(req_t *r) {
		add_queue.push_back(r);
	};

	req_t *fetch_req() {
		if(active_queue.empty()) {
			active_queue.swap(add_queue);
		}
		if(active_queue.empty()) {
			return nullptr;
		}
		else {
			list<req_t *>::iterator iter;
			list<req_t *>::iterator r_iter;
			int min_distance = INT_MAX;
			bool reverse = true;
			for(iter = active_queue.begin(); iter != active_queue.end(); iter++) {
				if((direction == 1 && (*iter)->track - head >= 0 && (*iter)->track - head < min_distance) || (direction == -1 && head - (*iter)->track >= 0 && head - (*iter)->track < min_distance)) {
					reverse = false;
					r_iter = iter;
					min_distance = abs(head - (*iter)->track);
				}
			}
			if(reverse) {
				direction = -direction;
				for(iter = active_queue.begin(); iter != active_queue.end(); iter++) {
					if((direction == 1 && (*iter)->track - head >= 0 && (*iter)->track - head < min_distance) || (direction == -1 && head - (*iter)->track >= 0 && head - (*iter)->track < min_distance)) {
						r_iter = iter;
						min_distance = abs(head - (*iter)->track);
					}
				}
			}
			req_t *r = *r_iter;
			active_queue.erase(r_iter);
			return r;
		}
	};

	bool empty() {
		return (active_queue.size() == 0 && add_queue.size() == 0? true : false);
	};
};

void simulation(iosched_t *iosched, vector<req_t *> &req_list) {

	int total_time = 0;
	int total_movement = 0;
	int total_turnaround = 0;
	int total_waittime = 0;
	int max_waittime = 0;

	int time = 0;
	head = 0;
	int op = 0;				// I/O op#
	req_t *r = nullptr;		// active I/O

	while(1) {
		// a new I/O arrive to the system at this time: add to I/O queue
		if(op < req_list.size() && req_list[op]->arrival_time == time) {
			iosched->add_req(req_list[op]);
			op++;
		}
		// an I/O active
		if(r) {
			// I/O completed at this time: compute and store info
			if(r->track == head) {
				r->end_time = time;
				total_turnaround += r->end_time - r->arrival_time;
				total_waittime += r->start_time - r->arrival_time;
				max_waittime = max(max_waittime, r->start_time - r->arrival_time);
				r = nullptr;
				if(iosched->empty() && op == req_list.size()) {
					break;
				}
			}
			// I/O not completed: move the head by one track in the direction it is going
			else {
				head += iosched->direction;
			}

		}
		// no I/O active after an I/O completed: fetch and start a new I/O
		if(!r) {
			r = iosched->fetch_req();
			if(r) {
				r->start_time = time;
				total_movement += abs(head - r->track);
				continue;
			}
		}
		// increment time by 1
		time++;
	}

	for(int i = 0; i < req_list.size(); i++) {
		r = req_list[i];
		printf("%5d: %5d %5d %5d\n", i, r->arrival_time, r->start_time, r->end_time);
	}

	printf("SUM: %d %d %.2lf %.2lf %d\n", time, total_movement, 1.0 * total_turnaround / req_list.size(), 1.0 * total_waittime / req_list.size(), max_waittime);

}

int main(int argc, char *argv[]) {

	// default settings
	string algo = "i";
	bool opt_v = false;
	bool opt_q = false;
	bool opt_f = false;

	// read command
	int c;
	while(1) {
		c = getopt(argc, argv, "s:vqf");
		if(c == -1)
			break;
		switch(c) {
			case 's':
				algo = optarg;
				break;
			case 'v':
				opt_v = true;
				break;
			case 'q':
				opt_q = true;
				break;
			case 'f':
				opt_f = true;
				break;
			default:
				break;
		}
	}
	char *inputfile = argv[optind];

	iosched_t *iosched = nullptr;
	switch(algo[0]) {
		case 'i':
			iosched = new FIFO();
			break;
		case 'j':
			iosched = new SSTF();
			break;
		case 's':
			iosched = new LOOK();
			break;
		case 'c':
			iosched = new CLOOK();
			break;
		case 'f':
			iosched = new FLOOK();
			break;
		default:
			iosched = new FIFO();
			break;
	}

	vector<req_t *> req_list;

	// read input file
	ifstream input;
	input.open(inputfile);
	if(!input.is_open()) {
		exit(0);
	}
	while(!input.eof()) {
		string line;
		getline(input, line);
		if(line[0] == '#' || line.length() == 0) {
			continue;
		}
		vector<string> spec = split(line, " ");
		req_t *temp_req = new req_t(stoi(spec[0]), stoi(spec[1]));
		req_list.push_back(temp_req);
	}

	simulation(iosched, req_list);

	return 0;

}
