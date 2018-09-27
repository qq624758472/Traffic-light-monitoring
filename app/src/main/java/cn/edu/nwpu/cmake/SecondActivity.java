package cn.edu.nwpu.cmake;

import android.media.AudioManager;
import android.media.SoundPool;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.imgproc.Imgproc;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.Toast;

import java.util.HashMap;

public class SecondActivity extends AppCompatActivity implements CameraBridgeViewBase.CvCameraViewListener {
    static {
        System.loadLibrary("opencv_java");
        System.loadLibrary("native-lib2");
    }
    private CameraBridgeViewBase openCvCameraView;
    //缓存相机每帧输入的数据
    private Mat foutImage,outImage,rgbaImage;
    //当前处理状态
    private static int Cur_State = 0;
    //按钮组件
    private Button deal;
    private Button exit;
    private SoundPool soundPool;

    final Handler handler = new Handler();
    int  runCount = 10;// 全局变量，用于判断是否是第一次执行
    private int sound_id;


    //OpenCV库加载并初始化成功后的回调函数
    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                    initializeOpenCVDependencies();
                    //Log.i(TAG, "OpenCV loaded successfully");
                    break;
                default:
                    super.onManagerConnected(status);
                    break;
            }
        }
    };


    private void initializeOpenCVDependencies() {
        // And we are ready to go
        openCvCameraView.enableView();
    }

    /**
     * Called when the activity is first created.
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);//防止运行应用时屏幕关闭

        setContentView(R.layout.activity_second);

        openCvCameraView = (CameraBridgeViewBase) findViewById(R.id.camera_view);
        openCvCameraView.setCvCameraViewListener(this);
        deal = (Button) findViewById(R.id.deal_btn);
        exit = (Button) findViewById(R.id.exit);

        initSound();

        Toast.makeText(SecondActivity.this, "init sucess!", Toast.LENGTH_SHORT).show();

        deal.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (Cur_State< 2) {
                    //切换状态
                    Cur_State++;
                } else {
                    //恢复初始状态
                    Cur_State = 0;
                }
            }
        });

        exit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                finish();
            }
        });

    }

    @Override
    public void onDestroy() {
        if (openCvCameraView != null) {
            openCvCameraView.disableView();
        }
    }


    @Override
    public void onPause() {
        if (openCvCameraView != null) {
            openCvCameraView.disableView();
        }
    }


    @Override
    public void onCameraViewStarted(int width, int height) {
        rgbaImage = new Mat(height, width, CvType.CV_8UC4);
        foutImage = new Mat(height, width, CvType.CV_8UC4);
        outImage = new Mat(height, width, CvType.CV_8UC4);
    }

    @Override
    public void onCameraViewStopped() {
        // TODO Auto-generated method stub
        foutImage.release();
        outImage.release();
        rgbaImage.release();
    }

    /**
     * 图像处理都写在此处,OnCameraFrame的返回值是RGBA格式的图像，一定要保证处理了之后的图像是RGBA格式的,这样Android系统才能正常显示！
     */
    @Override
    public Mat onCameraFrame(Mat aInputFrame) {
        switch (Cur_State) {
            case 1:
                rgbaImage=aInputFrame.clone();
                long addressIn=rgbaImage.getNativeObjAddr();
                long addressOut=outImage.getNativeObjAddr();
                int a=nativeVideoProcess(addressIn,addressOut);
                foutImage=new Mat(addressOut);

                if(a>0 && a<=3)
                {
                    if(runCount%10==0) {
                        playSound();
                    }
                    runCount++;
                }




                /*if(a=='Y')
                {
                    Runnable runnable1 = new Runnable(){
                        @Override
                        public void run() {
                            // TODO Auto-generated method stub
                            if(runCount1 == 1){// 第一次执行则关闭定时执行操作
                                // 在此处添加执行的代码
                                playSound1();
                                handler1.removeCallbacks(this);
                            }
                            handler1.postDelayed(this, 0);
                            runCount1++;
                        }

                    };
                    handler1.postDelayed(runnable1, 0);// 打开定时器，执行操作
                }

                if(a=='R')
                {
                    Runnable runnable2 = new Runnable(){
                        @Override
                        public void run() {
                            // TODO Auto-generated method stub
                            if(runCount2 == 1){// 第一次执行则关闭定时执行操作
                                // 在此处添加执行的代码
                                playSound2();
                                handler2.removeCallbacks(this);
                            }
                            handler2.postDelayed(this, 10);
                            runCount2++;
                        }

                    };
                    handler2.postDelayed(runnable2, 10);// 打开定时器，执行操作
                }*/


                //soundPool.release();
                // Toast.makeText(SecondActivity.this, "release!", Toast.LENGTH_SHORT).show();
                break;
        }
        //返回处理后的结果数据
        return foutImage;
    }

    @Override
    public void onResume() {
        super.onResume();
        if (!OpenCVLoader.initDebug()) {
            Log.e("log_wons", "OpenCV init error");
            // Handle initialization error
        }
        initializeOpenCVDependencies();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_11, this, mLoaderCallback);
    }


    void initSound()
    {
        soundPool= new SoundPool(3, AudioManager.STREAM_SYSTEM,1);
        sound_id=soundPool.load(SecondActivity.this,R.raw.voice,1);
    }
    void playSound()
    {
        if(sound_id!=0)
        {
            soundPool.play(sound_id,1, 1, 1, 0, 1.0f);
        }
    }



    /*void releaseSound()
    {
        if(StreamId!=0)
        {
            soundPool.stop(StreamId);
            soundPool.release();
        }

    }*/
    public static native int nativeVideoProcess(long matRgbaImage, long matRgbaOutput);
}