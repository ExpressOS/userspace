/*
 * Copyright (c) 2012-2013 University of Illinois at
 * Urbana-Champaign. All rights reserved.
 *
 * Developed by:
 *
 *     Haohui Mai
 *     University of Illinois at Urbana-Champaign
 *     http://haohui.me
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal with the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimers.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimers in the documentation and/or other materials
 *      provided with the distribution.
 *
 *    * Neither the names of University of Illinois at
 *      Urbana-Champaign, nor the names of its contributors may be
 *      used to endorse or promote products derived from this Software
 *      without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS WITH THE SOFTWARE.
 */

package me.haohui.expressos.browserbench;

import android.os.SystemClock;
import android.util.Log;
import android.webkit.WebView;
import java.util.ArrayList;

final class EvalRecorder {
	private int mCurrentEvaluation;
	private boolean mFinished;
	private final WebView mWebview;
	ArrayList<Pair<String, Long>> mResult;

	EvalRecorder(ArrayList<Pair<String, Long>> paramArrayList,
			WebView paramWebView) {
		this.mResult = paramArrayList;
		this.mWebview = paramWebView;
		this.mCurrentEvaluation = -1;
	}

	final void PageFinished(String url) {
		long l = SystemClock.uptimeMillis();

		if ((this.mCurrentEvaluation >= this.mResult.size())
				|| (this.mFinished) || (url.equals("about:blank")))
			return;

		Pair<String, Long> localPair = this.mResult
				.get(this.mCurrentEvaluation);

		localPair.first = url;
		localPair.second = l - localPair.second;

		Log.i("BrowserBench",
				String.format("%s,%d", localPair.first, localPair.second));
		loadNextPage();
	}

	final void PageStarted(String url) {
		long l = SystemClock.uptimeMillis();
		if ((this.mCurrentEvaluation >= this.mResult.size())
				|| (this.mFinished))
			return;
		
		Pair<String, Long> e = this.mResult.get(this.mCurrentEvaluation);

		e.first = url;
		e.second = l;
	}

	void loadNextPage() {
		this.mCurrentEvaluation = (1 + this.mCurrentEvaluation);
		if (this.mCurrentEvaluation >= this.mResult.size()) {
			this.mFinished = true;
			this.mWebview.loadUrl("about:blank");
			return;
		}
		this.mWebview.loadUrl("about:blank");
		this.mWebview.clearCache(true);
		this.mWebview.clearDisappearingChildren();
		this.mWebview.freeMemory();
		System.gc();
		this.mWebview.loadUrl(this.mResult.get(this.mCurrentEvaluation).first);
	}
}