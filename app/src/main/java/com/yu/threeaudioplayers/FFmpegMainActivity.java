package com.yu.threeaudioplayers;

import android.app.Activity;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.EditText;

public class FFmpegMainActivity extends Activity {

    private EditText fileUrl;
    private AudioTrack audioTrack;
    private  static final String TAG ="DEBUG";

    static {
        System.loadLibrary("native-lib");
    }

    private  native void native_play(String filepath, FFmpegMainActivity activity);
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_f_fmpeg_main);

        fileUrl = (EditText) findViewById(R.id.fileUrl);
        int bufSize = AudioTrack.getMinBufferSize(44100, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, 44100,
                AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT,
                /*audioData.length*/bufSize, AudioTrack.MODE_STREAM);

        audioTrack.play();
    }
    public void playEvent(View view){

        String filePath = fileUrl.getText().toString();
        native_play(filePath, this);
    }
    public void playback(byte[] data, int len){

        Log.d(TAG, "playback called. data len "+ data.length + " len " + len);
        Log.d(TAG, "data to string " + data.toString());
        audioTrack.write(data, 0, len);
    }
}