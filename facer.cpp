#include "facer.h"
#include <io.h>
#include <vector>
#include <algorithm>
#include <time.h>
#include <iostream>
#include <queue>
#include "sqlite3.h"

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <dlib/opencv.h>
#include <dlib/image_processing.h>
#include <dlib/dir_nav.h>
#include <dlib/threads.h>
#include <dlib/sqlite.h>
#include <dlib/ref.h>

#include "facedetect-dll.h"	//another fast face-detection algorithm for ms-windows only.[https://github.com/ShiqiYu/libfacedetection]
#define DETECT_BUFFER_SIZE 0x20000

using namespace dlib;
using namespace std;

int flag_detection, flag_recognition, flag_db, flag, fdone;

frontal_face_detector detector;// = get_frontal_face_detector();
shape_predictor sp;
anet_type net;

std::vector<matrix<float,0,1>> face_descriptors, old_face_descriptors;
std::vector<string> names, departments;
std::vector<string> old_names, old_departments;

int old_day, old_hour, old_min;
string s1 = "facer";
string dbfile = "faces.db";
database db(dbfile);
sqlite3 *dbp = NULL;
std::queue<char*> sts;


// ----------------------------------------------------------------------------------------
char *multichar_2_utf8(const char *m_string)  
{  
    int len=0;  
    wchar_t *w_string;  
    char *utf8_string;  
    //计算由ansi转换为unicode后，unicode编码的长度  
    len=MultiByteToWideChar(CP_ACP,0,(LPCTSTR)m_string, -1, NULL,0);//cp_acp指示了转换为unicode编码的编码类型  
    w_string=(wchar_t *)malloc(2*len+2);  
    memset(w_string,0,2*len+2);  
    //ansi到unicode转换  
    MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)m_string,-1,w_string, len);//cp_acp指示了转换为unicode编码的编码类型  
    //计算unicode转换为utf8后，utf8编码的长度  
    len = WideCharToMultiByte(CP_UTF8, 0, w_string, -1, NULL, 0, NULL, NULL);//cp_utf8指示了unicode转换为的类型  
    utf8_string=(char  *)malloc(len+1);  
    memset(utf8_string, 0, len + 1);  
    //unicode到utf8转换  
    WideCharToMultiByte (CP_UTF8, 0, w_string, -1, utf8_string, len, NULL,NULL);//cp_utf8指示了unicode转换为的类型  
    free(w_string);  
    return utf8_string;  
}  
// ----------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
bool table_exists (database& db, const std::string& tablename)
{
    return query_int(db, "select count(*) from sqlite_master where name = '" + tablename+"'")==1;
}
// ----------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
static cv::Rect dlib_rectangle2opencv_Rect(dlib::rectangle r)
{
  return cv::Rect(cv::Point2i(r.left(), r.top()), cv::Point2i(r.right() + 1, r.bottom() + 1));
}
// ----------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
static dlib::rectangle opencv_Rect2dlib_rectangle(cv::Rect r)
{
  return dlib::rectangle((long)r.tl().x, (long)r.tl().y, (long)r.br().x - 1, (long)r.br().y - 1);
}
// ----------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
void writedb()
{
	while(1)
	{
		if((sts.size() > 0))
		{
			//cout << sts.size() << std::endl;
			flag_db = 1;
			sqlite3_exec(dbp, sts.front(), NULL, NULL, NULL);
			sts.pop();
		}
		else 
		{
			flag_db = 0;
			sleep(20);
		}
	}	
}
// ----------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
void face_recognition(array2d<rgb_pixel>& cimg, std::vector<dlib::rectangle>& unknown_faces)
{
	std::vector<matrix<rgb_pixel>> unknown_face_chips;
		
	dlib::full_object_detection unknown_shape;
	matrix<rgb_pixel> unknown_face_chip;
	std::vector<matrix<float,0,1>> unknown_face_descriptors;
	string unknown_name, name, department, ss1;
	string dbnow;
	time_t t;
	struct tm * now;
	cv::Mat cvface;
	char sqlBuffer[100];
	
	//Demo compute 20 faces		
	size_t demosize = 20;
	if (face_descriptors.size() < demosize) demosize = face_descriptors.size();

	while(flag)
	{	
		if(unknown_faces.size() > 0)
		{
			unknown_face_chips.clear();
	
			for(size_t i = 0; i < unknown_faces.size(); i ++)
			{
				unknown_shape = sp(cimg, unknown_faces[i]);
				extract_image_chip(cimg, get_face_chip_details(unknown_shape,150,0.25), unknown_face_chip);
				unknown_face_chips.push_back(move(unknown_face_chip));			
			}

			t = time(0);
			now = localtime( & t );
	
			statement st2(db, "select datetime('now', 'localtime')");
			st2.exec();	
			st2.get_column(0, dbnow);					
	
			unknown_face_descriptors = net(unknown_face_chips);					
			
			for (size_t i = 0; i < unknown_face_descriptors.size(); i++)
			{
				size_t j = 0;
		
				//Demo compute 20 faces			
				while (j < demosize)
				//while (j < face_descriptors.size())
				{
					if (dlib::length(unknown_face_descriptors[i]-face_descriptors[j]) < 0.33) //tolerance=0.33
					{
						unknown_name = names[j];
						unknown_name.erase(unknown_name.end()-4,unknown_name.end());

						name = multichar_2_utf8(unknown_name.c_str());
						department = multichar_2_utf8(departments[j].c_str());
						ss1 = multichar_2_utf8(s1.c_str());
				
				
						sprintf(sqlBuffer, "insert into people VALUES('%s','%s','%s','%s')", name.c_str(), department.c_str(), dbnow.c_str(), ss1.c_str());
						sts.push(sqlBuffer);
						//cout << sts.size() << std::endl;
						//cout << sqlBuffer << std::endl;
				
						printf("%s, %s, %d-%d-%d, %2d:%2d:%2d, %s\n",
						unknown_name.c_str(), departments[j].c_str(), 
						now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
						now->tm_hour, now->tm_min, now->tm_sec,
						s1.c_str());							
						break;						
					}
					j++;
				}
			}		
			unknown_faces.clear();	
		}	
		else sleep(20);			
	}
}
// ----------------------------------------------------------------------------------------


// ----------------------------------------------------------------------------------------
void read_video(array2d<rgb_pixel>& cimg, std::vector<dlib::rectangle>& unknown_faces, std::vector<dlib::rectangle>& unknown_faces1, std::vector<dlib::rectangle>& unknown_faces2)
{
	cv::String window_name = s1;
	cv::VideoCapture capture;
	capture.open(0);	
		
	
    if (!capture.isOpened())
    {
        cerr << "Unable to connect to camera" << endl;
        //return 1;
		return;
    }
		
	cv::Mat frame;
	cv::Mat frame_gray;
	dlib::rectangle unknown_face;
	int * pResults = NULL; 
	unsigned char * pBuffer = (unsigned char *)malloc(DETECT_BUFFER_SIZE);
	int doLandmark = 0;
	
	cout <<"Name, Department, Time, Area"<< endl;
		
    // Grab and process frames until the main window is closed by the user.
	while ( capture.read(frame) )
    {
        if( frame.empty() )
		{
			printf(" --(!) No captured frame -- Break!");
			//return 1;
			return;
		}
        
		assign_image(cimg, cv_image<bgr_pixel>(frame));		

		/*
		//Fast but for MS-Windows only.
		cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
		pResults = facedetect_frontal(pBuffer, (unsigned char*)(frame_gray.ptr(0)), frame_gray.cols, frame_gray.rows, (int)frame_gray.step,
									1.2f, 2, 48, 0, doLandmark);						
		for(int i = 0; i < (pResults ? *pResults : 0); i++)
		{
			short * p = ((short*)(pResults+1))+142*i;
			int x = p[0];
			int y = p[1];
			int w = p[2];
			int h = p[3];
			
			//-- Put some boxes on the faces so we can see that the detector is finding them.
			//cv::rectangle(frame, cv::Rect(x, y, w, h), cv::Scalar(0, 255, 0), 2);
			
			//Draw landmarks
			if (doLandmark)
			{
				for (int j = 0; j < 68; j++)
					cv::circle(frame, cv::Point((int)p[6 + 2 * j], (int)p[6 + 2 * j + 1]), 1, cv::Scalar(0, 255, 0));
			}
			
			unknown_face = dlib::rectangle(x, y, x+w-1, y+h-1);
			unknown_faces.push_back(unknown_face);
		}
		*/
		//The dlib face detection method is slow.
		unknown_faces = detector(cimg);
		/*
		//-- Put some boxes on the faces so we can see that the detector is finding them.
		for (size_t i = 0; i < unknown_faces.size(); i++)
		{
			cv::rectangle(frame, dlib_rectangle2opencv_Rect(unknown_faces[i]),cv::Scalar( 0, 0, 255 ), 1, 8, 0);			
		}
		*/
									
		//-- Show what we got
		cv::imshow( window_name, frame );

		if(unknown_faces.size() > 0)
		{	
			if(unknown_faces1.size() == 0) unknown_faces1 = unknown_faces;
			else {if (unknown_faces2.size() == 0) unknown_faces2 = unknown_faces;}
			unknown_faces.clear();
		}
		
		char c = (char)cv::waitKey(20);
        if( c == 27 ) 
		{ 
			flag = 0;
			cv::waitKey(20);
			break; 
		}		
	}	
	return;
}
// ----------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	cout<<"Welcome to facer."<<endl;
	cout<<"Contact: 83892369@qq.com"<<endl;
	cout<<"Author： everchens"<<endl;
	cout<<endl;
	
	flag_detection = 0;
	flag_recognition = 0;
	flag_db = 0;
	flag = 1;
		
    //detector = get_frontal_face_detector();
	deserialize("face3.svm") >> detector; 
    deserialize("shape_predictor_5_face_landmarks.dat") >> sp;
    deserialize("dlib_face_recognition_resnet_model_v1.dat") >> net;

	string loc;
	if(argc == 2)
		loc = argv[1];
	else
		loc = "names";  //if no argument given then use the current working dir.
	
	directory test(loc);
	
	//Get all directories and files
	std::vector<directory> dirs = test.get_dirs();
	sort(dirs.begin(), dirs.end());
	
	if(dirs.size() == 0)
	{
		cout<<"No name data found, press any key to quit. "<<endl;
		getchar();
		return 1;
	}
	
	string t1 = "create table names (姓名 PRIMARY KEY, 部门)";
	t1 = multichar_2_utf8(t1.c_str());
	if (!table_exists(db, "names"))
        db.exec(t1);	
	
	statement st(db, "insert into names VALUES(?,?)");
	
	std::vector<file> filelists;	
	cout <<"Name Lists: "<<endl;
	for(auto dir : dirs)
	{	
		std::vector<file> files = dir.get_files();
		sort(files.begin(), files.end());
		for(auto file : files)
		{
			filelists.push_back(file);
			string department = dir.name();
			departments.push_back(department);
			old_departments.push_back(department);
			string name = file.name();
			names.push_back(name);
			old_names.push_back(name);
			name.erase(name.end()-4,name.end());
			cout<<department<<", "<<name<<endl;
			
			string namedb = multichar_2_utf8(name.c_str());
			string departmentdb = multichar_2_utf8(department.c_str());
			
			t1 = "select * from names WHERE 姓名 LIKE (?)";
			t1 = multichar_2_utf8(t1.c_str());
			statement st2(db, t1);
			st2.bind(1, namedb);
			st2.exec();
			if(!st2.move_next())
			{
				st.bind(1, namedb);
				st.bind(2, departmentdb);
				st.exec();
			}
		}
	}
	


	matrix<rgb_pixel> img;
	std::vector<matrix<rgb_pixel>> face_chips;
	
	for (auto file : filelists)
	{
		load_image(img, file);
		auto face = detector(img)[0];
		auto shape = sp(img, face);
		matrix<rgb_pixel> face_chip;
        extract_image_chip(img, get_face_chip_details(shape,150,0.25), face_chip);
        face_chips.push_back(move(face_chip));
	}
	
    if (face_chips.size() == 0)
    {
        cout << "No faces found in image directory." << endl;
        return 1;
    }
	
	face_descriptors = net(face_chips);
	old_face_descriptors = face_descriptors;
	
	
	sqlite3_open("faces.db", &dbp);
	if (dbp == NULL) return 0;
	
	cout<<endl;
	printf( "Start recognition. Press 'ESC' in the surveillance window to quit. \n" );
	
	t1 = "create table people (姓名, 部门, 出现时间, 监控区域)";
	t1 = multichar_2_utf8(t1.c_str());
	if (!table_exists(db, "people"))
        db.exec(t1);
	
	//Begin face detection and recognition...
	array2d<rgb_pixel> cimg;
	std::vector<dlib::rectangle> unknown_faces;
	std::vector<dlib::rectangle> unknown_faces1;	//for the 1st thread of recognition 
	std::vector<dlib::rectangle> unknown_faces2;	//for the 2nd thread of recognition	
	//thread for sqlite database writing
	thread th_writedb(writedb);
	th_writedb.detach();
	//the 1st thread for face recognition	
	thread th_face_recognition1(face_recognition, dlib::ref(cimg), dlib::ref(unknown_faces1));
	th_face_recognition1.detach();
	//the 2nd thread for face recognition
	thread th_face_recognition2(face_recognition, dlib::ref(cimg), dlib::ref(unknown_faces2));
	th_face_recognition2.detach();
	//thread for face detection in video stream of webcam
	read_video(dlib::ref(cimg), dlib::ref(unknown_faces), dlib::ref(unknown_faces1), dlib::ref(unknown_faces2));
		
	while (flag_db == 1)  
	{
		sleep(1); 
	}

	sqlite3_close(dbp);
	return 0;
}
// ----------------------------------------------------------------------------------------





