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
    
    int pass2Js(int f_idx, char *buff) {
        Local<Function> fn = Nan::New(gFn[f_idx]);
        Local<Context> ctxObj = Nan::New(gCtx);
        Local<Value> argv[] = { Nan::New(buff).ToLocalChecked() };
        Local<Value> retVal = fn->Call(ctxObj->Global(), 1, argv);
        if (!retVal.IsEmpty()) {
            int intVal = retVal->Int32Value(ctxObj).ToChecked();
            return intVal;
        }
        return -1;
    }
    int send_to_js(int f_idx, char *buff, int len) {
        /* 
        Local<Function> fn = Nan::New(gFn[f_idx]);
        Local<Context> ctxObj = Nan::New(gCtx);
        Local<Value> retVal;
        if (buff && len) {
            Local<Value> argv[] = { Nan::CopyBuffer(buff, len).ToLocalChecked() };
            retVal = fn->Call(ctxObj->Global(), 1, argv);
        }
        else {
            Local<Value> argv[] = { };
            retVal = fn->Call(ctxObj->Global(), 0, argv);
        }
        if (!retVal.IsEmpty()) {
            int intVal = retVal->Int32Value(ctxObj).ToChecked();
            return intVal;
        }
        */
        return -1;
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
        Isolate *isolate = args.GetIsolate();
        Local<Function> fn = Local<Function>::Cast(args[1]);
        if (str == "eflr-data") {
            gFn[_eflr_data_].Reset(fn);
        }
        else if (str == "iflr-header") {
            gFn[_iflr_header_].Reset(fn);
        }
        else if (str == "iflr-data") {
            gFn[_iflr_data_].Reset(fn);
        }
        else if (str == "get-repcode") {
            gFn[_get_repcode_].Reset(fn);
        }
        else if (str == "get-dimension") {
            gFn[_get_dimension_].Reset(fn);
        }

        Local<Context> ctxObj = isolate->GetCurrentContext();
        gCtx.Reset(ctxObj);
    }
    void init(Local<Object> exports) {
        jsprint_f = &pass2Js;
        send_to_js_f = &send_to_js;
        NODE_SET_METHOD(exports, "parse", parse);
        NODE_SET_METHOD(exports, "on", on);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, init);
}
