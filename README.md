# facer
Real time face detection and recognition for webcams

# Description
  This program can detect and recognize all faces in the video stream of a webcam, then save the results in a sqlite3 database very fast.

# Requirements
  cmake, dlib, opencv, sqlite3, ffmpeg
  
  For MS-Windows, MKL is also needed.
  
  The shape_predictor_5_face_landmarks.dat and dlib_face_recognition_resnet_model_v1.dat can be downloaded from the [dlib website](http://dlib.net).
  
  The optional libfacedetect library is from [here](https://github.com/ShiqiYu/libfacedetection).
  
# Usage
  1, Create a directory with the name "names". All face photos will be storaged here.
  2, CD into the "names" directory, create directories with some department names. For example, "mkdir DP1", where "DP1" is the department where some personal photos will be found here.
  3, CD into "DP1", and copy some face photos here. The file names for these photos shoud be the people's names.
  4, Run the program to recognize.
