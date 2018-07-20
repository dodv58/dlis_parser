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
    //do_parse((char *)str.c_str());
    args.GetReturnValue().Set(Nan::New(str.c_str()).ToLocalChecked());
}

void init(Local<Object> exports) {
    initSocket();
    NODE_SET_METHOD(exports, "parse", dlis_parse);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init);
