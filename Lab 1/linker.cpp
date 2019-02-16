#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <regex>
#include <vector>

using namespace std;

ifstream file;

int linenum;
int lineoffset;
int modulenum;
int baseaddress;
int totalinstrnum;

string lineStr;
char *line;
char *remain;

struct _symbol {
	string name;
	int module;
	int base;
	int offset;
	int address;
	int errcode;
	bool used;
};

vector<_symbol> symbolTable;

void _parseError(int linenum, int lineoffset, int errcode) {
	static char* errstr[] = {
		"NUM_EXPECTED",					// Number expected
		"SYM_EXPECTED",					// Symbol Expected
		"ADDR_EXPECTED",				// Addressing expected which is A/E/I/R
		"SYM_TOO_LONG",					// Symbol Name is too long
		"TOO_MANY_DEF_IN_MODULE",		// > 16
		"TOO_MANY_USE_IN_MODULE",		// > 16
		"TOO_MANY_INSTR",				// Total num_instr exceeds memory size (512)
	};
	printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset, errstr[errcode]);
}

void _errorMsg(int errcode, string name) {
	if(errcode == 3 && name.length() != 0) {
		cout << "Error: " << name << " is not defined; zero used\n";				// rule 3
	}
	else{
		static char* errstr[] = {
			" Error: Absolute address exceeds machine size; zero used",					// rule 8
			" Error: Relative address exceeds module size; zero used",					// rule 9
			" Error: External address exceeds length of uselist; treated as immediate",	// rule 6
			"",
			" Error: This variable is multiple times defined; first value used",		// rule 2
			" Error: Illegal immediate value; treated as 9999",							// rule 10
			" Error: Illegal opcode; treated as 9999",									// rule 11
		};
		printf("%s\n", errstr[errcode]);
	}
}

void _warnMsg(int errcode, int module, string name, int offset, int size) {
	switch(errcode) {
		case 0:																			// rule 5
			cout << "Warning: Module " << module << ": " << name << " too big " << offset << " (max=" << size << ") assume zero relative\n";
			break;
		case 1:																			// rule 7
			cout << "Warning: Module " << module << ": " << name << " appeared in the uselist but was not actually used\n";
			break;
		case 2:																			// rule 4
			cout << "Warning: Module " << module << ": " << name << " was defined but never used\n";
			break;
		default:
			break;
	}
}

char *getToken() {
	if(remain == NULL) {
		if(!file.eof()) {
			string lineStrOld = lineStr;
			getline(file, lineStr);
			if(file.eof()) {
				lineoffset = lineStrOld.length() + 1;
				return NULL;
			}
			delete[] line;
			line = new char[lineStr.length() + 1];
			strcpy(line, lineStr.c_str());
			linenum++;
			remain = strtok(line, " \t\n");
			if(remain == NULL) {
				lineoffset = lineStr.length() + 1;
				return getToken();
			}
			else {
				lineoffset = remain - line + 1;
				// printf("%s\n", remain);
				return remain;
			}
		}
		else {
			//cout << "case 4" << endl;
			//
			lineoffset = lineStr.length() + 1;
			return NULL;
		}
	}
	else {
		remain = strtok(NULL, " \t\n");
		if(remain == NULL) {
			return getToken();
		}
		else {
			lineoffset = remain - line + 1;
			//printf("%s\n", remain);
			return remain;
		}
	}
}

int readInt(bool isdefcount) {
	int linenumOld = linenum;
	char *intChar = getToken();
	if(intChar) {
		string intStr = intChar;
		regex intRe("[0-9]+");
		if(regex_match(intStr.begin(), intStr.end(), intRe)) {
			return atoi(intChar);
		}
		else {
			_parseError(linenum, lineoffset, 0);			// not an int
			exit(0);
		}
	}
	else {
		if(isdefcount) {									// special handling for defcount
			return -1;
		}
		else {
			_parseError(linenum, lineoffset, 0);
			exit(0);
		}
	}
}

_symbol readSymbol() {
	int linenumOld = linenum;
	char *symbolChar = getToken();
	if(symbolChar) {
		string symbolStr = symbolChar;
		regex symbolRe("[a-zA-Z][a-zA-Z0-9]*");
		if(regex_match(symbolStr.begin(), symbolStr.end(), symbolRe)) {
			if(symbolStr.length() > 16) {
				_parseError(linenum, lineoffset, 3);		// symbol too long
				exit(0);
			}
			else {
				_symbol symbol;
				symbol.name = symbolStr;
				symbol.module = modulenum;
				symbol.errcode = -1;
				symbol.used = false;
				return symbol;
			}
		}
		else {
			_parseError(linenum, lineoffset, 1);			// not a symbol name
			exit(0);
		}
	}
	else {
		_parseError(linenum, lineoffset, 1);
		exit(0);
	}
}

string readIEAR() {
	int linenumOld = linenum;
	char *IEARChar = getToken();
	if(IEARChar) {
		string IEARStr = IEARChar;
		if(IEARStr == "I" || IEARStr == "E" || IEARStr == "A" || IEARStr == "R") {
			return IEARStr;
		}
		else {
			_parseError(linenum, lineoffset, 2);
			exit(0);
		}
	}
	else {
		_parseError(linenum, lineoffset, 2);
		exit(0);
	}
}

void createSymbol(_symbol sym, int val) {
	/* RULE 2 */
	for(vector<_symbol>::iterator iter = symbolTable.begin(); iter != symbolTable.end(); iter++) {
		if((*iter).name == sym.name) {
			(*iter).errcode = 4;
			sym.errcode = 4;
			break;
		}
	}
	if(sym.errcode != 4) {
		sym.base = baseaddress;
		sym.offset = val;
		sym.address = baseaddress + val;
		symbolTable.push_back(sym);
	}
}

void printSymbolTable() {
	cout << "Symbol Table\n";
	for(vector<_symbol>::iterator iter = symbolTable.begin(); iter != symbolTable.end(); iter++) {
		cout << (*iter).name << "=" << (*iter).address;
		if((*iter).errcode == 4) {
			_errorMsg((*iter).errcode, "");
		}
		else {
			cout << "\n";
		}
	}
	cout << endl;
}

void pass1(char *path) {
	file.clear();
	file.open(path);
	if(!file.is_open()){
		cout << "Error: File cannot be opened" << endl;
		exit(0);
	}

	linenum = 0;
	lineoffset = 0;
	modulenum = 0;
	baseaddress = 0;
	totalinstrnum = 0;

	line = NULL;
	remain = NULL;
		
	while(!file.eof()) {
		//createModule();
		modulenum++;

		/* DEFINITION LIST */
		int defcount = readInt(true);
		if(defcount == -1) {
			if(file.eof()) {
				break;									// nothing to read
			}
			else {
				_parseError(linenum, lineoffset, 0);
				exit(0);
			}
		}
		if(defcount > 16) {
			_parseError(linenum, lineoffset, 4);		// too many defs
			exit(0);
		}
		for(int i = 0; i < defcount; i++) {
			_symbol sym = readSymbol();
			int val = readInt(false);
			createSymbol(sym, val);
		}

		/* USE LIST */
		int usecount = readInt(false);
		if(usecount > 16) {
			_parseError(linenum, lineoffset, 5);		// too many uses
			exit(0);
		}
		for(int i = 0; i < usecount; i++) {
			readSymbol();
		}

		/* PROGRAM LIST */
		int codecount = readInt(false);
		totalinstrnum += codecount;
		if(totalinstrnum > 512) {
			_parseError(linenum, lineoffset, 6);
			exit(0);
		}
		for(int i = 0; i < codecount; i++) {
			readIEAR();
			readInt(false);
		}

		/* RULE 5 */
		for(vector<_symbol>::iterator iter = symbolTable.begin(); iter != symbolTable.end(); iter++) {
			if((*iter).module == modulenum && (*iter).offset > codecount) {
				_warnMsg(0, modulenum, (*iter).name, (*iter).offset, codecount - 1);
				(*iter).offset = 0;
				(*iter).address = (*iter).base;
			}
		}

		baseaddress += codecount;
	}

	printSymbolTable();

	file.close();
}

void pass2(char *path) {
	file.clear();
	file.open(path);
	if(!file.is_open()){
		cout << "Error: File cannot be opened" << endl;
		exit(0);
	}

	linenum = 0;
	lineoffset = 0;
	modulenum = 0;
	baseaddress = 0;
	totalinstrnum = 0;

	line = NULL;
	remain = NULL;
		
	while(!file.eof()) {
		//createModule();
		modulenum++;

		/* DEFINITION LIST */
		int defcount = readInt(true);
		if(defcount == -1) {
			if(file.eof()) {
				break;									// nothing to read
			}
		}
		for(int i = 0; i < defcount; i++) {
			readSymbol();
			readInt(false);
		}

		/* USE LIST */
		int usecount = readInt(false);
		for(int i = 0; i < usecount; i++) {
			_symbol sym = readSymbol();
		}

		/* PROGRAM LIST */
		int codecount = readInt(false);
		totalinstrnum += codecount;
		for(int i = 0; i < codecount; i++) {
			string addressmode = readIEAR();
			int operand = readInt(false);
		}

		baseaddress += codecount;
	}

	file.close();
}

int main(int argc, char **argv) {
	pass1(argv[1]);
	return 0;
}
