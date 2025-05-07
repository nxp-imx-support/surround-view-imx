/*
*
* Copyright 2017,2022 NXP
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
#include "view.hpp"
#include <opencv2/imgproc/types_c.h>

using namespace std;
using namespace cv;

/***************************************************************************************
***************************************************************************************/
View::View(void)
{
	current_prog = 0;
	glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);  
	glDisable(GL_DEPTH_TEST);	
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/***************************************************************************************
***************************************************************************************/
View::~View(void)
{
	for (int i = (int)v4l2_cameras.size() - 1; i >= 0; i--)	{
		v4l2_cameras[i].stopCapturing();		
	}
	for (int i = (int)v_obj.size() - 1; i >= 0; i--) {
		glDeleteTextures(1, &v_obj[i].tex);
		glDeleteBuffers(1, &v_obj[i].vbo);
	}
	for (int i = (int)render_prog.size() - 1; i >= 0; i--) {
		render_prog[i].destroyShaders();
	}
}


/***************************************************************************************
***************************************************************************************/
int View::addProgram(const char* v_shader, const char* f_shader)
{
	int result = -1;
	// load and compiler vertex/fragment shaders.
	Programs new_prog;
	if (new_prog.loadShaders(v_shader, f_shader) == -1) // Non-overlap regions
	{
		cout << "Render program was not loaded" << endl;
	}
	else
	{
		render_prog.push_back(new_prog);
		result = (int)render_prog.size() - 1;
	}
	return result;
}


/***************************************************************************************
***************************************************************************************/
int View::setProgram(uint index)
{
	int result = 0;
	if (index >= render_prog.size())
	{
		cout << "A program with index " << index << " doesn't exist" << endl;
		result = -1;		
	}
	else 
	{
		current_prog = (int)index;	
		glUseProgram(render_prog[index].getHandle());
	}
	return result;
}


/***************************************************************************************
***************************************************************************************/
void View::cleanView(void)
{
	// Clear background.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


/***************************************************************************************
***************************************************************************************/
int View::addCamera(string device, int width, int height)
{
	v4l2Camera v4l2_camera(width, height, CAM_PIXEL_TYPE, V4L2_MEMORY_MMAP, device.c_str());
	v4l2_cameras.push_back(v4l2_camera);
	
	int current_index = (int)v4l2_cameras.size() - 1;

	if (v4l2_cameras[current_index].captureSetup(gst_shared) == -1)
	{
		cout << "v4l_capture_setup failed camera " << current_index << endl;
		current_index = -1;
	}

	return current_index;
}



/***************************************************************************************
***************************************************************************************/
int View::runCamera(int index)
{
	cout<<"enter @runCamera"<<endl;
	int result = 0;

	if (v4l2_cameras[index].startCapturing() == -1) { result = -1; }
	else  {
		if (v4l2_cameras[index].getFrame() == -1) { result = -1; }
	}

	cout<<"exit @runCamera, result: "<< result <<endl;
	return result;
}


/***************************************************************************************
***************************************************************************************/
int View::addMesh(string filename)
{
	///////////////////////////////// Load vertices arrays ///////////////////////////////
	vertices_obj vo_tmp;
	GLfloat* vert;
	vLoad(&vert, &vo_tmp.num, filename);
		
	//////////////////////// Camera textures initialization /////////////////////////////
	glGenVertexArrays(1, &vo_tmp.vao);
	glGenBuffers(1, &vo_tmp.vbo);

	bufferObjectInit(&vo_tmp.vao, &vo_tmp.vbo, vert, vo_tmp.num);
	texture2dInit(&vo_tmp.tex);
	
	v_obj.push_back(vo_tmp); 
	
	if(vert != NULL) { free(vert); }
	
	return ((int)v_obj.size() - 1);
}


/***************************************************************************************
***************************************************************************************/
void View::renderView(int camera, int mesh)
{
	// Render camera frames
	int i;

	// Lock the camera frame
	if (pthread_mutex_lock(&v4l2_cameras[camera].th_mutex) != (int)0) {                                        
		cout << "pthread_mutex_lock() error" << endl;
  	}

	// Get index of the newes camera buffer
	if (v4l2_cameras[camera].fill_buffer_inx == -1) { i = 0; }
	else  { i = v4l2_cameras[camera].fill_buffer_inx; }

	glBindVertexArray(v_obj[mesh].vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, v_obj[mesh].tex);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES,v4l2_cameras[camera].getFrame());
	glUniform1i(glGetUniformLocation(render_prog[current_prog].getHandle(), "myTexture"), 0);
	
	glDrawArrays(GL_TRIANGLES, 0, v_obj[mesh].num);
	glBindVertexArray(0);
	glFinish();
	
	// Release camera frame
	if (pthread_mutex_unlock(&v4l2_cameras[camera].th_mutex) != (int)0) {                                      
    		cout << "pthread_mutex_unlock() error" << endl;                                     
	}   
}

/***************************************************************************************
***************************************************************************************/
// 2D texture init
void View::texture2dInit(GLuint* texture)
{
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

/***************************************************************************************
***************************************************************************************/
void View::bufferObjectInit(GLuint* text_vao, GLuint* text_vbo, GLfloat* vert, int num)
{
	// rectangle
	glBindBuffer(GL_ARRAY_BUFFER, *text_vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 5 * num, &vert[0], GL_DYNAMIC_DRAW);
	glBindVertexArray(*text_vao);
	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLfloat) * 5, (GLvoid*)0);
	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLfloat) * 5, (GLvoid*)(3U * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

/***************************************************************************************
***************************************************************************************/
// Load vertices arrays
void View::vLoad(GLfloat** vert, int* num, string filename)
{
	ifstream input(filename.c_str());
	*num = (int)count(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>(), '\n'); // Get line number from the array file
	input.clear();
	input.seekg(0, ios::beg); // Returning to the beginning of fstream

	*vert = NULL; 
	*vert = (GLfloat*)malloc((size_t)(*num) * 5U * (size_t)sizeof(GLfloat));
	if (*vert == NULL) {
		cout << "Memory allocation did not complete successfully" << endl; 
	} 
	else {
		for (int k = 0; k < (*num) * 5; k++)
		{
			input >> (*vert)[k];
		}
	}
	input.close();
}

/***************************************************************************************
***************************************************************************************/
int View::createMesh(Mat xmap, Mat ymap, string filename, int density, Point2f top)
{
	if ((xmap.rows == 0) || (xmap.cols == 0) || (ymap.rows == 0) || (ymap.cols == 0))
	{
		cout << "Mesh was not generated. LUTs are empty" << endl;
		return (-1);	
	}		
	
	int rows = xmap.rows / density;
	int cols = xmap.cols / density;
	
	float x_norm = 1.0f / (float)xmap.cols;
	float y_norm = 1.0f / (float)xmap.rows;
	
	ofstream outC; // Output vertices/texels file
	outC.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc); // Any contents that existed in the file before it is open are discarded.
	for (int row = 1; row < rows; row++)
	{
		for (int col = 1; col < cols; col++)
		{
		    /****************************************** Get triangles *********************************************
		     *   							  v3 _  v2
		     *   Triangles orientation: 		| /|		1 triangle (v4-v1-v2)
		     *   								|/_|		2 triangle (v4-v2-v3)
		     *   							  v4   v1
		     *******************************************************************************************************/
			// Vertices
			Point2f v1 = Point2f(col * density, row * density);
			Point2f v2 = Point2f(col * density, (row - 1) * density);
			Point2f v3 = Point2f((col - 1) * density, (row - 1) * density);
			Point2f v4 = Point2f((col - 1) * density, row * density);

					        	// Texels
			Point2f p1 = Point2f(xmap.at<float>(v1), ymap.at<float>(v1));
			Point2f p2 = Point2f(xmap.at<float>(v2), ymap.at<float>(v2));
			Point2f p3 = Point2f(xmap.at<float>(v3), ymap.at<float>(v3));
			Point2f p4 = Point2f(xmap.at<float>(v4), ymap.at<float>(v4));

			if ((p2.x > 0.0f) && (p2.y > 0.0f) && (p2.x < (float)xmap.cols) && (p2.y < (float)xmap.rows) &&	// Check if p2 belongs to the input frame
			   (p4.x > 0.0f) && (p4.y > 0.0f) && (p4.x < (float)xmap.cols) && (p4.y < (float)xmap.rows))		// Check if p4 belongs to the input frame
			{
				// Save triangle points to the output file
				/*******************************************************************************************************
				 *   							  		v2
				 *   1 triangle (v4-v1-v2): 		  /|
				 *   								 /_|
				 *   							  v4   v1
				 *******************************************************************************************************/
				if ((p1.x > 0.0f) && (p1.y > 0.0f) && (p1.x < (float)xmap.cols) && (p1.y < (float)xmap.rows))	// Check if p1 belongs to the input frame
				{
					outC << (top.x + v1.x * x_norm)  << " " << (top.y - v1.y * y_norm) << " " << 0 << " " << p1.x * x_norm << " " << p1.y * y_norm << endl;
					outC << (top.x + v2.x * x_norm)  << " " << (top.y - v2.y * y_norm) << " " << 0 << " " << p2.x * x_norm << " " << p2.y * y_norm << endl;
					outC << (top.x + v4.x * x_norm)  << " " << (top.y - v4.y * y_norm) << " " << 0 << " " << p4.x * x_norm << " " << p4.y * y_norm << endl;
				}

				/*******************************************************************************************************
				 *   							  v3 _	v2
				 *   2 triangle (v4-v2-v3): 		| /
				 *   								|/
				 *   							  v4
				 *******************************************************************************************************/
				if ((p3.x > 0.0f) && (p3.y > 0.0f) && (p3.x < (float)xmap.cols) && (p3.y < (float)xmap.rows))	// Check if p3 belongs to the input frame)
				{
					outC << (top.x + v4.x * x_norm) << " " << (top.y - v4.y * y_norm) << " " << 0 << " " << p4.x * x_norm << " " << p4.y * y_norm << endl;
					outC << (top.x + v2.x * x_norm) << " " << (top.y - v2.y * y_norm) << " " << 0 << " " << p2.x * x_norm << " " << p2.y * y_norm << endl;
					outC << (top.x + v3.x * x_norm) << " " << (top.y - v3.y * y_norm) << " " << 0 << " " << p3.x * x_norm << " " << p3.y * y_norm << endl;
				}
			}
		}
	}
	outC.close(); // Close file	
	return(0);
}

/***************************************************************************************
***************************************************************************************/

void View::reloadMesh(int index, string filename)
{
	GLfloat* vert;
	vLoad(&vert, &v_obj[index].num, filename);	
	glBindBuffer(GL_ARRAY_BUFFER, v_obj[index].vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 5 * v_obj[index].num, &vert[0], GL_DYNAMIC_DRAW);
	if(vert != NULL) { free(vert); }
}



/***************************************************************************************
***************************************************************************************/
int View::changeMesh(Mat xmap, Mat ymap, int density, Point2f top, int index)
{
	if ((xmap.rows == 0) || (xmap.cols == 0) || (ymap.rows == 0) || (ymap.cols == 0))
	{
		cout << "Mesh was not generated. LUTs are empty" << endl;
		return (-1);	
	}		
	
	int rows = xmap.rows / density;
	int cols = xmap.cols / density;
	
	GLfloat* vert = NULL;
	v_obj[index].num = 6 * rows * cols;
	vert = (GLfloat*)calloc((size_t)v_obj[index].num * (size_t)5, sizeof(GLfloat));

	if (vert == NULL) {
		cout << "Memory allocation did not complete successfully" << endl; 
		return(-1);
	} 
	
	float x_norm = 1.0f / (float)xmap.cols;
	float y_norm = 1.0f / (float)xmap.rows;
	
	
	int k =  0;
	for (int row = 1; row < rows; row++) {
		for (int col = 1; col < cols; col++)
		{
		    /****************************************** Get triangles *********************************************
		     *   							  v3 _  v2
		     *   Triangles orientation: 		| /|		1 triangle (v4-v1-v2)
		     *   								|/_|		2 triangle (v4-v2-v3)
		     *   							  v4   v1
		     *******************************************************************************************************/
			// Vertices
			Point2f v1 = Point2f(col * density, row * density);
			Point2f v2 = Point2f(col * density, (row - 1) * density);
			Point2f v3 = Point2f((col - 1) * density, (row - 1) * density);
			Point2f v4 = Point2f((col - 1) * density, row * density);

					        	// Texels
			Point2f p1 = Point2f(xmap.at<float>(v1), ymap.at<float>(v1));
			Point2f p2 = Point2f(xmap.at<float>(v2), ymap.at<float>(v2));
			Point2f p3 = Point2f(xmap.at<float>(v3), ymap.at<float>(v3));
			Point2f p4 = Point2f(xmap.at<float>(v4), ymap.at<float>(v4));

			if ((p2.x > 0.0f) && (p2.y > 0.0f) && (p2.x < (float)xmap.cols) && (p2.y < (float)xmap.rows) &&	// Check if p2 belongs to the input frame
			   (p4.x > 0.0f) && (p4.y > 0.0f) && (p4.x < (float)xmap.cols) && (p4.y < (float)xmap.rows))		// Check if p4 belongs to the input frame
			{
				// Save triangle points to the output file
				/*******************************************************************************************************
				 *   							  		v2
				 *   1 triangle (v4-v1-v2): 		  /|
				 *   								 /_|
				 *   							  v4   v1
				 *******************************************************************************************************/
				if ((p1.x >= 0.0f) && (p1.y >= 0.0f) && (p1.x < (float)xmap.cols) && (p1.y < (float)xmap.rows))	// Check if p1 belongs to the input frame
				{
					vert[k] = v1.x * x_norm + top.x;
					vert[k + 1] = (top.y - v1.y * y_norm);
					vert[k + 2] = 0.0f;
					vert[k + 3] = p1.x * x_norm;
					vert[k + 4] = p1.y * y_norm;
					
					vert[k + 5] = v2.x * x_norm + top.x;
					vert[k + 6] = (top.y - v2.y * y_norm);
					vert[k + 7] = 0.0f;
					vert[k + 8] = p2.x * x_norm;
					vert[k + 9] = p2.y * y_norm;

					vert[k + 10] = v4.x * x_norm + top.x;
					vert[k + 11] = (top.y - v4.y * y_norm);
					vert[k + 12] = 0.0f;
					vert[k + 13] = p4.x * x_norm;
					vert[k + 14] = p4.y * y_norm;
					
					k += 15;
				}

				/*******************************************************************************************************
				 *   							  v3 _	v2
				 *   2 triangle (v4-v2-v3): 		| /
				 *   								|/
				 *   							  v4
				 *******************************************************************************************************/
				if ((p3.x > 0.0f) && (p3.y > 0.0f) && (p3.x < (float)xmap.cols) && (p3.y < (float)xmap.rows))	// Check if p3 belongs to the input frame)
				{
					vert[k] = v4.x * x_norm + top.x;
					vert[k + 1] = (top.y - v4.y * y_norm);
					vert[k + 2] = 0.0f;
					vert[k + 3] = p4.x * x_norm;
					vert[k + 4] = p4.y * y_norm;
					
					vert[k + 5] = v2.x * x_norm + top.x;
					vert[k + 6] = (top.y - v2.y * y_norm);
					vert[k + 7] = 0.0f;
					vert[k + 8] = p2.x * x_norm;
					vert[k + 9] = p2.y * y_norm;

					vert[k + 10] = v3.x * x_norm + top.x;
					vert[k + 11] = (top.y - v3.y * y_norm);
					vert[k + 12] = 0.0f;
					vert[k + 13] = p3.x * x_norm;
					vert[k + 14] = p3.y * y_norm;
					
					k += 15;
				}
			}
		}
	}
	

	glBindBuffer(GL_ARRAY_BUFFER, v_obj[index].vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 5 * v_obj[index].num, &vert[0], GL_DYNAMIC_DRAW);
	free(vert);
	return (0);
}

/***************************************************************************************
***************************************************************************************/
Mat View::takeFrame(int index)
{
	Mat out;

	// Lock the camera frame
	if (pthread_mutex_lock(&v4l2_cameras[index].th_mutex) != (int)0) {                                        
		cout << "pthread_mutex_lock() error" << endl;	       
	}

	GLubyte* pixels = new GLubyte[v4l2_cameras[index].getWidth() * v4l2_cameras[index].getHeight() * 4];
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES, v4l2_cameras[index].getFrame(), 0);
	glReadPixels(0, 0, v4l2_cameras[index].getWidth(), v4l2_cameras[index].getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES,0);

	Mat rgba = Mat(v4l2_cameras[index].getHeight(), v4l2_cameras[index].getWidth(), CV_8UC4, pixels);
	out = Mat(v4l2_cameras[index].getHeight(), v4l2_cameras[index].getWidth(), CV_8UC3);
	cvtColor(rgba, out, CV_RGBA2BGR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);

	glFinish();
	// Release camera frame
	if (pthread_mutex_unlock(&v4l2_cameras[index].th_mutex) != (int)0) {                                      
    		cout << "pthread_mutex_unlock() error" << endl;
	} 
	return out;
}


/***************************************************************************************
***************************************************************************************/
int View::addBuffer(GLfloat* buf, int num)
{
	///////////////////////////////// Load vertices arrays ///////////////////////////////
	vertices_obj vo_tmp;
	vo_tmp.num = num;
		
	//////////////////////// Camera textures initialization /////////////////////////////
	glGenVertexArrays(1, &vo_tmp.vao);
	glGenBuffers(1, &vo_tmp.vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vo_tmp.vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 3 * num, &buf[0], GL_DYNAMIC_DRAW);
	
	v_obj.push_back(vo_tmp); 
	
	return ((int)v_obj.size() - 1);
}

/***************************************************************************************
***************************************************************************************/
int View::setBufferAsAttr(int buf_num, int prog_num, const char* atr_name)
{
	if ((buf_num < 0) || (buf_num >= (int)v_obj.size()) || (prog_num < 0) || (prog_num >= (int)render_prog.size())) {
		return (-1);
	}
	glBindBuffer(GL_ARRAY_BUFFER, v_obj[buf_num].vbo);
	glBindVertexArray(v_obj[buf_num].vao);
	GLint position_attribute = glGetAttribLocation(render_prog[prog_num].getHandle(), atr_name);
	glVertexAttribPointer((GLuint)position_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray((GLuint)position_attribute);
	return (0);
}


/***************************************************************************************
***************************************************************************************/
void View::renderBuffer(int buf_num, int primitive_type, int vert_num)
{
	glBindVertexArray(v_obj[buf_num].vao);
	switch (primitive_type)
	{
	case 0: // lines
		glLineWidth(2.0f);
		glDrawArrays(GL_LINES, 0, vert_num);
		break;
	default: // points
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, vert_num);
		glEndTransformFeedback();
		break;		
	}
}

/***************************************************************************************
***************************************************************************************/
void View::updateBuffer(int buf_num, GLfloat* buf, int num)
{
	v_obj[buf_num].num = num;
	glBindBuffer(GL_ARRAY_BUFFER, v_obj[buf_num].vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 3 * num, &buf[0], GL_DYNAMIC_DRAW);
}
