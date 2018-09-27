package cn.edu.nwpu.cmake;

import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;
import android.widget.VideoView;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.highgui.VideoCapture;

import java.io.File;

import static cn.edu.nwpu.cmake.R.id.picture;

public class ThirdActivity extends AppCompatActivity  implements View.OnClickListener{
    static {
        System.loadLibrary("opencv_java");
        System.loadLibrary("native-lib2");
    }

    private VideoView videoView;
    private VideoCapture capture;
    //OpenCV库加载并初始化成功后的回调函数
    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case BaseLoaderCallback.SUCCESS:
                    //Log.i(BaseLoaderCallback.TAG, "成功加载");
                    break;
                default:
                    super.onManagerConnected(status);
                    //Log.i(BaseLoaderCallback.TAG, "加载失败");
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);//防止运行应用时屏幕关闭

        setContentView(R.layout.activity_third);
        videoView = (VideoView) findViewById(R.id.video_view);
        Button begin = (Button) findViewById(R.id.begin);
        Button pause = (Button) findViewById(R.id.pause);
        Button deal = (Button) findViewById(R.id.deal);
        Button voice = (Button) findViewById(R.id.voice);
        Button exit = (Button) findViewById(R.id.exit);
        begin.setOnClickListener(this);
        pause.setOnClickListener(this);
        deal.setOnClickListener(this);
        voice.setOnClickListener(this);
        exit.setOnClickListener(this);
        if (ContextCompat.checkSelfPermission(ThirdActivity.this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(ThirdActivity.this, new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
        } else {
            initVideoPath();//初始化MediaPlayer
        }
        /*deal.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

            }
        });

        voice.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

            }
        });*/
}

    private void initVideoPath(){
        File file=new File(Environment.getExternalStorageDirectory(),"movie.mp4");


        videoView.setVideoPath(file.getPath());
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,String[] permissions,int[]grantResults){
        switch(requestCode) {
            case 1:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    initVideoPath();
                } else {
                    Toast.makeText(this, "拒绝权限将无法使用程序", Toast.LENGTH_SHORT).show();
                    finish();
                }
                break;
            default:
        }
    }

    @Override
    public void onClick(View v) {
        switch(v.getId()){
            case R.id.begin:
                if(!videoView.isPlaying()){
                    videoView.start();
                }
                break;
            case R.id.pause:
                if(videoView.isPlaying()){
                    videoView.pause();
                }
                break;
            case R.id.deal:



                break;
            case R.id.voice:
                break;
            case R.id.exit:
                finish();
                break;
        }
    }




    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (videoView != null) {
            videoView.suspend();
        }
    }

    /*******加载opencv库*******/
    @Override
    public void onResume() {
        super.onResume();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_11, this, mLoaderCallback);
    }

}
