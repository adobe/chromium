// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

@JNINamespace("android_webview")
public class AwTraceLog {

	public void start() {
		if (mTraceLogPtr != 0)
			return;
		mTraceLogPtr = nativeStart();
	}

	public void stop() {
		if (mTraceLogPtr == 0)
			return;
		nativeStop(mTraceLogPtr);
	}

	@CalledByNative
	private void onLogDataInternal(String data) {
		if (mTraceLogPtr == 0)
			return;
		onLogData(data);
	}

	@CalledByNative
	private void onLogDataCompleteInternal() {
		if (mTraceLogPtr == 0)
			return;
		mTraceLogPtr = 0;
		onLogDataComplete();
	}

	// Override and implement.
	public void onLogData(String data) { }
	public void onLogDataComplete() { }

	private int mTraceLogPtr = 0;
	private native int nativeStart();
	private native void nativeStop(int traceLogPtr);

}
