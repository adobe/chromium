// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/native/aw_trace_log.h"

#include "jni/AwTraceLog_jni.h"
#include "base/android/jni_string.h"
#include "content/public/browser/trace_subscriber.h"
#include "content/public/browser/trace_controller.h"

namespace android_webview {

class AwTraceLogSubscriberAdapter : public content::TraceSubscriber {
public:
  AwTraceLogSubscriberAdapter(AwTraceLog* traceLog)
    : trace_log_(traceLog) {
  }
private:
  // Called once after TraceController::EndTracingAsync.
  virtual void OnEndTracingComplete() {
    trace_log_->OnEndTracingComplete();
    // This object is deleted at this point.
  }

  // Called 0 or more times between TraceController::BeginTracing and
  // OnEndTracingComplete. Use base::debug::TraceResultBuffer to convert one or
  // more trace fragments to JSON.
  virtual void OnTraceDataCollected(const scoped_refptr<base::RefCountedString>& trace_fragment) {
    trace_log_->OnTraceDataCollected(trace_fragment);
  }

  AwTraceLog* trace_log_;
};

AwTraceLog::AwTraceLog(JNIEnv* env, jobject obj) {
  java_ref_.Reset(env, obj);
  subscriber_.reset(new AwTraceLogSubscriberAdapter(this));
  content::TraceController::GetInstance()->BeginTracing(subscriber_.get(), "", 
      base::debug::TraceLog::RECORD_UNTIL_FULL);
}

AwTraceLog::~AwTraceLog() {
}

void AwTraceLog::Destroy() {
  if (!subscriber_) {
    delete this;
    return;
  }
  content::TraceController::GetInstance()->EndTracingAsync(subscriber_.get());
}

void AwTraceLog::OnTraceDataCollected(const scoped_refptr<base::RefCountedString>& trace_fragment) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> jdata = base::android::ConvertUTF8ToJavaString(env, trace_fragment->data());
  Java_AwTraceLog_onLogDataInternal(env, java_ref_.obj(), jdata.obj());
}

void AwTraceLog::OnEndTracingComplete() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_AwTraceLog_onLogDataCompleteInternal(env, java_ref_.obj());
  delete this;
}

static jint Start(JNIEnv* env, jobject obj) {
  return reinterpret_cast<jint>(new AwTraceLog(env, obj));
}

static void Stop(JNIEnv* env, jobject obj,
    jint traceLogPtr) {
  AwTraceLog* traceLog = reinterpret_cast<AwTraceLog*>(traceLogPtr);
  traceLog->Destroy();
}

bool RegisterAwTraceLog(JNIEnv* env) {
  return RegisterNativesImpl(env) >= 0;
}

}
