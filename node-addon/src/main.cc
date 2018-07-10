#include <nan.h>
#include "dlis.h"

namespace dlis {
    using v8::Local;
    using v8::Object;
    using v8::FunctionCallbackInfo;
    using v8::Value;
    using v8::String;
    using v8::Function;
    using v8::Isolate;
    using Nan::Persistent;
    using v8::Context;

    Persistent<Function> gFn[NCALLBACKS];
    Persistent<Context> gCtx;
    
    void pass2Js(char *buff) {
        Local<Function> fn = Nan::New(gFn);
        Local<Context> ctxObj = Nan::New(gCtx);
        Local<Value> argv[] = { Nan::New(buff).ToLocalChecked() };
        Local<Value> retVal = fn->Call(ctxObj->Global(), 1, argv);
        int intVal = retVal->Int32Value(ctxObj).ToChecked();
        printf("### %d \n", intVal);
    }
    void parse(const FunctionCallbackInfo<Value>& args) {
        String::Utf8Value strVal(args[0]);
        std::string str(*strVal);
        do_parse((char *)str.c_str());
        args.GetReturnValue().Set(Nan::New(str.c_str()).ToLocalChecked());
    }

    void on(const FunctionCallbackInfo<Value>& args) {
        String::Utf8Value sVal(args[0]);
        std::string str(*sVal);
        printf("set %s callback\n", str.c_str());
        Isolate *isolate = args.GetIsolate();
        Local<Function> fn = Local<Function>::Cast(args[1]);
        gFn.Reset(fn);
        Local<Context> ctxObj = isolate->GetCurrentContext();
        gCtx.Reset(ctxObj);
    }
    void init(Local<Object> exports) {
        jsprint_f = &pass2Js;
        NODE_SET_METHOD(exports, "parse", parse);
        NODE_SET_METHOD(exports, "on", on);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, init);
}
