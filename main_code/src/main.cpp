#include <iostream>

#include <curl/curl.h>

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QImage>
#include <QPalette>
#include <QGridLayout>
#include <QTimer>

#include <opencv2/opencv.hpp>

#include <pthread.h> 
#include <mutex>
#include <string>
#include <sys/time.h>

#include "json.hpp"
#include "base64.h"
#include "face.cpp"
#include "kmeans.cpp"
#include "HDC1080.h"
#include "MFRC522.h" 

// Attendance status
#define RFID_ATTENDANCE 0
#define RFID_FACE_ATTENDANCE 1
#define RFID_DIF_FACE_ATTENDANCE 2

const char ip[] = "192.168.137.1";
const char port[] = "3001";

using json = nlohmann::json;
using namespace cv;

bool isDoneLoadData = false;
enum ButtonAction{
  isGetData,
  isSaveData,
  isDone
};
ButtonAction buttonAction;

void *ReadAndSendRFID(void * );
void *ReadAndSaveRFID(void * );
// void *FaceAttendance(void * );
void SaveFeature();
void ExtractFeature();
void *LoadFeature(void *);
void *Comfirm(void *);
void *Refuse(void *);
bool IsEnoughFeature(String , bool &);

void delay(int ms){
  usleep(ms*1000);  // Function to introduce a delay in milliseconds
}

void SendEnvParams(float temp, float hum){
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    char url[100];
    sprintf(url,"http://%s:%s/user/addSensor", ip, port);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    char post_field[100];
    sprintf(post_field, "json= {\"temp\": %f,\"hum\": %f}", temp, hum);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    cout << endl;
  }
  curl_global_cleanup();
}

void SendID(string id){
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    char url[100];
    sprintf(url,"http://%s:%s/user/id", ip, port);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    string post_field = "id="+ id;
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field.c_str());

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}

void PostFeature(string id,Mat pFeature){
  static int seconds_last = 99;
	char timeString[128];
	timeval currTime;
  double feature[128];
  for(int k=0; k<pFeature.rows; k++)
  {
      feature[k]=pFeature.at<float>(k);
  }

	gettimeofday(&currTime, NULL);
	seconds_last = currTime.tv_sec;
	strftime(timeString, 80, "%Y-%m-%d %H:%M:%S", localtime(&currTime.tv_sec));

  CURL *curl;
  CURLcode res;
  char url[100];
  snprintf(url, 100,"http://%s:%s/user/updateAttendance", ip, port);
  curl_global_init(CURL_GLOBAL_ALL);
 
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    char s2[10000];
    char s1[10000];
    int flag = 1;
    int tt = 128;
    snprintf(s2,sizeof(s2), "json={\"id\": \"%s\",\"time\": \"%s\",\"feature\": [",id.c_str(), timeString);
    for(int i =0; i < tt-1; i++){
      if (flag == 1){
        snprintf(s1,sizeof(s1), "%s%.10f,",s2, feature[i]);
      }
      else 
        snprintf(s2,sizeof(s2), "%s%.10f,",s1, feature[i]);
      flag*=-1;
    }
    if (flag == 1){
      snprintf(s1,sizeof(s1), "%s%.10f]}",s2, feature[tt-1]);
      snprintf(s2,sizeof(s2), "%s",s1);
    }
      else 
        snprintf(s2,sizeof(s2), "%s%.10f]}",s1, feature[tt-1]);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s2);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}

void PostAttendanceStatus(char id[50],int len, string image, const int attendanceStatus){
  cout<<id<<endl;

  if(len>0) id[len]='\0';
  
  char string_image[1000000];

  strcpy(string_image, image.c_str());
  CURL *curl;
  CURLcode res;
  char url[100];
  snprintf(url, 100,"http://%s:%s/user/updateTimeStudent", ip, port);
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    char s2[1000000];
    snprintf(s2,sizeof(s2), "json={\"id\": \"%s\",\"img\": \"%s\",\"status\": \"%d\"}", id, string_image, attendanceStatus);
    // cerr<<s2<<endl;
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s2);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}

class MyWidget : public QWidget {
private:
  QPushButton *rfidButton;
  QPushButton *faceButton;
  QPushButton *dataButton;
  QPushButton *loadButton;
  
  QTimer *envTimer;
  QTimer *imageTimer;

  QString anouncement;

  QGridLayout *layout;

  MFRC522 *mfrc;
  int *hdc1080;

  float temperature;
  float humidity;

  pthread_t threads[7];

public:

  static QPushButton *confirmButton;
  static QPushButton *cancelButton;
  
  static VideoCapture cap;
  static Mat frameCapture;
  static FacialRecognition fr;
  static FaceR facer;

  static vector<cv::Mat> aligneds;
  static vector<cv::Mat> fcsChosen;

  static bool isSaveIDDone;
  static bool isTakeCap;

  static QLabel *imageLabel, *textLabel;
  static QLabel *smallImageLabels[5];

  static string currentID;


  MyWidget(QWidget *parent = 0, int *_hdc1080 = 0, MFRC522 *_mfrc = NULL) : QWidget(parent), hdc1080(_hdc1080), mfrc(_mfrc) {

      mfrc->PCD_Init();

      CreateWidget();

      connect(rfidButton, &QPushButton::clicked, this, &MyWidget::SendRFID);
      connect(faceButton, &QPushButton::clicked, this, &MyWidget::SendFace);
      connect(dataButton, &QPushButton::clicked, this, &MyWidget::GetData);
      connect(loadButton, &QPushButton::clicked, this, &MyWidget::LoadData);
      connect(confirmButton, &QPushButton::clicked, this, &MyWidget::ComfirmEvent);
      connect(cancelButton, &QPushButton::clicked, this, &MyWidget::RefuseEvent);

      cap.open(0);
      
      // Create a imageTimer to continous update image from camera
      imageTimer = new QTimer(this);
      connect(imageTimer, &QTimer::timeout, this, &MyWidget::UpdateImage);
      imageTimer->start(30);

      envTimer = new QTimer(this);
      connect(envTimer, &QTimer::timeout, this, &MyWidget::UpdateEnv);
      envTimer->start(36000);

      LoadData();

      pthread_create(&threads[0], NULL, ReadAndSaveRFID, (void *)mfrc); 
  }
  void CreateWidget()
  {
    setWindowTitle("Attendance system");
    setFixedSize(1024, 560); // Size of window
    
    // Create buttons for widget
    rfidButton = new QPushButton("RFID Attendance", this);
    faceButton = new QPushButton("Face Attendance", this);
    dataButton = new QPushButton("Collect Data", this);
    loadButton = new QPushButton("Load Data", this);
    confirmButton = new QPushButton("Yes", this);
    cancelButton = new QPushButton("No", this);

    QPalette palConfirmButton = confirmButton->palette();
    palConfirmButton.setColor(QPalette::Button, QColor(Qt::green)); // Set the color you want
    confirmButton->setPalette(palConfirmButton);

    QPalette palCancelButton = cancelButton->palette();
    palCancelButton.setColor(QPalette::Button, QColor(Qt::red)); // Set the color you want
    cancelButton->setPalette(palCancelButton);

    // Fix height for button
    rfidButton->setFixedHeight(0);
    faceButton->setFixedHeight(0);
    dataButton->setFixedHeight(0);
    loadButton->setFixedHeight(0);
    confirmButton->setFixedHeight(0);
    cancelButton->setFixedHeight(0);

    rfidButton->setFont(QFont("Helvetica", 15));
    faceButton->setFont(QFont("Helvetica", 15));
    dataButton->setFont(QFont("Helvetica", 15));
    loadButton->setFont(QFont("Helvetica", 15));
    confirmButton->setFont(QFont("Helvetica", 15));
    cancelButton->setFont(QFont("Helvetica", 15));

    // Create a label to show the camera images

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignHCenter);
    for (int i = 0; i < 5; i++)
    smallImageLabels[i] = new QLabel(this);

    textLabel = new QLabel(this);
    textLabel->setAlignment(Qt::AlignHCenter);
    textLabel->setFont(QFont("Helvetica", 20));

    // Create layout grid
    layout = new QGridLayout(this);

    layout->addWidget(rfidButton, 0, 0, 1, 4); // Button 1 at row 0, col 0, take 1 row, 4 col
    layout->addWidget(faceButton, 0, 4, 1, 4); // Button 1 at row 0, col 4, take 1 row, 4 col
    layout->addWidget(dataButton, 0, 8, 1, 2); // Button 1 at row 0, col 8, take 1 row, 2 col
    layout->addWidget(loadButton, 0, 10, 1, 2); // Button 1 at row 0, col 10, take 1 row, 2 col
    layout->addWidget(confirmButton, 0, 8, 1, 2); // Button 1 at row 0, col 8, take 1 row, 2 col
    layout->addWidget(cancelButton, 0, 10, 1, 2); // Button 1 at row 0, col 8, take 1 row, 2 col

    layout->addWidget(textLabel, 1, 0, 1, 12); // Image label at row 1, col 0, take 1 row, 12 col
    layout->addWidget(imageLabel, 2, 0, 4, 12); // Image label at row 2, col 0, take 1 row, 12 col

    for (int i = 0; i < 5; i++)
    layout->addWidget(smallImageLabels[i], 2, i+2, 1, 1);
  }
  void SendRFID(){
    pthread_create(&threads[0], NULL, ReadAndSendRFID, (void *)mfrc); 
  }
  void SendFace(){
    // pthread_create(&threads[1], NULL, FaceAttendance, NULL); 
  }
  void GetData(){
    isSaveIDDone = false;
    pthread_create(&threads[2], NULL, ReadAndSaveRFID, (void *)mfrc); 
    // pthread_create(&threads[3], NULL, ExtractFeature, NULL); 
  }
  void LoadData(){
    pthread_create(&threads[4], NULL, LoadFeature, NULL); 
  }
  void ComfirmEvent(){
    pthread_create(&threads[5], NULL, Comfirm, NULL); 
  }
  void RefuseEvent(){
    pthread_create(&threads[6], NULL, Refuse, NULL); 
  }
 
  void UpdateImage() {
    // Take frame from camera
    cap >> frameCapture;
    if (!frameCapture.empty()) {
      // Turn image to RGB
      cvtColor(frameCapture, frameCapture, COLOR_BGR2RGB); 
      QImage img(frameCapture.data, frameCapture.cols, frameCapture.rows, frameCapture.step, QImage::Format_RGB888); // Create QImage from frame

      // Fix size for image to suitable with label
      QPixmap pixmap = QPixmap::fromImage(img).scaled(800, 480, Qt::KeepAspectRatio);
      // Put image to label
      imageLabel->setPixmap(pixmap); 
    }
  }

  void UpdateEnv(){
    read_hdc1080(*hdc1080, temperature, humidity);
    SendEnvParams(temperature, humidity);
  }
};
QPushButton *MyWidget::confirmButton;
QPushButton *MyWidget::cancelButton;

QLabel *MyWidget::textLabel = NULL;
QLabel *MyWidget::imageLabel = NULL;
QLabel *MyWidget::smallImageLabels[5];

bool MyWidget::isSaveIDDone = false;
bool MyWidget::isTakeCap = false;

FacialRecognition MyWidget::fr;
FaceR MyWidget::facer;

VideoCapture MyWidget::cap;
Mat MyWidget::frameCapture;
vector<cv::Mat> MyWidget::aligneds;
vector<cv::Mat> MyWidget::fcsChosen;

string MyWidget::currentID = "";


string create_output_for_binary(const string &full_path)
{
    const char* file_name = full_path.c_str();
    FILE* file_stream = fopen(file_name, "rb");
    string file_str;
    size_t file_size;
    if(file_stream != nullptr)
    {
      fseek(file_stream, 0, SEEK_END);
      long file_length = ftell(file_stream);
      rewind(file_stream);
      char* buffer = (char*) malloc(sizeof(char) * file_length);
      if(buffer != nullptr)
      {
          file_size = fread(buffer, 1, file_length, file_stream);
          stringstream out;
          for(int i = 0; i < file_size; i++)
          {
              out << buffer[i];
          }
          string copy = out.str();
          file_str = copy;
      }
      else
      {
          printf("buffer is null!");
      }
    }
    else
    {
      printf("file_stream is null! file name -> %s\n", file_name);
    }
    if(file_str.length() > 0)
    {
      string file_size_str = to_string(file_str.length());
    }
    return file_str;
}

string weighted_sum(vector <FaceR> facers)
{
    vector<string> ids;
    vector<string> ids_;
    vector<string>::iterator id;
    vector<double> confs;

    map <string,double> wsm;

    string idr;
    double score_max=-0.1;

    for(int i=0;i<facers.size();i++){
        ids.push_back(facers[i].face_id);
        ids_.push_back(facers[i].face_id);
        confs.push_back(facers[i].conf);
    }
    sort(ids.begin(),ids.begin()+ids.size());
    
    id=unique(ids.begin(),ids.begin()+ids.size());

    ids.resize(distance(ids.begin(),id));
    
    for(auto itr : ids) {
      wsm.insert({itr,0.0});
    }

    for(int i=0;i<ids_.size();i++) wsm[ids_[i]]+=confs[i];

    for(auto itr=wsm.begin();itr!=wsm.end();++itr){
        if(itr->second>score_max){
            score_max=itr->second;
            idr=itr->first;
        }
    }
    return idr;
}

void FaceAttendance(){
  vector <FaceR> facers;
  facers.clear();
  int sumNumFaces;
  int faceNumber=0;
  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  beginTime = chrono::steady_clock::now();
  QString anouncement;
    
  while(faceNumber < 5){
    int numFaces = 0;
    MyWidget::facer = MyWidget::fr.get_name(MyWidget::frameCapture, numFaces);
    if(MyWidget::facer.blur >= -25 && numFaces > 0){
      anouncement = "Facial recognizing...";
      MyWidget::textLabel->setText(anouncement);
      cout << numFaces << ", " << faceNumber;
      faceNumber++;
      facers.push_back(MyWidget::facer);
    }
    endTime = chrono::steady_clock::now();
    waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
    if (waitingTime > 3000)
    {
      anouncement = "Please put your face in front camera.";
      MyWidget::textLabel->setText(anouncement);
    }
    sumNumFaces += numFaces;
  }
  
  
  
  string predictionId = weighted_sum(facers);

  char char_predictionId[predictionId.length()];

  for(int i=0; i<predictionId.length(); i++)
    char_predictionId[i] = predictionId[i];

  cvtColor(MyWidget::facer.aligned, MyWidget::facer.aligned, COLOR_BGR2RGB); 
  
  imwrite("cam_picture.png", MyWidget::facer.aligned);

  string strImage = create_output_for_binary("cam_picture.png");
  auto encodedImage = base64_encode(strImage, strImage.size());

  PostAttendanceStatus(char_predictionId, predictionId.length(), encodedImage, RFID_FACE_ATTENDANCE);
  
  MyWidget::aligneds.clear();

  char _anouncement[50];
  if (predictionId != MyWidget::currentID)
  {
    sprintf(_anouncement, "Your ID %s, not match with current face", predictionId.c_str());
  }
  else
  {
    sprintf(_anouncement, "Marked attendance, welcome %s", predictionId.c_str());
  for(int i=0; i<MyWidget::fr.Persons.size(); i++)
    if (predictionId == MyWidget::fr.Persons[i].id){
      sprintf(_anouncement, "Marked attendance, welcome %s", (MyWidget::fr.Persons[i].name).c_str());
      break;
    }
  }
  anouncement = _anouncement;
  MyWidget::textLabel->setText(anouncement);

  delay(5000);

  anouncement = "";
  MyWidget::textLabel->setText(anouncement);
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void* LoadFeature(void* args){

  char url[100];
  string featureData;
  string id;
  string name;
  vector<cv::Mat> faces;
  vector<float> feature;
  feature.resize(128);

  snprintf(url, 100,"http://%s:%s/user/getFeature", ip, port);
  
  curl_global_init(CURL_GLOBAL_ALL);
  
  CURL *curl = curl_easy_init();

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &featureData);
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  }

  json json_featureData = json::parse(featureData);
  MyWidget::fr.FolderCnt=json_featureData.size();
  // cout << json_featureData << endl;
  for (int i = 0; i < MyWidget::fr.FolderCnt; i++)
  {
    Person person;

    person.n = json_featureData[i]["facial_recognition_data"].size();

    name = json_featureData[i]["name"];
    id = json_featureData[i]["card_id"];

    person.name = name.c_str();
    person.id = id.c_str();

    for (int j = 0; j < person.n; j++)
    {
      for (int k = 0; k < 128; k++)
        feature[k] = json_featureData[i]["facial_recognition_data"][j][k];
      faces.push_back(cv::Mat(feature, true));
    }
    person.fcs = faces;

    person.centroid = faces[(size_t)0];
    for (int j = 1; j < person.n; j++)
    { 
      person.centroid = person.centroid + faces[j];
    }
    person.centroid = person.centroid / person.n;

    MyWidget::fr.Persons.push_back(person);
    
    faces.clear();
  }

  QString anouncement = "Loaded data";
  MyWidget::textLabel->setText(anouncement);
  
  delay(2000);

  anouncement = "";
  MyWidget::textLabel->setText(anouncement);
  isDoneLoadData = true;
  curl_global_cleanup();

  pthread_exit(NULL); 
}

double Distance(const cv::Mat &v1, const cv::Mat &v2){
  double norm_sim = norm(v2-v1,NORM_L2);
  return norm_sim;
}

void* Refuse(void * args){
  for(size_t i=0; i < MyWidget::aligneds.size(); i++){
    QImage img(MyWidget::aligneds[i].data, MyWidget::aligneds[i].cols, MyWidget::aligneds[i].rows, MyWidget::aligneds[i].step, QImage::Format_RGB888); // Create QImage from frame

    // Fix size for image to suitable with label
    QPixmap pixmap = QPixmap::fromImage(img).scaled(0, 0, Qt::KeepAspectRatio);

    MyWidget::smallImageLabels[i]->setPixmap(pixmap);
  }
  MyWidget::confirmButton->setFixedHeight(0);
  MyWidget::cancelButton->setFixedHeight(0);
  QString anouncement;

  if (buttonAction == isGetData)
  {
    anouncement = "Canceled provide data";
    MyWidget::textLabel->setText(anouncement);
  }
  else if (buttonAction == isSaveData)
  {
    anouncement = "Canceled save data";
    MyWidget::textLabel->setText(anouncement);
  }
  delay(5000);
  buttonAction = isDone;
  anouncement = "";
  MyWidget::textLabel->setText(anouncement);
  pthread_exit(NULL); 
}

void* Comfirm(void * args){
  if (buttonAction == isGetData)
  {
    ExtractFeature();
  }
  else if (buttonAction == isSaveData)
  {
    SaveFeature();
  }
  pthread_exit(NULL); 
}
void SaveFeature(){
  {
    vector<cv::Mat> faces; 
    Person person;
    for(size_t i = 0; i < MyWidget::fcsChosen.size(); i++){
      PostFeature(MyWidget::currentID, MyWidget::fcsChosen[i]);
      faces.push_back(MyWidget::fcsChosen[i]);
    }
    // update database local
    person.n=MyWidget::fcsChosen.size();
    person.fcs=faces;
    person.name= MyWidget::currentID;
    person.id= MyWidget::currentID;
    person.centroid = faces[(size_t)0];

    for (int j = 1; j < person.n; j++)
    {
      person.centroid = person.centroid + faces[j];
    }
    person.centroid = person.centroid / person.n;

    MyWidget::fr.Persons.push_back(person);
    MyWidget::fr.FolderCnt+=1;

    faces.clear();

    for(size_t i=0; i < MyWidget::aligneds.size(); i++){
        QImage img(MyWidget::aligneds[i].data, MyWidget::aligneds[i].cols, MyWidget::aligneds[i].rows, MyWidget::aligneds[i].step, QImage::Format_RGB888); // Create QImage from frame

        // Fix size for image to suitable with label
        QPixmap pixmap = QPixmap::fromImage(img).scaled(0, 0, Qt::KeepAspectRatio);

        MyWidget::smallImageLabels[i]->setPixmap(pixmap);
    }

    MyWidget::confirmButton->setFixedHeight(0);
    MyWidget::cancelButton->setFixedHeight(0);
    
    QString anouncement = "Saved data";
    MyWidget::textLabel->setText(anouncement);
    delay(5000);
    
    buttonAction = isDone; 
  }
}
void ExtractFeature(){

  MyWidget::confirmButton->setFixedHeight(0);
  MyWidget::cancelButton->setFixedHeight(0);

  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;

  cv::Mat frameCap;
  vector<cv::Mat> images;
  vector<cv::Mat> features;
  vector<cv::Mat> centroidsCluster;
  centroidsCluster.clear();

  int pointId = 1;
  vector<Point_ft> allPoints;
  vector<double> score;

  KMeans kmeans(5, 20, "cluster-details");
  beginTime = chrono::steady_clock::now();
  cout << "Read id success!"<< endl; 
  while (1)
  {
    frameCap = MyWidget::frameCapture.clone();
    // if (frameCap.empty())
    // {
    //   break;
    // }
    cv::resize(frameCap, frameCap, cv::Size(), 0.5, 0.5);

    endTime = chrono::steady_clock::now();
    waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();

    if(waitingTime > 5000) break;
    int numFaces;
    MyWidget::facer = MyWidget::fr.get_name(frameCap, numFaces);
    
    if(MyWidget::facer.blur >= -25 && MyWidget::facer.Angle_face < 28){
        images.push_back(MyWidget::facer.aligned);
        features.push_back(MyWidget::fr.extractFT(MyWidget::facer.aligned));
    }
  }
  
  for (auto& element : features) {
      Point_ft point(pointId, element);
      allPoints.push_back(point);
      pointId++;
  }

  centroidsCluster=kmeans.run(allPoints);

  
  MyWidget::aligneds.clear();
  MyWidget::fcsChosen.clear();
  for(int i=0;i<5;i++){

      score.clear();

      for(size_t j=0;j<features.size();j++){
          double norm=Distance(centroidsCluster[i],features[j]);
          score.push_back(norm);
      }

      int Pmin = min_element(score.begin(),score.end()) - score.begin();
      MyWidget::aligneds.push_back(images[Pmin]);
      MyWidget::fcsChosen.push_back(features[Pmin]);
  }

  QString anouncement = "Press Yes button to save data";
  MyWidget::textLabel->setText(anouncement);
  buttonAction = isSaveData;
  MyWidget::confirmButton->setFixedHeight(60);
  MyWidget::cancelButton->setFixedHeight(60);
  
  for(size_t i=0; i < MyWidget::aligneds.size(); i++){
        QImage img(MyWidget::aligneds[i].data, MyWidget::aligneds[i].cols, MyWidget::aligneds[i].rows, MyWidget::aligneds[i].step, QImage::Format_RGB888); // Create QImage from frame

        // Fix size for image to suitable with label
        QPixmap pixmap = QPixmap::fromImage(img).scaled(MyWidget::facer.aligned.cols, MyWidget::facer.aligned.rows, Qt::KeepAspectRatio);

        MyWidget::smallImageLabels[i]->setPixmap(pixmap);
  }
  pthread_exit(NULL); 
}

void *ReadAndSaveRFID(void * _mfrc) 
{ 
  MFRC522* mfrc = (MFRC522*) _mfrc;

  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  beginTime = chrono::steady_clock::now();
  while (!isDoneLoadData)
  {
    /* code */
  }
  QString anouncement = "Please face in front of camera and swipe the ID card";
  MyWidget::textLabel->setText(anouncement);

  while (1)
    {
      delay(100);  // Introduce a delay of 100 milliseconds
      // Look for a card
      if(!mfrc->PICC_IsNewCardPresent()) // Check if a new card is present
      continue;  // If no new card is present, continue to the next iteration

      if( !mfrc->PICC_ReadCardSerial())  // Attempt to read the card's serial number
      continue;  // If reading fails, continue to the next iteration

      std::cout << std::endl << mfrc->PICC_TakeStringID() << std::endl;  // Print the string ID of the card
      
      MyWidget::currentID = mfrc->PICC_TakeStringID();

      anouncement = "Receive ID!";
      
      MyWidget::textLabel->setText(anouncement);

      chrono::steady_clock::time_point beginTime, endTime;
      float waitingTime = 0.0;
      beginTime = chrono::steady_clock::now();

      while (1)
      {
        if (waitingTime >= 2000)
        {
          bool isStranger;
          if (IsEnoughFeature(MyWidget::currentID, isStranger)) 
          {
            anouncement = "Facial recognizing...";
            MyWidget::textLabel->setText(anouncement);
            FaceAttendance();
          }
          else
          {
            if (isStranger)
            {
              anouncement = "Hello stranger, please call manager to create profile";
              MyWidget::textLabel->setText(anouncement);
              delay(2000);
            }
            anouncement = "Not enough features for recognition, do you want to provide some for system";
            MyWidget::textLabel->setText(anouncement);
            buttonAction = isGetData;
            MyWidget::confirmButton->setFixedHeight(60);
            MyWidget::cancelButton->setFixedHeight(60);
            while(!(buttonAction == isDone)){
              delay(100);
            }
          }
          break;
        }
        endTime = chrono::steady_clock::now();
        waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
      }

      // anouncement = "";
      // MyWidget::textLabel->setText(anouncement);

      QString anouncement = "Please face in front of camera and swipe the ID card";
      MyWidget::textLabel->setText(anouncement);
    }
  pthread_exit(NULL); 
}
bool IsEnoughFeature(String currentID, bool &isStranger){
  isStranger = 0;
  for (int i = 0; i < MyWidget::fr.FolderCnt; i++)
  {
    if (currentID == MyWidget::fr.Persons[i].id)
    {
      if (MyWidget::fr.Persons[i].n >=5)
        return 1;
      else
        return 0;
    }
  }
  isStranger = 1;
  return 0;
}
void *ReadAndSendRFID(void * _mfrc) 
{ 
  MFRC522* mfrc = (MFRC522*) _mfrc;

  QString anouncement = "Please wipes the ID card!";
  MyWidget::textLabel->setText(anouncement);

  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  beginTime = chrono::steady_clock::now();
  while (1)
  {
    endTime = chrono::steady_clock::now();
    waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
    if (waitingTime > 20000) 
    {
      anouncement = "";
      MyWidget::textLabel->setText(anouncement);
      break;
    }
    // Look for a card
    if(!mfrc->PICC_IsNewCardPresent()) // Check if a new card is present
    continue;  // If no new card is present, continue to the next iteration

    if( !mfrc->PICC_ReadCardSerial())  // Attempt to read the card's serial number
    continue;  // If reading fails, continue to the next iteration

    std::cout << std::endl << mfrc->PICC_TakeStringID() << std::endl;  // Print the string ID of the card
    SendID(mfrc->PICC_TakeStringID());

    anouncement = "Take attendance by RFID success!";
    MyWidget::textLabel->setText(anouncement);

    delay(5000);

    anouncement = "";
    MyWidget::textLabel->setText(anouncement);
    break;

    delay(100);  // Introduce a delay of 100 milliseconds (1 second)
  }
  pthread_exit(NULL); 
}
int main(int argc, char *argv[]) {
  QWidget *parent = 0;
  
  int i2c_fd = init_hdc1080();
  MFRC522 mfrc;  // Initialize an instance of the MFRC522 class

  // close(i2c_fd); // Close the I2C connection
  QApplication app(argc, argv);
  MyWidget widget(parent, &i2c_fd, &mfrc);
  widget.show();
  return app.exec();
}