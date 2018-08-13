#include <nan.h>
#include "dlis.h"

using v8::Local;
using v8::Object;
using v8::FunctionCallbackInfo;
using v8::Value;
using v8::String;
using v8::Function;

char fname[50];

void dlis_parse(const FunctionCallbackInfo<Value>& args) {
    pthread_t parsing_thread;
    String::Utf8Value strVal(args[0]);
    std::string str(*strVal);

    strcpy(fname, str.c_str());

    if(pthread_create(&parsing_thread, NULL, &do_parse, fname) != 0) {
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
