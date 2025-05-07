//M*//////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
// Copyright 2018,2022 NXP
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/************************************************************************************\
    This is improved variant of chessboard corner detection algorithm that
    uses a graph of connected quads. It is based on the code contributed
    by Vladimir Vezhnevets and Philip Gruebele.
    Here is the copyright notice from the original Vladimir's code:
    ===============================================================
    The algorithms developed and implemented by Vezhnevets Vldimir
    aka Dead Moroz (vvp@graphics.cs.msu.ru)
    See http://graphics.cs.msu.su/en/research/calibration/opencv.html
    for detailed information.
    Reliability additions and modifications made by Philip Gruebele.
    <a href="mailto:pgruebele@cox.net">pgruebele@cox.net</a>
    Some further improvements for detection of partially ocluded boards at non-ideal
    lighting conditions have been made by Alex Bovyrin and Kurt Kolonige
\************************************************************************************/

#include "src_contours.hpp"

/**************************************************************************************************************
 *
 * @brief  			Search contours in input image
 *
 * @param  	in		const Mat &img - input image
 * 			in/out	CvSeq** root - sequence of contours
 * 			in		CvMemStorage *mem_storage - memory storage
 *			in		int min_size - empiric bound for minimal allowed perimeter for contour squares
 *
 * @return 			Functions returns the number of contours which were found.
 *
 * @remarks 		The function applies adaptive threshold on input image and searches contours in image.
 * 					If 4 contours are found then function has been terminated. Otherwise it changes block size
 * 					for adaptive threshold and tries again.
 *
 **************************************************************************************************************/
int GetContours(const Mat &img, CvSeq** root, CvMemStorage *mem_storage, int min_size)
{
	const int min_dilations = 0;
	const int max_dilations = 0;

	Mat temp_threshold_rgb(img.rows, img.cols, CV_8UC3, Scalar(0, 0, 0, 0));
	
	for (int k = 0; k < 6; k++) {
		int block_size = static_cast<int>((uint)cvRound((float)MIN(img.cols, img.rows) * (k % 2 == 0 ? 0.2 : 0.1)) | 1U);
		for (int dilations = min_dilations; dilations <= max_dilations; dilations++)
		{
			Mat temp_threshold;
			Mat temp_threshold_rgb(img.rows, img.cols, CV_8UC3, Scalar(0, 0, 0, 0));

			/*********************** Thresholding ***************************/
#if 1
			//Adaptive threshold
			adaptiveThreshold(img, temp_threshold, 255.0, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, block_size, static_cast<double>(k / 2) * 5.0);
#else
			// Empiric threshold level
			double mean = cvAvg(img).val[0];
			int thresh_level = cvRound(mean - 10);
			thresh_level = MAX(thresh_level, 10);

			threshold(img, temp_threshold, thresh_level, 255, CV_THRESH_BINARY);
#endif

			/*********************** Generate Quads ***********************/
			IplImage copy = cvIplImage(temp_threshold);
			IplImage* tmp = &copy;

			// create temporary storage for contours and the sequence of pointers to found quadrangles
			Ptr<CvMemStorage> temp_storage;
			temp_storage = cvCreateChildMemStorage( mem_storage );
			*root = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvSeq*), temp_storage );

			 // initialize contour retrieving routine
			CvContourScanner scanner = cvStartFindContours(tmp, temp_storage, sizeof(CvContourEx), RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );

			// get all the contours one by one
			CvSeq *src_contour = 0;
			while( (src_contour = cvFindNextContour( scanner )) != 0 )
			{
		        	CvSeq *dst_contour = 0;
			        CvRect rect = ((CvContour*)src_contour)->rect;

			        // reject contours with too small perimeter
			        if(rect.width*rect.height >= min_size )
		        	{
					int approx_level;
		            		const int min_approx_level = 1, max_approx_level = MAX_CONTOUR_APPROX;
		            		for( approx_level = min_approx_level; approx_level <= max_approx_level; approx_level++ )
		            		{
		                		dst_contour = cvApproxPoly( src_contour, sizeof(CvContour), temp_storage, CV_POLY_APPROX_DP, (float)approx_level );

						if(dst_contour != NULL) {
			                		if( dst_contour->total == 4 ) { 
			                    			break;
							}
						}

		                		// we call this again on its own output, because sometimes
			                	// cvApproxPoly() does not simplify as much as it should.
			                	dst_contour = cvApproxPoly( dst_contour, sizeof(CvContour), temp_storage, CV_POLY_APPROX_DP, (float)approx_level );
				
						if(dst_contour != NULL) {
				                	if( dst_contour->total == 4 ) { 
			        	        		break;
							}
						}
					}
	
					if(dst_contour != NULL) { 	
						// reject non-quadrangles
				       		if((dst_contour->total == 4) && (cvCheckContourConvexity(dst_contour) != 0))
						{
							if(fabs(cvContourArea(dst_contour, CV_WHOLE_SEQ)) > (float)min_size )
							{
								CvPoint pt[4];
								for(int i = 0; i < 4; i++ ) {
									pt[i] = *(CvPoint*)cvGetSeqElem(dst_contour, i);
								}
		
        		  	        	        	if((pt[0].x > 10) && (pt[0].y > 10) && (pt[1].x > 10) && (pt[1].y > 10) &&
          			                		   (pt[2].x > 10) && (pt[2].y > 10) && (pt[3].x > 10) && (pt[3].y > 10)) {
									CvContourEx* parent = (CvContourEx*)(src_contour->v_prev);
									parent->counter++;
									dst_contour->v_prev = (CvSeq*)parent;
									cvSeqPush(*root, &dst_contour);
								}
							}
						}
					}
				}
			}
			// finish contour retrieving
 			cvEndFindContours(&scanner);

			// filter found contours
			if((*root) != NULL) {
	 			FilterContours(root);
			}
		}
		// if 4 contours are detected, then break
		if((*root) != NULL) {
			if((*root)->total == CONTOURS_NUM) {
				break;
			}
		}
	}
	
	if((*root) != NULL) { return ((*root)->total); }
	else { return(0); }
}


/**************************************************************************************************************
 *
 * @brief  			Filter found contours.
 *
 * @param  	in/out	CvSeq** root - sequence of contours
 *
 * @return 			-
 *
 * @remarks 		The function checks all contours from the input sequence and removes contours
 * 					which are not located inside another sequence contour or which do not contain
 * 					another sequence contour.
 *
 **************************************************************************************************************/
void FilterContours(CvSeq** root)
{
	int idx = (*root)->total - 1; // index of the last sequence contour
	int contours_num = (*root)->total; // number of contours
	// for each sequence contour
	while(idx >= 0)
	{
		if(cvGetSeqElem(*root, idx) == NULL) { return; }
		CvSeq * src_contour = *(CvSeq**)cvGetSeqElem(*root, idx); // get idx contour
		if(src_contour == NULL) { return; }
		// get contour points
		vector<Point2f> contour_1;
		for(int i = 0; i < src_contour->total; i++) {
			if(cvGetSeqElem(src_contour, i) != NULL) {
				contour_1.push_back(*(CvPoint*)cvGetSeqElem(src_contour, i));
			}
		}

		bool contours_test = false;
		for(int j = 0; j < contours_num; j++)
		{
			if(cvGetSeqElem(*root, j) != NULL) {
				CvSeq * contour = *(CvSeq**)cvGetSeqElem(*root, j); // get j contour
				// get contour points
				vector<Point2f> contour_2;
				for(int i = 0; i < contour->total; i++) {
					if(cvGetSeqElem(contour, i) != NULL) {
						contour_2.push_back(*(CvPoint*)cvGetSeqElem(contour, i));
					}
				}

				bool is_inside = true; // idx contour is inside another contour
				bool is_outside = true; // another contour is inside the idx contour
				for(int i = 0; i < contour->total; i++) {
					is_inside = is_inside && (pointPolygonTest(contour_1, contour_2[i], false) > 0.0f); // check all points of contour
					is_outside = is_outside && (pointPolygonTest(contour_2, contour_1[i], false) > 0.0f); // check all points of contour
				}
				if(is_inside || is_outside) // if idx contour is inside j contour or j contour is inside idx contour
				{
					contours_test = true; // don't remove contour
					break;
				}
			}
		}
		if(!contours_test)
		{
			cvSeqRemove(*root, idx); // remove contour
			contours_num--; // decrease contours number
		}
		idx--; // next contour
	}
}


/**************************************************************************************************************
 *
 * @brief  			Sort contours from left to right
 *
 * @param  	in		CvSeq** root - sequence of contours
 *
 * @return 			-
 *
 * @remarks 		The function searches min value of contour points in X axis and then sorts all contours
 * 					according to this value from left to right.
 *
 **************************************************************************************************************/
void SortContours(CvSeq** root)
{
	CvSeq * src_contour[CONTOURS_NUM];
	CvPoint pt[CONTOURS_NUM][4];
	int min_x[CONTOURS_NUM] = {0};
	int sort_seq[CONTOURS_NUM] = {0};

	// Search min value of contour points in X axis
    for(int idx = 0; idx < CONTOURS_NUM; idx++)
    {
	if(cvGetSeqElem(*root, idx) != NULL) {
	    	src_contour[idx] = *(CvSeq**)cvGetSeqElem(*root, idx);
	
        	for(int i = 0; i < 4; i++) {
	            if(cvGetSeqElem(src_contour[idx], i) != NULL) {
        	        pt[idx][i] = *(CvPoint*)cvGetSeqElem(src_contour[idx], i);
	            }
        	}

	        min_x[idx] = MIN4(pt[idx][0].x, pt[idx][1].x, pt[idx][2].x, pt[idx][3].x);
        	sort_seq[idx] = idx;
	}
    }

    // Sort contours from left to right
    for(int j = 0; j < CONTOURS_NUM; j++) {
        for(int i = j + 1; i < CONTOURS_NUM; i++)
        {
        	if(min_x[sort_seq[i]] < min_x[sort_seq[j]]){
        		int tmp = sort_seq[j];
        		sort_seq[j] = sort_seq[i];
        		sort_seq[i] = tmp;
        	}
        }
    }

    // Push sorted contours to the input vector
    cvClearSeq(*root);
    for(int idx = 0; idx < CONTOURS_NUM; idx++)
    {
    	cvSeqPush(*root, &src_contour[sort_seq[idx]]);
    }
}


/**************************************************************************************************************
 *
 * @brief  			Generate vector of contour points in proper order.
 *
 * @param  	in		CvSeq** root - sequence of contours
 * 			out		vector<Point2f> * feature_points
 * 			in		Point2f shift
 *
 * @return 			-
 *
 * @remarks 		The function sorts contours corners from top-left clockwise, applies shift on corners
 * 					coordinates and push the corners to the feature_points array in proper order.
 *
 **************************************************************************************************************/
void GetFeaturePoints(CvSeq** root, vector<Point2f> &feature_points, Point2f shift)
{
	if((*root) == NULL) { return; }
	if((*root)-> total != 4) { return; }

    	// Sort contour points clockwise (start from the top left point)
   	for(int idx = 0; idx < 4; idx++)
    	{
		if(cvGetSeqElem(*root, idx) == NULL) { return; }
    		CvSeq * src_contour = *(CvSeq**)cvGetSeqElem(*root, idx);
		if(src_contour == NULL) { return; }
	        CvPoint pt[4];

        	for(int i = 0; i < 4; i++) {
			if(cvGetSeqElem(src_contour, i) != NULL) {
				pt[i] = *(CvPoint*)cvGetSeqElem(src_contour, i);
			}
			else { pt[i] = CvPoint{0, 0}; }
 		}

	        // Calculate Y coordinate of the contour center point
        	float ym;
	       	if(pt[0].x == pt[2].x) {
        	   	ym = static_cast<float>(pt[0].y + pt[2].y) / 2.0f;
	       	}
        	else if(pt[1].x == pt[3].x) {
       			ym = static_cast<float>(pt[1].y + pt[3].y) / 2.0f;
       	 	}
        	else {
			float a1 = static_cast<float>(pt[2].y - pt[0].y) / static_cast<float>(pt[2].x - pt[0].x);
			float b1 = static_cast<float>(pt[0].y * pt[2].x - pt[2].y * pt[0].x) / static_cast<float>(pt[2].x - pt[0].x);

			float a2 = static_cast<float>(pt[3].y - pt[1].y) / static_cast<float>(pt[3].x - pt[1].x);
			float b2 = static_cast<float>(pt[1].y * pt[3].x - pt[3].y * pt[1].x) / static_cast<float>(pt[3].x - pt[1].x);

			ym = (a1 * b2 - a2 * b1) / (a1 - a2);
        	}

        	// Sort contours corners from top-left clockwise
        	CvPoint pt_[4] = {0};
        	for(uint i = 0U; i < 4U; i++)
        	{
			bool flag = false;
        		// Contours found by findContours function has direction. Objects are counter-clockwise, and holes are clockwise
	        	if(CV_IS_SEQ_HOLE(src_contour)) { // Holes
        			if(((float)pt[i].y <= ym) && ((float)pt[(i + 1U) & 3U].y <= ym)){
        		        	pt_[0] = pt[i];
        	        		pt_[1] = pt[(i + 1U) & 3U];
        	        		pt_[2] = pt[(i + 2U) & 3U];
	        	        	pt_[3] = pt[(i + 3U) & 3U];
					flag = true;
        			}
	        	}
        		else { // Objects
        			if(((float)pt[i].y <= ym) && ((float)pt[(i + 3U) & 3U].y <= ym)){
					pt_[0] = pt[i];
 					pt_[1] = pt[(i + 3U) & 3U];
					pt_[2] = pt[(i + 2U) & 3U];
					pt_[3] = pt[(i + 1U) & 3U];
					flag = true;
        			}
	        	}
			if (flag) { break; }
	        }
        	for(int i = 0; i < 4; i++) {
	        	feature_points.push_back(Point2f((float)pt_[i].x + shift.x, (float)pt_[i].y + shift.y));
		}
    	}
}


/**************************************************************************************************************
 *
 * @brief  			Convert contours sequence into the vector of points.
 *
 * @param  	in		CvSeq** root - sequence of contours
 * 			out		vector<Point2f> * feature_points
 * 			in		Point2f shift
 *
 * @return 			-
 *
 * @remarks 		The function convert sequence of contours into points array. The shift is applied on corners
 * 					coordinates.
 *
 **************************************************************************************************************/
void sec2vector(CvSeq** root, vector<Point2f> &feature_points, Point2f shift)
{
	if((*root) != NULL) {
		for (int idx = 0; idx < (*root)->total; idx++)
		{
			if(cvGetSeqElem(*root, idx) != NULL) {
				CvSeq * src_contour = *(CvSeq**)cvGetSeqElem(*root, idx);
				if(src_contour == NULL) { return; }
				for (int i = 0; i < (*root)->elem_size; i++) {
					if(cvGetSeqElem(src_contour, i) != NULL) {
						CvPoint pt = *(CvPoint*)cvGetSeqElem(src_contour, i);
 						feature_points.push_back(Point2f((float)pt.x + shift.x, (float)pt.y + shift.y));
					}
				}
			}
		}
	}
}
