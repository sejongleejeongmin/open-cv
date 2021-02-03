#include <iostream>
#include <stdio.h>

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/xfeatures2d.hpp>


using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;

#define DEBUG 1

Mat makePanorama(Mat matLeftImage, Mat matRightImage, int direction) {

	//imshow("L", matLeftImage);
	//imshow("R", matRightImage);
	//waitKey(1);

	Mat matGrayLImage;
	Mat matGrayRImage;

	//Gray 이미지로 변환
	cvtColor(matLeftImage, matGrayLImage, CV_RGB2GRAY);
	cvtColor(matRightImage, matGrayRImage, CV_RGB2GRAY);

	//step 1 SURF이용해서 특징점 결정
	int nMinHessian = 300; // threshold (한계점)
	Ptr<SurfFeatureDetector> Detector = SURF::create(nMinHessian);

	vector <KeyPoint> vtKeypointsObject, vtKeypointsScene;

	Detector->detect(matGrayLImage, vtKeypointsObject);
	Detector->detect(matGrayRImage, vtKeypointsScene);

	Mat matLImageKeypoints;
	Mat matRImageKeypoints;
	drawKeypoints(matGrayLImage, vtKeypointsObject, matLImageKeypoints, Scalar::all(-1), DrawMatchesFlags::DEFAULT);
	drawKeypoints(matGrayRImage, vtKeypointsScene, matRImageKeypoints, Scalar::all(-1), DrawMatchesFlags::DEFAULT);

	imshow("LK", matLImageKeypoints);
	imshow("RK", matRImageKeypoints);
	waitKey(1);

	//step 2 기술자
	Ptr<SurfDescriptorExtractor> Extractor = SURF::create();
	//Ptr<SurfDescriptorExtractor> Extractor = SURF::create(100, 4, 3, false,true);

	Mat matDescriptorsObject, matDescriptorsScene;

	Extractor->compute(matGrayLImage, vtKeypointsObject, matDescriptorsObject);
	Extractor->compute(matGrayRImage, vtKeypointsScene, matDescriptorsScene);
	//descriptor(기술자)들 사이의 매칭 결과를 matches에 저장한다.
	FlannBasedMatcher Matcher; //kd트리를 사용하여 매칭을 빠르게 수행
	vector <DMatch> matches;
	Matcher.match(matDescriptorsObject, matDescriptorsScene, matches);

	Mat matAllMatches;
	drawMatches(matGrayLImage, vtKeypointsObject, matGrayRImage, vtKeypointsScene, matches, matAllMatches, Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	imshow("allmatches", matAllMatches);
	waitKey(1);
	double dMaxDist = matches[0].distance;
	double dMinDist = matches[0].distance;
	double dDistance;

	// 두 개의 keypoint 사이에서 min-max를 계산한다 (min값만 사용)
	for (int i = 0; i < matDescriptorsObject.rows; i++) {
		dDistance = matches[i].distance;

		if (dDistance < dMinDist) dMinDist = dDistance;
		if (dDistance > dMaxDist) dMaxDist = dDistance;
	}
	printf("max_dist : %f \n", dMaxDist);
	printf("min_dist : %f \n", dMinDist);

	//match의 distance 값이 작을수록 matching이 잘 된 것
	//min의 값의 3배 또는 good_matches.size() > 60 까지만 goodmatch로 인정해준다.
	vector<DMatch>good_matches;
	int distance = 10;
	do {
		vector<DMatch>good_matches2;
		for (int i = 0; i < matDescriptorsObject.rows; i++) {
			if (matches[i].distance < distance * dMinDist)
				good_matches2.push_back(matches[i]);
		}
		good_matches = good_matches2;
		distance -= 1;
	} while (distance != 2 && good_matches.size() > 60);

	//keypoint들과 matching 결과 ("good" matched point)를 선으로 연결하여 이미지에 그려 표시
	Mat matGoodMatches;
	drawMatches(matGrayLImage, vtKeypointsObject, matGrayRImage, vtKeypointsScene, good_matches, matGoodMatches, Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	imshow("good-matches", matGoodMatches);
	waitKey(3000);

	//Point2f형으로 변환
	vector <Point2f> obj;
	vector <Point2f> scene;

	// goodmatch에서의 keypoint를 저장

	Mat warpImage, stayImage;
	if (direction == 1) {
		for (int i = 0; i < good_matches.size();i++) {
			obj.push_back(vtKeypointsObject[good_matches[i].queryIdx].pt);
			scene.push_back(vtKeypointsScene[good_matches[i].trainIdx].pt);
		}
		warpImage = matRightImage.clone();
		stayImage = matLeftImage.clone();
	}
	else {
		for (int i = 0; i < good_matches.size();i++) {
			scene.push_back(vtKeypointsObject[good_matches[i].queryIdx].pt);
			obj.push_back(vtKeypointsScene[good_matches[i].trainIdx].pt);
		}
		warpImage = matLeftImage.clone();
		stayImage = matRightImage.clone();
	}
	printf("%d %d", stayImage.rows, stayImage.cols);
	/*
	Mat transImage(stayImage.rows, stayImage.cols+100, CV_8UC3, Scalar(0));
	for (int i = 100; i < stayImage.cols+100; i++) {
		for (int j = 0; j < stayImage.rows; j++) {
			Vec3b v = stayImage.at<Vec3b>(j, i-100);
			transImage.at<Vec3b>(j, i) = v;
		}
	}
	
	imshow("trans", transImage);*/
	Mat HomoMatrix = findHomography(scene, obj, CV_RANSAC);
	//RANSAC기법을 이용하여 첫 번째 매개변수와 두번째 매개변수 사이의 3*3 크기의 투영행렬변환 H를 구한다
	cout << HomoMatrix << endl;

	//Homograpy matrix를 사용하여 이미지를 삐뚤게
	Mat matResult;
	warpPerspective(warpImage, matResult, HomoMatrix, Size(warpImage.cols * 2, warpImage.rows * 2), INTER_CUBIC);
	

	Mat matPanorama;
	matPanorama = matResult.clone(); //복사본 대입

	imshow("wrap", matResult);
	waitKey(3000);

	Mat matROI(matPanorama, Rect(0, 0, stayImage.cols, stayImage.rows));
	stayImage.copyTo(matROI);


	imshow("Panorama", matPanorama);
	
	return matPanorama;
}

int main()
{
	Mat matImage[3];
	Mat re;

	matImage[0] = imread("C:/opencv-3.4.3/imaget2.jpg", IMREAD_COLOR);
	matImage[1] = imread("C:/opencv-3.4.3/imaget3.jpg", IMREAD_COLOR);
	matImage[2] = imread("C:/opencv-3.4.3/imaget1.jpg", IMREAD_COLOR);

	if (matImage[0].empty() || matImage[1].empty()) return -1;

	re = makePanorama(matImage[0], matImage[1], 1);
	makePanorama(matImage[2], re, 2);

	waitKey();

	return 0;
}
