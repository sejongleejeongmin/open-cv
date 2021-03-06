#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp> 
#include <opencv2/nonfree/features2d.hpp> 
#include <opencv2/flann/flann.hpp>
#include <opencv2/calib3d/calib3d.hpp>
//특징 추출과 매칭에 필요한 헤더파일
using namespace cv;

#define RED Scalar(0,0,255)

int main()
{
	Mat img1 = imread("C:/opencv-2-4-13-6/image/model3.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	Mat img2 = imread("C:/opencv-2-4-13-6/image/scene.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	// CV_LOAD_IMAGE_GRAYSCALE 영상을 읽어들일 때 자동으로 명암 영상으로 변환
	assert(img1.data && img2.data);
	//인자로 들어가는 조건이 참이면 그냥 지나가고 아니면 에러

	SiftFeatureDetector detector(0.3); //SIFT를 검출
	std::vector<KeyPoint> keypoint1, keypoint2;

	detector.detect(img1, keypoint1);
	detector.detect(img2, keypoint2);
	//멤버 함수 detect()는 첫 번째 매개변수에 있는 영상에서 키포인트를 검출하여 
	//두 번째 매개변수의 키포인트를 그려 세번째 매개변수의 영상에 저장해준다

	Mat disp;
	drawKeypoints(img1, keypoint1, disp, RED, DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	namedWindow("키포인트"); imshow("키포인트", disp);

	//기술자 추출
	SiftDescriptorExtractor extractor;
	Mat descriptor1, descriptor2;
	extractor.compute(img1, keypoint1, descriptor1);
	extractor.compute(img2, keypoint2, descriptor2);
	//compute() 첫번째 매개변수의 영상에 대해 두번째 매개변수의 키포인트 위치에서 기술자를 추출 후
	//세번째 매개변수에 저장해준다.
	FlannBasedMatcher matcher;
	std::vector<DMatch> match;
	matcher.match(descriptor1, descriptor2, match);

	double maxd = 0; double mind = 100;
	for (int i = 0; i < descriptor1.rows; i++) {
		double dist = match[i].distance;
		if (dist < mind) mind = dist;
		if (dist > maxd) maxd = dist;
	}

	std::vector<DMatch> good_match;
	for (int i = 0; i < descriptor1.rows; i++)
		if (match[i].distance <= max(2 * mind, 0.02)) good_match.push_back(match[i]);

	Mat img_match;
	drawMatches(img1, keypoint1, img2, keypoint2, good_match, img_match, Scalar::all(-1), Scalar::all(-1),
		vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

	namedWindow("매칭 결과"); imshow("매칭 결과", img_match);

	for (int i = 0; i < (int)good_match.size(); i++)
		printf("키포인트 %d~%d\n", good_match[i].queryIdx, good_match[i].trainIdx);

	std::vector<Point2f> model_pt;
	std::vector<Point2f> scene_pt;
	for (int i = 0; i < good_match.size(); i++) {
		model_pt.push_back(keypoint1[good_match[i].queryIdx].pt);
		scene_pt.push_back(keypoint2[good_match[i].trainIdx].pt);
	}
	Mat H = findHomography(model_pt, scene_pt, CV_RANSAC);

	std::vector<Point2f> model_corner(4); // 모델 물체의 네 구석점 좌표 생성
	model_corner[0] = cvPoint(0, 0);
	model_corner[1] = cvPoint(img1.cols, 0);
	model_corner[2] = cvPoint(img1.cols, img1.rows);
	model_corner[3] = cvPoint(0, img1.rows);

	std::vector<Point2f> scene_corner(4);
	perspectiveTransform(model_corner, scene_corner, H);
  //좌표에 투영변환 H를 적용하여 장면 영상에 나타날 좌표 scene_corner를 계산한다

	Point2f p(img1.cols, 0); //4개의 선분영상 그려넣음으로써 인식 결과 표시
  //하나의 윈도우에 모델 영상과 장면 영상을 같이 그려넣었기때문에 모델 영상의 너비만큼 이동시켜 그리기 위해 
  //Point2f 형의 p를  
	line(img_match, scene_corner[0] + p, scene_corner[1] + p, RED, 3);
	line(img_match, scene_corner[1] + p, scene_corner[2] + p, RED, 3);
	line(img_match, scene_corner[2] + p, scene_corner[3] + p, RED, 3);
	line(img_match, scene_corner[3] + p, scene_corner[0] + p, RED, 3);

	namedWindow("인식 결과"); imshow("인식 결과", img_match);
	waitKey();
	return 0;
}
