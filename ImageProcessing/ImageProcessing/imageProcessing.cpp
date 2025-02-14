#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <atomic>
#include <omp.h>

namespace fs = std::filesystem;
using namespace cv;

// Below Function is used to collect all .jpg and .png file paths in the specified directory
std::vector<std::string> getImagePaths(const std::string& directory) {
    std::vector<std::string> paths;
    for (const auto& entry : fs::directory_iterator(directory)) {
        auto extension = entry.path().extension().string();
        if (extension == ".jpg" || extension == ".png") {
            paths.push_back(entry.path().string());
        }
    }
    return paths;
}

// Below Function to create a binary mask and return the count of "max" pixels
cv::Mat createBinaryMask(const cv::Mat& image, int& pixelCount) {
    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8U);
    pixelCount = 0;

    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            cv::Vec3b color = image.at<cv::Vec3b>(y, x);
            if (color[0] > 200 && color[1] > 200 && color[2] > 200) //Checking with all the channels 
            {
                mask.at<uchar>(y, x) = 255;
                pixelCount++;
            }
        }
    }
    return mask;
}

// Function to process a single image: create mask, count pixels, and save mask
void processImage(const std::string& imagePath, std::atomic<int>& totalPixelCount, int index, const std::string& outputDir) {
    cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Could not open or find the image: " << imagePath << std::endl;
        return;
    }

    int pixelCount = 0;
    cv::Mat mask = createBinaryMask(image, pixelCount);
    totalPixelCount += pixelCount;

    // Save mask as a .png file
    std::string maskPath = outputDir + "/mask_" + std::to_string(index) + ".png";
    cv::imwrite(maskPath, mask);
    std::cout << "Processed image " << imagePath << " in thread " << omp_get_thread_num() << std::endl;  // Debug info
}

// Function to save the total pixel count to a text file
void saveTotalPixelCount(const std::string& filePath, int totalPixelCount) {
    std::ofstream outputFile(filePath);
    if (outputFile.is_open()) {
        outputFile << "Total number of 'max' pixels across all images: " << totalPixelCount << std::endl;
        outputFile.close();
        std::cout << "Total pixel count written to " << filePath << std::endl;
    }
    else {
        std::cerr << "Unable to open file for writing." << std::endl;
    }
    std::cout << "Total Pixel counts are " << totalPixelCount << std::endl;
}

int main() {
    std::string imageDir = "E:\\EagleViewTesst\\imagesToload"; //Please Enter  Image directory here
    std::string outputDir = "E:\\EagleViewTesst\\Total_pixel"; //Please Enter Output directory here
    std::string pixelCountFile = outputDir + "\\pixel_count.txt";

    std::vector<std::string> imagePaths = getImagePaths(imageDir);
    std::atomic<int> totalPixelCount = 0;

    // Process images in parallel
#pragma omp parallel for
    for (int i = 0; i < imagePaths.size(); ++i) {
        processImage(imagePaths[i], totalPixelCount, i, outputDir);
    }

    // Save total pixel count to a file
    saveTotalPixelCount(pixelCountFile, totalPixelCount.load());

    return 0;
}
