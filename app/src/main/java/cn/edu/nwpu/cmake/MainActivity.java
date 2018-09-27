package cn.edu.nwpu.cmake;

import android.annotation.TargetApi;
import android.content.ContentUris;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.net.Uri;
import android.os.Build;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

import static android.R.attr.scaleHeight;
import static android.R.attr.scaleWidth;
import static android.graphics.Bitmap.Config.RGB_565;
import static org.opencv.imgproc.Imgproc.COLOR_BGR2BGRA;
import static org.opencv.imgproc.Imgproc.COLOR_BGRA2BGR;
import static org.opencv.imgproc.Imgproc.COLOR_RGB2RGBA;
import static org.opencv.imgproc.Imgproc.cvtColor;


public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("opencv_java");
        System.loadLibrary("native-lib2");
    }
    public static final int CHOOSE_PHOTO = 2;
    private ImageView picture;
    //private ImageView result;
    Bitmap bitmap,bm,bmp_dst,bm1,bmp_dst1;
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
        setContentView(R.layout.activity_main);
        picture = (ImageView) findViewById(R.id.picture);
       // result=(ImageView)findViewById(R.id.result);
        Button chooseFromAlbum = (Button) findViewById(R.id.choose_from_album);
        Button detection = (Button) findViewById(R.id.detection);
        Button night_detection = (Button) findViewById(R.id.night_detection);
        /********选择图片button响应事件********/
        chooseFromAlbum.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (ContextCompat.checkSelfPermission(MainActivity.this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(MainActivity.this, new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
                } else {
                    openAlbum();
                }
            }
        });

        /********处理button1响应事件*******/
        detection.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(MainActivity.this, "Show detection result!", Toast.LENGTH_SHORT).show();

                if(bmp_dst!=null&&!bmp_dst.isRecycled()){
                    bmp_dst.recycle();
                    bmp_dst=null;
                }
                else if(bmp_dst1!=null&&!bmp_dst1.isRecycled()){
                    bmp_dst1.recycle();
                    bmp_dst1=null;
                }System.gc();

/*******获取imageView中加载的图片*******/
                picture.setDrawingCacheEnabled(true);
                bm = Bitmap.createBitmap(picture.getDrawingCache());
                picture.setDrawingCacheEnabled(false);
                /*******将从imageView上获得的图片进行转换********/
                Mat mat_src = new Mat(bm.getWidth(), bm.getHeight(), CvType.CV_8UC4);//安卓平台为4通道，windows为3通道
                Utils.bitmapToMat(bm, mat_src);
                Mat dst = new Mat();
                long addressIn = mat_src.getNativeObjAddr();
                long addressOut = dst.getNativeObjAddr();
               // Mat med = new Mat();
                //long addressmed = med.getNativeObjAddr();
                 nativeImageProcess(addressIn, addressOut);//a=0,1,2(或)调用本地函数进行图片处理
                /****将CPP处理结果进行转换并显示到imageView上******/
                Mat output = new Mat(addressOut);
               // Mat medNew = new Mat(addressmed);//可以将这个图片重新传入到一个本地函数中进行数字识别，返回识别数字。
                bmp_dst = Bitmap.createBitmap(output.cols(), output.rows(), Bitmap.Config.ARGB_8888);
                Utils.matToBitmap(output, bmp_dst);
                picture.setImageBitmap(bmp_dst);
            }
        });

        /********处理button2响应事件*******/
        night_detection.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(MainActivity.this, "Show detection result!", Toast.LENGTH_SHORT).show();

                if(bmp_dst1!=null&&!bmp_dst1.isRecycled()){
                    bmp_dst1.recycle();
                    bmp_dst1=null;
                }
                else if(bmp_dst!=null&&!bmp_dst.isRecycled()){
                    bmp_dst.recycle();
                    bmp_dst=null;
                }  System.gc();

                /*******获取imageView中加载的图片*******/
                picture.setDrawingCacheEnabled(true);
                bm1 = Bitmap.createBitmap(picture.getDrawingCache());
                picture.setDrawingCacheEnabled(false);
                /*******将从imageView上获得的图片进行转换********/
                Mat mat_src = new Mat(bm1.getWidth(), bm1.getHeight(), CvType.CV_8UC4);//安卓平台为4通道，windows为3通道
                Utils.bitmapToMat(bm1, mat_src);
                Mat dst = new Mat();
                long addressIn = mat_src.getNativeObjAddr();
                long addressOut = dst.getNativeObjAddr();
                // Mat med = new Mat();
                //long addressmed = med.getNativeObjAddr();
                nightNativeImageProcess(addressIn, addressOut);//a=0,1,2(或)调用本地函数进行图片处理
                /****将CPP处理结果进行转换并显示到imageView上******/
                Mat output = new Mat(addressOut);
                // Mat medNew = new Mat(addressmed);//可以将这个图片重新传入到一个本地函数中进行数字识别，返回识别数字。
                bmp_dst1 = Bitmap.createBitmap(output.cols(), output.rows(), Bitmap.Config.ARGB_8888);
                Utils.matToBitmap(output, bmp_dst1);
                picture.setImageBitmap(bmp_dst1);
            }
        });
    }

    private void openAlbum(){
        Intent intent = new Intent("android.intent.action.GET_CONTENT");
        intent.setType("image/*");
        startActivityForResult(intent, CHOOSE_PHOTO);//打开相册
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,String[] permissions,int[]grantResults){
        switch(requestCode) {
            case 1:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    openAlbum();
                } else {
                    Toast.makeText(this, "You denied the permission", Toast.LENGTH_SHORT).show();
                }
                break;
            default:
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case CHOOSE_PHOTO:
                if (resultCode == RESULT_OK) {
                    //判断手机系统版本号
                    if(Build.VERSION.SDK_INT >= 19){
                        //4.4及以上系统使用这个方法处理图片
                        handleImageOnKitKat(data);
                    }else{
                        //4.4以下系统使用这个方法处理图片
                        handleImageBeforeKitKat(data);
                    }
                }
                break;
            default:
                break;
        }
    }

    @TargetApi(19)
    private void handleImageOnKitKat(Intent data){
        String imagePath=null;
        Uri uri=data.getData();
        if(DocumentsContract.isDocumentUri(this,uri)) {
            //如果是document类型的Uri，则通过document id处理
            String docId = DocumentsContract.getDocumentId(uri);
            if ("com.android.providers.media.documents".equals(uri.getAuthority())) {
                String id = docId.split(":")[1];//解析出数字格式的id
                String selection = MediaStore.Images.Media._ID + "=" + id;
                imagePath = getImagePath(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, selection);
            } else if ("com.android.providers.downloads.documents".equals(uri.getAuthority())) {
                Uri contentUri = ContentUris.withAppendedId(Uri.parse("content://downLoads/public_downloads"), Long.valueOf(docId));
                imagePath = getImagePath(contentUri, null);
            }
        } else if ("content".equalsIgnoreCase(uri.getScheme())) {
            //如果是content类型的Uri，则使用普通处理方式
            imagePath = getImagePath(uri, null);
        } else if ("file".equalsIgnoreCase(uri.getScheme())) {
            //如果是file类型的Uri,直接获取图片路径即可
            imagePath = uri.getPath();
        }
        displayImage(imagePath);
    }

    private void handleImageBeforeKitKat(Intent data){
        Uri uri=data.getData();
        String imagePath = getImagePath(uri, null);
        displayImage(imagePath);
    }

    private String getImagePath(Uri uri,String selection) {
        String path = null;
        //通过Uri和selection来获取真实的图片路径
        Cursor cursor = getContentResolver().query(uri, null, selection, null, null);
        if (cursor != null) {
            if (cursor.moveToFirst()) {
                path = cursor.getString(cursor.getColumnIndex(MediaStore.Images.Media.DATA));
            }
            cursor.close();
        }
        return path;
    }


    private void displayImage(String imagePath ) {

        if(  bmp_dst1!=null&&!bmp_dst1.isRecycled()){
            bmp_dst1.recycle();
            bmp_dst1=null;
        }else if(bmp_dst!=null&&!bmp_dst.isRecycled()){
            bmp_dst.recycle();
            bmp_dst=null;
        }
        else if( bitmap!=null&&!bitmap.isRecycled()){
            bitmap.recycle();
            bitmap=null;
        }
        System.gc();

        if (imagePath != null) {
             bitmap = BitmapFactory.decodeFile(imagePath);
            /****设置Bitmap大小*****/
           // Bitmap newBp=setScaleBitmap(bitmap ,1,1) ;//改变原图大小
            picture.setImageBitmap(bitmap);
            } else {
                Toast.makeText(this, "falied to get image", Toast.LENGTH_SHORT).show();
            }
        }
    private Bitmap setScaleBitmap(Bitmap photo,float SCALE1,float SCALE2) {
        if (photo != null) {
            //为防止原始图片过大导致内存溢出，这里先缩小原图显示，然后释放原始Bitmap占用的内存
            //根据当前的比例缩小,即如果当前是15M,那如果定缩小后是6M,那么SCALE= 15/6
            Bitmap smallBitmap = zoomBitmap(photo, photo.getWidth() / SCALE1, photo.getHeight() / SCALE2);
            //释放原始图片占用的内存，防止out of memory异常发生
            photo.recycle();
            return smallBitmap;

        }
        return null;
    }
    /** 缩放Bitmap图片 **/
    public Bitmap zoomBitmap(Bitmap bitmap, float width, float height) {
        int w = bitmap.getWidth();
        int h = bitmap.getHeight();
        Matrix matrix = new Matrix();
        float scaleWidth =  width / w;
        float scaleHeight =  height / h;
        matrix.postScale(scaleWidth, scaleHeight);// 利用矩阵进行缩放不会造成内存溢出,scaleWidth, scaleHeight为长宽缩小放大的比例
        Bitmap newBmp = Bitmap.createBitmap(bitmap, 0, 0, w, h, matrix, true);
        return newBmp;
    }



    /******创建菜单*****/
    public boolean onCreateOptionsMenu(Menu menu){
        getMenuInflater().inflate(R.menu.main,menu);
        return true;
    }
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.real_recognition:
                Intent intent=new Intent(MainActivity.this,SecondActivity.class);
                startActivity(intent);
                break;
            case R.id.about:
                AlertDialog.Builder dialog1=new AlertDialog.Builder(MainActivity.this);
                dialog1.setTitle("关于本程序");
                dialog1.setMessage("本程序为行车辅助系统");
                dialog1.setCancelable(false);
                dialog1.setPositiveButton("我知道了", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog1,int which){
                    }
                });
                dialog1.show();
            break;
            case R.id.exit:
                finish();
                break;
            default:
        }
        return true;
    }



    /*******加载opencv库*******/
    @Override
    public void onResume() {
        super.onResume();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_11, this, mLoaderCallback);
    }
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
   public static native void nativeImageProcess(long input,long output);
    public static native void nightNativeImageProcess(long input,long output);
}





