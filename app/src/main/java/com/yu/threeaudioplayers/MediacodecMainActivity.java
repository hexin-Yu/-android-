package com.yu.threeaudioplayers;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

import com.yu.threeaudioplayers.AACFileDecoderActivity;
import com.yu.threeaudioplayers.H264FileDecodeActivity;

public class MediacodecMainActivity extends AppCompatActivity {



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_mediacodec_main);
    }

    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.play_video:
                Intent i = new Intent(this, H264FileDecodeActivity.class);
                startActivity(i);
                break;
            case R.id.play_audio:
                Intent i1 = new Intent(this, AACFileDecoderActivity.class);
                startActivity(i1);
                break;
        }
    }
}