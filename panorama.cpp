//112줄부터 오류 
#include <iostream>
#include <time.h>
#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/highgui.hpp>



using namespace cv;
using namespace std;

#define DEBUG 1

int main()
{
	Mat matLeftImage;
	Mat matRightImage;
	Mat matGrayLImage;
	Mat matGrayRImage;

	matLeftImage = imread("C:/opencv-2-4-13-6/S5.jpg", IMREAD_COLOR);
	matRightImage = imread("C:/opencv-2-4-13-6/S6.jpg", IMREAD_COLOR);
	
	if (matLeftImage.empty() || matRightImage.empty()) return -1;

	imshow("L", matLeftImage);
	imshow("R", matRightImage);
	waitKey(3000);
	
	//Gray 이미지로 변환(안됨)
	//cvtColor(matLeftImage, matGrayLImage, CV_RGB2GRAY);
	//cvtColor(matRightImage, matGrayRImage, CV_RGB2GRAY);

	matGrayLImage = imread("C:/opencv-2-4-13-6/S5.jpg", CV_GRAY2BGR);
	matGrayRImage = imread("C:/opencv-2-4-13-6/S6.jpg", CV_GRAY2BGR);
	if (!matGrayLImage.data || !matGrayRImage.data) return -1;

	//step 1 SURF이용해서 특징점 결정
	int nMinHessian = 400; // threshold (한계점)???????
	SurfFeatureDetector Detector(nMinHessian);
	
	vector <KeyPoint> vtKeypointsObject, vtKeypointsScene;

	Detector.detect(matGrayLImage, vtKeypointsObject);
	Detector.detect(matGrayRImage, vtKeypointsScene);

	Mat matLImageKeypoints;
	Mat matRImageKeypoints;
	drawKeypoints(matGrayLImage, vtKeypointsObject, matLImageKeypoints, Scalar::all(-1), DrawMatchesFlags::DEFAULT);
	drawKeypoints(matGrayRImage, vtKeypointsScene, matRImageKeypoints, Scalar::all(-1), DrawMatchesFlags::DEFAULT);
	
	imshow("LK", matLImageKeypoints);
	imshow("RK", matRImageKeypoints);
	waitKey(3000);
	
	//step 2 기술자
	SurfDescriptorExtractor Extractor;

	Mat matDescriptorsObject, matDescriptorsScene;

	Extractor.compute(matGrayLImage, vtKeypointsObject, matDescriptorsObject);
	Extractor.compute(matGrayRImage, vtKeypointsScene, matDescriptorsScene);
	//descriptor를 사이의 매칭 결과를 matches에 저장한다.
	FlannBasedMatcher Matcher;
	vector <DMatch> matches;
	Matcher.match(matDescriptorsObject, matDescriptorsScene, matches);
	
	Mat matGoodMatches1;
	drawMatches(matGrayLImage, vtKeypointsObject, matGrayRImage, vtKeypointsScene, matches, matGoodMatches1, Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	imshow("allmatches", matGoodMatches1);
	waitKey(3000);

	double dMaxDist = 0;
	double dMinDist = 100;
	double dDistance;

	// 두 개의 keypoint 사이에서 min-max를 계산한다
	for (int i = 0; i < matDescriptorsObject.rows; i++) {
		dDistance = matches[i].distance;

		if (dDistance < dMinDist) dMinDist = dDistance;
		if (dDistance < dMaxDist) dMaxDist = dDistance;
	}
	printf("max_dist : %f \n", dMaxDist);
	printf("max_dist : %f \n", dMinDist);
	// 값이 작을수록 matching이 잘 된 것
	//min의 값의 3배까지만 goodmatch로 인정해주겠다

	vector<DMatch>good_matches;

	for (int i = 0; i < matDescriptorsObject.rows; i++) {
		if (matches[i].distance < 3 * dMinDist)
			good_matches.push_back(matches[i]);
	}

	//keypoint들과 matching 결과 ("good" matched point)를 선으로 연결하여 이미지에 그려 표시
	Mat matGoodMatches;
	drawMatches(matGrayLImage, vtKeypointsObject, matGrayRImage, vtKeypointsScene, good_matches, matGoodMatches,Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	imshow("good-matches", matGoodMatches);
	waitKey(3000);

	vector <Point2f> obj;
	vector <Point2f> scene;

	for (int i = 0; i < good_matches.size();i++) {
		obj.push_back(vtKeypointsObject[good_matches[i].queryIdx].pt);
		scene.push_back(vtKeypointsScene[good_matches[i].trainIdx].pt);
	}
	Mat HomoMatrix = findHomography(scene, obj, CV_RANSAC);

	cout << HomoMatrix << endl;

	//Homograpy matrix를 사용하여 이미지를 삐뚤게
	Mat matResult;

	warpPerspective(matRightImage, matResult, HomoMatrix, Size(matRightImage.cols * 2, matRightImage.rows), INTER_CUBIC);

	Mat matPanorama;
	matPanorama = matResult.clone();

	imshow("wrap", matResult);
	waitKey(3000);

	Mat matROI(matPanorama, Rect(0, 0, matLeftImage.cols, matLeftImage.rows));
	matLeftImage.copyTo(matROI);
	imshow("Panorama", matPanorama);
	waitKey();
	return 0;
}
