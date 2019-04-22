/* Lab 3 - OS Spring 2019		*/
/* Xinyi Liu (xl2700)			*/

#define MAX_PTE 64

#define MAP_COST 400
#define PAGE_COST 3000
#define FILE_COST 2500
#define ZERO_COST 150
#define SEGV_COST 240
#define SEGPROT_COST 300
#define CTX_EXTRA 120
#define EXIT_EXTRA 174

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
		int num = randvals[ofs] % burst;
		ofs++;
		return num;
	};

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

struct vma_t {
	int start_vpage;
	int end_vapge;
	int write_protected;
	int file_mapped;

	vma_t(int sv, int ev, int wp, int fm) {
		start_vpage = sv;
		end_vapge = ev;
		write_protected = wp;
		file_mapped = fm;
	}
};

struct pte_t {
	// not used
	unsigned int NOT_USED:19;
	// optionally added
	unsigned int FILE_MAPPED:1;
	// must have
	unsigned int VALID:1;
	unsigned int WRITE_PROTECTED:1;
	unsigned int MODIFIED:1;
	unsigned int REFERENCED:1;
	unsigned int PAGEDOUT:1;
	unsigned int FRAME_INDEX:7;

	pte_t() {
		NOT_USED = 0;
		FILE_MAPPED = 0;
		VALID = 0;
		WRITE_PROTECTED = 0;
		MODIFIED = 0;
		REFERENCED = 0;
		PAGEDOUT = 0;
		FRAME_INDEX = 0;
	}
};


struct pstats_t {
	unsigned long unmaps;
	unsigned long maps;
	unsigned long ins;
	unsigned long outs;
	unsigned long fins;
	unsigned long fouts;
	unsigned long zeros;
	unsigned long segv;
	unsigned long segprot;

	pstats_t() {
		unmaps = 0;
		maps = 0;
		ins = 0;
		outs = 0;
		fins = 0;
		fouts = 0;
		zeros = 0;
		segv = 0;
		segprot = 0;
	}
};

struct process_t {
	int pid;
	vector<vma_t*> vmas;
	pte_t* page_table[MAX_PTE];
	pstats_t pstats;

	process_t(int proc_id) {
		pid = proc_id;
		for(int i = 0; i < MAX_PTE; i++) {
			pte_t* temp_pte = new pte_t;
			page_table[i] = temp_pte;
		}
	}
};

vector<process_t*> process_list;

struct instr_t {
	string op;
	int vpage;
};

int curr_instr = 0;

struct frame_t {
	int frame_index;
	int pid;
	int vpage;
	int mapped;
	unsigned int age;
	int tau;

	frame_t(int i) {
		frame_index = i;
		pid = 0;
		vpage = 0;
		mapped = 0;
		age = 0;
		tau = 0;
	}
};

class pager_t {
public:
	int frame_num;
	int hand;
	vector<frame_t*> frame_table;		// equivalent to global
	list<frame_t*> free_list;

	frame_t* allocate_frame() {
		if(free_list.empty()) {
			return nullptr;
		}
		else {
			frame_t* f = free_list.front();
			free_list.pop_front();
			return f;
		}
	};

	virtual frame_t* select_victim_frame() = 0;

	frame_t* get_frame() {
		frame_t *f = allocate_frame();
		if(f == nullptr) {
			f = select_victim_frame();
		}
		return f;
	};

	void print() {
		cout << "FT:";
		for(int i = 0; i < frame_table.size(); i++) {
			if(frame_table[i]->mapped) {
				cout << " " << frame_table[i]->pid << ":" << frame_table[i]->vpage;
			}
			else {
				cout << " *";
			}
		}
		cout << "\n";
	}

	//constructor
	pager_t() {};
	pager_t(int fn) {
		frame_num = fn;
		for(int i = 0; i < frame_num; i++) {
			frame_t* temp_frame = new frame_t(i);
			frame_table.push_back(temp_frame);
			free_list.push_back(temp_frame);
		}
		hand = 0;
	};

	//destructor
	~pager_t() {};
};

class FIFO : public pager_t {
public:
	FIFO(int fn) : pager_t(fn) {};

	frame_t* select_victim_frame() {
		frame_t* f = frame_table[hand % frame_num];
		hand++;
		return f;
	};
};

class Random : public pager_t {
private:
	myrandom* r;

public:
	Random(int fn, char* randfile) : pager_t(fn) {
		r = new myrandom(randfile);
	};

	frame_t* select_victim_frame() {
		int rand = r->get_random(frame_num);
		frame_t* f = frame_table[rand];
		return f;
	};
};

class Clock : public pager_t {
public:
	Clock(int fn) : pager_t(fn) {};

	frame_t* select_victim_frame() {
		while(1) {
			frame_t* f = frame_table[hand % frame_num];
			pte_t* pte = process_list[f->pid]->page_table[f->vpage];
			hand++;
			if(pte->REFERENCED) {
				pte->REFERENCED = 0;
			}
			else {
				return f;
			}
		}
	};
};

class NRU : public pager_t {
private:
	int last_instr;

public:
	NRU(int fn) : pager_t(fn) {
		last_instr = -1;
	};

	frame_t* select_victim_frame() {
		int NRU_class[4];
		NRU_class[0] = -1;
		NRU_class[1] = -1;
		NRU_class[2] = -1;
		NRU_class[3] = -1;
		int victim_index = hand % frame_num;
		bool reset = curr_instr - last_instr >= 50? true : false;
		for(int i = 0; i < frame_num; i++) {
			frame_t* f = frame_table[hand % frame_num];
			hand++;
			pte_t* pte = process_list[f->pid]->page_table[f->vpage];
			int class_index = pte->REFERENCED * 2 + pte->MODIFIED;
			if(NRU_class[class_index] == -1) {
				NRU_class[class_index] = f->frame_index;
				if(class_index == 0) {
					if(reset) {
						victim_index = f->frame_index;
						break;
					}
					else {
						return f;
					}
				}
			}
		}
		for(int i = 0; i < 4; i++) {
			if(NRU_class[i] != -1) {
				victim_index = NRU_class[i];
				break;
			}
		}
		if(reset) {
			for(int i = 0; i < frame_num; i++) {
				process_list[frame_table[i]->pid]->page_table[frame_table[i]->vpage]->REFERENCED = 0;
			}
			last_instr = curr_instr;
		}
		hand = (victim_index + 1) % frame_num;
		return frame_table[victim_index];
	};
};

class Aging : public pager_t {
public:
	Aging(int fn) : pager_t(fn) {};

	frame_t* select_victim_frame() {
		int victim_index = hand % frame_num;
		for(int i = 0; i < frame_num; i++) {
			frame_t* f = frame_table[hand % frame_num];
			pte_t* pte = process_list[f->pid]->page_table[f->vpage];
			f->age = (f->age >> 1) + (pte->REFERENCED << 31);
			pte->REFERENCED = 0;
			if(f->age < frame_table[victim_index]->age) {
				victim_index = hand % frame_num;
			}
			hand++;
		}
		hand = (victim_index + 1) % frame_num;
		return frame_table[victim_index];
	};
};

class WorkingSet : public pager_t {
public:
	WorkingSet(int fn) : pager_t(fn) {};

	frame_t* select_victim_frame() {
		int victim_index = hand % frame_num;
		int age = -1;
		for(int i = 0; i < frame_num; i++) {
			frame_t* f = frame_table[hand % frame_num];
			pte_t* pte = process_list[f->pid]->page_table[f->vpage];
			if(pte->REFERENCED) {
				f->tau = curr_instr;
				pte->REFERENCED = 0;
			}
			else {
				if (curr_instr - f->tau >= 50) {
					hand++;
					return f;
				}
				else if(curr_instr - f->tau > age) {
					victim_index = hand % frame_num;
					age = curr_instr - f->tau;
				}
			}
			hand++;
		}
		hand = (victim_index + 1) % frame_num;
		return frame_table[victim_index];
	};
};

void print_process_list(vector<process_t*> &process_list) {
	for(int i = 0; i < process_list.size(); i++) {
		cout << "process " << process_list[i]->pid << "\n";
		cout << process_list[i]->vmas.size() << "\n";
		for(int j = 0; j < process_list[i]->vmas.size(); j++) {
			cout << process_list[i]->vmas[j]->start_vpage << " " << process_list[i]->vmas[j]->end_vapge << " " << process_list[i]->vmas[j]->write_protected << " " << process_list[i]->vmas[j]->file_mapped << "\n";
		}
	}
}

void print_instr_list(vector<instr_t> &instr_list) {
	for(int i = 0; i < instr_list.size(); i++) {
		cout << instr_list[i].op << " " << instr_list[i].vpage << "\n";
	}
}

void print_page_table(int pid, pte_t** page_table) {
	cout << "PT[" << pid << "]:";
	for(int i = 0; i < MAX_PTE; i++) {
		//cout << page_table[i]->VALID << " " << page_table[i]->WRITE_PROTECTED << " " << page_table[i]->MODIFIED << " " << page_table[i]->REFERENCED << " " << page_table[i]->PAGEDOUT << " " << page_table[i]->FRAME_INDEX << "\n";
		if(page_table[i]->VALID) {
			string ref = page_table[i]->REFERENCED? "R" : "-";
			string mod = page_table[i]->MODIFIED? "M" : "-";
			string pag = page_table[i]->PAGEDOUT? "S" : "-";
			cout << " " << i << ":" << ref << mod << pag;
		}
		else {
			cout << " " << (page_table[i]->PAGEDOUT? "#" : "*");
		}
	}
	cout << "\n";
}

void simulation(pager_t* pager, vector<process_t*> &process_list, vector<instr_t> &instr_list, bool opt_O, bool opt_P, bool opt_F, bool opt_S, bool opt_x) {

	unsigned long ctx_switches = 0;
	unsigned long process_exits = 0;

	process_t* curr_process = nullptr;

	for(curr_instr = 0; curr_instr < instr_list.size(); curr_instr++) {
		string op = instr_list[curr_instr].op;
		int vpage = instr_list[curr_instr].vpage;

		if(opt_O) {
			cout << curr_instr << ": ==> " << op << " " << vpage << "\n";
		}

		// handle special case of "c" instruction
		if(op == "c") {
			ctx_switches++;
			curr_process = process_list[vpage];
		}
		// handle special case of "e" instruction
		else if(op == "e") {
			if(opt_O) {
				cout << "EXIT current process " << curr_process->pid << "\n";
			}
			for(int i = 0; i < MAX_PTE; i++) {
				if(curr_process->page_table[i]->VALID) {
					if(opt_O) {
						cout << " UNMAP " << curr_process->pid << ":" << i << "\n";
					}
					curr_process->pstats.unmaps++;
					if(curr_process->page_table[i]->MODIFIED && curr_process->page_table[i]->FILE_MAPPED) {
						if(opt_O) {
							cout << " FOUT\n";
						}
						curr_process->pstats.fouts++;
					}
					int frame_index = curr_process->page_table[i]->FRAME_INDEX;
					pager->frame_table[frame_index]->pid = 0;
					pager->frame_table[frame_index]->vpage = 0;
					pager->frame_table[frame_index]->mapped = 0;
					pager->frame_table[frame_index]->age = 0;
					pager->frame_table[frame_index]->tau = 0;
					pager->free_list.push_back(pager->frame_table[frame_index]);
				}
				curr_process->page_table[i]->FILE_MAPPED = 0;
				curr_process->page_table[i]->VALID = 0;
				curr_process->page_table[i]->WRITE_PROTECTED = 0;
				curr_process->page_table[i]->MODIFIED = 0;
				curr_process->page_table[i]->REFERENCED = 0;
				curr_process->page_table[i]->PAGEDOUT = 0;
				curr_process->page_table[i]->FRAME_INDEX = 0;
			}
			process_exits++;
			curr_process = nullptr;
		}
		// now the real instructions for read and write
		else {
			pte_t *pte = curr_process->page_table[vpage];	// in reality this is done by hardware
			if(!pte->VALID) {
				// this in reality will generate the page fault exception and now you execute

				// handle SEGV
				bool SEGV = true;
				for(int i = 0; i < curr_process->vmas.size(); i++) {
					if(curr_process->vmas[i]->start_vpage <= vpage && vpage <= curr_process->vmas[i]->end_vapge) {
						SEGV = false;
						pte->WRITE_PROTECTED = curr_process->vmas[i]->write_protected;
						pte->FILE_MAPPED = curr_process->vmas[i]->file_mapped;
						break;
					}
				}
				if(SEGV) {
					if(opt_O) {
						cout << " SEGV\n";
					}
					curr_process->pstats.segv++;
					continue;
				}

				frame_t* new_frame = pager->get_frame();

				// figure out if/what to do with old frame if it was mapped: UNMAP, OUT, FOUT
				if(new_frame->mapped) {
					if(opt_O) {
						cout << " UNMAP " << new_frame->pid << ":" << new_frame->vpage << "\n";
					}
					process_list[new_frame->pid]->page_table[new_frame->vpage]->VALID = 0;
					process_list[new_frame->pid]->page_table[new_frame->vpage]->REFERENCED = 0;
					process_list[new_frame->pid]->pstats.unmaps++;
					if(process_list[new_frame->pid]->page_table[new_frame->vpage]->MODIFIED) {
						if(process_list[new_frame->pid]->page_table[new_frame->vpage]->FILE_MAPPED) {
							if(opt_O) {
								cout << " FOUT\n";
							}
							process_list[new_frame->pid]->pstats.fouts++;
						}
						else {
							if(opt_O) {
								cout << " OUT\n";
							}
							process_list[new_frame->pid]->page_table[new_frame->vpage]->PAGEDOUT = 1;
							process_list[new_frame->pid]->pstats.outs++;
						}
						process_list[new_frame->pid]->page_table[new_frame->vpage]->MODIFIED = 0;
					}
				}

				// IN, FIN, ZERO
				if(pte->FILE_MAPPED) {
					if(opt_O) {
						cout << " FIN\n";
					}
					curr_process->pstats.fins++;
				}
				else if(pte->PAGEDOUT) {
					if(opt_O) {
						cout << " IN\n";
					}
					curr_process->pstats.ins++;
				}
				else {
					if(opt_O) {
						cout << " ZERO\n";
					}
					curr_process->pstats.zeros++;
				}

				if(opt_O) {
					cout << " MAP " << new_frame->frame_index << "\n";
				}
				new_frame->pid = curr_process->pid;
				new_frame->vpage = vpage;
				new_frame->mapped = 1;
				new_frame->age = 0;
				new_frame->tau = curr_instr;
				curr_process->pstats.maps++;

				pte->VALID = 1;
				pte->FRAME_INDEX = new_frame->frame_index;
			}

			pte->REFERENCED = 1;

			if(op == "w") {
				if(pte->WRITE_PROTECTED) {
					if(opt_O) {
						cout << " SEGPROT\n";
					}
					curr_process->pstats.segprot++;
				}
				else {
					pte->MODIFIED = 1;
				}
			}

			if(opt_x) {
				print_page_table(curr_process->pid, curr_process->page_table);
			}
		}
	}

	// PT
	if(opt_P) {
		for(int i = 0; i < process_list.size(); i++) {
			print_page_table(process_list[i]->pid, process_list[i]->page_table);
		}
	}

	// FT
	if(opt_F) {
		pager->print();
	}

	
	if(opt_S) {
		unsigned long long cost = 0;
		// PROC
		for(int i = 0; i < process_list.size(); i++) {
			pstats_t pstats = process_list[i]->pstats;
			printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
				process_list[i]->pid,
				pstats.unmaps,
				pstats.maps,
				pstats.ins,
				pstats.outs,
				pstats.fins,
				pstats.fouts,
				pstats.zeros,
				pstats.segv,
				pstats.segprot);
			cost += (pstats.unmaps + pstats.maps) * MAP_COST +
				(pstats.ins + pstats.outs) * PAGE_COST +
				(pstats.fins + pstats.fouts) * FILE_COST +
				pstats.zeros * ZERO_COST +
				pstats.segv * SEGV_COST +
				pstats.segprot * SEGPROT_COST;
		}
		// TOTALCOST
		cost += instr_list.size() + ctx_switches * CTX_EXTRA + process_exits * EXIT_EXTRA;
		printf("TOTALCOST %lu %lu %lu %llu\n", instr_list.size(), ctx_switches, process_exits, cost);
	}
	
}

int main(int argc, char *argv[]) {

	// default settings
	string algo = "f";
	string options = "OPFS";
	int frame_num = 16;

	// read command
	int c;
	while(1) {
		c = getopt(argc, argv, "a:o:f:");
		if(c == -1)
			break;
		switch(c) {
			case 'a':
				algo = optarg;
				break;
			case 'o':
				options = optarg;
				break;
			case 'f':
				frame_num = atoi(optarg);
				break;
			default:
				break;
		}
	}
	char *inputfile = argv[optind];
	char *randfile = argv[optind + 1];

	// algo
	pager_t* pager = nullptr;
	switch(algo[0]) {
		case 'f':
			pager = new FIFO(frame_num);
			break;
		case 'r':
			pager = new Random(frame_num, randfile);
			break;
		case 'c':
			pager = new Clock(frame_num);
			break;
		case 'e':
			pager = new NRU(frame_num);
			break;
		case 'a':
			pager = new Aging(frame_num);
			break;
		case 'w':
			pager = new WorkingSet(frame_num);
			break;
		default:
			pager = new FIFO(frame_num);
			break;
	}

	// options
	bool opt_O = false;
	bool opt_P = false;
	bool opt_F = false;
	bool opt_S = false;
	bool opt_x = false;
	bool opt_y = false;
	bool opt_f = false;
	bool opt_a = false;
	if(options.find("O") != string::npos) opt_O = true;
	if(options.find("P") != string::npos) opt_P = true;
	if(options.find("F") != string::npos) opt_F = true;
	if(options.find("S") != string::npos) opt_S = true;
	if(options.find("x") != string::npos) opt_x = true;
	if(options.find("y") != string::npos) opt_y = true;
	if(options.find("f") != string::npos) opt_f = true;
	if(options.find("a") != string::npos) opt_a = true;

	// read input file
	ifstream input;
	input.open(inputfile);
	if(!input.is_open()) {
		exit(0);
	}

	string line = "";
	while(!input.eof()) {
		getline(input, line);
		if(line[0] != '#' && line.length() > 0) {
			break;
		}
	}

	//vector<process_t*> process_list;

	int proc_num = stoi(line);
	for(int i = 0; i < proc_num; i++) {
		process_t* temp_process = new process_t(i);
		process_list.push_back(temp_process);
		while(1) {
			getline(input, line);
			if(line[0] != '#' && line.length() > 0) {
				break;
			}
		}
		int vma_num = stoi(line);
		for(int j = 0; j < vma_num; j++) {
			while(1) {
				getline(input, line);
				if(line[0] != '#' && line.length() > 0) {
					break;
				}
			}
			vector<string> spec = split(line, " ");
			vma_t* temp_vma = new vma_t(stoi(spec[0]), stoi(spec[1]), stoi(spec[2]), stoi(spec[3]));
			temp_process->vmas.push_back(temp_vma);
		}
	}

	vector<instr_t> instr_list;

	while(!input.eof()) {
		getline(input, line);
		if(line[0] == '#' || line.length() == 0) {
			continue;
		}
		vector<string> spec = split(line, " ");
		instr_t temp_instr = {spec[0], stoi(spec[1])};
		instr_list.push_back(temp_instr);
	}

	simulation(pager, process_list, instr_list, opt_O, opt_P, opt_F, opt_S, opt_x);

	// for input tests
	//print_process_list(process_list);
	//print_instr_list(instr_list);
	//print_page_table(process_list[0]->page_table);

	return 0;
}
