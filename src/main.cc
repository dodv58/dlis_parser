#include <nan.h>
#include "dlis.h"

using v8::Local;
using v8::Object;
using v8::FunctionCallbackInfo;
using v8::Value;
using v8::String;
using v8::Function;

args_t _args;

void dlis_parse(const FunctionCallbackInfo<Value>& args) {
    pthread_t parsing_thread;

    String::Utf8Value strVal(args[0]);
    std::string str(*strVal);
    strcpy(_args.fname, str.c_str());

    String::Utf8Value dirVal(args[1]);
    std::string dir(*dirVal);
    strcpy(_args.data_dir, dir.c_str());

    if(pthread_create(&parsing_thread, NULL, &do_parse, (void*) &_args) != 0) {
        fprintf(stderr, "create thread failed");
        exit(-1);
    };
    //pthread_join(parsing_thread, NULL);
    args.GetReturnValue().Set(Nan::New(str.c_str()).ToLocalChecked());
}

void dlis_parse_segment(const FunctionCallbackInfo<Value>& args) {
    
}

void init(Local<Object> exports) {
    NODE_SET_METHOD(exports, "parseFile", dlis_parse);
    //NODE_SET_METHOD(exports, "parseSegment", dlis_parse_segment);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init);
