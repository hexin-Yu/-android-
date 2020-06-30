package com.yu.threeaudioplayers;

import android.content.Intent;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;


//import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    private Button   mBtnmediaplayer;
    private Button   mBtnmediacodec;
    private Button   mBtnffmpeg;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mBtnmediaplayer =(Button) findViewById(R.id.btn_mediaplayer);
        mBtnmediaplayer.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent =new Intent(MainActivity.this, MediaplayerMainActivity.class);
                startActivity(intent);
            }
        });
        mBtnmediacodec =(Button) findViewById(R.id.btn_mediacodec);
        mBtnmediacodec.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent =new Intent(MainActivity.this, MediacodecMainActivity.class);
                startActivity(intent);
            }
        });
        mBtnffmpeg =(Button) findViewById(R.id.btn_ffmpeg);
        mBtnffmpeg.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent =new Intent(MainActivity.this,FFmpegMainActivity.class);
                startActivity(intent);
            }
        });


    }


}