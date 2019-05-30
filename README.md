# facer
Real time face detection and recognition for webcams

# Description
  This program can detect and recognize all faces in the video stream of a webcam, then save the results in a sqlite3 database very fast.

# Requirements
  cmake, dlib, opencv, sqlite3, ffmpeg
  For MS-Windows, MKL is also needed.
  The shape_predictor_5_face_landmarks.dat and dlib_face_recognition_resnet_model_v1.dat can be downloaded from the [dlib website](dlib.net).
  The optional libfacedetect library is from [here](https://github.com/ShiqiYu/libfacedetection).
