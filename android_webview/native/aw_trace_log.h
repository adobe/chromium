// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_NATIVE_AW_TRACE_LOG_H_
#define ANDROID_WEBVIEW_NATIVE_AW_TRACE_LOG_H_

#include <jni.h>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_ptr.h"
#include "base/android/jni_helper.h"
#include "base/android/scoped_java_ref.h"

namespace android_webview {

class AwTraceLogSubscriberAdapter;

class AwTraceLog {
 public:
  AwTraceLog(JNIEnv* env, jobject obj);
  virtual ~AwTraceLog();

  void Destroy();
  void OnEndTracingComplete();
  void OnTraceDataCollected(const scoped_refptr<base::RefCountedString>& trace_fragment);

 private:
  base::android::ScopedJavaGlobalRef<jobject> java_ref_;
  scoped_ptr<AwTraceLogSubscriberAdapter> subscriber_;
};

bool RegisterAwTraceLog(JNIEnv* env);

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_NATIVE_AW_TRACE_LOG_H_
