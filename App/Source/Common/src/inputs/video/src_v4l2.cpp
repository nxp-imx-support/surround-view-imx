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

#include "src_v4l2.hpp"


static int th_num = 0;
static int camera_count = 0;
static int cur_num = 0;

static int current_frame = 0;
static pthread_mutex_t frame_mutex = PTHREAD_MUTEX_INITIALIZER;


int v4l2Camera::exit_flag = 0; // Exit flag
pthread_mutex_t v4l2Camera::th_mutex = PTHREAD_MUTEX_INITIALIZER;	// Mutex for camera access sinchronization

/**************************************************************************************************************
 *
 * @brief  			v4l2Camera class constructor.
 *
 * @param  in 		int in_width - input frame width
 *					int in_height - input frame height
 *					int in_pixel_fmt - camera pixel format
 *					int in_mem_type - memory type
 *					const char* in_device - camera device name
 *
 * @return 			The function creates the v4l2Camera object.
 *
 * @remarks 		The function creates v4l2Camera object and initializes object attributes.
 *
 **************************************************************************************************************/
v4l2Camera::v4l2Camera(int in_width, int in_height, int in_pixel_fmt, int in_mem_type, const char* in_device)
{
	camera_num = ++camera_count;
	
	width = in_width;
	height = in_height;
	pixel_fmt = in_pixel_fmt;
	mem_type = in_mem_type;
	fill_buffer_inx = 0;

	//init gstreamer struct
	//gst_context = (gst_data){(GMainLoop*)0x255,(GstGLDisplayEGL*)0x254,(GstGLContext*)0x253,(GstElement*)0x252,0};

	th_arg = (thread_arg){0, "", NULL, NULL};

	device = string(in_device) + ".avi";
	//device = string(in_device) + ".mp4";
	//cout<<"Camera Device : "<< device << endl;

	pthread_mutex_lock(&frame_mutex);
	th_num++;
	pthread_mutex_unlock(&frame_mutex);
}

/**************************************************************************************************************
 *
 * @brief  			v4l2Camera class destructor.
 *
 * @param  in 		-
 *
 * @return 			The function deletes the v4l2Camera object.
 *
 * @remarks 		The function deletes v4l2Camera object and cleans object attributes.
 *
 **************************************************************************************************************/
v4l2Camera::~v4l2Camera(void)
{

}

/**************************************************************************************************************
 *
 * @brief  			Setup camera capturing
 *
 * @param   		-
 *
 * @return 			The function returns 0 if capturing device was set successfully. Otherwise -1 has been returned.
 *
 * @remarks 		The function opens camera devices and sets capturing mode.
 *
 **************************************************************************************************************/

gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
    //GMainLoop *loop = (GMainLoop*)data;
	GstElement *pipeline = GST_ELEMENT(data);
	//printf("Got %s message\n", GST_MESSAGE_TYPE_NAME (msg));
	g_print ("Got %s message\n", GST_MESSAGE_TYPE_NAME (msg));

    switch (GST_MESSAGE_TYPE (msg))
    {
        case GST_MESSAGE_STATE_CHANGED:
		{
			GstState old_state, new_state, pending_state;

        	gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

			/* we only care about pipeline state change messages */
			if (GST_MESSAGE_SRC (msg) != GST_OBJECT_CAST (pipeline))
				break;

			/* dump graph for pipeline state changes */
			gchar *dump_name = g_strdup_printf("SV_%s_%s_%s",
				GST_MESSAGE_SRC_NAME(msg),
				gst_element_state_get_name (old_state),
              	gst_element_state_get_name (new_state));
        
          	GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN (pipeline),GST_DEBUG_GRAPH_SHOW_ALL, dump_name);
          	g_free (dump_name);
			break;
		}
		case GST_MESSAGE_EOS:
		{
		//loop the pipeline on EOS
			g_print ("End-of-stream\n");
			if (!gst_element_seek(pipeline, 
				1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
				GST_SEEK_TYPE_SET,  0, 
				GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
				g_print("Seek failed!\n");
			}
              //g_main_loop_quit (loop);
			break;
		}
        case GST_MESSAGE_ERROR:
        {
			gchar *debug = NULL;
			GError *err = NULL;

			gst_message_parse_error (msg, &err, &debug);

			g_print ("Error: %s\n", err->message);
			g_error_free (err);

			if (debug)
			{
				g_print ("Debug deails: %s\n", debug);
				g_free (debug);
			}

			//g_main_loop_quit (loop);
			break;
        }
        default:
          break;
    }

    return TRUE;
}

GstPadProbeReturn v4l2Camera::OnQuery(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
	//DirectVideoToTexture* ctx = static_cast<DirectVideoToTexture*>(data);
	v4l2Camera* self = static_cast<v4l2Camera*>(data);

	// cout << "__________________________________________________" << endl;
	// cout << " Query CB for Cam " << self->camera_num << endl;
	// cout<<"self addr @onQuery: "<< (void*)(self) <<endl;
	// cout<<"gst_pipeline addr @onQuery: "<< (void*)&(self->gst_pipeline) <<endl;
	// cout<<"gst_pipeline val @onQuery: "<< (void*)(self->gst_pipeline) <<endl;
	// cout<<"gst_display addr @onQuery: "<< (void*)&(self->gst_display) <<endl;
	// cout<<"gst_display val @onQuery: "<< (void*)(self->gst_display) <<endl;
	// cout<<"gst_gl_context addr @onQuery: "<< (void*)&(self->gst_gl_context) <<endl;
	// cout<<"gst_gl_context val @onQuery: "<< (void*)(self->gst_gl_context) <<endl;
	// cout << "__________________________________________________" << endl << endl ;
	
	
	GstQuery* query = GST_PAD_PROBE_INFO_QUERY(info);

	switch (GST_QUERY_TYPE(query))
	{
		case GST_QUERY_CONTEXT:
			if (gst_gl_handle_context_query(self->gst_pipeline, query, (GstGLDisplay*)self->gst_display, nullptr, (GstGLContext*)self->gst_gl_context))
			{
				cout << "GL context Query suceeded!" << endl; 
				return GST_PAD_PROBE_HANDLED;
			}
			break;
		default:
			break;
	}

	return GST_PAD_PROBE_OK;
}

int v4l2Camera::captureSetup(gst_data* gst_shared)
{	

	// cout<<"gst_context addr @CaptureSetup: "<< (void*)&(this->gst_context) <<endl;
	// cout<<"gst_loop addr @CaptureSetup: "<< (void*)(this->gst_context.loop) <<endl;
	// cout<<"gst_pipeline addr @CaptureSetup: "<< (void*)(this->gst_context.pipeline) <<endl;
	// cout<<"gst_display addr @CaptureSetup: "<< (void*)(this->gst_context.gst_display) <<endl;
	// cout<<"gst_gl_context addr @CaptureSetup: "<< (void*)(this->gst_context.gl_context) <<endl;

	//gst_context.loop = gst_shared->loop;
	//gst_context.gst_display = gst_shared->gst_display;
	//gst_context.gl_context = gst_shared->gl_context;

	gst_loop = gst_shared->loop;
	gst_display = gst_shared->gst_display;
	gst_gl_context = gst_shared->gl_context;
	
	//make sure initial buffer pointers are null
	buffer_render = nullptr;
	buffer_last = nullptr;

	for(int i=0; i<BUFFER_NUM;i++){
		// create memory for texture copy (there need to be a smarter way)
		guint8* videoFramePixelData = (guint8 *)malloc((size_t)(4 * width * height));
		if( videoFramePixelData == NULL )
		{
			printf("CaptureSetup() -> Failed to create memory for texture data!\n");
		}
		
		// cout << "____________________________________" << endl;
		// cout<<"buffer indx @CaptureSetup(): "<< i <<endl;
		// cout<<"Frame width @CaptureSetup(): "<< width <<endl;
		// cout<<"Frame height @CaptureSetup(): "<< height <<endl;
		// cout<<"buffer addr  @CaptureSetup(): "<< (void*)videoFramePixelData <<endl;
		// cout << "____________________________________" << endl << endl;
	
		memset(videoFramePixelData, 95*(i+1), (size_t)(4 * width * height));

		buffers[i].start = videoFramePixelData;
		buffers[i].height = height;
		buffers[i].width = width;
	}	
	



	// gchar *pipeline_cmd = g_strdup_printf("filesrc typefind=true location=%s "
	// 		"! h264parse ! queue ! v4l2h264dec ! video/x-raw,width=1280,height=720,format=I420 " 
	// 		"! glupload ! glcolorconvert ! appsink name=TextureSink",device.c_str());


/*	gchar *pipeline_cmd = g_strdup_printf("filesrc typefind=true location=%s "
			"! video/x-msvideo ! aiurdemux ! h264parse ! queue ! v4l2h264dec " 
			"! glupload ! appsink name=TextureSink",device.c_str());

*/

	gchar *pipeline_cmd = g_strdup_printf("filesrc location=%s "
			"! decodebin ! glupload ! appsink name=TextureSink",device.c_str());




	// gchar *pipeline_cmd = g_strdup_printf("filesrc typefind=true location=%s "
	// 		"! h264parse ! queue ! v4l2h264dec " 
	// 		"! waylandsink ",device.c_str());

	// gchar *pipeline_cmd = g_strdup_printf("filesrc typefind=true location=%s "
	// 		"! h264parse ! queue ! v4l2h264dec " 
	// 		"! glupload ! appsink name=TextureSink",device.c_str());
	
	//gchar *pipeline_cmd = g_strdup_printf("videotestsrc ! video/x-raw,format=I420 ! glupload ! glcolorconvert ! appsink name=TextureSink");

	cout << "Video pipeline: " << pipeline_cmd << endl;
	//GstElement* pipeline = gst_parse_launch(pipeline_cmd, NULL);
	gst_pipeline = gst_parse_launch(pipeline_cmd, NULL);
	
	if(gst_pipeline == NULL){
		cout << "Video pipeline failed to launch " << endl;
		return -1;
	}
	
	//gst_context.pipeline = pipeline;
	//gst_pipeline = pipeline;

	//cout << "pipeline Addr post parse : "<< (uint32_t*)pipeline << endl;

	// cout << "___________Cam " << this->camera_num << "_________________________" << endl;
	// cout<<"self addr @CaptureSetup: "<< (void*)(this) <<endl;
	// cout<<"gst_pipeline addr @CaptureSetup: "<< (void*)&(this->gst_pipeline) <<endl;
	// cout<<"gst_pipeline val @CaptureSetup: "<< (void*)(this->gst_pipeline) <<endl;
	// cout<<"gst_display addr @CaptureSetup: "<< (void*)&(this->gst_display) <<endl;
	// cout<<"gst_display val @CaptureSetup: "<< (void*)(this->gst_display) <<endl;
	// cout<<"gst_gl_context addr @CaptureSetup: "<< (void*)&(this->gst_gl_context) <<endl;
	// cout<<"gst_gl_context val @CaptureSetup: "<< (void*)(this->gst_gl_context) <<endl;
	// cout << "_______________________________________________________" << endl << endl;
	
	
	
	return 0;
}


/**************************************************************************************************************
 *
 * @brief  			Start capturing
 *
 * @param   		-
 *
 * @return 			The function returns 0 if capturing was run successfully. Otherwise -1 has been returned.
 *
 * @remarks			The function starts camera capturing
 *
 **************************************************************************************************************/
int v4l2Camera::startCapturing(void)
{
	cout<<"Starting Capture for Cam" << camera_num <<endl;

	cout << "___________Cam " << this->camera_num << "_________________________" << endl;
	cout<<"self addr @startCapturing: "<< (void*)(this) <<endl;
	cout<<"gst_pipeline addr @startCapturing: "<< (void*)&(this->gst_pipeline) <<endl;
	cout<<"gst_pipeline val @startCapturing: "<< (void*)(this->gst_pipeline) <<endl;
	cout<<"gst_display addr @startCapturing: "<< (void*)&(this->gst_display) <<endl;
	cout<<"gst_display val @startCapturing: "<< (void*)(this->gst_display) <<endl;
	cout<<"gst_gl_context addr @startCapturing: "<< (void*)&(this->gst_gl_context) <<endl;
	cout<<"gst_gl_context val @startCapturing: "<< (void*)(this->gst_gl_context) <<endl;
	cout << "_______________________________________________________" << endl << endl;

	//init frame mutex
	g_mutex_init(&frame_lock);

	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(gst_pipeline));
    guint bus_watch_id = gst_bus_add_watch(bus, bus_call, gst_pipeline);
	//gst_context.bus_watch_id = bus_watch_id;
    gst_object_unref(bus);

	//Get sink
    GstElement* sink = gst_bin_get_by_name(GST_BIN(gst_pipeline), "TextureSink");
    g_object_set(sink, "emit-signals", TRUE, nullptr);
    g_signal_connect(G_OBJECT(sink), "new-sample", G_CALLBACK(v4l2Camera::OnNewSample), this);

	gst_pad_add_probe(gst_element_get_static_pad(sink, "sink"), GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, v4l2Camera::OnQuery,
                      static_cast<gpointer>(this), nullptr);

	//cout << "pipeline Addr: "<< (uint32_t*)gst_context.pipeline << endl;
	GstStateChangeReturn state_return = gst_element_set_state(gst_pipeline, GST_STATE_PLAYING);
    if (state_return == GST_STATE_CHANGE_FAILURE)
    {
      cout<<"Failed to play GStreamer pipeline"<<endl;
	  return -1;
    }

	return 0;
}

/**************************************************************************************************************
 *
 * @brief  			Stop camera capturing
 *
 * @param   		-
 *
 * @return 			-
 *
 * @remarks			The function stop camera capturing, releases all memory and close camera device.
 *
 **************************************************************************************************************/
void v4l2Camera::stopCapturing(void)
{	
	v4l2Camera::exit_flag = 1;
	// Clean GStreamer
    gst_element_set_state(gst_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(gst_pipeline));

}


/**************************************************************************************************************
 *
 * @brief  			Capturing thread
 *
 * @param   in		void* input_args (thread_arg):	int* fd; - Thread id
 *													int* fill_buffer_inx; - Index of buffer which contain last captured camera frame
 *													v4l2_buffer* capture_buf[BUFFER_NUM]; - Pointer to captured buffer
 *													videobuffer* buffers[BUFFER_NUM]; - Pointer to videobuffer structures
 *
 * @return 			-
 *
 * @remarks 		The function creates thread with camera frame capturing loop. The capturing loop is terminated
 *					when the exit flag exit_flag is set to 1. Access to camera is synchronized by the mutex th_mutex.
 *
 **************************************************************************************************************/
void* v4l2Camera::getFrameThread(void* input_args)
{

}

/**************************************************************************************************************
 *
 * @brief  			Create thread with camera frame capturing loop
 *
 * @param   		-
 *
 * @return 			The function returns 0 if capturing thread was created successfully.
 *
 * @remarks 		The function creates thread with camera frame capturing loop.
 *
 **************************************************************************************************************/
int v4l2Camera::getFrame(void)
{
	/*
        //GstMemory *mem = gst_buffer_peek_memory (buffer, 0);
		GstMapInfo map;
		if (gst_buffer_map(buffer, &map, GST_MAP_READ)) 
        {
			//i = (i+1)%4;
			cout << "Map Size: " << map.size << endl;
			cout << "Buf Size: " << 4*self->buffers[i].width*self->buffers[i].height << endl;
			cout << "buffer indx @NewSample(): "<< i <<endl;
			cout << "Frame width @NewSample(): "<< self->buffers[i].width <<endl;
			cout << "Frame height @NewSample(): "<< self->buffers[i].height <<endl;
			cout << "start addr @NewSample(): "<< self->buffers[i].start <<endl;
        	
			//cout<<"Frame location @NewSample(): "<<(uint32_t*)map.data<<endl;
			//self->buffers[i].start = map.data;
			memcpy(self->buffers[i].start, map.data, 4*self->buffers[i].width*self->buffers[i].height);
			self->fill_buffer_inx = (i+1)%4;
            
        }
        gst_buffer_unmap (buffer, &map);
		*/
	g_mutex_lock(&frame_lock);
	if (buffer_render != nullptr && buffer_last != buffer_render)
    {
      // New buffer available, release previously rendered buffer
      gst_buffer_unref(buffer_render);
    }
	buffer_render = buffer_last;
	g_mutex_unlock(&frame_lock);
	if (buffer_render != nullptr)
    {
		// Get OpenGL texture ID
		GstMemory* memory = gst_buffer_peek_memory(buffer_render, 0);
		if (gst_is_gl_memory(memory))
		{
			//self->texID 
			guint texID = ((GstGLMemory*)memory)->tex_id;
			//cout << "Cam " << camera_num << " gave Tex ID: " << texID << endl;
			return texID;
		}
		else
		{
			cout << "Input from appsink is not an OpenGL texture. Consider using glupload in the pipeline." << endl;
			return -1;
		}
	}

	return(0);
}


GstFlowReturn v4l2Camera::OnNewSample(GstElement* appsink, gpointer data)
{
	//cout << " New sample CB " << endl;
	v4l2Camera* self = (v4l2Camera*)data;
	int i = self->fill_buffer_inx;
	//  Get buffer from GStreamer
    GstSample* sample;
    g_signal_emit_by_name(appsink, "pull-sample", &sample);

    if (sample)
    {
		g_mutex_lock(&self->frame_lock);

      	GstBuffer* buffer;
		buffer = gst_sample_get_buffer (sample);

		if (self->buffer_last != nullptr && self->buffer_last != self->buffer_render)
		{
			// Previous stored buffer has not been rendered, release it.
			gst_buffer_unref(self->buffer_last);
		}
		// Lock new buffer
		self->buffer_last = gst_buffer_ref(buffer);

		gst_sample_unref (sample);
		g_mutex_unlock(&self->frame_lock);
		
    }

    return GST_FLOW_OK;
}