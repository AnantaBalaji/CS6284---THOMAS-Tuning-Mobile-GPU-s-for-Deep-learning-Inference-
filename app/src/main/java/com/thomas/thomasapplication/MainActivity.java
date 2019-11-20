package com.thomas.thomasapplication;

import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Build;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.View;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    String applicationName;
    TextView tv;
    ScrollView logContainer;
    private static final int PERMISSION_REQUEST_WRITE_CODE = 1;
    private static final int PERMISSION_REQUEST_READ_CODE = 2;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("transferCLNative");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (Build.VERSION.SDK_INT >= 23) {
            if (!checkReadPermission()) {
                requestReadPermission(); // Code for permission
            }
            if (!checkWritePermission()) {
                requestWritePermission(); // Code for permission
            }
        }


        applicationName = getApplicationContext().getPackageName();
        tv = (TextView)findViewById(R.id.textView4);

        try {
            Process process = Runtime.getRuntime().exec("logcat -c"); // we clear the logcat
        } catch (IOException e) {
            e.printStackTrace();
        }
        //////////////////

        tv = (TextView)findViewById(R.id.textView4);
        logContainer = (ScrollView)findViewById(R.id.SCROLLER_ID);

        new AsyncTask<Void, String, Void>() {

            @Override
            protected Void doInBackground(Void... params) {
                try {
                    Process process = Runtime.getRuntime().exec("logcat ActivityManager:I TransferCL:D *:S");
                    BufferedReader bufferedReader = new BufferedReader(
                            new InputStreamReader(process.getInputStream()));

                    String line = "";
                    while ((line = bufferedReader.readLine()) != null) {
                        line=line+"\n";
                        if (line.contains(":"))
                            line=line.substring(line.indexOf(":")+1);
                        publishProgress(line);
                    }
                }
                catch (IOException e) {
                }
                return null;
            }

            @Override
            protected void onProgressUpdate(String... values) {
                tv.append(values[0] + "\n");
                logContainer.post(new Runnable() {
                    @Override
                    public void run() {
                        logContainer.fullScroll(View.FOCUS_DOWN);
                    }
                });
            }
        }.execute();


    }

    private boolean checkReadPermission() {
        int result = ContextCompat.checkSelfPermission(MainActivity.this, android.Manifest.permission.READ_EXTERNAL_STORAGE);
        if (result == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            return false;
        }
    }

    private boolean checkWritePermission() {
        int result = ContextCompat.checkSelfPermission(MainActivity.this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (result == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            return false;
        }
    }

    private void requestReadPermission() {

        if (ActivityCompat.shouldShowRequestPermissionRationale(MainActivity.this, android.Manifest.permission.READ_EXTERNAL_STORAGE)) {
            Toast.makeText(MainActivity.this, "Read External Storage permission allows us to do read images. Please allow this permission in App Settings.", Toast.LENGTH_LONG).show();
        } else {
            ActivityCompat.requestPermissions(MainActivity.this, new String[]{android.Manifest.permission.READ_EXTERNAL_STORAGE}, PERMISSION_REQUEST_READ_CODE);
        }
    }

    private void requestWritePermission() {

        if (ActivityCompat.shouldShowRequestPermissionRationale(MainActivity.this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            Toast.makeText(MainActivity.this, "Write External Storage permission allows us to do store images. Please allow this permission in App Settings.", Toast.LENGTH_LONG).show();
        } else {
            ActivityCompat.requestPermissions(MainActivity.this, new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQUEST_WRITE_CODE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQUEST_READ_CODE:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.e("value", "Permission Granted, Now you can use read in local drive .");
                } else {
                    Log.e("value", "Permission Denied, You cannot use read in local drive .");
                }
                break;
            case PERMISSION_REQUEST_WRITE_CODE:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.e("value", "Permission Granted, Now you can store in local drive .");
                } else {
                    Log.e("value", "Permission Denied, You cannot store in local drive .");
                }
                break;
        }
    }


    public void prepareTrainingFiles(View v) {
        //this method prepares the training files (the training file and their labels are respectively stored in one binary file) and the mean And stdDev are stored in one file

        Runnable runnable = new Runnable() {
            public void run() {

                String fileNameStoreData="/data/data/"+applicationName+"/directoryTest/mem2Character2ManifestMapFileData2.raw";
                String fileNameStoreLabel= "/data/data/"+applicationName+"/directoryTest/mem2Character2ManifestMapFileLabel2.raw";
                String fileNameStoreNormalization="/data/data/"+applicationName+"/directoryTest/normalizationTranfer.txt";

                prepareFiles("/data/data/"+applicationName+"/", fileNameStoreData,fileNameStoreLabel, fileNameStoreNormalization,"/mnt/sdcard/Download/manifest_mnist_train.txt",60000, 1);

            }
        };
        Thread mythread = new Thread(runnable);
        mythread.start();



    }

    public void trainingModel(View v) {
        // this method trains our neural network at the native level

        Runnable runnable = new Runnable() {
            public void run() {


                String filename_label="/data/data/"+applicationName+"/directoryTest/mem2Character2ManifestMapFileLabel2.raw";
                String filename_data="/data/data/"+applicationName+"/directoryTest/mem2Character2ManifestMapFileData2.raw";
                int imageSize=28;
                int numOfChannel=1;//black and white => 1; color =>3
                String storeweightsfile="/data/data/"+applicationName+"/directoryTest/weightstTransferedTEST.dat";
                String loadweightsfile="/mnt/sdcard/Download/preloadingData/weightstface1.dat";
                String loadnormalizationfile="/data/data/"+applicationName+"/directoryTest/normalizationTranfer.txt";
                String networkDefinition="1s8c5z-relu-mp2-1s16c5z-relu-mp3-152n-tanh-10n";// see https://github.com/hughperkins/DeepCL/blob/master/doc/Commandline.md
                int numepochs=1;
                int batchsize=32;
                int numtrain=60000;
                float learningRate=0.01f;

                String cmdString="train filename_label="+filename_label;
                cmdString=cmdString+" filename_data="+filename_data;
                cmdString=cmdString+" imageSize="+Integer.toString(imageSize);
                cmdString=cmdString+" numPlanes="+Integer.toString(numOfChannel);
                cmdString=cmdString+" storeweightsfile="+storeweightsfile;
                cmdString=cmdString+" loadweightsfile="+loadweightsfile;
                cmdString=cmdString+" loadnormalizationfile="+loadnormalizationfile;
                cmdString=cmdString+" netdef="+networkDefinition;
                cmdString=cmdString+" numepochs="+Integer.toString(numepochs);
                cmdString=cmdString+" batchsize="+Integer.toString(batchsize);
                cmdString=cmdString+" numtrain="+Integer.toString(numtrain);
                cmdString=cmdString+" learningrate="+Float.toString(learningRate);

                String appDirctory ="/data/data/"+applicationName+"/";

                training(appDirctory,cmdString);
            }
        };
        Thread mythread = new Thread(runnable);
        mythread.start();


    }

    public void predictImages(View v) {
        // this method performs the prediction and sore the result in a file

        Runnable runnable = new Runnable() {
            public void run() {
                String appDirctory ="/data/data/"+applicationName+"/";
                String cmdString ="./predict weightsfile=/data/data/"+applicationName+"/directoryTest/weightstTransferedTEST.dat  inputfile=/mnt/sdcard/Download/manifest_mnist_test.txt outputfile=/data/data/"+applicationName+"/directoryTest/pred.txt batchsize=32";
                prediction(appDirctory,cmdString);

            }
        };
        Thread mythread = new Thread(runnable);
        mythread.start();

    }


    @Override
    public void onClick(View v) {

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native String foobar();
    public static native int training(String path, String cmdTrain);
    public static native int prediction(String path, String cmdPrediction);
    public static native int prepareFiles(String path, String fileNameStoreData,String fileNameStoreLabel, String fileNameStoreNormalization, String manifestPath, int nbImage, int imagesChannelNb);

}