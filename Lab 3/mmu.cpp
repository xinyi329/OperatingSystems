/* Lab 3 - OS Spring 2019		*/
/* Xinyi Liu (xl2700)			*/

#define MAX_PTE 64

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

struct pte_t {
	// must have
	unsigned int VALID:1;
	unsigned int WRITE_PROTECT:1;
	unsigned int MODIFIED:1;
	unsigned int REFERENCED:1;
	unsigned int PAGEDOUT:1;
	unsigned int FRAME_INDEX:7;
};

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

struct pstats_t {
	unsigned long unmaps;
	unsigned long maps;
	unsigned long ins;
	unsigned long outs;
	unsigned long fins;
	unsigned long fouts;
	unsigned long zero;
	unsigned long segv;
	unsigned long segprot;

	pstats_t() {
		unmaps = 0;
		maps = 0;
		ins = 0;
		outs = 0;
		fins = 0;
		fouts = 0;
		zero = 0;
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
			*temp_pte = {0};
			page_table[i] = temp_pte;
		}
	}
};

struct instr_t {
	string op;
	int num;
};

struct frame_t {
	int pid;
	int vpage;
	int frame_index;
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
		cout << instr_list[i].op << " " << instr_list[i].num << "\n";
	}
}

int main(int argc, char *argv[]) {

	// read random file

	//char *randfile = argv[optind + 1];
	//myrandom r(randfile);

	// read input file

	//char *inputfile = argv[optind];
	char *inputfile = argv[1];

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

	vector<process_t*> process_list;

	cout << line << endl;
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

	print_process_list(process_list);
	print_instr_list(instr_list);

	return 0;
}
