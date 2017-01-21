#include <iostream>
#include <deque>
#include <map>
#include <fstream>

using namespace std;

struct IN {
	string opcode;
	int dest,source1,source2;
};
deque<IN> INM; //instruction memory
map <int,int> RGF;
deque <pair <int,int> > REB; //can hold 2 tokens: ADD and MUL
deque <pair <int,int> > ADB;
map <int,int> DAM;

IN InBuffer; // INB
IN AIB; //arithmetic inst buffer
IN SIB; //store inst buffer
IN PRB; //partial result buffer for MUL instructions

int addr_flag = 0,store_flag = 0,write_flag = 0, decode_flag = 0, asu_flag = 0, mlu1_flag = 0, mlu2_flag = 0, issue_flag = 0;

//instructions can be a vector of strings.

void readInstructions() {
	ifstream infile("instructions.txt");
	string line, token; 
	//if OPCODE is STRING, source2 is immediate offset.
	string delimiter = ",";
	size_t pos = 0;
	while (getline(infile,line)) {
		string temp ={};
		IN in;
		int i=0;
		while ((pos = line.find(delimiter)) != string::npos) {
			token = line.substr(1,pos-1);
			switch(i) {
				case 0:
					temp = token; 
					//cout<<token<<endl; break;
					in.opcode = token; break;
				case 1: 
					//cout<<token<<endl; break;
					in.dest = stoi(token); break; 
				case 2: 
					//cout<<token<<endl; break;
					in.source1 = stoi(token); break; 
				default: 
					cout<<"Error in input format!\n"; exit(-1);
			}
			line.erase(0,pos + delimiter.length());
			i++;
		}
		if (temp == "ST")
			in.source2 = stoi(line.substr(0,line.length()-1));
			//cout<<line.substr(0,line.length()-1)<<endl;
		else
			//cout<<line.substr(1,line.length()-2)<<endl;
		in.source2 = stoi(line.substr(1,line.length()-2));
		INM.push_back(in);
	}
}
//registers, data memory must be an ordered map.
void readRegisters() {
	ifstream infile("registers.txt");
	string line;
	int key, val;
	string delimiter = ",";
	size_t pos = 0;
	while (getline(infile,line)) {
		while ((pos = line.find(delimiter)) != string::npos) {
			key = stoi(line.substr(2,pos-2));
			line.erase(0,pos + delimiter.length());
			val = stoi(line.substr(0,line.length()-1));
			RGF[key] = val;
		}
	}	
}

void readDataMemory() {
	ifstream infile("datamemory.txt");
	string line,token;
	int key, val;
	string delimiter = ",";
	size_t pos = 0;
	while (getline(infile,line)) {
		while ((pos = line.find(delimiter)) != string::npos) {
			key =stoi(line.substr(1,pos-1));
			line.erase(0,pos + delimiter.length());
			val = stoi(line.substr(0,line.length()-1));
			DAM[key] = val;
		}
	}	
}

void Read(int &srcA, int &srcB) {
	if (RGF.count(srcA) && RGF.count(srcB)) {
		srcA = RGF[srcA];
		srcB = RGF[srcB];
	}
	else {
		cout<<"Error, source register values not available for Arithmetic.\n"<<endl;
		exit(-1);
	}
}
void Read(int &srcA) {
	if (RGF.count(srcA))
		srcA = RGF[srcA];
	else {
		cout<<"Error, source reg value not available for ST.\n"<<endl;
		exit(-1);
	}
}
//Decode: How to test whether any source reg is 'available'
void Decode(deque<IN> &INM) {
	if(!INM.empty()) {
	IN inToken = INM.front();
	if (inToken.opcode == "ST") Read(inToken.source1);
	else if(inToken.opcode == "ADD" || "SUB" || "MUL") 
		Read(inToken.source1, inToken.source2);
	else {
		cout<<"Invalid opcode! Exiting...\n"<<endl;
		exit(-1);
		}
	INM.pop_front();
	InBuffer = inToken;
	decode_flag = 0;
	}
	else decode_flag = 1;
}
//if ST or Arithmetic operation should be asked in Main.
void Issue1() {
	if (!InBuffer.opcode.empty()) {
	AIB = InBuffer;
	InBuffer = {};
	issue_flag = 0;
	}
	else issue_flag = 1;
}
void Issue2() {
	if (!InBuffer.opcode.empty()) {
	SIB = InBuffer;
	InBuffer = {};
	issue_flag = 0;
	}
	else issue_flag = 1;
}
void ASU() {
	if (!AIB.opcode.empty()) {
		if (AIB.opcode == "ADD") {
			REB.push_back(make_pair(AIB.dest,AIB.source1+AIB.source2));
			AIB = {};
		}
		else if (AIB.opcode == "SUB") {
			REB.push_back(make_pair(AIB.dest,AIB.source1-AIB.source2));
			AIB = {};
		}
		asu_flag = 0;
	}
	else asu_flag = 1;
}

void MLU1() {
	if (!AIB.opcode.empty() && AIB.opcode == "MUL") {
		PRB = AIB;
		AIB = {};
		mlu1_flag = 0;
	}
	else mlu1_flag = 1;
}
void MLU2() {
	if (!PRB.opcode.empty()) {
		REB.push_back(make_pair(PRB.dest,PRB.source1*PRB.source2));
		PRB = {};
		mlu2_flag = 0;
	}
	else mlu2_flag = 1;
}
void Write() {
	if (!REB.empty()) {
		RGF[REB.front().first] = REB.front().second;
		REB.pop_front();
		write_flag = 0;
	}
	else write_flag = 1;
}
void ADDR() {
	//ADB = SIB;
	if (SIB.opcode == "ST") {
		ADB.push_back(make_pair(SIB.dest,SIB.source1+SIB.source2));
		// ADB.first = SIB.dest;
		// ADB.second = SIB.source1 + SIB.source2;
		SIB = {};
		addr_flag = 0;	
	}
	else addr_flag = 1;
}
void Store() {
		if (!ADB.empty()) {
		//ADB: <R1,10>
		DAM[ADB.front().second] = RGF[ADB.front().first];
		ADB.pop_front();
		store_flag = 0;
	}
	else store_flag = 1;
}
void printINM() {
	cout<<"INM:";
	for (auto it = INM.cbegin(); it != INM.cend(); ++it) {
		if (it->opcode == "ST") {
	 		cout<<"<"<<it->opcode<<",R"<<it->dest<<",R"<<it->source1<<","
				<<it->source2<<">";
			if (next(it) != INM.cend()) cout<<",";
		}
		else {
			cout<<"<"<<it->opcode<<",R"<<it->dest<<",R"<<it->source1<<",R"
				<<it->source2<<">";
			if (next(it) != INM.cend()) cout<<",";
		}
	}
  cout<<endl;
}
void printINB() {
	cout<<"INB:";
	if (InBuffer.opcode.empty()) cout<<endl;
	else {
		cout<<"<"<<InBuffer.opcode<<",R"<<InBuffer.dest<<","
			<<InBuffer.source1<<","<<InBuffer.source2<<">"<<endl;
	}
}
void printAIB() {
	cout<<"AIB:";
	if (AIB.opcode.empty()) cout<<endl;
	else {
		cout<<"<"<<AIB.opcode<<",R"<<AIB.dest<<","<<AIB.source1<<
			","<<AIB.source2<<">"<<endl;
	}
}
void printSIB() {
	cout<<"SIB:";
	if (SIB.opcode.empty()) cout<<endl;
	else {
		cout<<"<"<<SIB.opcode<<",R"<<SIB.dest<<","<<SIB.source1<<
			","<<SIB.source2<<">"<<endl;		
	}
}
void printPRB() {
	cout<<"PRB:";
	if (PRB.opcode.empty()) cout<<endl;
	else {
		cout<<"<"<<PRB.opcode<<",R"<<PRB.dest<<","<<PRB.source1<<
			","<<PRB.source2<<">"<<endl;
	}
}
void printADB() {
	cout<<"ADB:";
	if (ADB.empty()) cout<<endl;
	else {
		cout<<"<R"<<ADB.front().first<<","<<ADB.front().second<<
			">"<<endl;
	}
}
void printREB() {
	cout<<"REB:";
	if (REB.empty()) cout<<endl;
	else {
		for (auto it = REB.cbegin(); it != REB.cend(); ++it) {
			cout<<"<R"<<it->first<<","<<it->second<<">";
			if (next(it) != REB.cend()) cout<<",";
		}
		cout<<endl;
	}
}
void printRGF() {
	cout<<"RGF:";
	for (auto it = RGF.cbegin(); it != RGF.cend(); ++it) {
		cout<<"<R"<<it->first<<","<<it->second<<">";
		if (next(it) != RGF.cend()) cout<<",";
	}
	cout<<endl;
}
void printDAM() {
	cout<<"DAM:";
	for (auto it = DAM.cbegin(); it != DAM.cend(); ++it) {
		cout<<"<"<<it->first<<","<<it->second<<">";
		if (next(it) != DAM.cend()) cout<<",";
	}
	cout<<endl;
}
int main(void) {
	readInstructions();
	readRegisters();
	readDataMemory();
	int i=0;
	FILE *fp;
	fp = freopen("simulation.txt","w",stdout);
	while(1) {
		cout<<"STEP "<<i<<":"<<endl;
		printINM();
		printINB();
		printAIB();
		printSIB();
		printPRB();
		printADB();
		printREB();
	  	printRGF();
	  	printDAM();
		cout<<endl;
		//for Arithmetic ops
		Write();
		MLU2();
		MLU1();
		ASU();
		//for store inst
		Store();
		ADDR();
		if (InBuffer.opcode == "ST")
			Issue2();
		else if (InBuffer.opcode == "ADD" || "MUL" || "SUB")
			Issue1();
		Decode(INM);
		i++;
		//if flag is set, it means the input place to that transition function is empty. Exit when all flags are set.
		cout<<addr_flag<<store_flag<<write_flag<<decode_flag<<asu_flag<<mlu1_flag<<mlu2_flag<<issue_flag<<endl;
		if (addr_flag == 1 && store_flag == 1 && write_flag == 1 && decode_flag == 1 && asu_flag == 1 && mlu1_flag == 1 && mlu2_flag == 1 && issue_flag == 1)
			exit(0);
	}
}
