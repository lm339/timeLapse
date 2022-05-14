#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc,char *argv[])
{

    Mat image;
    vector<String> file_names;

    if(argc < 2) //检测输入
    {
        printf("input erro \n");
        return 1;
    }

    printf("%s \n",argv[1]);

    glob(argv[1],file_names); //获取图片列表，这里返回的就是创建时间顺序，所以不需要再排序

    image = imread(file_names[0]);//这里只是想知道图片的大小
    bool isColor = (image.type() == CV_8UC3);

    VideoWriter writer;

    int codec = VideoWriter::fourcc('M','J','P','G');
    double fps = 25.0;
    char filename[128] = {'\0'};

    sprintf(filename,"%s/output.avi",argv[1]);

    writer.open(filename,codec,fps,image.size(),isColor);

    if(!writer.isOpened())
    {
        printf("input erro \n");
        return -1;
    }

    for(long unsigned int i =0;i<file_names.size();i++)
    {
        Mat img;
        img = imread(file_names[i]);
        cout << file_names[i] << endl;
        writer.write(img);
    }

    return 0;
}

