#include <jni.h>
#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv/cv.h"
#include <string>
#include <list>
#include <vector>
#include <map>
#include <stack>

using namespace std;
using namespace cv;

extern "C"{
vector<Mat>  ImageNum;
int m_State_Count = 0;      //数字位数
char State[30] = "  ";   //颜色状态  红灯或者绿灯   黄灯
int m_Red_Num = 0;
int m_Yellow_Num = 0;
int m_Green_Num = 0;
bool m_Red_State = false;
bool m_Yellow_State = false;
Mat hsvimage,hole;
bool m_Green_State = false;

int List[10][2] = {-1};  //1列坐标，2列字符数值
int l = 0;  //字符个数
struct Coordinate
{
    int Top_left_x;
    int Top_left_y;
    int Right_down_x;
    int Right_down_y;
    int Width;
    int Height;
};
std::vector<Coordinate>  m_coord;
std::vector<Coordinate>  m_coord_cut,m_coord_cut0;
void detect(IplImage * img, char * strnum/*, CvRect rc*/);
bool GreaterSort (Coordinate a,Coordinate b) { return (a.Top_left_x<b.Top_left_x); }  //降序排列
int a[9][2] = {0};

Coordinate coord;//夜间参数
Mat imageRoI;//夜间参数

/******* 边缘修补***********/
void ConnectEdge(IplImage * src)
{

    int width = src->width;
    int height = src->height;

    uchar * data = (uchar *)src->imageData;
    for (int i = 2;i < height - 2;i++)
    {
        for (int j = 2;j < width - 2;j++)
        {
            //如果该中心点为255,则考虑它的八邻域
            if(data[i * src->widthStep + j] == 255)
            {
                int num = 0;
                for (int k = -1;k < 2;k++)
                {
                    for (int l = -1;l < 2;l++)
                    {
                        //如果八邻域中有灰度值为0的点，则去找该点的十六邻域
                        if(k != 0 && l != 0 &&data[(i + k) * src->widthStep + j + l] == 255)
                            num++;
                    }
                }
                //如果八邻域中只有一个点是255，说明该中心点为端点，则考虑他的十六邻域
                if(num == 1)
                {
                    for (int k = -2;k < 3;k++)
                    {
                        for (int l = -2;l < 3;l++)
                        {
                            //如果该点的十六邻域中有255的点，则该点与中心点之间的点置为255
                            if(!(k < 2 && k > -2 && l < 2 && l > -2) && data[(i + k) * src->widthStep + j + l] == 255)
                            {
                                data[(i + k / 2) * src->widthStep + j + l / 2] = 255;
                            }
                        }
                    }
                }
            }
        }
    }
}

/*******孔洞填充*********/
void fillHole(const Mat srcBw, Mat &dstBw)
{
    Size m_Size = srcBw.size();
    Mat Temp=Mat::zeros(m_Size.height+2,m_Size.width+2,srcBw.type());//延展图像
    srcBw.copyTo(Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)));

    cv::floodFill(Temp, Point(0, 0), Scalar(255));

    Mat cutImg;//裁剪延展的图像
    Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)).copyTo(cutImg);

    dstBw = srcBw | (~cutImg);
}

void Recognition(Mat image)//  检测框
{
    Mat gray1,gray2,HSVimage;
    cvtColor(image,gray1,COLOR_BGR2GRAY);
    cvtColor(image,gray2,COLOR_BGR2GRAY);
    cvtColor(image, HSVimage, COLOR_BGR2HSV);
    int R=0,G=0,B=0;
    float H,S,V;

    for(int j=0;j<image.rows;j++)
        for (int i=0;i<image.cols;i++)
        {
            {
                B=image.ptr<uchar>(j,i)[0];
                G=image.ptr<uchar>(j,i)[1];
                R=image.ptr<uchar>(j,i)[2];
                V= HSVimage.ptr<uchar>(j,i)[2];
                if ((R+G+B)/3<85)  //颜色特征           //图片像素降低时，需要适当增大阈值   原70
                {
                    gray1.at<uchar>(j,i) = 255;

                }
                else
                {
                    gray1.at<uchar>(j,i) = 0;
                }

                if(V<90)  //亮度特征                     //图片像素降低时，需要适当增大阈值
                {
                    gray2.at<uchar>(j,i) = 255;

                }
                else
                {
                    gray2.at<uchar>(j,i) = 0;
                }
            }
        }

    for(int j=0;j<gray1.rows;j++)   //比较两个图片的像素值   也就是并
        for (int i=0;i<gray1.cols;i++)
        {
            {
                int b = gray1.at<uchar>(j,i);
                int c = gray2.at<uchar>(j,i);
                if (b==255||c==255)
                {
                    gray1.at<uchar>(j,i) = 255;//(x,y)点的像素值，取大的 （一个0一个255取255，两个都是0取0）
                }
                else
                {
                    gray1.at<uchar>(j,i) = 0;
                }

            }
        }

    /***********边缘修补*****/
    IplImage imgTmp =(IplImage)gray1;
    IplImage *input = cvCloneImage(&imgTmp);
    ConnectEdge(input);      //边缘修补
    Mat input1(input, true);

    /***********孔洞填充*****/
    fillHole(input1,hole);

    Mat element = getStructuringElement( MORPH_RECT,Size(5,9));   //图片像素降低时，size需要减小
    morphologyEx( hole, hole, MORPH_OPEN,element );                   //开运算去除比探针小的孤立噪声点

    Mat element1 = getStructuringElement( MORPH_RECT,Size(11,11));    // 左右两个联通区域相距小于等于15时，将它们连在一起形成一个联通区域，上下两个连通区域小于等于13时，将它们连在一起
    morphologyEx( hole, hole, MORPH_CLOSE,element1);                 //闭运算将间隔较小距离的联通区域连接起来，可以减少联通区域个数

}

void GetContours(Mat image,Mat dst,Mat dst1)//轮廓
{
    vector<std::vector<cv::Point> >coutours;
    findContours(dst,coutours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0));
    drawContours(dst1,coutours,-1,Scalar(100),2);
    //	imshow("binayu",dst);
    //	imshow("luokuo2",dst1);
    double a;
    for (int i=0;i<coutours.size();i++)
    {
        vector<Rect> rect(coutours.size());
        rect[i]= boundingRect(Mat(coutours[i]));
        //根据面积，长宽比，坐标位置，排除不符合的矩形框
        //a =rect[i].height/rect[i].width;
        double H = rect[i].height;
        double W = rect[i].width;
        a = H / W;
        double carea=contourArea(coutours[i]);
        float area=rect[i].height*rect[i].width;

        //  两个框 检测出来
        //if(3< H&&H <45&&1< W&&W <45)//长宽
        if(area>200&&area<1500/*1800*/){
            if((1.5<a&&a<3.2))
            {
                if(rect[i].x>image.cols/4&&rect[i].x<image.cols*4/5)//&&rect[i].y<image.rows/3){//&&rect[i].x<src.cols*2/3)
                {
                    if(carea/area>0.7/*0.6*/)
                    {
                        CvPoint pt1,pt2;
                        pt1.x = rect[i].x;
                        pt1.y = rect[i].y;
                        pt2.x = rect[i].x + rect[i].width ;
                        pt2.y = rect[i].y + rect[i].height;

                        Coordinate coord;
                        coord.Top_left_x = pt1.x;
                        coord.Top_left_y = pt1.y;
                        coord.Right_down_x = pt2.x;
                        coord.Right_down_y = pt2.y;
                        coord.Height = rect[i].height;
                        coord.Width = rect[i].width;
                        m_coord.push_back(coord);
                        rectangle(image, pt1, pt2, CV_RGB(255,0,0), 2);
                    }
                }
            }
        }
    }
}


void Color(Mat image,int x1,int y1,int x2,int y2)//颜色识别
{
    float s = 0,h = 0;
    float H = 0, S = 0, V = 0;

    /*  颜色识别 */
    for(int j=y1;j<y2;j++)  //0表示检测到的联通区域标号  第几个
    {
        for (int i=x1;i<x2;i++)
        {
            H=  image.ptr<uchar>(j,i)[0];
            h = H / 180;
            if (h>0.45&&h<0.55)
            {
                m_Green_Num ++;
            }
            else if (h>0.06&&h<0.18)
            {
                m_Yellow_Num ++;
            }
            else if ((h>0.95&&h<1)||(h>0&&h<0.06))
            {
                m_Red_Num ++;
            }
        }
    }
    int m_Max = max(max(m_Green_Num,m_Yellow_Num),m_Red_Num);
    if (m_Max == m_Red_Num)
    {
        m_Red_State = true;
    }
    else if(m_Max == m_Green_Num)
    {
        m_Green_State = true;
    }
    else if(m_Max == m_Yellow_Num)
    {
        m_Yellow_State = true;
    }
    if(m_Green_State)
    {
        //strcpy(State,"绿灯，可以通行");//拷贝字符
        strcpy(State,"G");//拷贝字符
    }
    else if(m_Yellow_State)
    {
        //strcpy(State,"红灯，禁止通行");//拷贝字符
        strcpy(State,"Y");//拷贝字符
    }
    else if(m_Red_State)
    {
        //strcpy(State,"红灯，禁止通行");//拷贝字符
        strcpy(State,"R");//拷贝字符
    }
    printf("%s\t",State);
}




void detect(IplImage * img, char * strnum/*, CvRect rc*/)
{
    m_State_Count++;
    //List[l][0] = rc.x;  //第l个字符横坐标

    //if the height is more two longer than width,number is 1
    if((img->height/img->width)>2)
    {
        List[l][1] = 1;
        //printf("字符是 1.\n");
    }
    else
    {
        int ld[3] = {0};
        CvScalar pix;

        int i = img->width/2;
        int j = img->height/3;
        int k = img->height*2/3;

        //竖向扫描
        for(int m=0; m<3; m++)
        {
            for(int n=m*j; n<(m+1)*j; n++)
            {
                pix = cvGet2D(img, n, i);
                if(pix.val[0]==255)
                {
                    ld[0]++;
                    break;
                }
            }
        }
        //printf("竖向 %d\n", ld[0]);

        //横向扫描
        for(int m=0; m<2; m++)
        {
            for(int n=m*i; n<(m+1)*i; n++)
            {
                //横向1/3扫描
                pix = cvGet2D(img, j, n);
                if(pix.val[0]==255)
                {
                    ld[1]++;
                    break;
                }
            }
        }
        //printf("横向1 %d\n", ld[1]);

        //横向扫描
        for(int m=0; m<2; m++)
        {
            for(int n=m*i; n<(m+1)*i; n++)
            {
                //横向2/3扫描
                pix = cvGet2D(img, k, n);
                if(pix.val[0]==255)
                {
                    ld[2]++;
                    break;
                }
            }
        }
        //printf("横向2 %d\n", ld[2]);

        if((ld[0]==2)&&(ld[1]==2)&&(ld[2]==2))
        {
            //printf("字符是 0\n");
            List[l][1] = 0;
        }
        else if((ld[0]==1)&&(ld[1]==2)&&(ld[2]==1))  //ld[1]  上面横穿
        {
            //printf("字符是 4\n");
            List[l][1] = 4;
        }
        else if((ld[0]==3)&&(ld[1]==1)&&(ld[2]==2))
        {
            //	printf("字符是 6\n");
            List[l][1] = 6;
        }
        else if((ld[0]==1)&&(ld[1]==1)&&(ld[2]==1))
        {
            //printf("字符是 7\n");
            List[l][1] = 7;
        }
        else if((ld[0]==3)&&(ld[1]==2)&&(ld[2]==2))
        {
            //printf("字符是 8\n");
            List[l][1] = 8;
        }
        else if((ld[0]==3)&&(ld[1]==2)&&(ld[2]==1))
        {
            //printf("字符是 9\n");
            List[l][1] = 9;
        }


        else if((ld[0]==3)&&(ld[1]==1)&&(ld[2]==1))
        {
            int l1=0, l2=0;
            //横向扫描
            int k = img->height/3;
            for(int i=0; i<img->width/2; i++)
            {
                //横向2/3扫描
                pix = cvGet2D(img, k, i);
                if(pix.val[0]==255)
                {
                    l1++;
                    break;
                }
            }
            //横向扫描
            k = img->height*2/3;
            for(int i=0; i<img->width/2; i++)
            {
                //横向2/3扫描
                pix = cvGet2D(img, k, i);
                if(pix.val[0]==255)
                {
                    l2++;
                    break;
                }
            }
            if((l1==0)&&(l2==0))
            {
                //printf("字符是 3\n");
                List[l][1] = 3;
            }
            else if((l1==0)&&(l2==1))
            {
                //printf("字符是 2\n");
                List[l][1] = 2;
            }
            else if((l1==1)&&(l2==0))
            {
                //printf("字符是 5\n");
                List[l][1] = 5;
            }
        }
        else printf("识别失败\n");
    }
    l++;
}


/*************白天静态图片识别(本地函数)*****************************/
JNIEXPORT void JNICALL
Java_cn_edu_nwpu_cmake_MainActivity_nativeImageProcess(JNIEnv *, jobject, jlong inputImg,
                                                       jlong outputImg ) {
    Mat &src = *(Mat *) inputImg;
    Mat &outImg = *(Mat *) outputImg;//输出图像
    //resize(src,src,Size(1920,1080),0,0,INTER_LINEAR);//大部分都不需要resize,有的图片需要resize才能识别出来(YINYU 绿灯1和0)，有的不需要resize才能识别出来(QINN 绿灯9,8,11，YINYU黄灯1)，有的需不需要都可以
    cvtColor(src, src, COLOR_RGBA2BGR);
    //resize(src,src,Size(1920,1080),0,0,INTER_LINEAR);
    Mat image;
    Mat hsvimage_cut,gray;
    image = src(Rect(src.cols/3,src.rows/4,src.cols/3,src.rows/3));
    Mat Src_cut = image.clone();
    cvtColor(image, hsvimage, COLOR_BGR2HSV);  //转换到HSV空间
    Recognition(image);//检测框
    Mat image1 = image.clone();//原图感兴趣区域，即image
    Mat dst =hole.clone() ;//复制图片,二值图
    Mat dst1=dst.clone();
    GetContours(image1,dst,dst1);//轮廓
    sort(m_coord.begin(),m_coord.end(),GreaterSort);//升序排列

    //剪裁灯区感兴趣区域
    Mat image_cut;
    if (m_coord.size()>1)
    {
        int x0 = m_coord[0].Top_left_x;
        int y0 = m_coord[0].Top_left_y;
        int m_Width = m_coord[0].Width;
        int m_Height = m_coord[0].Height;

        int x1 = m_coord[1].Top_left_x;
        int y1 = m_coord[1].Top_left_y;
        int m_Width1 = m_coord[1].Width;
        int m_Height1 = m_coord[1].Height;

        image_cut = Src_cut(Rect(x0,y0,x1 - x0 +m_Width, m_Height));
    }
    else
    {
        resize(src,src,Size(1600,900),0,0,INTER_LINEAR);
        outImg = src.clone();
    }
    Mat image_cut1 = image_cut.clone();
    Mat image_cut2 = image_cut.clone();
    Mat image_cut0= image_cut.clone();
    //HSV颜色空间根据亮度V进行分割
    cvtColor(image_cut2, hsvimage_cut, CV_BGR2HSV);
    cvtColor(image_cut1, gray,COLOR_RGB2GRAY);
    float H, S, V,h0;

    for (int y = 0; y < hsvimage_cut.rows; y++)
    {
        for (int x = 0; x < hsvimage_cut.cols; x++)
        {
            H = hsvimage_cut.at<Vec3b>(y, x)[0];
            V =  hsvimage_cut.at<Vec3b>(y, x)[2];  //V;0-255
            h0 = H/180;
            if(V>=215)//205也可以
                gray.at<uchar>(y,x) =255;
            else
                gray.at<uchar>(y,x) =0;
        }
    }

/******闭运算作用：连接数字断裂处********/
    Mat element1 = getStructuringElement( MORPH_RECT,Size(1,2));
    morphologyEx( gray, gray, MORPH_CLOSE,element1);
    Mat GrayCut=gray.clone();


    //Mat Gray8=gray.clone();
  // resize(Gray8,Gray8,Size(1000,600),0,0,INTER_LINEAR);
   // outImg=Gray8.clone();


//根据外接矩形的面积，长宽比，坐标位置，提取灯区
    Mat gray2=gray.clone();
    vector<std::vector<cv::Point> >coutours_cut0;
    findContours(gray2,coutours_cut0,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0));
    double a0;
    for (int i=0;i<coutours_cut0.size();i++)
    {
        vector<Rect> rect0(coutours_cut0.size());
        rect0[i]= boundingRect(Mat(coutours_cut0[i]));
        double H0 = rect0[i].height;
        double W0 = rect0[i].width;
        a0 = H0 / W0;
        double carea0=contourArea(coutours_cut0[i]);
        float area0=rect0[i].height*rect0[i].width;
        if(area0>10)
        {
            if(0.5<a0&&a0<1.5)
            {
                if(W0>3){
                    if(rect0[i].x>image_cut.cols*2/3||rect0[i].x<image_cut.cols/5){
                        CvPoint pt1,pt2;
                        pt1.x = rect0[i].x;
                        pt1.y = rect0[i].y;
                        pt2.x = rect0[i].x + rect0[i].width ;
                        pt2.y = rect0[i].y + rect0[i].height;

                        Coordinate coord_cut;
                        coord_cut.Top_left_x = pt1.x;
                        coord_cut.Top_left_y = pt1.y;
                        coord_cut.Right_down_x = pt2.x;
                        coord_cut.Right_down_y = pt2.y;
                        coord_cut.Height = rect0[i].height;
                        coord_cut.Width = rect0[i].width;
                        m_coord_cut0.push_back(coord_cut);

                        // 						auto it=m_coord_cut.begin();
                        // 							while(it!=m_coord_cut.end())
                        // 						        if(it->Right_down_y<image_cut.rows/2&&it->Right_down_x-it->Top_left_x<3)
                        // 						           it=  m_coord_cut.erase(it);
                        // 								else
                        // 									++it;

                        rectangle(image_cut0, pt1, pt2, CV_RGB(255,0,0), 1);
                    }
                }
            }
        }
    }
    sort(m_coord_cut0.begin(),m_coord_cut0.end(),GreaterSort);//升序排列
//灯区颜色识别
    for (int i=0;i<m_coord_cut0.size();i++)
    {
        char color[6];
        Color( hsvimage_cut,m_coord_cut0[i].Top_left_x,m_coord_cut0[i].Top_left_y,m_coord_cut0[i].Right_down_x,m_coord_cut0[i].Right_down_y);//颜色识别
        sprintf(color,"%s",State);
        putText(image1,color,Point(m_coord[i].Top_left_x-8,m_coord[i].Top_left_y -10),3,1,Scalar(0,0,255));
    }

//根据外界矩形的面积，长宽比，坐标位置，提取数字倒计时区域
    vector<std::vector<cv::Point> >coutours_cut;
    findContours(gray,coutours_cut,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0));
    double a1;
    for (int i=0;i<coutours_cut.size();i++)
    {
        vector<Rect> rect(coutours_cut.size());
        rect[i]= boundingRect(Mat(coutours_cut[i]));
        double H = rect[i].height;
        double W = rect[i].width;
        a1 = H / W;
       double carea=contourArea(coutours_cut[i]);
        float area=rect[i].height*rect[i].width;
        if(area>5)
        {
            if(1.0<a1&&a1<12)
            {
                if(5< H)//长宽
                {
                    if(rect[i].x>image_cut.cols/3&&rect[i].x<image_cut.cols*2/3)
                    {
                        if(rect[i].y<image_cut.rows*2/3/*image_cut.rows*2/3*/)
                        {
                            CvPoint pt1,pt2;
                            pt1.x = rect[i].x;
                            pt1.y = rect[i].y;
                            pt2.x = rect[i].x + rect[i].width ;
                            pt2.y = rect[i].y + rect[i].height;

                            Coordinate coord_cut;
                            coord_cut.Top_left_x = pt1.x;
                            coord_cut.Top_left_y = pt1.y;
                            coord_cut.Right_down_x = pt2.x;
                            coord_cut.Right_down_y = pt2.y;
                            coord_cut.Height = rect[i].height;
                            coord_cut.Width = rect[i].width;
                            m_coord_cut.push_back(coord_cut);

// 						auto it=m_coord_cut.begin();
// 							while(it!=m_coord_cut.end())
// 						        if(it->Right_down_y<image_cut.rows/2&&it->Right_down_x-it->Top_left_x<3)
// 						           it=  m_coord_cut.erase(it);
// 								else
// 									++it;

                            rectangle(image_cut, pt1, pt2, CV_RGB(255,0,0), 1);
                        }
                    }
                }
            }
        }
    }
    sort(m_coord_cut.begin(),m_coord_cut.end(),GreaterSort);//升序排列

//倒计时数字放缩
    Mat image_little,image_little1;
    Mat GrayCut1=GrayCut.clone();

    if (m_coord_cut.size()==1){

        image_little = GrayCut(Rect(m_coord_cut[0].Top_left_x,m_coord_cut[0].Top_left_y,m_coord_cut[0].Width,m_coord_cut[0].Height));
        ImageNum.push_back(image_little);
        resize(ImageNum[0],ImageNum[0],Size(10*m_coord_cut[0].Width,10*m_coord_cut[0].Height),0,0,INTER_NEAREST);

    }else if (m_coord_cut.size()==2){

        image_little = GrayCut1(Rect(m_coord_cut[0].Top_left_x,m_coord_cut[0].Top_left_y,m_coord_cut[0].Width,m_coord_cut[0].Height));
        ImageNum.push_back(image_little);
        image_little1 = GrayCut1(Rect(m_coord_cut[1].Top_left_x,m_coord_cut[1].Top_left_y,m_coord_cut[1].Width,m_coord_cut[1].Height));
        ImageNum.push_back(image_little1);
        resize(ImageNum[0],ImageNum[0],Size(10*m_coord_cut[0].Width,10*m_coord_cut[0].Height),0,0,INTER_NEAREST);
        resize(ImageNum[1],ImageNum[1],Size(10*m_coord_cut[1].Width,10*m_coord_cut[1].Height),0,0,INTER_NEAREST);
        /*resize(image_little,image_little,Size(10*Width,10*Height),0,0,INTER_NEAREST); //放大抠图,最后一个参数代表插图方式，0,3都可以,INTER_AREA或者*/

    }else	{

        image_little = image1(Rect(10,27,20,20));   //无数字
    }


/*        数字识别          */
    int idx = 0;
    char szName[56] = {0};
    if (ImageNum.size() == 1)
    {
        IplImage *IplImg /* =cvCreateImage(Size(200,200),8, 1)*/;//Mat 到IplImage *
        IplImage temp=(IplImage)ImageNum[0];
        IplImg = &temp;
        sprintf(szName, "Number_%d", idx++);
        detect(IplImg, szName);
    }
    else if(ImageNum.size() == 2)
    {
        for (int i=0;i<ImageNum.size();i++)
        {
            IplImage *IplImg /* =cvCreateImage(Size(200,200),8, 1)*/;//Mat 到IplImage *
            IplImage temp=(IplImage)ImageNum[i];
            IplImg = &temp;
            sprintf(szName, "Number_%d", idx++);
            detect(IplImg, szName);
        }
    }

    //	printf("\字符识别结果为：");
    for(int i=0; i<l; i++)
    {
        for(int j=0; i+j<l-1; j++)
        {
            if(List[j][0] > List[j + 1][0])
            {
                //坐标排序
                int temp = List[j][0];
                List[j][0] = List[j + 1][0];
                List[j + 1][0] = temp;
                //数值随之变化
                temp = List[j][1];
                List[j][1] = List[j+1][1];
                List[j+1][1] = temp;
            }
        }
    }
    int m = List[1][1];
    int n = List[0][1];
    int Num = 0;
    if (m_State_Count == 1)
    {
        printf("当前数字是\t%d ",List[0][1]);
        Num = List[0][1];
    }
    else if(m_State_Count == 2)
    {
        printf("当前数字是\t%d ",List[0][1]*10 + List[1][1]);
        Num = List[0][1]*10 + List[1][1];
    }
    else{
        //return 0;
    }

    char str_lx[6];
    /*int num=5;*/
    sprintf(str_lx,"%d",Num);

    if (m_coord.size()>1)
    {
        putText(image1,str_lx,Point((m_coord[0].Right_down_x+m_coord[1].Top_left_x)/2-17,m_coord[0].Top_left_y-10),3,1,Scalar(0,0,255));//h8:-28 ,0:-38
    }

    Mat imageGray;
    cvtColor(image1, imageGray, COLOR_BGR2GRAY);
    Mat mask = imageGray.clone();       //掩膜必须为灰度图
    image1.copyTo(image, mask);
    resize(src,src,Size(1920, 1080),0,0,INTER_LINEAR);
    cvtColor(src, src, COLOR_BGR2RGBA);
    outImg=src.clone();
}











void GetContours1(Mat image,Mat dst,Mat dst1)//轮廓
{
    vector<std::vector<cv::Point> >coutours;
    findContours(dst,coutours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0));
    drawContours(dst1,coutours,-1,Scalar(100),2);
    double a;
    for (int i=0;i<coutours.size();i++)
    {
        vector<Rect> rect(coutours.size());
        rect[i]= boundingRect(Mat(coutours[i]));
        //根据面积，长宽比，坐标位置，排除不符合的矩形框

        double H = rect[i].height;
        double W = rect[i].width;
        a = H / W;
        //float carea=contourArea(coutours[i]);
        double area=rect[i].height*rect[i].width;
// 		cout<<"外接矩形面积 =  "<<area<<endl;
// 		cout<<"外接矩形长宽比 =  "<<a<<endl;
        if(area>120&&area<1000){
            if(0.5<a&&a<3||a>3&&a<8.5){
                if(rect[i].x>imageRoI.cols/4&&rect[i].x<imageRoI.cols*7/8&&rect[i].y>imageRoI.rows/8&&rect[i].y<imageRoI.rows*7/8){
                    if(H>12){
                        CvPoint pt1,pt2;
                        pt1.x = rect[i].x;
                        pt1.y = rect[i].y;
                        pt2.x = rect[i].x + rect[i].width ;
                        pt2.y = rect[i].y + rect[i].height;

                        coord.Top_left_x = pt1.x;
                        coord.Top_left_y = pt1.y;
                        coord.Right_down_x = pt2.x;
                        coord.Right_down_y = pt2.y;
                        coord.Height = rect[i].height;
                        coord.Width = rect[i].width;
                        m_coord.push_back(coord);
                        rectangle(image, pt1, pt2, CV_RGB(255,0,0), 2);
                    }
                }
            }
        }
    }
}





/*************夜晚静态图片识别(本地函数)*****************************/
JNIEXPORT void JNICALL
Java_cn_edu_nwpu_cmake_MainActivity_nightNativeImageProcess(JNIEnv *, jobject, jlong inputImg,
                                                       jlong outputImg ) {
    Mat &src = *(Mat *) inputImg;
    Mat &outImg = *(Mat *) outputImg;//输出图像
    resize(src,src,Size(1920,1080),0,0,INTER_LINEAR);//需要resize才能识别出来
    cvtColor(src, src, COLOR_RGBA2BGR);
    imageRoI = src(Rect(src.cols/3, src.rows / 5, src.cols/3, src.rows / 2));
    Mat dst2, gray;
    //顶帽彩色变换
    morphologyEx(imageRoI, dst2, MORPH_TOPHAT, getStructuringElement(MORPH_RECT, Size(25, 25)));

    //HSV颜色空间根据亮度V进行分割
    cvtColor(dst2, hsvimage, CV_BGR2HSV);
    cvtColor(dst2, gray, COLOR_RGB2GRAY);
    float H, S, V;
    for (int y = 0; y < hsvimage.rows; y++) {
        for (int x = 0; x < hsvimage.cols; x++) {

            V = hsvimage.at<Vec3b>(y, x)[2];  //V;0-255
            if (V >= 205)

                gray.at<uchar>(y, x) = 255;

            else
                gray.at<uchar>(y, x) = 0;
        }
    }

// 	IplImage temp=(IplImage)gray;
// 	//IplImage *ipl_gray=&temp;
// 	//cvShowImage("ipl",ipl_gray);
// 	ConnectEdge(&temp);
// 	Mat zGray=Mat(&temp);
//  imshow("边缘修补",zGray);

    Mat close;
    Mat element1 = getStructuringElement(MORPH_RECT, Size(1, 5));    // 左右两个联通区域相距小于等于1时，将它们连在一起形成一个联通区域，上下两个连通区域小于等于7时，将它们连在一起
    morphologyEx(gray, close, MORPH_CLOSE, element1);                 //闭运算将间隔较小距离的联通区域连接起来，可以减少联通区域个数

    Mat closeClone = close.clone();
    Mat closeClone2 = close.clone();
    Mat imageRoIclone = imageRoI.clone();
    GetContours1(imageRoIclone, closeClone, closeClone2);//轮廓

    cvtColor(imageRoI, hsvimage, CV_BGR2HSV);  //转换到HSV空间
    sort(m_coord.begin(), m_coord.end(), GreaterSort);//升序排列

    if (m_coord.size() == 4) {
        for (int i = 0; i < m_coord.size(); i++) {
            //if(coord.Height/coord.Width<1.0)
            if (i != 1 && i != 2) {
                char color[6];
                Color(hsvimage, m_coord[i].Top_left_x, m_coord[i].Top_left_y, m_coord[i].Right_down_x, m_coord[i].Right_down_y);//颜色识别
                sprintf(color, "%s", State);
                putText(imageRoIclone, color,
                        Point(m_coord[i].Top_left_x, m_coord[i].Top_left_y - 50), 3, 1, Scalar(0, 0, 255));
            }
        }
    }

    if (m_coord.size() == 3) {
        for (int i = 0; i < m_coord.size(); i++) {
            //if(coord.Height/coord.Width<1.0)
            if (i != 1) {
                char color[6];
                Color(hsvimage, m_coord[i].Top_left_x, m_coord[i].Top_left_y,
                      m_coord[i].Right_down_x, m_coord[i].Right_down_y);//颜色识别
                sprintf(color, "%s", State);
                putText(imageRoIclone, color,
                        Point(m_coord[i].Top_left_x, m_coord[i].Top_left_y - 50), 3, 1,
                        Scalar(0, 0, 255));
            }
        }
    }

    Mat image_little;
    if (m_coord.size() == 3) {
        image_little = close(Rect(m_coord[1].Top_left_x, m_coord[1].Top_left_y, m_coord[1].Width,
                                  m_coord[1].Height));
    }
    else if (m_coord.size() == 4) {
        image_little = close(Rect(m_coord[1].Top_left_x, m_coord[1].Top_left_y,
                                  m_coord[1].Width + m_coord[2].Width + 25, m_coord[1].Height));
    }

    resize(image_little, image_little, Size(image_little.cols * 5, image_little.rows * 5), 0, 0, 3); //放大抠图

    CvSeq *contours = NULL;
    CvMemStorage *storage = cvCreateMemStorage(0);
    Mat image2 = image_little.clone();

    IplImage *IplImg; /* =cvCreateImage(Size(200,200),8, 1)*///Mat 到IplImage *
    IplImage temp = (IplImage) image2;
    IplImg = &temp;

    IplImage *IplImg1;
    IplImg1 = cvCloneImage(IplImg);//找轮廓之前一定要复制一个   找完轮廓图片像素会变

    int count = cvFindContours(IplImg, storage, &contours, sizeof(CvContour), CV_RETR_EXTERNAL);

    //printf("The count is :%d\n", count);
    int idx = 0;
    char szName[56] = {0};
    int tempcount = 0;
    CvSeq *c = contours;
    for (; c != NULL; c = c->h_next) {
        CvRect rc = cvBoundingRect(c, 0);
        double tmparea = fabs(cvContourArea(c));
        IplImage *sub_img = cvCreateImage(cvSize(rc.width, rc.height), IplImg1->depth,
                                          IplImg1->nChannels);
        cvSetImageROI(IplImg1, rc);
        cvCopyImage(IplImg1, sub_img);
        cvResetImageROI(IplImg1);
        sprintf(szName, "win_%d", idx++);
        detect(sub_img, szName);   //DETECT NUMBER
        //cvReleaseImage(&sub_img);
    }
    for (int i = 0; i < l; i++) {
        for (int j = 0; i + j < l - 1; j++) {
            if (List[j][0] > List[j + 1][0]) {
                //坐标排序
                int temp = List[j][0];
                List[j][0] = List[j + 1][0];
                List[j + 1][0] = temp;
                //数值随之变化
                temp = List[j][1];
                List[j][1] = List[j + 1][1];
                List[j + 1][1] = temp;
            }
        }
    }
    int m = List[1][1];
    int n = List[0][1];
    int Num = 0;
    if (m_State_Count == 1) {
        printf("当前数字是\t%d ", List[0][1]);
        Num = List[0][1];
    }
    else if (m_State_Count == 2) {
        printf("当前数字是\t%d ", List[0][1] * 10 + List[1][1]);
        Num = List[0][1] * 10 + List[1][1];
    }

    char str_lx[6];
    sprintf(str_lx, "%d", Num);
    putText(imageRoIclone, str_lx, Point(m_coord[1].Top_left_x, m_coord[1].Top_left_y - 27), 3, 1,
            Scalar(0, 0, 255));

    Mat imageRoI1gray;
    cvtColor(imageRoIclone, imageRoI1gray, COLOR_BGR2GRAY);
    Mat mask = imageRoI1gray.clone();       //掩膜必须为灰度图
    imageRoIclone.copyTo(imageRoI, mask);
    resize(src, src, Size(1920, 1080), 0, 0, INTER_LINEAR);
    cvtColor(src, src, COLOR_BGR2RGBA);
    outImg=src.clone();
}



/*******************************RGB方法****************/
   /* Mat imageRoI1;
    Mat imageRoI2;
    int B = 0,G=0,R=0;
    cvtColor(imageRoI,imageRoI1,COLOR_RGB2GRAY);//  Mat  得到两个和imageRoI图像一样大小的灰度图
    cvtColor(imageRoI,imageRoI2,COLOR_RGB2GRAY);//  Mat   得到两个和imageRoI图像一样大小的灰度图

    for(int j=0;j<imageRoI.rows;j++)   //行数
        for (int i=0;i<imageRoI.cols;i++)
        {
            {
                B = imageRoI.at<cv::Vec3b>(j,i)[0];
                G = imageRoI.at<cv::Vec3b>(j,i)[1];
                R = imageRoI.at<cv::Vec3b>(j,i)[2];
                if (max(max(B,G),R)<20)
                {
                    imageRoI1.at<uchar>(j,i) = 255;

                }
                else
                {
                    imageRoI1.at<uchar>(j,i) = 0;
                }
                if (max(max(abs(R-G),abs(R-B)),abs(G-B))<60)
                {
                    imageRoI2.at<uchar>(j,i) = 255;
                }
                else
                {
                    imageRoI2.at<uchar>(j,i) = 0;
                }
            }
        }
    for(int j=0;j<imageRoI1.rows;j++)   //比较两个imageRoI1图片和imageRoI2图片的像素值   也就是并
        for (int i=0;i<imageRoI1.cols;i++)
        {
            {
                int b = imageRoI1.at<uchar>(j,i);
                int c = imageRoI2.at<uchar>(j,i);
                if (b==255||c==255)
                {
                    imageRoI2.at<uchar>(j,i) = 0;//(x,y)点的像素值，取大的 （一个0一个255取255，两个都是0取0）
                }
                else
                {
                    imageRoI2.at<uchar>(j,i) = 255;
                }
            }
        }*/



/****************顶帽+V+HSV 方法*******************/
    //彩色图像直方图均衡化,顶帽变换后效果较好
    /*dst1=equalizeIntensityHist(imageRoI);

    //顶帽彩色变换
    morphologyEx(dst1, dst2, MORPH_TOPHAT, getStructuringElement(MORPH_RECT, Size(15,15)));

    //HSV颜色空间根据亮度V进行分割
    cvtColor(dst2, hsvimage, CV_BGR2HSV);
    cvtColor(dst2, gray,COLOR_RGB2GRAY);
    float H, S, V;
    for (int y = 0; y < hsvimage.rows; y++)
    {
        for (int x = 0; x < hsvimage.cols; x++)
        {
            V =  hsvimage.at<Vec3b>(y, x)[2];  //V;0-255
            if(V>=90)

                gray.at<uchar>(y,x) =255;

            else
                gray.at<uchar>(y,x) =0;
        }
    }

    cvtColor(imageRoI , hsvimage1, CV_BGR2HSV);
    cvtColor(imageRoI, gray1,COLOR_RGB2GRAY);

    for (int y = 0; y < gray1.rows; y++)
    {
        //uchar* data = dst.ptr<uchar>(y);
        for (int x = 0; x < gray1.cols; x++)
        {
            H =  hsvimage1.at<Vec3b>(y, x)[0];  //H;0-180
            S =  hsvimage1.at<Vec3b>(y, x)[1];  //S:0-255
            V =  hsvimage1.at<Vec3b>(y, x)[2];  //V;0-255
            H=H*2;
            S=S/255;
            //这个阈值很重要,需要实验确定
            if(V>90&&V<255)
            {
                if(0.2<S&&S<0.85)
                {
                    if(0<H&&H<50||140<H&&H<280||330<H&&H<360)
                    {
                        gray1.at<uchar>(y,x) =255;
                    }

                    else
                        gray1.at<uchar>(y,x) =0;
                }
                else
                    gray1.at<uchar>(y,x) =0;
            }
            else
                gray1.at<uchar>(y,x) =0;
        }
    }
    Mat hole;
    fillHole(gray1,hole);//HSV阈值化后空洞填充,保证信号灯区域都保留于图片中

    Mat element = getStructuringElement( MORPH_RECT,Size(9,9));//改变Size大小可以改变矩形框的大小
    dilate( hole, dst3, element );

    for(int j=0;j<dst3.rows;j++)   //比较两个imageRoI1图片和imageRoI2图片的像素值   也就是并
        for (int i=0;i<dst3.cols;i++)
        {
            {
                int b = dst3.at<uchar>(j,i);
                int c = gray.at<uchar>(j,i);
                if (b==0||c==0)
                {
                    gray1.at<uchar>(j,i) = 0;//(x,y)点的像素值，取大的 （一个0一个255取255，两个都是0取0）
                }
                else
                {
                    gray1.at<uchar>(j,i) =255;
                }
            }
        }




   /****************顶帽+HSV 方法*******************/
    /*Mat dst;
     resize(srcImg,dst,Size(2080*(3/2),1560*(3/2)), (0, 0), (0, 0), INTER_LINEAR);
    Mat imageRoI = dst(Rect(0,dst.rows/3,dst.cols,dst.rows/3));//将图片横向平均分为3份，提取最中间的那一份图片*/
   /* dst1=equalizeIntensityHist(imageRoI);//彩色图像直方图均衡化
    ////////////顶帽变换/////////////////////
    cvtColor(dst1, dst2,COLOR_RGB2GRAY);
    morphologyEx(dst2, dst2, MORPH_TOPHAT, getStructuringElement(MORPH_RECT, Size(15,15)));
    threshold( dst2,dst3,80,255,3);//这个阈值很重要,需要实验确定
    //////////////////////HSV阈值分割/////////////////////////
    GaussianBlur(dst1 ,dst4,Size(5,5),0,0);//直方图均衡化后进行高斯滤波
    cvtColor(dst4, hsvimage, CV_BGR2HSV);
    cvtColor(imageRoI,gray,COLOR_RGB2GRAY);
    float H, S, V;
    for (int y = 0; y < gray.rows; y++)
    {
        for (int x = 0; x < gray.cols; x++)
        {
            H =  hsvimage.at<Vec3b>(y, x)[0];  //H;0-180
            S =  hsvimage.at<Vec3b>(y, x)[1];  //S:0-255
            V =  hsvimage.at<Vec3b>(y, x)[2];  //V;0-255
            H=H*2;
            S=S/255;
            if(V>90&&V<255)        //这个阈值很重要,需要实验确定
            {
                if(0.2<S&&S<0.85)
                {
                    if(0<H&&H<50||140<H&&H<280||330<H&&H<360)
                    {
                        gray.at<uchar>(y,x) =255;
                    }

                    else
                        gray.at<uchar>(y,x) =0;
                }
                else
                    gray.at<uchar>(y,x) =0;
            }
            else
                gray.at<uchar>(y,x) =0;
        }
    }
    Mat hole;
    fillHole(gray,hole);//HSV阈值化后空洞填充,保证信号灯区域都保留于图片中
   /* Mat element = getStructuringElement( MORPH_RECT,Size(9,9));//改变Size大小可以改变矩形框的大小
    dilate( hole, dst5, element );//空洞填充后膨胀,确保信号灯区域都保留于图片中*/

    /////////////////////////////////顶帽结果与HSV阈值化做与运算///////////////////
    /*for(int j=0;j<dst3.rows;j++)   //比较两个imageRoI1图片和imageRoI2图片的像素值
        for (int i=0;i<dst3.cols;i++)
        {
            {
                int b = hole.at<uchar>(j,i);
                int c = dst3.at<uchar>(j,i);
                if (b==0||c==0)
                {
                    dst3.at<uchar>(j,i) = 0;//同取同，不同取0
                }
                else
                {
                    dst3.at<uchar>(j,i) =255;
                }
            }
        }
    outImg=dst3.clone();
    int n=4;
    return n;*/
//}


/*JNIEXPORT void JNICALL Java_cn_edu_nwpu_cmake_SecondActivity_FindFeatures(JNIEnv*, jobject, jlong addrin, jlong addrRgba)
{
    Mat& mRgb = *(Mat*)addrRgba;
    Mat& out = *(Mat*)addrin;
    Mat mGr;
    cvtColor(out,mGr,COLOR_RGBA2GRAY);
    vector<KeyPoint> v;

    FastFeatureDetector detector(50);
    detector.detect(mGr, v);//在灰度图上检测
    for( unsigned int i = 0; i < v.size(); i++ )
    {
        const KeyPoint& kp = v[i];
        circle(mRgb, Point(kp.pt.x, kp.pt.y), 10, Scalar(255,0,0,255));//在原图上画圆
    }
}*/



/** @function main */
/*JNIEXPORT void JNICALL Java_cn_edu_nwpu_cmake_SecondActivity_FindFeatures(JNIEnv*, jobject, long addrin, long addrRgba) {
    Mat &mRgb = *(Mat *) addrRgba;
    Mat& input = *(Mat*)addrin;
    String face_cascade_name = "haarcascade_frontalface_alt.xml";
    String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
    CascadeClassifier face_cascade;
    CascadeClassifier eyes_cascade;
    RNG rng(12345);
    std::vector<Rect> faces;
    Mat frame_gray;
    face_cascade.load( face_cascade_name);
    eyes_cascade.load( eyes_cascade_name ) ;

    cvtColor(input, frame_gray, CV_RGBA2GRAY );
    equalizeHist( frame_gray, frame_gray );

    //-- Detect faces
    face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(0, 0) );

    for( size_t i = 0; i < faces.size(); i++ )
    {
        Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
        ellipse( mRgb, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );

        Mat faceROI = frame_gray( faces[i] );
        std::vector<Rect> eyes;

        //-- In each face, detect eyes
        eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE, Size(30, 30) );

        for( size_t j = 0; j < eyes.size(); j++ )
        {
            Point center( faces[i].x + eyes[j].x + eyes[j].width*0.5, faces[i].y + eyes[j].y + eyes[j].height*0.5 );
            int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
            circle( mRgb, center, radius, Scalar( 255, 0, 0 ), 4, 8, 0 );
        }
    }

}*/





/*************实时识别(本地函数)*****************************/
int m_State_Count_V = 0;      //数字位数
char State_V[2] = " ";   //颜色状态  红灯或者绿灯   黄灯

int m_Coolr_V = -1;

int m_Red_Num_V = 0;
int m_Yellow_Num_V = 0;
int m_Green_Num_V = 0;
bool m_Red_State_V = false;
bool m_Yellow_State_V = false;
Mat hsvimage_v,hole_v;
bool m_Green_State_V = false;

int List_V[10][2] = {-1};  //1列坐标，2列字符数值
int l_v = 0;  //字符个数

struct Coordinate_V
{
    int Top_left_x;
    int Top_left_y;
    int Right_down_x;
    int Right_down_y;
    int Width;
    int Height;
};
std::vector<Coordinate_V>  m_coord_v;    //第一次检测框  找灯框
std::vector<Coordinate_V>  m_coord_cut_v;
void detect_v(IplImage * img, char * strnum);
bool GreaterSort_V (Coordinate_V a,Coordinate_V b) { return (a.Top_left_x<b.Top_left_x); }  //降序排列

bool GetSort_y_V (Coordinate_V a,Coordinate_V b) { return (a.Top_left_y<b.Top_left_y); }  //降序排列


/*******孔洞填充*********/
void fillHole_v(const Mat srcBw, Mat &dstBw)
{
    Size m_Size = srcBw.size();
    Mat Temp=Mat::zeros(m_Size.height+2,m_Size.width+2,srcBw.type());//延展图像
    srcBw.copyTo(Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)));

    cv::floodFill(Temp, Point(0, 0), Scalar(255));

    Mat cutImg;//裁剪延展的图像
    Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)).copyTo(cutImg);

    dstBw = srcBw | (~cutImg);
}


void Recognition_V(Mat image)//  检测框
{
    Mat gray1,gray2,HSVimage;
    cvtColor(image,gray1,COLOR_BGR2GRAY);
    cvtColor(image,gray2,COLOR_BGR2GRAY);
    cvtColor(image, HSVimage, COLOR_BGR2HSV);
    int R=0,G=0,B=0;
    float V=0;

    for(int j=0;j<image.rows;j++)
        for (int i=0;i<image.cols;i++)
        {
            {
                B=image.ptr<uchar>(j,i)[0];
                G=image.ptr<uchar>(j,i)[1];
                R=image.ptr<uchar>(j,i)[2];
                V= HSVimage.ptr<uchar>(j,i)[2];

                if ((R+G+B)/3<85)  //颜色特征           //图片像素降低时，需要适当增大阈值   原70
                {
                    gray1.at<uchar>(j,i) = 255;

                }
                else
                {
                    gray1.at<uchar>(j,i) = 0;
                }


                if(V<90)  //亮度特征                     //图片像素降低时，需要适当增大阈值
                {
                    gray2.at<uchar>(j,i) = 255;

                }
                else
                {
                    gray2.at<uchar>(j,i) = 0;
                }
            }
        }

    for(int j=0;j<gray1.rows;j++)   //比较两个图片的像素值   也就是并
        for (int i=0;i<gray1.cols;i++)
        {
            {
                int b = gray1.at<uchar>(j,i);
                int c = gray2.at<uchar>(j,i);
                if (b==255||c==255)
                {
                    gray1.at<uchar>(j,i) = 255;//(x,y)点的像素值，取大的 （一个0一个255取255，两个都是0取0）
                }
                else
                {
                    gray1.at<uchar>(j,i) = 0;
                }

            }
        }
    //	imshow("综合背板1",gray1);
    //	imwrite("3000.jpg",gray1);

    /***********边缘修补*****/

    /*IplImage temp=(IplImage)gray1;
    IplImage* input=&temp;

    ConnectEdge(input);      //边缘修补


    Mat input1(input, true);*/
    //imshow("边缘修补",input1);
    //imwrite("3001.jpg",gray1);
    /***********孔洞填充*****/
    fillHole_v(gray1,hole_v);
    //imshow("孔洞填充",hole);
    //	imwrite("3002.jpg",hole);

    Mat element = getStructuringElement( MORPH_RECT,Size(5,9));   //图片像素降低时，size需要减小
    morphologyEx( hole_v, hole_v, MORPH_OPEN,element );                   //开运算去除比探针小的孤立噪声点
    //	imshow("开运算",hole);
    //	imwrite("3003.jpg",hole);

    //waitKey(0);
    Mat element1 = getStructuringElement( MORPH_RECT,Size(11,11));    // 左右两个联通区域相距小于等于15时，将它们连在一起形成一个联通区域，上下两个连通区域小于等于13时，将它们连在一起
    morphologyEx( hole_v, hole_v, MORPH_CLOSE,element1);                 //闭运算将间隔较小距离的联通区域连接起来，可以减少联通区域个数
    //   imshow("闭运算",hole);
}

void GetContours_V(Mat image,Mat dst,Mat dst1)//轮廓
{
    vector<std::vector<cv::Point> >coutours;
    findContours(dst,coutours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0));
    drawContours(dst1,coutours,-1,Scalar(100),2);
    //	imshow("binayu",dst);
    //	imshow("luokuo2",dst1);
    double a;
    for (int i=0;i<coutours.size();i++)
    {
        vector<Rect> rect(coutours.size());
        rect[i]= boundingRect(Mat(coutours[i]));
        //根据面积，长宽比，坐标位置，排除不符合的矩形框
        //a =rect[i].height/rect[i].width;
        double H = rect[i].height;
        double W = rect[i].width;
        a = H / W;
        double carea=contourArea(coutours[i]);
        double area=rect[i].height*rect[i].width;
        //cout<<"轮廓面积  =  "<<carea<< "   。  "<<"  外接矩形面积 =  "<<area<<endl;

        //  两个箭头框 检测出来
        //if(3< H&&H <45&&1< W&&W <45)//长宽
        if(area>150&&area<1500/*1800*/){
            if((2.0<a&&a<3.2))
            {
                if (rect[i].x > 100)//&&rect[i].y<image.rows/3){//&&rect[i].x<src.cols*2/3)
                {
                    if(carea/area>0.7)
                    {
                        CvPoint pt1,pt2;
                        pt1.x = rect[i].x;
                        pt1.y = rect[i].y;
                        pt2.x = rect[i].x + rect[i].width ;
                        pt2.y = rect[i].y + rect[i].height;

                        Coordinate_V coord;
                        coord.Top_left_x = pt1.x;
                        coord.Top_left_y = pt1.y;
                        coord.Right_down_x = pt2.x;
                        coord.Right_down_y = pt2.y;
                        coord.Height = rect[i].height;
                        coord.Width = rect[i].width;
                        m_coord_v.push_back(coord);
                        //	rectangle(image, pt1, pt2, CV_RGB(255,0,0), 2);
                    }
                }
            }
        }
    }
    /*imshow("裁剪区域",image);*/
}

void DrawContours_V(Mat image)//画轮廓
{
    if (m_coord_v.size())
    {
        sort(m_coord_v.begin(),m_coord_v.end(),GetSort_y_V);  //升序排列   按y位置  从上到下排列
    }
    if(m_coord_v.size() == 1)
    {
        CvPoint pt1,pt2;
        pt1.x = m_coord_v[0].Top_left_x;
        pt1.y = m_coord_v[0].Top_left_y;
        pt2.x = m_coord_v[0].Right_down_x;
        pt2.y = m_coord_v[0].Right_down_y;
        rectangle(image, pt1, pt2, CV_RGB(255,0,0), 2);
    }
    else if (m_coord_v.size() > 1)
    {
        for (int i = 0;i<2;i++)
        {
            CvPoint pt1,pt2;
            pt1.x = m_coord_v[i].Top_left_x;
            pt1.y = m_coord_v[i].Top_left_y;
            pt2.x = m_coord_v[i].Right_down_x;
            pt2.y = m_coord_v[i].Right_down_y;
            rectangle(image, pt1, pt2, CV_RGB(255,0,0), 2);
        }
    }
}
void Color_V(Mat image,int x1,int y1,int x2,int y2)//颜色识别
{
    float h = 0;
    float H = 0;

    /*  颜色识别 */
    for(int j=y1;j<y2;j++)  //0表示检测到的联通区域标号  第几个
    {
        for (int i=x1;i<x2;i++)
        {
            H=  image.ptr<uchar>(j,i)[0];
            h = H / 180;
            if (h>0.4&&h<0.55)
            {
                m_Green_Num_V ++;
            }
            else if (h>0.06&&h<0.18)
            {
                m_Yellow_Num_V ++;
            }
            else if ((h>0.95&&h<1)||(h>0&&h<0.06))
            {
                m_Red_Num_V ++;
            }
        }
    }
    int m_Max = max(max(m_Green_Num_V,m_Yellow_Num_V),m_Red_Num_V);
    if (m_Max == m_Red_Num_V)
    {
        m_Red_State_V = true;
        m_Coolr_V = 0;
    }
    else if(m_Max == m_Green_Num_V)
    {
        m_Green_State_V = true;
        m_Coolr_V = 1;
    }
    else if(m_Max == m_Yellow_Num_V)
    {
        m_Yellow_State_V = true;
        m_Coolr_V = 2;
    }
    if(m_Green_State_V)
    {
        strcpy(State_V,"G");//拷贝字符
    }
    else if(m_Yellow_State_V)
    {
        strcpy(State_V,"Y");//拷贝字符
    }
    else if(m_Red_State_V)
    {
        strcpy(State_V,"R");//拷贝字符
    }
    m_Green_Num_V = 0;
    m_Yellow_Num_V = 0;
    m_Red_Num_V = 0;
    m_Green_State_V = false;
    m_Yellow_State_V = false;
    m_Red_State_V = false;

//	printf("%s\t",State);
}


void detect_v(IplImage * img, char * strnum/*, CvRect rc*/)
{
    m_State_Count_V++;
    //List[l][0] = rc.x;  //第l个字符横坐标

    //if the height is more two longer than width,number is 1
    if((img->height/img->width)>2)
    {
        List_V[l_v][1] = 1;
        //printf("字符是 1.\n");
    }
    else
    {
        int ld[3] = {0};
        CvScalar pix;

        int i = img->width/2;
        int j = img->height/3;
        int k = img->height*2/3;

        //竖向扫描
        for(int m=0; m<3; m++)
        {
            for(int n=m*j; n<(m+1)*j; n++)
            {
                pix = cvGet2D(img, n, i);
                if(pix.val[0]==255)
                {
                    ld[0]++;
                    break;
                }
            }
        }
        //printf("竖向 %d\n", ld[0]);

        //横向扫描
        for(int m=0; m<2; m++)
        {
            for(int n=m*i; n<(m+1)*i; n++)
            {
                //横向1/3扫描
                pix = cvGet2D(img, j, n);
                if(pix.val[0]==255)
                {
                    ld[1]++;
                    break;
                }
            }
        }
        //printf("横向1 %d\n", ld[1]);

        //横向扫描
        for(int m=0; m<2; m++)
        {
            for(int n=m*i; n<(m+1)*i; n++)
            {
                //横向2/3扫描
                pix = cvGet2D(img, k, n);
                if(pix.val[0]==255)
                {
                    ld[2]++;
                    break;
                }
            }
        }
        //printf("横向2 %d\n", ld[2]);

        if((ld[0]==2)&&(ld[1]==2)&&(ld[2]==2))
        {
            //printf("字符是 0\n");
            List_V[l_v][1] = 0;
        }
        else if((ld[0]==1)&&(ld[1]==2)&&(ld[2]==1))  //ld[1]  上面横穿
        {
            //printf("字符是 4\n");
            List_V[l_v][1] = 4;
        }
        else if((ld[0]==3)&&(ld[1]==1)&&(ld[2]==2))
        {
            //	printf("字符是 6\n");
            List_V[l_v][1] = 6;
        }
        else if((ld[0]==1)&&(ld[1]==1)&&(ld[2]==1))
        {
            //printf("字符是 7\n");
            List_V[l_v][1] = 7;
        }
        else if((ld[0]==3)&&(ld[1]==2)&&(ld[2]==2))
        {
            //printf("字符是 8\n");
            List_V[l_v][1] = 8;
        }
        else if((ld[0]==3)&&(ld[1]==2)&&(ld[2]==1))
        {
            //printf("字符是 9\n");
            List_V[l_v][1] = 9;
        }


        else if((ld[0]==3)&&(ld[1]==1)&&(ld[2]==1))
        {
            int l1=0, l2=0;
            //横向扫描
            int k1 = img->height/3;
            for(int i1=0; i1<img->width/2; i1++)
            {
                //横向2/3扫描
                pix = cvGet2D(img, k1, i1);
                if(pix.val[0]==255)
                {
                    l1++;
                    break;
                }
            }
            //横向扫描
            int k2 = img->height*2/3;
            for(int i2=0; i2<img->width/2; i2++)
            {
                //横向2/3扫描
                pix = cvGet2D(img, k2, i2);
                if(pix.val[0]==255)
                {
                    l2++;
                    break;
                }
            }
            if((l1==0)&&(l2==0))
            {
                //printf("字符是 3\n");
                List_V[l_v][1] = 3;
            }
            else if((l1==0)&&(l2==1))
            {
                //printf("字符是 2\n");
                List_V[l_v][1] = 2;
            }
            else if((l1==1)&&(l2==0))
            {
                //printf("字符是 5\n");
                List_V[l_v][1] = 5;
            }
        }
        //else printf("识别失败\n");
    }
    l_v++;
}


JNIEXPORT  int JNICALL
Java_cn_edu_nwpu_cmake_SecondActivity_nativeVideoProcess(JNIEnv *, jobject, jlong BGRImage,
                                                         jlong outImage) {
    Mat &src = *(Mat *) BGRImage;
    Mat &outImg = *(Mat *) outImage;//输出图像
    cvtColor(src, src, COLOR_RGBA2BGR);
    vector<Mat>  ImageNum;

    m_coord_v.clear();
    m_coord_cut_v.clear();
    l_v = 0;
    memset(List_V,0,20);   //置零
    Mat image,imageRoI,Src_cut;                                    //C0053:数字分割阈值：215。1,7帧多框一个框。380多框两个框。10帧程序停工。646帧及以后面积太大框不住
    Mat hsvimage_cut,gray;

    imageRoI = src(Rect(src.cols/3,src.rows/5,src.cols*2/3,src.rows*2/3));//src.rows*2/3));
    image = imageRoI.clone();
    Src_cut = image.clone();

    cvtColor(image, hsvimage_v, COLOR_BGR2HSV);  //转换到HSV空间

    //double t = (double)getTickCount();

    Recognition_V(image);//检测实心灯框

    Mat image1 = image.clone();//原图感兴趣区域，即image
    Mat dst =hole_v.clone() ;//复制图片,二值图

    Mat dst1=dst.clone();

    GetContours_V(image,dst,dst1);//找到两个灯框位置轮廓

    DrawContours_V(image);//画灯框轮廓    20170825 新加

    if(m_coord_v.size())
    {
        sort(m_coord_v.begin(),m_coord_v.end(),GreaterSort_V);//升序排列
    }

    Mat image_cut;   //只裁剪灯和背板框         只裁剪数字框
    if (m_coord_v.size()>1)
    {
        int x0 = m_coord_v[0].Top_left_x;
        int y0 = m_coord_v[0].Top_left_y;
        int m_Width = m_coord_v[0].Width;
        int m_Height = m_coord_v[0].Height;

        int x1 = m_coord_v[1].Top_left_x;

        //两个背板框中间的灯框（不包括背板框）
        image_cut = Src_cut(Rect(x0+m_Width,y0,x1 - x0-m_Width, m_Height));
    }
    else
    {
        image_cut = Src_cut(Rect(100,100,10, 10));
    }
    //imshow("数字区域",image_cut);

    Mat image_cut1 = image_cut.clone();
    Mat image_cut2 = image_cut.clone();

    //HSV颜色空间根据亮度V进行分割
    cvtColor(image_cut2, hsvimage_cut, CV_BGR2HSV);
    cvtColor(image_cut1, gray,COLOR_RGB2GRAY);
    float  V;

    for (int y = 0; y < hsvimage_cut.rows; y++)
    {
        for (int x = 0; x < hsvimage_cut.cols; x++)
        {

            V =  hsvimage_cut.at<Vec3b>(y, x)[2];  //V;0-255

            if(V>=215)//YINYU:230，C0044：220
                gray.at<uchar>(y,x) =255;
            else
                gray.at<uchar>(y,x) =0;
        }
    }

    /******闭运算作用：连接数字断裂处********/
    Mat element1 = getStructuringElement( MORPH_RECT,Size(1,2));
    morphologyEx( gray, gray, MORPH_CLOSE,element1);


    Mat gray2=gray.clone();
    Mat GrayCut=gray.clone();
    vector<std::vector<cv::Point> >coutours_cut;

    findContours(gray,coutours_cut,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0));

    double a1;
    for (int i=0;i<coutours_cut.size();i++)
    {
        vector<Rect> rect(coutours_cut.size());
        rect[i]= boundingRect(Mat(coutours_cut[i]));
        //根据面积，长宽比，坐标位置，排除不符合的矩形框
        double h = rect[i].height;
        double W = rect[i].width;
        a1 = h/ W;

        float area=rect[i].height*rect[i].width;
        if(area>5&&area<500)
        {
            if((1.5<a1&&a1<2.6)||3<a1&&a1<12)
            {
                if(h <30&&W <30)//长宽
                {
                    if(rect[i].y+rect[i].height>image_cut.rows/2||W>3){   //排除似1的干扰物
                        CvPoint pt1,pt2;
                        pt1.x = rect[i].x;
                        pt1.y = rect[i].y;
                        pt2.x = rect[i].x + rect[i].width ;
                        pt2.y = rect[i].y + rect[i].height;

                        Coordinate_V coord_cut;
                        coord_cut.Top_left_x = pt1.x;
                        coord_cut.Top_left_y = pt1.y;
                        coord_cut.Right_down_x = pt2.x;
                        coord_cut.Right_down_y = pt2.y;
                        coord_cut.Height = rect[i].height;
                        coord_cut.Width = rect[i].width;
                        m_coord_cut_v.push_back(coord_cut);

                        //   rectangle(image_cut, pt1, pt2, CV_RGB(0,0,255), 1);
                    }
                }
            }
        }
    }

    if (m_coord_cut_v.size())
    {
        sort(m_coord_cut_v.begin(),m_coord_cut_v.end(),GreaterSort_V);//升序排列
    }

    //imshow("数字裁剪轮廓",image_cut);

    if (m_coord_v.size()>0)
    {
        if (m_coord_cut_v.size())
        {
            char color[6];
            Color_V( hsvimage_cut,m_coord_cut_v[0].Top_left_x,m_coord_cut_v[0].Top_left_y,m_coord_cut_v[0].Right_down_x,m_coord_cut_v[0].Right_down_y);//颜色识别
            char m_color[2];//颜色修改
            strcpy(m_color,"G");
            //  sprintf(color,"%s",m_color);//颜色修改
            sprintf(color,"%s",State_V);//   识别颜色
            for (int i=0;i<m_coord_v.size();i++)   //颜色识别  写颜色
            {
                putText(image,color,Point(m_coord_v[i].Top_left_x-m_coord_v[i].Width/3,m_coord_v[i].Top_left_y-m_coord_v[i].Width),3,1,Scalar(0,0,255));
            }
        }
    }


    Mat image_little,image_little1;

    Mat GrayCut1=GrayCut.clone();
    ImageNum.clear();
    if (m_coord_cut_v.size()==1){

        image_little = GrayCut(Rect(m_coord_cut_v[0].Top_left_x,m_coord_cut_v[0].Top_left_y,m_coord_cut_v[0].Width,m_coord_cut_v[0].Height));
        ImageNum.push_back(image_little);
        resize(ImageNum[0],ImageNum[0],Size(10*m_coord_cut_v[0].Width,10*m_coord_cut_v[0].Height),0,0,INTER_NEAREST);

    }else if (m_coord_cut_v.size()==2){

        image_little = GrayCut1(Rect(m_coord_cut_v[0].Top_left_x,m_coord_cut_v[0].Top_left_y,m_coord_cut_v[0].Width,m_coord_cut_v[0].Height));
        ImageNum.push_back(image_little);
        image_little1 = GrayCut1(Rect(m_coord_cut_v[1].Top_left_x,m_coord_cut_v[1].Top_left_y,m_coord_cut_v[1].Width,m_coord_cut_v[1].Height));
        ImageNum.push_back(image_little1);
        resize(ImageNum[0],ImageNum[0],Size(10*m_coord_cut_v[0].Width,10*m_coord_cut_v[0].Height),0,0,INTER_NEAREST);
        resize(ImageNum[1],ImageNum[1],Size(10*m_coord_cut_v[1].Width,10*m_coord_cut_v[1].Height),0,0,INTER_NEAREST);
        /*resize(image_little,image_little,Size(10*Width,10*Height),0,0,INTER_NEAREST); //放大抠图,最后一个参数代表插图方式，0,3都可以,INTER_AREA或者*/

    }else	{

        image_little = image1(Rect(100,27,55,55));   //无数字
    }

    /*        数字识别          */
    int idx = 0;
    char szName[56] = {0};
    if (ImageNum.size() == 1)
    {
        IplImage temp=(IplImage)ImageNum[0];
        IplImage* IplImg=&temp;
        sprintf(szName, "Number_%d", idx++);
        detect_v(IplImg, szName);
        //cvSaveImage("1.bmp",IplImg);
    }
    else if(ImageNum.size() == 2)
    {
        for (int i=0;i<ImageNum.size();i++)
        {
            IplImage temp=(IplImage)ImageNum[i];
            IplImage* IplImg=&temp;
            sprintf(szName, "Number_%d", idx++);
            detect_v(IplImg, szName);
        }
    }

    //	printf("\字符识别结果为：");
    for(int i=0; i<l_v; i++)
    {
        for(int j=0; i+j<l_v-1; j++)
        {
            if(List_V[j][0] > List_V[j + 1][0])
            {
                //坐标排序
                int temp = List_V[j][0];
                List_V[j][0] = List_V[j + 1][0];
                List_V[j + 1][0] = temp;
                //数值随之变化
                temp = List_V[j][1];
                List_V[j][1] = List_V[j+1][1];
                List_V[j+1][1] = temp;
            }
        }
    }
    int m = List_V[1][1];
    int n = List_V[0][1];
    int Num = 0;
    if (m_State_Count_V == 1)
    {
        //	printf("当前数字是\t%d ",List[0][1]);
        Num = List_V[0][1];
    }
    else if(m_State_Count_V == 2)
    {
        //printf("当前数字是\t%d ",List[0][1]*10 + List[1][1]);
        Num = List_V[0][1]*10 + List_V[1][1];
    }
    else{
        ;
    }



    char str_lx[6];

    int m_nNum = 18;  //数字修改
    m_State_Count_V = 2;  //1 一位数字 2两位数字
    // sprintf(str_lx,"%d",m_nNum);//    数字修改

    sprintf(str_lx,"%d",Num);//    识别数字

    if (m_coord_v.size()>1&&m_State_Count_V == 1)
    {
        putText(image,str_lx,Point((m_coord_v[0].Top_left_x+m_coord_v[1].Top_left_x)/2,m_coord_v[0].Top_left_y -m_coord_v[0].Width),3,1,Scalar(0,0,255));
    }
    if (m_coord_v.size()>1&&m_State_Count_V == 2)
    {
        putText(image,str_lx,Point((m_coord_v[0].Top_left_x+m_coord_v[1].Top_left_x)/2-2*m_coord_v[0].Width/3,m_coord_v[0].Top_left_y -m_coord_v[1].Width),3,1,Scalar(0,0,255));
    }
    m_State_Count_V = 0;
    //imshow("裁剪区域",image);

    // Mat image2(iplImage);
    //将image复制到src的imageRoI上
    Mat imageGray;
    cvtColor(image, imageGray, COLOR_BGR2GRAY);
    Mat mask = imageGray.clone();       //掩膜必须为灰度图
    image.copyTo(imageRoI, mask);
    cvtColor(src, src, COLOR_BGR2RGBA);
    outImg = src.clone();//返回图像必须为BGR图像

    return Num;
}
}





