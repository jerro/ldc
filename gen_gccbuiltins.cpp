#include <map>
#include <string>
#include <algorithm>

#include <string.h>
#include <assert.h>

#include <llvm/Config/llvm-config.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/TableGenAction.h>
#include <llvm/TableGen/Record.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/PathV1.h>

#define XSTR(s) STR(s)
#define STR(s) #s
#define ARCH_STR "_" XSTR(ARCH) "_"

using namespace std;
using namespace llvm;

string dtype(Record* rec)
{
    Init* typeInit = rec->getValueInit("VT");
    if(!typeInit) 
        return "";

    string type = typeInit->getAsString();

    if(type == "iPTR")
        return "void*";
    
    string vec = "";

    if(type[0] == 'v')
    {
        size_t i = 1;
        while(i != type.size() && type[i] <= '9' && type[i] >= '0')
            i++;
        
        vec = type.substr(1, i - 1);
        type = type.substr(i);
    }

    if(type == "i8")
        return "byte" + vec;
    else if(type == "i16")
        return "short" + vec;
    else if(type == "i32")
        return "int" + vec;
    else if(type == "i64")
        return "long" + vec;
    else if(type == "f32")
        return "float" + vec;
    else if(type == "f64")
        return "double" + vec;
    else
        return "";
}

void processRecord(raw_ostream& os, Record& rec)
{
    if(!rec.getValue("GCCBuiltinName"))
        return; 

    string builtinName = rec.getValueAsString("GCCBuiltinName");

    string name =  rec.getName();

    if(name.substr(0, 4) != "int_" || name.find(ARCH_STR) == string::npos)
        return;

    name = name.substr(4);
    replace(name.begin(), name.end(), '_', '.');

    ListInit* paramsList = rec.getValueAsListInit("ParamTypes"); 
    vector<string> params;
    for(int i = 0; i < paramsList->getSize(); i++)
    {
        string t = dtype(paramsList->getElementAsRecord(i));
        if(t == "") 
            return; 
        
        params.push_back(t);
    }

    ListInit* retList = rec.getValueAsListInit("RetTypes");
    string ret;
    if(retList->getSize() == 0)
        ret = "void";
    else if(retList->getSize() == 1)
    {
        ret = dtype(retList->getElementAsRecord(0));
        if(ret == "")
            return;
    }
    else
        return; 

    os << "pragma(intrinsic, \"" + name + "\")\n    ";
    os << ret + " " + builtinName + "(";
   
    if(params.size())
        os << params[0];
 
    for(int i = 1; i < params.size(); i++)
        os << ", " << params[i];

    os << ");\n\n";
}

struct ActionImpl : TableGenAction 
{
    bool operator()(raw_ostream& os, RecordKeeper& records)
    {
        os << "module llvm.gccbuiltins; \n\nimport core.simd;\n\n";

        map<string, Record*> defs = records.getDefs();
        
        for(
            map<string, Record* >::iterator it = defs.begin(); 
            it != defs.end(); 
            it++)
        {
            processRecord(os, *(*it).second);
        } 

        return false;
    }
};

vector<char> cstrVec(string str)
{
    vector<char> r(str.begin(), str.end());
    r.push_back(0);
    return r;
}
#include <stdio.h>
int main(int argc, char** argv)
{
    sys::Path file(LLVM_INCLUDEDIR);
    file.appendComponent("llvm");
    file.appendComponent("Intrinsics.td");

    vector<char> fileStr = cstrVec(file.str()); 
    vector<char> iStr = cstrVec(string("-I=") + string(LLVM_INCLUDEDIR)); 

    vector<char*> args2(argv, argv + argc);
    args2.push_back(&fileStr[0]);
    args2.push_back(&iStr[0]);

    cl::ParseCommandLineOptions(args2.size(), &args2[0]);
    ActionImpl act;
    return TableGenMain(argv[0], act);     
}


// build with g++ -I /usr/lib/llvm-3.1/include/ -I /usr/lib/llvm-3.1/ gen_llvm_intrinsics.cpp   -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -L /usr/lib/llvm-3.1/lib -lLLVMTableGen -lLLVMSupport -ldl -lpthread 

