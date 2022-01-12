package com.example.studienarbeit21;

import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AppCompatActivity;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity {

    GraphView graph1;
    Button btRefresh;
    TextView latestHumData;
    TextView latestTempData;
    TextView latestFireData;
    TextView latestDoorData;

    Timer timer = new Timer();

    Integer downloadStatus = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        graph1 = findViewById(R.id.graph1);
        btRefresh = findViewById(R.id.buttonRefresh);
        latestHumData = findViewById(R.id.latestHumData);
        latestTempData = findViewById(R.id.latestTempData);
        latestFireData = findViewById(R.id.latestFireData);
        latestDoorData = findViewById(R.id.latestDoorData);

        btRefresh.setOnClickListener(b -> startDownload());

        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                startDownload();
            }
        },100,30000);
    }

    private void startDownload() {
        Downloader.DownloadCompleteListener dcl = new Downloader.DownloadCompleteListener() {
            @Override
            public void onDownloadComplete(String result) {
                if (result.contains("Error")) {
                    printErrorMsg();
                }else {
                    Log.d("download", "onDownloadComplete");
                    String[][] htmlTable = getHTMLTables(result);
                    Log.d("tables", "htmlTable.length=" + htmlTable.length);

                    printDataGraph(htmlTable);
                    printDataLatest(htmlTable);
                    Log.d("print", "printed stuff");
                }
                downloadStatus = 0;
            }
        };
        if (downloadStatus == 0) {
            Downloader downloader = new Downloader(dcl);        //new object of async task in new thread
            URL domain;
            downloadStatus = 1;
            try {
                domain = new URL("http://studienarbeit21.ddns.net/");
                Log.d("start", "check start download");
                downloader.execute(domain);
            } catch (MalformedURLException e) {
                Log.e("URL", "not able to create url from string, aborting download");
                e.printStackTrace();
            }
        }
        else
            Log.d("download", "download already started");
    }

    private void printErrorMsg() {
        latestHumData.setText("Error");
        latestTempData.setText("Error");
        latestFireData.setText("Error");
        latestDoorData.setText("Error");
    }

    private void printDataGraph(@NonNull String[][] table) {
        LineGraphSeries<DataPoint> series_hum = new LineGraphSeries<>();
        LineGraphSeries<DataPoint> series_temp = new LineGraphSeries<>();
        series_hum.setColor(Color.BLUE);
        series_temp.setColor(Color.RED);
        double y_temp,y_hum;
        int max_datapoints = table.length - 1;                  // to exclude null character at the end of string
        Log.d("table","tablelength: "+table.length);
        int relativeTime = 0;


        for (int i = 0; i < max_datapoints; i++) {
            y_hum = Double.parseDouble(table[i][0]);
            y_temp = Double.parseDouble(table[i][1]);
            series_hum.appendData(new DataPoint(relativeTime,y_hum), true, max_datapoints);
            series_temp.appendData(new DataPoint(relativeTime,y_temp),true,max_datapoints);

            relativeTime += 5;
            Log.d("table","table["+i+"][0] "+table[i][0]);
        }

        graph1.removeAllSeries();
        graph1.addSeries(series_hum);
        graph1.addSeries(series_temp);
    }

    private void printDataLatest(@NonNull String[][] table) {
        int latestElement = table.length - 2;
        latestHumData.setText(table[latestElement][0]);

        latestTempData.setText(table[latestElement][1]);

        if (Integer.parseInt(table[latestElement][2]) == 0)
            latestFireData.setText(getResources().getString(R.string.fire_inactive));
        else
            latestFireData.setText(getResources().getString(R.string.fire_active));

        if (Integer.parseInt(table[latestElement][3]) == 0)
            latestDoorData.setText(getResources().getString(R.string.door_closed));
        else
            latestDoorData.setText(getResources().getString(R.string.door_open));
    }

    @NonNull
    private String[][] getHTMLTables(String sourcecode){

        Document doc = Jsoup.parse(sourcecode);
        Log.d("get_tables","got doc:");
        Element table = doc.select("table").get(0); //select first table
        Log.d("get_tables","got tables:");
        Elements rows = table.select("tr");
        Elements columns = rows.get(0).select("th");
        String[][] data = new String[rows.size()][columns.size()];
        Log.d("get_tables","table_rows.size()="+rows.size());
        Log.d("get_tables","table_columns.size()="+columns.size());

        for (int i = 1; i < rows.size(); i++) {             //first row is column names -> skip
            columns = rows.get(i).select("td");
            for (int j = 0; j < columns.size(); j++) {
                data[i-1][j] = columns.get(j).text();
            }
        }
        Log.d("get_tables","return data.length()="+data.length);

        return data;
    }
}