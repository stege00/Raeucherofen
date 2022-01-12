package com.example.studienarbeit21;

import android.net.http.HttpResponseCache;
import android.os.AsyncTask;
import android.support.v4.os.IResultReceiver;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;

public class Downloader extends AsyncTask<URL, Void, String> {

    private String downloadWebPage(URL url) {
        try {
            Log.d("dowload","downloadWebPage started");
            URLConnection connection = (URLConnection) url.openConnection();
            connection.connect();
            Log.d("dowload","connection complete");
            InputStream inStream = connection.getInputStream();
            BufferedReader in = new BufferedReader(new InputStreamReader(inStream));
            Log.d("dowload","buffered reader finished");
            String sourceCode = "";
            String tmp;
            while ((tmp = in.readLine()) != null) {
                sourceCode += tmp;
            }
            Log.d("dowload","read data:"+sourceCode);
            return sourceCode;
        }
        catch (IOException io) {
            Log.e("Downloader", "Couldn't downlaod " + url.toString());
            io.printStackTrace();
            return "Error when downloading Webpage " + url.toString();
        }
    }

    public static interface DownloadCompleteListener {
        void onDownloadComplete(String result);
    }

    DownloadCompleteListener dc = null;
    public Downloader(DownloadCompleteListener dc) {
        this.dc = dc;
    }

    @Override
    protected String doInBackground(URL... urls) {
        String sourceCode = "";
        for (URL url : urls) {
            sourceCode += downloadWebPage(url);
        }
        return sourceCode;
    }

    @Override
    protected void onPostExecute(String result) {
        dc.onDownloadComplete(result);
    }
}

