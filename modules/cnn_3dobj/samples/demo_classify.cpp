/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @file demo_classify.cpp
 * @brief Feature extraction and classification.
 * @author Yida Wang
 */
#define HAVE_CAFFE
#include <opencv2/cnn_3dobj.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <iomanip>
using namespace cv;
using namespace std;
using namespace cv::cnn_3dobj;

/**
 * @function listDir
 * @brief Making all files names under a directory into a list
 */
void listDir(const char *path, std::vector<string>& files, bool r)
{
    DIR *pDir;
    struct dirent *ent;
    char childpath[512];
    pDir = opendir(path);
    memset(childpath, 0, sizeof(childpath));
    while ((ent = readdir(pDir)) != NULL)
    {
        if (ent->d_type & DT_DIR)
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            {
                continue;
            }
            if(r)
            {
                sprintf(childpath, "%s/%s", path, ent->d_name);
                listDir(childpath,files,false);
            }
        }
        else
        {
            files.push_back(ent->d_name);
        }
    }
    sort(files.begin(),files.end());
};

/**
 * @function main
 */
int main(int argc, char** argv)
{
    const String keys = "{help | | This sample will extract featrues from reference images and target image for classification. You can add a mean_file if there little variance in data such as human faces, otherwise it is not so useful}"
    "{src_dir | ../data/images_all/ | Source direction of the images ready for being used for extract feature as gallery.}"
    "{caffemodel | ../../testdata/cv/3d_triplet_iter_30000.caffemodel | caffe model for feature exrtaction.}"
    "{network_forIMG | ../../testdata/cv/3d_triplet_testIMG.prototxt | Network definition file used for extracting feature from a single image and making a classification}"
    "{mean_file | no | The mean file generated by Caffe from all gallery images, this could be used for mean value substraction from all images. If you want to use the mean file, you can set this as ../data/images_mean/triplet_mean.binaryproto.}"
    "{target_img | ../data/images_all/4_78.png | Path of image waiting to be classified.}"
    "{feature_blob | feat | Name of layer which will represent as the feature, in this network, ip1 or feat is well.}"
    "{num_candidate | 15 | Number of candidates in gallery as the prediction result.}"
    "{device | CPU | Device type: CPU or GPU}"
    "{dev_id | 0 | Device id}";
    /* get parameters from comand line */
    cv::CommandLineParser parser(argc, argv, keys);
    parser.about("Feature extraction and classification");
    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }
    string src_dir = parser.get<string>("src_dir");
    string caffemodel = parser.get<string>("caffemodel");
    string network_forIMG   = parser.get<string>("network_forIMG");
    string mean_file    = parser.get<string>("mean_file");
    string target_img   = parser.get<string>("target_img");
    string feature_blob = parser.get<string>("feature_blob");
    int num_candidate = parser.get<int>("num_candidate");
    string device = parser.get<string>("device");
    int dev_id = parser.get<int>("dev_id");
    /* Initialize a net work with Device */
    cv::cnn_3dobj::descriptorExtractor descriptor(device);
    std::cout << "Using" << descriptor.getDeviceType() << std::endl;
    /* Load net with the caffe trained net work parameter and structure */
    if (strcmp(mean_file.c_str(), "no") == 0)
        descriptor.loadNet(network_forIMG, caffemodel);
    else
        descriptor.loadNet(network_forIMG, caffemodel, mean_file);
    std::vector<string> name_gallery;
    /* List the file names under a given path */
    listDir(src_dir.c_str(), name_gallery, false);
    for (unsigned int i = 0; i < name_gallery.size(); i++)
    {
        name_gallery[i] = src_dir + name_gallery[i];
    }
    std::vector<cv::Mat> img_gallery;
    cv::Mat feature_reference;
    for (unsigned int i = 0; i < name_gallery.size(); i++)
    {
        img_gallery.push_back(cv::imread(name_gallery[i], -1));
    }
    /* Extract feature from a set of images */
    descriptor.extract(img_gallery, feature_reference, feature_blob);
    std::cout << std::endl << "---------- Prediction for " << target_img << " ----------" << std::endl;
    cv::Mat img = cv::imread(target_img, -1);
    std::cout << std::endl << "---------- Features of gallery images ----------" << std::endl;
    std::vector<std::pair<string, float> > prediction;
    /* Print features of the reference images. */
    for (unsigned int i = 0; i < feature_reference.rows; i++)
        std::cout << feature_reference.row(i) << endl;
    cv::Mat feature_test;
    descriptor.extract(img, feature_test, feature_blob);
    /* Initialize a matcher which using L2 distance. */
    cv::BFMatcher matcher(NORM_L2);
    std::vector<std::vector<cv::DMatch> > matches;
    /* Have a KNN match on the target and reference images. */
    matcher.knnMatch(feature_test, feature_reference, matches, num_candidate);
    /* Print feature of the target image waiting to be classified. */
    std::cout << std::endl << "---------- Features of target image: " << target_img << "----------" << endl << feature_test << std::endl;
    /* Print the top N prediction. */
    std::cout << std::endl << "---------- Prediction result(Distance - File Name in Gallery) ----------" << std::endl;
    for (size_t i = 0; i < matches[0].size(); ++i)
    {
        std::cout << i << " - " << std::fixed << std::setprecision(2) << name_gallery[matches[0][i].trainIdx] << " - \""  << matches[0][i].distance << "\"" << std::endl;
    }
    return 0;
}