#include "mrdir.h"
#include "mropencv.h"
#include "mrutil.h"
#include "fstream"
#include "AnnotationFile.h"
#include "DataSetConfig.h"
#include "time.h"
#if _WIN32
const std::string rootdir = "../../Datasets";
const int numofbgs = 1;
const std::string datasetprefix ="D:/Detection/Datasets/";
#else
const std::string rootdir = "/home/yanhe/data";
const int numofbgs = 2;
const std::string datasetprefix = "/home/yanhe/data/";
#endif

const std::string datasetname = "Mobile2017";
const std::string datasetdir = rootdir + "/" + datasetname;
const std::string videodir = datasetdir + "/" + "videos";
const std::string origimagedir = datasetdir + "/" + "origimages";
const std::string fgsimgdir = datasetdir + "/" + "fgs";
const std::string bgsdir = rootdir + "/" + "bgs";
const std::string badimgdir = datasetdir + "/" + "bad";
std::vector<std::string>bgsfiles;
DatasetConfig ds;
const int numofObjectsinOneImage = 3;
const bool userandbg = true;
const bool bsavemtcnnlabel = true;
ofstream  flabelmtcnn;
int startindex = 0;
int minAreaSize = 100000;

// const int numofscales = 3;
// const int numofCrops = 3;
// const int numofRotateinPlane = 5;
// const int numofRotateoutPlane = 5;

const int numofscales = 1;
const int numofCrops = 1;
const int numofRotateinPlane = 1;
const int numofRotateoutPlane = 1;

const int minSize = 192;
const int stride = 64;
const int targetwidth = 640;
const int targetheight = 480;
const int heightannoratio = 0.24;

struct TransformParam
{
	int scaleid = 0;
	int cropid = 0;
	int rotateidinplaneid = 0;
	int rotateoutplaneid = 0;
	int bgid = 0;
};

Rect ConvertRect(Rect Ori, Mat homography, int rows, int cols, int flag = 0) //homography 3x3矩阵 flag==0 透视变换 flag==1 仿射变换
{
	Rect result;

	vector<Point2f> ori;
	vector<Point2f> curr(4);

	Point p0 = Ori.tl();
	Point p1(Ori.x + Ori.width, Ori.y);
	Point p2(Ori.x, Ori.y + Ori.height);
	Point p3 = Ori.br();

	ori.push_back(p0);
	ori.push_back(p1);
	ori.push_back(p2);
	ori.push_back(p3);

	if (flag == 0)
		perspectiveTransform(ori, curr, homography);
	else
		transform(ori, curr, homography);

	int xmin = max(0, min(min((int)curr[0].x, (int)curr[1].x), min((int)curr[2].x, (int)curr[3].x)));
	int ymin = max(0, min(min((int)curr[0].y, (int)curr[1].y), min((int)curr[2].y, (int)curr[3].y)));
	int xmax = min(cols - 1, max(max((int)curr[0].x, (int)curr[1].x), max((int)curr[2].x, (int)curr[3].x)));
	int ymax = min(rows - 1, max(max((int)curr[0].y, (int)curr[1].y), max((int)curr[2].y, (int)curr[3].y)));

	result.x = xmin;
	result.y = ymin;
	result.width = xmax - xmin;
	result.height = ymax - ymin;
	return result;
}

cv::Point pointRot(cv::Point pt, Mat rotH)
{
	cv::Point rslt;
	rslt.x = rotH.ptr<float>(0)[0] * pt.x + rotH.ptr<float>(0)[1] * pt.y + rotH.ptr<float>(0)[2];
	rslt.y = rotH.ptr<float>(1)[0] * pt.x + rotH.ptr<float>(1)[1] * pt.y + rotH.ptr<float>(1)[2];
	return rslt;
}

void rotateImage1(Mat &img, Mat &mask, Rect &r, int degree)
{
	degree = -degree;//warpAffine默认的旋转方向是逆时针，所以加负号表示转化为顺时针
	double angle = degree  * CV_PI / 180.; // 弧度  
	double a = sin(angle), b = cos(angle);
	int width = img.cols;
	int height = img.rows;
	int width_rotate = int(height * fabs(a) + width * fabs(b));
	int height_rotate = int(width * fabs(a) + height * fabs(b));

	float map[6];
	Mat map_matrix = Mat(2, 3, CV_32F, map);
	// 旋转中心
	CvPoint2D32f center = cvPoint2D32f(width / 2, height / 2);
	CvMat map_matrix2 = map_matrix;
	cv2DRotationMatrix(center, degree, 1.0, &map_matrix2);//计算二维旋转的仿射变换矩阵
	map[2] += (width_rotate - width) / 2;
	map[5] += (height_rotate - height) / 2;
	Mat img_rotate, mask_rotate;
	Rect rr;

	//cout << map_matrix << endl;
	warpAffine(img, img_rotate, map_matrix, Size(width_rotate, height_rotate), 1, 0, 0);
	warpAffine(mask, mask_rotate, map_matrix, Size(width_rotate, height_rotate), 1, 0, 0);
	Point p0 = r.tl();
	Point p1(r.x + r.width, r.y);
	Point p2(r.x, r.y + r.height);
	Point p3 = r.br();

	Point p0r = pointRot(p0, map_matrix);
	Point p1r = pointRot(p1, map_matrix);
	Point p2r = pointRot(p2, map_matrix);
	Point p3r = pointRot(p3, map_matrix);

	int xmin = max(0, min(min(p0r.x, p1r.x), min(p2r.x, p3r.x)));
	int ymin = max(0, min(min(p0r.y, p1r.y), min(p2r.y, p3r.y)));
	int xmax = min(img_rotate.cols - 1, max(max(p0r.x, p1r.x), max(p2r.x, p3r.x)));
	int ymax = min(img_rotate.rows - 1, max(max(p0r.y, p1r.y), max(p2r.y, p3r.y)));

	rr.x = xmin;
	rr.y = ymin;
	rr.width = xmax - xmin;
	rr.height = ymax - ymin;

	img = img_rotate;
	mask = mask_rotate;
	r = rr;
}

int xRot(Mat &src, Mat &mask, Rect &r)
{
	int rows = src.rows;
	int cols = src.cols;

	int a = 1, b = 11;
	int scale = (rand() % (b - a) + a);
	a = 1, b = 3;
	int dir = (rand() % (b - a) + a);

	vector<Point2f> corners(4);

	corners[0] = Point2f(0, 0);
	corners[1] = Point2f(cols - 1, 0);
	corners[2] = Point2f(0, rows - 1);
	corners[3] = Point2f(cols - 1, rows - 1);

	vector<Point2f> cornerst(4);
	if (dir == 1)
	{
		cornerst[0] = Point2f(0, 0);
		cornerst[1] = Point2f(cols - 1, 0);
		cornerst[2] = Point2f(0.015*scale*cols, rows - 1 - 0.026*scale*rows);
		cornerst[3] = Point2f(cols - 1 - (0.015*scale*cols), rows - 1 - 0.026*scale*rows);
	}
	if (dir == 2)
	{
		cornerst[0] = Point2f(0.015*scale*cols, 0.026*scale*rows);
		cornerst[1] = Point2f(cols - 1 - (0.015*scale*cols), 0.026*scale*rows);
		cornerst[2] = Point2f(0, rows - 1);
		cornerst[3] = Point2f(cols - 1, rows - 1);
	}
	Mat tran = cv::getPerspectiveTransform(corners, cornerst);
	Mat s, m;
	cv::warpPerspective(src, s, tran, Size(cols, rows));
	cv::warpPerspective(mask, m, tran, Size(cols, rows));
	Rect rr = ConvertRect(r, tran, s.rows, s.cols);

	src = s;
	mask = m;
	r = rr;

	return 0;
}

int yRot(Mat &src, Mat &mask, Rect &r)
{
	int rows = src.rows;
	int cols = src.cols;

	int a = 1, b = 11;
	int scale = (rand() % (b - a) + a);
	a = 1, b = 3;
	int dir = (rand() % (b - a) + a);

	vector<Point2f> corners(4);

	corners[0] = Point2f(0, 0);
	corners[1] = Point2f(cols - 1, 0);
	corners[2] = Point2f(0, rows - 1);
	corners[3] = Point2f(cols - 1, rows - 1);

	vector<Point2f> cornerst(4);
	if (dir == 1)
	{
		cornerst[0] = Point2f(0.0532*scale*cols, 0.01*scale*rows);
		cornerst[1] = Point2f(cols - 1, 0);
		cornerst[2] = Point2f(0.0532*scale*cols, rows - 1 - 0.01*scale*rows);
		cornerst[3] = Point2f(cols - 1, rows - 1);
	}
	if (dir == 2)
	{
		cornerst[0] = Point2f(0, 0);
		cornerst[1] = Point2f(cols - 1 - (0.0532*scale*cols), 0.01*scale*rows);
		cornerst[2] = Point2f(0, rows - 1);
		cornerst[3] = Point2f(cols - 1 - (0.0532*scale*cols), rows - 1 - (0.01*scale*rows));
	}
	Mat tran = cv::getPerspectiveTransform(corners, cornerst);
	Mat s, m;
	cv::warpPerspective(src, s, tran, Size(cols, rows));
	cv::warpPerspective(mask, m, tran, Size(cols, rows));
	Rect rr = ConvertRect(r, tran, s.rows, s.cols);

	src = s;
	mask = m;
	r = rr;

	return 0;
}

void dataResize(Mat &img, Mat &mask, Rect &r, TransformParam tf)
{
	int rid = tf.scaleid;
	int rows = img.rows;
	int cols = img.cols;

	Mat imgrslt, maskrslt;
	int rs = minSize + rid * stride;
	float scale = (float)(rs) / (float)(rows);
	int cs = cols * scale;

	cv::resize(img, imgrslt, Size(cs, rs));
	cv::resize(mask, maskrslt, Size(cs, rs));

	img = imgrslt;
	mask = maskrslt;
	r.width *= scale;
	r.height *= scale;
}

void crossLine(Mat &img, Mat &mask, int y)  //y：横切线的坐标
{
	int wmax = img.cols;
	int hmax = img.rows;
	Point lp1, lp2, crossp;
	lp1.x = 0;
	lp2.x = wmax;

	Mat rslt;

	int a = y, b = hmax;
	crossp.y = (rand() % (b - a) + a);
	a = 0; b = wmax;
	crossp.x = (rand() % (b - a) + a);

	a = y; b = hmax;
	lp1.y = (rand() % (b - a) + a);
	lp2.y = (rand() % (b - a) + a);
	for (int i = 0; i < img.rows; i++)
	{
		int startx = (i - lp1.y)*crossp.x / (img.rows - lp1.y);
		int endx = img.cols - (i - lp2.y)*(img.cols - crossp.x) / (img.rows - lp2.y);
		if (startx <= endx)
		{
			for (int j = 0; j < startx; j++)
				mask.at<uchar>(i, j) = 0;
			for (int j = endx; j < img.cols; j++)
			{
				mask.at<uchar>(i, j) = 0;
			}
		}
	}
	img.copyTo(rslt, mask);
	img = rslt;
}

void dataCrop(Mat &img, Mat &mask, Rect &r, TransformParam tf, float cropScale)
{
	int cid = tf.cropid;
	Mat imgrslt, maskrslt;
	Rect crop;
	int rows = img.rows;
	int cols = img.cols;

	switch (cid)
	{
	case 0:
		r.height *= cropScale;
		break;
	case 1:
		r.height *= cropScale;
		imgrslt = img(r);
		maskrslt = mask(r);
		img = imgrslt; mask = maskrslt;
		break;
	case 2:
		r.height *= cropScale;
		crossLine(img, mask, r.height);
	}
}

void dataInRot(Mat &img, Mat &mask, Rect &r, TransformParam tf)
{
	int irid = tf.rotateidinplaneid;
	int a = -90, b = 90;
	int angle = (rand() % (b - a) + a);
	if (irid != 0)
		rotateImage1(img, mask, r, (float)angle);
}

void dataOutRot(Mat &img, Mat &mask, Rect &r, TransformParam tf)
{
	int orid = tf.rotateoutplaneid;
	if (orid != 0)
	{
		xRot(img, mask, r);
		yRot(img, mask, r);
	}
}
int generatetransformimage(Mat &img, Mat &mask, Rect &r, TransformParam tf)
{
	float scale = 0.3;
	dataResize(img, mask, r, tf);
	dataCrop(img, mask, r, tf, scale);
	dataOutRot(img, mask, r, tf);
	dataInRot(img, mask, r, tf);
	return 0;
}

int generatefromonevideo(const string videopath, const string label)
{
	cv::VideoCapture capture(videopath);
	cv::Mat img,gray, thd;
	while (true)
	{
		capture >> img;
		if (!img.data)
			break;
		cv::cvtColor(img, gray, CV_BGR2GRAY);
		cv::threshold(gray, thd, 0, 255, CV_THRESH_OTSU);
		std::vector<std::vector<cv::Point>>contours;
		cv::findContours(thd, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

		int maxcontourindex = 0, maxarea = 0;
		for (int j = 0; j < contours.size(); j++)
		{
			if (contours[j].size() > maxarea)
			{
				maxarea = contours[j].size();
				maxcontourindex = j;
			}
		}
		cv::Rect r = cv::boundingRect(contours[maxcontourindex]);
		cv::Mat c = img.clone();
		cv::drawContours(c, contours, maxcontourindex, CV_RGB(255, 0, 0));
		AnnotationFile af;		
		af.set_width(img.cols);
		af.set_height(img.rows);
		af.set_depth(img.channels());
		vector<Object>objects;
		for (int k = 0; k < 1; k++)
		{
			Object object;
			object.xmin = r.x;
			object.ymin = r.y;
			object.xmax = r.x + r.width;
			object.ymax = r.y + r.height;
			object.name = label;
			objects.push_back(object);
		}
		af.objects = objects;
		cv::Mat mask = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);
		cv::drawContours(mask, contours, maxcontourindex, Scalar(255), CV_FILLED);
		cv::Mat saveimg = img.clone();
		if (userandbg)
		{
			for (int k = 0; k < numofbgs; k++)
			{
				string filename = int2string(startindex)+"_" + int2string(k)+ ".jpg";//
				af.filename = filename;
				int nb = rand() % bgsfiles.size();
				cv::Mat bgMat = cv::imread(bgsdir + "/" + bgsfiles[nb]);
				cv::resize(bgMat, bgMat, img.size());
				cv::Mat fgMat = saveimg(r);
				saveimg.copyTo(bgMat, mask);
				saveimg = bgMat.clone();
				string filepath = ds.datasetdir + "/" + ds.imagedir + "/" + filename;
				cv::imwrite(filepath, saveimg);
				if (ds.bsavexml)
				{
					string xmlpath = ds.datasetdir + "/" + ds.annotationdir + "/" + filename;
					xmlpath = xmlpath.substr(0, xmlpath.length() - 3) + "xml";
					af.save_xml(xmlpath);
				}
				if (ds.bsavetxt)
				{
					string txtpath = ds.datasetdir + "/" + ds.labelsdir + "/" + filename;
					txtpath = txtpath.substr(0, txtpath.length() - 3) + "txt";
					af.save_txt(txtpath);
				}
				cout << "\r" <<filename<<"             ";
			}
		}
		else
		{
			std::string filename = int2string(startindex) + ".jpg";//
			af.filename = filename;
			saveimg = img.clone();
			std::string filepath = ds.datasetdir + "/" + ds.imagedir + "/" + filename;
			cv::imwrite(filepath, saveimg);
			if (ds.bsavexml)
			{
				string xmlpath = ds.datasetdir + "/" + ds.annotationdir + "/" + filename;
				xmlpath = xmlpath.substr(0, xmlpath.length() - 3) + "xml";
				af.save_xml(xmlpath);
			}
			if (ds.bsavetxt)
			{
				string txtpath = ds.datasetdir + "/" + ds.labelsdir + "/" + filename;
				txtpath = txtpath.substr(0, txtpath.length() - 3) + "txt";
				af.save_txt(txtpath);
			}
			cout << "\r" << filename << "             ";
		}
		startindex++;
	}
	return 0;
}

int generatefromvideodir(const std::string dir, const std::string label)
{
	auto files = getAllFilesinDir(dir);
	cout << dir << endl;
#pragma omp parallel for
	for (int i = 0; i < files.size(); i++)
	{
		string videpath = dir + "/" + files[i];
		cout << files[i] << endl;
		generatefromonevideo(videpath, label);
		cout << endl;
	}
	return 0;
}

int generatefromvideos()
{
	bgsfiles = getAllFilesinDir(bgsdir);
	auto subdirs = getAllSubdirs(videodir);
	for (int i = 0; i < subdirs.size(); i++)
	{
		std::string subdir = videodir + "/" + subdirs[i];
		generatefromvideodir(subdir, subdirs[i]);
	}
	cout << endl;
	return 0;
}

int generatefromorigimage(const string dir,const string imagefilename, const string label)
{
	cv::Mat img, gray, thd;
	string imagepath = dir + "/" + imagefilename;
	img = cv::imread(imagepath);
	if (!img.data)
		return -1;
	cv::GaussianBlur(img, img, cv::Size(3, 3), 1.0);
	cv::cvtColor(img, gray, CV_BGR2GRAY);
	cv::threshold(gray, thd, 0, 255, CV_THRESH_OTSU);
	std::vector<std::vector<cv::Point>>contours;
	cv::findContours(thd, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	vector<int>sizes;
	vector<int>selectedindex;
	for (int j = 0; j < contours.size(); j++)
	{
		int size=contourArea(contours[j]);
		if (size > minAreaSize)
		{
			selectedindex.push_back(j);
		}
	}
	for (int j = 0; j < selectedindex.size(); j++)
	{
		int size = contourArea(contours[selectedindex[j]]);
		sizes.push_back(size);
	}
	AnnotationFile af;
	af.set_width(img.cols);
	af.set_height(img.rows);
	af.set_depth(img.channels());
	af.set_filename(imagefilename);
	if (selectedindex.size() != 3)
	{
		cout << imagefilename <<" "<<selectedindex.size()<< endl;
		string badfilepath = badimgdir + "/" + imagefilename;
		cv::imwrite(badfilepath, img);
		return -1;
	}
	for (int j = 0; j < selectedindex.size(); j++)
	{
		vector<Object>objects;
		cv::Rect r = cv::boundingRect(contours[selectedindex[j]]);
		cv::Mat c = img.clone();
		cv::drawContours(c, contours, selectedindex[j], CV_RGB(255, 0, 0));
		Object object;
		object.xmin = r.x;
		object.ymin = r.y;
		object.xmax = r.x + r.width;
		object.ymax = r.y + r.height;
		object.name = label;
		objects.push_back(object);
		af.objects = objects;
		cv::Mat mask = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);
		cv::drawContours(mask, contours, selectedindex[j], Scalar(255), CV_FILLED);
		cv::Mat saveimg = img.clone();
		if (userandbg)
		{
			for (int k = 0; k < numofbgs; k++)
			{
				string filename = imagefilename.substr(0,imagefilename.length()-4) + "_" +int2string(j)+"_"+ int2string(k) + ".jpg";//
				af.filename = filename;
				int nb = rand() % bgsfiles.size();
				cv::Mat bgMat = cv::imread(bgsdir + "/" + bgsfiles[nb]);
				cv::resize(bgMat, bgMat, img.size());
				cv::Mat fgMat = saveimg(r);
				saveimg.copyTo(bgMat, mask);
				saveimg = bgMat.clone();
				string filepath = ds.datasetdir + "/" + ds.imagedir + "/" + filename;
				cv::imwrite(filepath, saveimg);
				if (ds.bsavexml)
				{
					string xmlpath = ds.datasetdir + "/" + ds.annotationdir + "/" + filename;
					xmlpath = xmlpath.substr(0, xmlpath.length() - 3) + "xml";
					af.save_xml(xmlpath);
				}
				if (ds.bsavetxt)
				{
					string txtpath = ds.datasetdir + "/" + ds.labelsdir + "/" + filename;
					txtpath = txtpath.substr(0, txtpath.length() - 3) + "txt";
					af.save_txt(txtpath);
				}				
				cout << "\r" << filename << "             ";
			}
		}
		startindex++;
	}
	return 0;
}

int getfgimage(const string dir, const string imagefilename, const string label)
{
	cv::Mat img, gray, thd;
	string imagepath = dir + "/" + imagefilename;
	img = cv::imread(imagepath);
	if (!img.data)
		return -1;
	cv::GaussianBlur(img, img, cv::Size(3, 3), 1.0);
	cv::cvtColor(img, gray, CV_BGR2GRAY);
	thd = gray.clone();
	cv::Mat mask;
	Rect ccomp;
	cv::floodFill(thd, cv::Point(3200, 100), cv::Scalar(0, 0, 0), &ccomp, cv::Scalar(1, 1, 1), cv::Scalar(1,2, 5));
	std::vector<std::vector<cv::Point>>contours;
	cv::findContours(thd, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	vector<int>sizes;
	vector<int>selectedindex;
	for (int j = 0; j < contours.size(); j++)
	{
		int size = contourArea(contours[j]);
		if (size > minAreaSize)
		{
			selectedindex.push_back(j);
		}
	}
	for (int j = 0; j < selectedindex.size(); j++)
	{
		int size = contourArea(contours[selectedindex[j]]);
		sizes.push_back(size);
	}
	if (selectedindex.size() != 3)
	{
		cout << endl;
		cout << imagefilename << " " << selectedindex.size() << endl;
		string badfilepath = badimgdir + "/" + imagefilename;
		cv::imwrite(badfilepath, img);
	}
	for (int j = 0; j < selectedindex.size(); j++)
	{
		vector<Object>objects;
		int areasize = contours[selectedindex[j]].size();
		cv::Rect r = cv::boundingRect(contours[selectedindex[j]]);
		cv::Mat c = img.clone();
		cv::drawContours(c, contours, selectedindex[j], CV_RGB(255, 0, 0));
		string filename = imagefilename.substr(0, imagefilename.length() - 4) + "_" + int2string(j) + ".jpg";//
		cv::Mat mask = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);
		cv::drawContours(mask, contours, selectedindex[j], Scalar(255), CV_FILLED);
		cv::Mat saveimg = img.clone();
		string filepath =fgsimgdir + "/Mobile/" + filename;
		cv::Mat fgMat = cv::Mat::zeros(cv::Size(r.width, r.height), CV_8UC1);
		saveimg.copyTo(fgMat, mask);
		cv::Mat fg = fgMat(r);
		cv::imwrite(filepath,fg);
		cout << "\r" << filename << "             ";
		startindex++;
	}
	return 0;
}

int generatefromfgimage(const string dir, const string imagefilename, const string label)
{
	cv::Mat img;
	string imagepath = dir + "/" + imagefilename;
	img = cv::imread(imagepath);
	if (!img.data)
		return -1;
	AnnotationFile af;
	af.set_width(640);
	af.set_height(480);
	af.set_depth(img.channels());
	af.set_filename(imagefilename);
	for (int i = 0; i < numofscales; i++)
	{
		for (int j = 0; j < numofCrops; j++)
		{
			for (int k = 0; k < numofRotateinPlane; k++)
			{
				for (int t = 0; t < numofRotateoutPlane; t++)
				{
					TransformParam tf;
					tf.scaleid = i;
					tf.cropid = j;
					tf.rotateidinplaneid = k;
					tf.rotateoutplaneid = t;
					cv::Mat gray;
					if (img.channels() == 3)
						cv::cvtColor(img, gray, CV_BGR2GRAY);
					else
						gray = img.clone();
					cv::Mat thd;
					int m = cv::mean(gray)[0];
					cv::Mat mask= cv::Mat::zeros(img.rows, img.cols, CV_8UC1);
					if (m >= 168)
					{
						cv::threshold(gray, thd, 0, 255, THRESH_OTSU);
						std::vector<std::vector<cv::Point>>contours;
						cv::findContours(thd, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
						int maxsize = 0, maxindex = 0;
						for (int c = 0; c < contours.size(); c++)
						{
							int size = contourArea(contours[c]);
							if (size > maxsize)
							{
								maxindex = c;
								maxsize = size;
							}
						}
						cv::drawContours(mask, contours, maxindex, Scalar(255), CV_FILLED);
					}
					else
					{
						mask = 255*cv::Mat::ones(img.rows, img.cols, CV_8UC1);
					}					
					cv::Mat targetimg = img.clone();
					cv::Rect r0(0, 0, img.cols, img.rows);
					if (0 == generatetransformimage(targetimg, mask, r0, tf))
					{
						for (int b = 0; b < numofbgs; b++)
						{
							cv::Rect r = r0;
							string filename = imagefilename.substr(0,imagefilename.length()-4)+"_"+int2string(i)+"_"+int2string(j)+"_"+int2string(k)+"_"+int2string(t)+"_"+int2string(b)+".jpg";
							int nb = rand() % bgsfiles.size();
							cv::Mat bgMat = cv::imread(bgsdir + "/" + bgsfiles[nb]);
							cv::resize(bgMat, bgMat, cv::Size(640, 480));
							int offestX = rand() % max(10, (640 - targetimg.cols - 1));//
							int offsetY = rand() % max(10, (480 - targetimg.rows - 1));
							r.x=r.x + offestX;
							r.y=r.y + offsetY;
							if (r.x < 0)
								r.x = 0;
							if (r.y < 0)
								r.y = 0;
							if (r.x + r.width >= bgMat.cols)
								r.x = bgMat.cols - r.x;
							if (r.y + r.height >= bgMat.rows)
								r.y = bgMat.rows - r.y;
							cv::Mat saveimg = bgMat.clone();
							cv::Mat fg = saveimg(r);
							//targetimg.copyTo(fg, mask);
							for (int m = 0; m < targetimg.rows; m++)
							{
								for (int n = 0; n < targetimg.cols; n++)
								{
									if (mask.at<uchar>(m, n) == 255)
									{
										saveimg.at<Vec3b>(m + offsetY, n + offestX) = targetimg.at<Vec3b>(m, n);
									}
								}
							}
							vector<Object>objects;
							Object object;
							object.xmin = r.x;
							object.ymin = r.y;
							object.xmax = r.x + r.width ;
							object.ymax = r.y + r.height;
							object.name = label;
							objects.push_back(object);
							af.objects = objects;
							string filepath = ds.datasetdir + "/" + ds.imagedir + "/" + filename;
							cv::imwrite(filepath, saveimg);
							if (ds.bsavexml)
							{
								string xmlpath = ds.datasetdir + "/" + ds.annotationdir + "/" + filename;
								xmlpath = xmlpath.substr(0, xmlpath.length() - 3) + "xml";
								af.save_xml(xmlpath);
							}
							if (ds.bsavetxt)
							{
								string txtpath = ds.datasetdir + "/" + ds.labelsdir + "/" + filename;
								txtpath = txtpath.substr(0, txtpath.length() - 3) + "txt";
								af.save_txt(txtpath);
							}
							if (bsavemtcnnlabel)
							{
								string line =ds.imagedir+"/" +filename+ " " + int2string(r.x) + " " + int2string(r.y) + " " + int2string(r.x + r.width) + " " + int2string(r.y + r.height) + "\n";
								flabelmtcnn << line;
								flabelmtcnn.flush();
							}
							cout << "\r" << filename << "             ";
//							cv::imshow("img", saveimg);
//							cv::waitKey();
						}
					}
				}
			}
		}
	}
	return 0;
}

int generatefrmimagedir(const std::string dir, const std::string label)
{
	auto files = getAllFilesinDir(dir);
	cout << dir << endl;
	for (int i = 0; i < files.size(); i++)
	{
//		generatefromorigimage(dir, files[i], label);
//		getfgimage(dir, files[i], label);
		generatefromfgimage(dir,files[i], label);
	}
	return 0;
}

int generatefromimages(const std::string dir=fgsimgdir)
{
	bgsfiles = getAllFilesinDir(bgsdir);
	auto subdirs = getAllSubdirs(dir);
	for (int i = 0; i < subdirs.size(); i++)
	{
		std::string subdir = dir + "/" + subdirs[i];
		generatefrmimagedir(subdir, subdirs[i]);
	}
	cout << endl;
	return 0;
}

void init()
{
	srand((unsigned)time(0));
	ds.init(datasetdir);
	if (!EXISTS((ds.datasetdir + "/" + ds.imagedir).c_str()))
		MKDIR((ds.datasetdir + "/" + ds.imagedir).c_str());
	if (!EXISTS((ds.datasetdir + "/" + ds.labelsdir).c_str()))
		MKDIR((ds.datasetdir + "/" + ds.labelsdir).c_str());
	if (!EXISTS((ds.datasetdir + "/" + ds.annotationdir).c_str()))
		MKDIR((ds.datasetdir + "/" + ds.annotationdir).c_str());
// 	if (!EXISTS(badimgdir.c_str()))
// 		MKDIR(badimgdir.c_str());
	if (bsavemtcnnlabel)
	{
		flabelmtcnn = ofstream(ds.datasetdir + "/" + "label.txt");
	}
}

int main()
{	
	init();
//	generatefromvideos();
	generatefromimages();//origimagedir
	ds.generatetrainvaltxt();
	return 0;
}