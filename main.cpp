#include<opencv2/opencv.hpp>
#include<iostream>

using namespace cv;
using namespace std;

double g_centerX, g_centerY;

int findMaxArea(vector<vector<cv::Point>>contours)
{
	int max_area = -1;
	int max_index = -1;
	for (int i = 0; i < contours.size(); i++)
	{
		int area = contourArea(contours[i]);
		Rect rect = boundingRect(contours[i]);

		if ((rect.width * rect.height) * 0.4 > area)
			continue;

		if (rect.width > rect.height)
			continue;

		if (area > max_area)
		{
			max_area = area;
			max_index = i;
		}
	}

	if (max_area < 1000)
		max_index = -1;

	return max_index;
}

double calculateAngle(Point A, Point B)
{
	double dot = A.x * B.x + A.y * B.y;
	double det = A.x * B.y - A.y * B.x;
	double angle = atan2(det, dot) * 180 / CV_PI;

	return angle;
}

double distanceBetweenTwoPoints(Point start, Point end)
{
	int x1 = start.x;
	int y1 = start.y;
	int x2 = end.x;
	int y2 = end.y;

	return (int)(sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2)));
}

bool sort_custum(const cv::Point p1, const cv::Point p2)
{
	return ((p1.x + p1.y) < (p2.x + p2.y));
}

int getFingerPosition(vector<Point>max_contour, Mat img_result, vector<cv::Point>& new_points, bool debug)
{
	// 3-1 ������ max_contour�� ���� �߽��� ã��
	vector<cv::Point>points1;

	Moments M;
	M = moments(max_contour);

	int cx = (int)(M.m10 / M.m00);
	int cy = (int)(M.m01 / M.m00);
	g_centerX = cx;
	g_centerY = cy;

	// 3-2 ����� �ٻ�ȭ
	vector<cv::Point>approx;
	approxPolyDP(Mat(max_contour), approx, arcLength(Mat(max_contour), true) * 0.02, true);

	// 3-3 ������ ���� ����
	vector<cv::Point>hull;
	convexHull(Mat(approx), hull, true);

	// 3-4 ������ ���� �������� �հ��� �� �ĺ��� ����
	//�հ��� 1���� �ν��ϱ� ���� ���
	for (int i = 0; i < hull.size(); i++)
		if (cy > hull[i].y)
			points1.push_back(hull[i]);

	if (debug)
	{
		vector<vector<cv::Point>>result;
		result.push_back(hull);
		drawContours(img_result, result, -1, Scalar(0, 255, 0), 2);

		for (int i = 0; i < points1.size(); i++)
			circle(img_result, points1[i], 15, Scalar(0, 0, 0), -1);
	}

	// 3-5 Convexity Defects�� ����
	vector<int> hull2;
	convexHull(Mat(approx), hull2, false);

	vector<Vec4i> defects;
	convexityDefects(approx, hull2, defects);

	if (defects.size() == 0)
		return -1;

	// 3-6 Convexity Defects�� �̿��ؼ� �ձ�� ���� ����
	vector<cv::Point>points2;
	for (int j = 0; j < defects.size(); j++)
	{
		cv::Point start = approx[defects[j][0]];
		cv::Point end = approx[defects[j][1]];
		cv::Point far = approx[defects[j][2]];
		int d = defects[j][3];

		double angle = calculateAngle(cv::Point(start.x - far.x, start.y - far.y), cv::Point(end.x - far.x, end.y - far.y));

		if (angle > 0 && angle < 45 && d > 1000)
		{
			if (start.y < cy)
				points2.push_back(start);

			if (end.y < cy)
				points2.push_back(end);
		}
	}

	if (debug)
	{
		vector<vector<cv::Point>>result;
		result.push_back(approx);
		drawContours(img_result, result, -1, Scalar(255, 0, 255), 2);

		for (int i = 0; i < points2.size(); i++)
			circle(img_result, points2[i], 20, Scalar(0, 255, 0), 5);
	}

	// 3-7 2���� ������� ���� �հ��� �� ��ǥ�� �ϳ��� ��ģ �� �ߺ��� ����
	points1.insert(points1.end(), points2.begin(), points2.end());
	sort(points1.begin(), points1.end(), sort_custum);
	points1.erase(unique(points1.begin(), points1.end()), points1.end());

	// 3-8 �հ��� ���� �ĺ��� approx���� ã�Ƽ� �¿� ���������� ������ 45�� �̰� �¿� �������� ���� �Ÿ� ������ ��츦 �հ��� ������ ����
	for (int i = 0; i < points1.size(); i++)
	{
		Point point1 = points1[i];
		int idx = -1;

		for (int j = 0; j < approx.size(); j++)
		{
			Point point2 = approx[j];

			if (point1 == point2)
			{
				idx = j;
				break;
			}
		}

		if (idx == -1)
			continue;

		Point pre, next;

		if (idx - 1 >= 0)
			pre = approx[idx - 1];
		else
			pre = approx[approx.size() - 1];

		if (idx + 1 < approx.size())
			next = approx[idx + 1];
		else
			next = approx[0];

		double angle = calculateAngle(pre - point1, next - point1);
		double distance1 = distanceBetweenTwoPoints(pre, point1);
		double distance2 = distanceBetweenTwoPoints(next, point1);

		if (angle < 45.0 && distance1 > 40 && distance2 > 40)
			new_points.push_back(point1);
	}
	return 1;
}

Mat process(Mat img_bgr, Mat img_binary, bool debug)
{
	Mat img_result = img_bgr.clone();

	putText(img_result, "11011 Shin Jian", Point(20, 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 0));

	// 2-1 ���̳ʸ� �̹������� ����� ����
	vector<vector<Point>>contours;
	findContours(img_binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	// 2-2 ���� ū ������ ������ ã�� (�� �������� ����)
	int max_idx = findMaxArea(contours);

	if (max_idx == -1)
		return img_result;

	if (debug)
	{
		vector<vector<cv::Point>>result;
		result.push_back(contours[max_idx]);
		drawContours(img_result, result, -1, Scalar(0, 0, 255), 3);
	}

	// 2-3 �հ��� ���� ã��
	vector<cv::Point>points;
	int ret = getFingerPosition(contours[max_idx], img_result, points, debug);


	if (ret > 0 && points.size() > 0)
	{
		if (points.size() >= 4)
		{
			printf("�հ���");
		}
		for (int i = 0; i < points.size(); i++)
		{
			//���� ǥ��
			circle(img_result, points[i], 8, Scalar(0, 0, 255), 2);
			putText(img_result, "center", Point(g_centerX, g_centerY + 60), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 255));
			circle(img_result, Point(g_centerX, g_centerY + 80), 6, Scalar(0, 0, 255), -1);
		}
		for (int i = 0; i < points.size(); i++)
		{
			line(img_result, Point(g_centerX, g_centerY + 80), points[i], Scalar(0, 255, 0), 1);
		}
	}
	return img_result;
}

int main()
{
	Mat img_hsv;

	VideoCapture cap(0);
	if (!cap.isOpened()) {

		cout << "ī�޶� �� �� �����ϴ�." << endl;
		return -1;
	}

	//�׽�Ʈ �ɼǿ� ���� �� ��° �ɼ� ����
	Ptr<BackgroundSubtractorMOG2>foregroundBackground = createBackgroundSubtractorMOG2(500, 250, false);

	Mat img_frame;

	while (1)
	{
		// 1-1 �Է� ����
		bool ret = cap.read(img_frame);
		if (!ret)
			break;

		// 1-2 ������ �¿츦 �ٲ�
		flip(img_frame, img_frame, 1);

		// 1-3 �� ������ ã��
		Mat img_blur;
		GaussianBlur(img_frame, img_blur, Size(5, 5), 0);

		// 1-4 ����� �����ϰ� ������Ʈ ������ ���̴� ����ũ �̹����� ����
		Mat img_mask;
		foregroundBackground->apply(img_blur, img_mask, 0);

		// 1-5 �������� �������� �� ������ ä��
		Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
		morphologyEx(img_mask, img_mask, MORPH_CLOSE, kernel);

		// 1-7 �հ��� �ν� ���� ó��
		Mat img_result = process(img_frame, img_mask, false);

		imshow("mask", img_mask);
		imshow("Color", img_result);

		int key = waitKey(30);
		if (key == 27)
			break;
	}
	cap.release();
	return 0;

}