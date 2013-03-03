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

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebSettings;
import android.webkit.WebView;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 * 
 * @see SystemUiHider
 */
public class BrowserActivity extends Activity {

	private static final String sDefaultSite = "http://www.android.com";
	private static final String sSiteList = "/data/bench.url";
	private static final int sRepeatTimes = 5;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_browser);
		final WebView webView = (WebView) findViewById(R.id.webView);
		ArrayList<Pair<String, Long>> sites = loadSiteLists(sSiteList);
		EvalRecorder recorder = new EvalRecorder(sites, webView);
		EvalWebViewClient client = new EvalWebViewClient(recorder);

		WebSettings settings = webView.getSettings();
		settings.setSavePassword(false);
		settings.setCacheMode(2);
		settings.setAppCacheEnabled(false);
		webView.setWebViewClient(client);

		recorder.loadNextPage();
	}

	private static ArrayList<Pair<String, Long>> loadSiteLists(String siteList) {
		ArrayList<String> sites = new ArrayList<String>();
		BufferedReader in = null;
		try {
			in = new BufferedReader(new FileReader(siteList));

			String line;
			while ((line = in.readLine()) != null)
				sites.add(line);
		} catch (IOException e) {
		} finally {
			if (in != null) {
				try {
					in.close();
				} catch (IOException e) {
				}
			}
		}

		if (sites.size() == 0)
			sites.add(sDefaultSite);

		ArrayList<Pair<String, Long>> ret = new ArrayList<Pair<String, Long>>();
		for (String url : sites) {
			for (int i = 0; i < sRepeatTimes; ++i)
				ret.add(new Pair<String, Long>(url, (long) 0));
		}

		return ret;
	}
}
