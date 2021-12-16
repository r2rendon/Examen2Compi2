#include "ast.h"
#include <iostream>
#include <sstream>
#include <set>
#include <map>
#include "asm.h"

const char * floatTemps[] = {"$f0",
                            "$f1",
                            "$f2",
                            "$f3",
                            "$f4",
                            "$f5",
                            "$f6",
                            "$f7",
                            "$f8",
                            "$f9",
                            "$f10",
                            "$f11",
                            "$f12",
                            "$f13",
                            "$f14",
                            "$f15",
                            "$f16",
                            "$f17",
                            "$f18",
                            "$f19",
                            "$f20",
                            "$f21",
                            "$f22",
                            "$f23",
                            "$f24",
                            "$f25",
                            "$f26",
                            "$f27",
                            "$f28",
                            "$f29",
                            "$f30",
                            "$f31"
                        };

#define FLOAT_TEMP_COUNT 32
set<string> intTempMap;
set<string> floatTempMap;

int labelCounter = 0;

extern Asm assemblyFile;

class VariableInfo{
    public:
        VariableInfo(int offset, bool isParameter){
            this->offset = offset;
            this->isParameter = isParameter;
        }
        int offset;
        bool isParameter;
};

map<string, VariableInfo *> codeGenerationVars;

int globalStackPointer = 0;

string getNewLabel(string prefix){
    stringstream ss;
    ss<<prefix << labelCounter;
    labelCounter++;
    return ss.str();
}

string getFloatTemp(){
    for (int i = 0; i < FLOAT_TEMP_COUNT; i++)
    {
        if(floatTempMap.find(floatTemps[i]) == floatTempMap.end()){
            floatTempMap.insert(floatTemps[i]);
            return string(floatTemps[i]);
        }
    }
    cout<<"No more float registers!"<<endl;
    return "";
}

void releaseFloatTemp(string temp){
    floatTempMap.erase(temp);
}

void FloatExpr::genCode(Code &code){
    string floatTemp = getFloatTemp();
    code.place = floatTemp;
    stringstream ss;
    ss << "li.s " << floatTemp <<", "<< this->number <<endl;
    code.code = ss.str();
}

void SubExpr::genCode(Code &code){
    Code leftCode;
    Code rightCode;
    this->expr1->genCode(leftCode);
    this->expr2->genCode(rightCode);
    code.code = leftCode.code + "\n" + rightCode.code + "\n";
    releaseFloatTemp(leftCode.place);
    releaseFloatTemp(rightCode.place);
    string newTempReg = getFloatTemp();
    code.code = "sub.s " + newTempReg + ", " + leftCode.place + ", " + rightCode.place;
    code.place = newTempReg;
}

void DivExpr::genCode(Code &code){
    Code leftCode;
    Code rightCode;
    this->expr1->genCode(leftCode);
    this->expr2->genCode(rightCode);
    code.code = leftCode.code + "\n" + rightCode.code + "\n";
    releaseFloatTemp(leftCode.place);
    releaseFloatTemp(rightCode.place);
    string newTempReg = getFloatTemp();
    code.code = "div.s " + newTempReg + ", " + leftCode.place + ", " + rightCode.place;
    code.place = newTempReg;
}

void IdExpr::genCode(Code &code){
    string floatTemp = getFloatTemp();
    code.place = floatTemp;
    code.code = "l.s "+ floatTemp + ", "+ this->id + "\n";
}

string ExprStatement::genCode(){
    Code exprCode;
    this->expr->genCode(exprCode);
    releaseFloatTemp(exprCode.place);
    return exprCode.code;
}

string IfStatement::genCode(){
    string endIfLabel = getNewLabel("endif");
    Code exprCode;
    this->conditionalExpr->genCode(exprCode);
    stringstream code;
    code << exprCode.code << endl;
    code << "bc1f "<< endIfLabel <<endl;
    // list<Statement *>::iterator it = this->trueStatement->begin();
    // code << this->trueStatement->genCode() << endl
    // << endIfLabel <<" :"<< endl;
    releaseFloatTemp(exprCode.place);
    return code.str();
}

void MethodInvocationExpr::genCode(Code &code){
    list<Expr *>::iterator it = this->expressions.begin();
    list<Code> codes;
    stringstream ss;
    Code argCode;
    while (it != this->expressions.end())
    {
        (*it)->genCode(argCode);
        ss << argCode.code <<endl;
        codes.push_back(argCode);
        it++;
    }

    int i = 0;
    list<Code>::iterator placesIt = codes.begin();
    while (placesIt != codes.end())
    {
        releaseFloatTemp((*placesIt).place);
        ss << "mfc1 $a"<<i<<", "<< (*placesIt).place<<endl;
        i++;
        placesIt++;
    }

    ss<< "jal "<< this->id<<endl;
    string reg;
    reg = getFloatTemp();
        ss << "mtc1 $v0, "<< reg<<endl;
    code.code = ss.str();
    code.place = reg;
}

string AssignationStatement::genCode(){
    Code rightSideCode;
    stringstream ss;

    if (codeGenerationVars.find(this->id) == codeGenerationVars.end())
    {
        codeGenerationVars[this->id] = new VariableInfo(globalStackPointer, false);
        globalStackPointer +=4;
    }

    this->value->genCode(rightSideCode);
    ss<< rightSideCode.code <<endl;
    string name = this->id;
    ss << "s.s "<<rightSideCode.place << ", "<<name <<endl;
    releaseFloatTemp(rightSideCode.place);
    return ss.str();
}

void GteExpr::genCode(Code &code){
    Code leftSideCode; 
    Code rightSideCode;
    stringstream ss;
    this->expr1->genCode(leftSideCode);
    this->expr2->genCode(rightSideCode);
    ss << leftSideCode.code <<endl<< rightSideCode.code<<endl;
    releaseFloatTemp(leftSideCode.place);
    releaseFloatTemp(rightSideCode.place);
    ss<< "c.lt.s "<< rightSideCode.place<< ", "<< leftSideCode.place<<endl;
    code.code = ss.str();
}

void LteExpr::genCode(Code &code){
    Code leftSideCode; 
    Code rightSideCode;
    stringstream ss;
    this->expr1->genCode(leftSideCode);
    this->expr2->genCode(rightSideCode);
    ss << leftSideCode.code <<endl<< rightSideCode.code<<endl;
    releaseFloatTemp(leftSideCode.place);
    releaseFloatTemp(rightSideCode.place);
    ss<< "c.lt.s "<< leftSideCode.place<< ", "<< rightSideCode.place<<endl;
    code.code = ss.str();
}

void EqExpr::genCode(Code &code){
    Code leftSideCode; 
    Code rightSideCode;
    this->expr1->genCode(leftSideCode);
    this->expr2->genCode(rightSideCode);
    stringstream ss;
    releaseFloatTemp(leftSideCode.place);
    releaseFloatTemp(rightSideCode.place);
    ss<< "c.eq.s "<< leftSideCode.place<< ", "<< rightSideCode.place<<endl;
    code.code = ss.str();
}

void ReadFloatExpr::genCode(Code &code){
    
}

string PrintStatement::genCode(){
    Code exprCode;
    list<Expr *>::iterator it = this->expressions.begin();
    while (it != this->expressions.end())
    {   
        (*it)->genCode(exprCode);
        it++;
    }
    releaseFloatTemp(exprCode.place);
    stringstream code;
    code<< exprCode.code<<endl;
    code << "mov.s $f12, "<< exprCode.place<<endl
        << "li $v0, 2"<<endl
        << "syscall"<<endl;
    return code.str();
}

string ReturnStatement::genCode(){
    return "Return statement code generation\n";
}

string saveState(){
    set<string>::iterator it = floatTempMap.begin();
    stringstream ss;
    ss<<"sw $ra, " <<globalStackPointer<< "($sp)\n";
    globalStackPointer+=4;
    return ss.str();
}

string retrieveState(string state){
    std::string::size_type n = 0;
    string s = "sw";
    while ( ( n = state.find( s, n ) ) != std::string::npos )
    {
    state.replace( n, s.size(), "lw" );
    n += 2;
    globalStackPointer-=4;
    }
    return state;
}

string MethodDefinitionStatement::genCode(){
    if(this->stmts.empty() == NULL)
        return "";

    int stackPointer = 4;
    globalStackPointer = 0;
    stringstream code;
    code << this->id<<": "<<endl;
    string state = saveState();
    code <<state<<endl;
    if(this->params.size() > 0){
        list<string>::iterator it = this->params.begin();
        for(int i = 0; i< this->params.size(); i++){
            code << "sw $a"<<i<<", "<< stackPointer<<"($sp)"<<endl;
            codeGenerationVars[it->c_str()] = new VariableInfo(stackPointer, true);
            stackPointer +=4;
            globalStackPointer +=4;
            it++;
        }
    }
    list<Statement *>::iterator it = this->stmts.begin();
    while (it != this->stmts.end())
    {   
        code << (*it)->genCode() << endl;
        it++;
    }
    stringstream sp;
    int currentStackPointer = globalStackPointer;
    sp << endl<<"addiu $sp, $sp, -"<<currentStackPointer<<endl;
    code << retrieveState(state);
    code << "addiu $sp, $sp, "<<currentStackPointer<<endl;
    code <<"jr $ra"<<endl;
    codeGenerationVars.clear();
    string result = code.str();
    result.insert(id.size() + 2, sp.str());
    return result;
}