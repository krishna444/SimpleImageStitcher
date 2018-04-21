//============================================================================
// Name        : HelloWorld.cpp
// Author      : Krishna
// Version     : Version1.0(2017-12-07)
// Copyright   : Your copyright notice
// Description : Image Stitcher with Shift Parameters X and Y
//============================================================================
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include <stdlib.h>

using namespace std;

cv::Mat stichImage(char* path1, char* path2, int X, int Y);
void readXYZFileDimension(char *fileName, unsigned short* width,
		unsigned short* height);
void readXYZFile(char *fileName, unsigned short* imageData);
void writeAsPNG(cv::Mat image, cv::String location);
void showImage(cv::String title, cv::Mat image);
int calculateCombinedHeight(int height1, int height2, int Y);
int calculateCombinedWidth(int width1, int width2, int X);
int calculateBottomRightXPoint(int width1, int width2, int X);
void writeXYZFile(unsigned short * imageData, int width, int height,
		char *fileName);

int main() {
	char* path1 = "Samples2/4.xyz";
	char* path2 = "Samples2/5.xyz";
	int X = -24; //100; //-20;
	int Y = 220;
	cv::Mat result = stichImage(path1, path2, X, Y);
	writeAsPNG(result, "Samples2/Result/45.png");
	writeXYZFile((unsigned short*) result.data, result.cols, result.rows,
			"Samples2/Result/45.xyz");
}

/**
 * Main function to stitch two images
 * @path1 Path of first raw image
 * @path2 Path of second raw image
 */
cv::Mat stichImage(char* path1, char* path2, int X, int Y) {
	unsigned short width1, width2, height1, height2;
	readXYZFileDimension(path1, &width1, &height1);
	readXYZFileDimension(path2, &width2, &height2);

	unsigned short* image1 = (unsigned short *) malloc(
			width1 * height1 * sizeof(unsigned short));
	unsigned short* image2 = (unsigned short *) malloc(
			width2 * height2 * sizeof(unsigned short));

	readXYZFile(path1, image1);
	readXYZFile(path2, image2);

	cv::Mat mat1(height1, width1, CV_16U, image1);
	cv::Mat mat2(height2, width2, CV_16U, image2);

	writeAsPNG(mat1, "image1.png");
	writeAsPNG(mat2, "image2.png");

//Preprocessing: Set actual 0 image pixels to value 1, to disregard the background pixels from original pixels
// Blending bugfix

	for (int x = 0; x < mat1.rows; x++) {
		for (int y = 0; y < mat1.cols * 2; y++) {
			if (mat1.at<uchar>(x, y) == 0) {
				mat1.at<uchar>(x, y) = 1;
			}
		}

	}

	for (int y = 0; y < mat2.rows; y++) {
		for (int x = 0; x < mat2.cols * 2; x++) {
			if (mat2.at<uchar>(y, x) == 0) {
				mat2.at<uchar>(y, x) = 1;
			}
		}
	}

	writeAsPNG(mat1, "image1_B.png");
	writeAsPNG(mat2, "image2_B.png");

//calculate combine image size
	int combineHeight = calculateCombinedHeight(height1, height2, Y);
	int combineWidth = calculateCombinedWidth(width1, width2, X);

	cv::Mat combined1 = cv::Mat::zeros(combineHeight, combineWidth, CV_16U);
	cv::Mat combined2 = cv::Mat::zeros(combineHeight, combineWidth, CV_16U);

//Find intersection points
	cv::Point2d topLeft(abs(X), height1 - Y);
	cv::Point2d bottomRight(calculateBottomRightXPoint(width1, width2, X),
			height1);

	if (X < 0) {
		//if X is negative shift, reference image need to be shifted.
		mat1.copyTo(
				combined1(cv::Range(0, height1),
						cv::Range(abs(X), abs(X) + width1)));
		mat2.copyTo(
				combined2(cv::Range(combineHeight - height2, combineHeight),
						cv::Range(0, width2)));

	} else {
		//shift the float image
		mat1.copyTo(combined1(cv::Range(0, height1), cv::Range(0, width1)));
		mat2.copyTo(
				combined2(cv::Range(combineHeight - height2, combineHeight),
						cv::Range(X, X + width2)));
	}

	writeAsPNG(combined1, "combined1.png");
	writeAsPNG(combined2, "combined2.png");

	cv::Mat stitched = cv::Mat::zeros(combineHeight, combineWidth, CV_16U);

//METHOD1. FIXED BLENDING(NOT GOOD)
	cv::addWeighted(combined1, 0.5, combined2, 0.5, 30000, stitched);
	writeAsPNG(stitched, "stitched1.png");

//Combine to blended2
	for (int i = 0; i < combineHeight; i++) {
		for (int j = 0; j < combineWidth * 2; j++) {
			ushort blenValue =
					combined1.at<uchar>(i, j) == 0
							|| combined2.at<uchar>(i, j) == 0 ?
							max(combined1.at<uchar>(i, j),
									combined2.at<uchar>(i, j)) :
							combined1.at<uchar>(i, j) / 2
									+ combined2.at<uchar>(i, j) / 2;
			stitched.at<uchar>(i, j) = blenValue;
		}
		cout << i << endl;
	}

	writeAsPNG(stitched, "stitched2.png");

//Implement blending
//int commonWidth = width1 - abs(X);
	int commonWidth = min(width1, width2) - abs(X);
	int commonHeight = Y;
	cout << "commonWidth" << commonWidth;
	cv::Mat common1 = cv::Mat::zeros(commonHeight, commonWidth, CV_16U);
	cv::Mat common2 = cv::Mat::zeros(commonHeight, commonWidth, CV_16U);
	int startX = X < 0 ? 0 : X;
	mat1(cv::Range(height1 - commonHeight, height1),
			cv::Range(startX, startX + commonWidth)).copyTo(common1);

	startX = X < 0 ? abs(X) : 0;
	mat2(cv::Range(0, commonHeight), cv::Range(startX, startX + commonWidth)).copyTo(
			common2);

//writeAsPNG(common1, "common1.png");
//writeAsPNG(common2, "common2.png");
	cv::Mat commonBlended = cv::Mat::zeros(commonHeight, commonWidth, CV_16U);
//METHOD 2. Vertical Blending
	/*for (int i = 0; i < commonHeight; i++) {
	 for (int j = 0; j < commonWidth * 2; j++) {
	 ushort blendedValue = 0;
	 double alpha = (commonHeight - i) / (double) commonHeight;
	 double beta = i / (double) commonHeight;
	 blendedValue = common1.at<uchar>(i, j) * alpha
	 + common2.at<uchar>(i, j) * beta;
	 commonBlended.at<uchar>(i, j) =
	 common1.at<uchar>(i, j) == 0
	 || common2.at<uchar>(i, j) == 0 ?
	 max(common1.at<uchar>(i, j),
	 common2.at<uchar>(i, j)) :
	 blendedValue;
	 }
	 }
	 writeAsPNG(commonBlended, "VerticalCommonBlended.png");
	 commonBlended.copyTo(
	 blended2(cv::Range(height1 - Y, height1),
	 cv::Range(abs(X), abs(X) + commonWidth)));
	 writeAsPNG(blended2, "stitched.png");*/

//METHOD 3: Bidirectional Blending(Best Method)
	for (int i = 0; i < commonHeight; i++) {
		for (int j = 0; j < max(width2, width1) * 2; j++) {
			ushort blendedValue = 0;
			int image1Distance = 0;
			int image2Distance = 0;
			if (X < 0) {
				image1Distance = min(i, commonWidth * 2 - j);
				image2Distance = min(commonHeight - i, j);
			} else {
				image1Distance = min(i, j);
				image2Distance = min(commonHeight - i, combineWidth * 2 - j);
			}
			double alpha = 1;

			double beta = 0;
			if (image1Distance > 0 || image2Distance > 0) {
				alpha = image2Distance
						/ (double) (image1Distance + image2Distance);
				beta = 1 - alpha;
			}
			blendedValue = common1.at<uchar>(i, j) * alpha
					+ common2.at<uchar>(i, j) * beta;
			commonBlended.at<uchar>(i, j) =
					common1.at<uchar>(i, j) == 0
							|| common2.at<uchar>(i, j) == 0 ?
							max(common1.at<uchar>(i, j),
									common2.at<uchar>(i, j)) :
							blendedValue;

		}

	}

	commonBlended.copyTo(
			stitched(cv::Range(height1 - Y, height1),
					cv::Range(abs(X), abs(X) + commonWidth)));

	return stitched;
}

int calculateCombinedHeight(int height1, int height2, int Y) {
	return height1 + height2 - Y;
}

/**
 * Calculates combined width from shift X
 */
int calculateCombinedWidth(int width1, int width2, int X) {
	int combinedWidth = 0;
	if (width2 <= width1) {
		if (X <= 0) {
			combinedWidth = width1 + abs(X);
		} else {
			int diff = width1 - width2;
			if (diff <= X)
				combinedWidth = width2 + X;
			else
				combinedWidth = width1;
		}
	} else {
		if (X <= 0) {
			int diff = width2 - width1;
			if (diff > X) {
				combinedWidth = width2;
			} else {
				combinedWidth = width1 + X;
			}
		} else {
			combinedWidth = width2 + X;
		}
	}
	return combinedWidth;
}

int calculateBottomRightXPoint(int width1, int width2, int X) {
	int bottomRightPoint = 0;
	if (X <= 0) {
		bottomRightPoint = abs(X) + min(width1, width2);
	} else {
		int diff = abs(width1 - width2);
		if (width2 <= width1) {
			if (diff > X)
				bottomRightPoint = X + width2;
			else
				bottomRightPoint = width1;
		} else {
			bottomRightPoint = width1;
		}
	}
	return bottomRightPoint;
}

void showImage(cv::String title, cv::Mat image) {
	cv::namedWindow(title, cv::WINDOW_AUTOSIZE); // Create a window for display.
	cv::imshow(title, image);
	cv::waitKey(0);
	return;
}

/**
 * Writes image as PNG format
 */
void writeAsPNG(cv::Mat image, cv::String location) {
	vector<int> compression_params;
	compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(9);
	try {
		cv::imwrite(location, image, compression_params);
	} catch (cv::Exception& ex) {
		fprintf(stderr, "Exception converting image to PNG format: %s\n",
				ex.what());
	}
}

void readXYZFile(char *fileName, unsigned short* imageData) {
	FILE * handle;

	unsigned short width;
	unsigned short height;

	handle = fopen(fileName, "rb");
	if (!handle)
		throw cv::Exception(0, "Read file error!", "", "", 0);

	if (fread(&width, sizeof(unsigned short), 1, handle) != 1) {
		fclose(handle);
		throw cv::Exception(0, "Read file error!", "", "", 0);
	}

	if (fread(&height, sizeof(unsigned short), 1, handle) != 1) {
		fclose(handle);
		throw cv::Exception(0, "Read file error!", "", "", 0);
	}

	int size = width * height;
	if (fread(imageData, sizeof(unsigned short), size, handle) != size) {
		fclose(handle);
		throw cv::Exception(0, "Read file error!", "", "", 0);
	}

	fclose(handle);
}

void readXYZFileDimension(char *fileName, unsigned short* width,
		unsigned short* height) {
	FILE * handle;

	handle = fopen(fileName, "rb");
	if (!handle)
		throw cv::Exception(0, "Read file error!", "", "", 0);

	if (fread(width, sizeof(unsigned short), 1, handle) != 1) {
		fclose(handle);
		throw cv::Exception(0, "Read file error!", "", "", 0);
	}

	if (fread(height, sizeof(unsigned short), 1, handle) != 1) {
		fclose(handle);
		throw cv::Exception(0, "Read file error!", "", "", 0);
	}

	fclose(handle);
}

void writeXYZFile(unsigned short * imageData, int width, int height,
		char *fileName) {
	FILE * handle;

	unsigned short x = (unsigned short) width;
	unsigned short y = (unsigned short) height;

	handle = fopen(fileName, "wb");
	if (!handle)
		throw cv::Exception(0, "Write file error!", "", "", 0);

	if (fwrite(&x, sizeof(unsigned short), 1, handle) != 1) {
		fclose(handle);
		throw cv::Exception(0, "Write file error!", "", "", 0);
	}

	if (fwrite(&y, sizeof(unsigned short), 1, handle) != 1) {
		fclose(handle);
		throw cv::Exception(0, "Write file error!", "", "", 0);
	}

	int size = height * width;

	if (fwrite(imageData, sizeof(unsigned short), size, handle) != size) {
		fclose(handle);
		throw cv::Exception(0, "Write file error!", "", "", 0);
	}

	fclose(handle);
}
