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
#include "MFRC522.h"  // Include the header file for MFRC522


const char ip[] = "192.168.137.1";
const char port[] = "3001";

using json = nlohmann::json;

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

void postFeature(string id,Mat pFeature){
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
    // std::cout<<s2<<std::endl;
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s2);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}

void postStudentPic(char id[50],int len, string image){
  cout<<id<<endl;
  static int seconds_last = 99;
  if(len>0) id[len]='\0';
	char timeString[128];
	timeval currTime;
	gettimeofday(&currTime, NULL);
	seconds_last = currTime.tv_sec;
	strftime(timeString, 80, "%Y-%m-%d %H:%M:%S", localtime(&currTime.tv_sec));
  char data_send[1000000];
  strcpy(data_send, image.c_str());
  CURL *curl;
  CURLcode res;
  char url[100];
  snprintf(url, 100,"http://%s:%s/user/updateTimeStudent", ip, port);
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    char s2[1000000];
    snprintf(s2,sizeof(s2), "json={\"id\": \"%s\",\"time\": \"%s\",\"img\": \"%s\"}", id, timeString, data_send);
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

void *ReadAndSendRFID(void * );
void *ReadAndSaveRFID(void * );
void *FaceAttendance(void * );
void *ExtractFeature(void *);
void *LoadFeature(void *);
void *SendData(void *);
void *NotSendData(void *);

using namespace cv;

class MyWidget : public QWidget {
private:
  QPushButton *rfidButton;
  QPushButton *faceButton;
  QPushButton *dataButton;
  QPushButton *loadButton;
  
  QTimer *envTimer;
  
  float temperature;
  float humidity;

  pthread_t threads[7];

  int *hdc1080;
  MFRC522 *mfrc;
  
public:
  static QPushButton *confirmButton;
  static QPushButton *cancelButton;
  static QGridLayout *layout;
  static QTimer *imageTimer;
  static VideoCapture cap;
  static FacialRecognition fr;
  static FaceR facer;
  static vector<cv::Mat> aligneds;
  static vector<cv::Mat> fcs_chossed;
  static bool isSaveIDDone;
  static bool isTakeCap;
  static QLabel *imageLabel, *textLabel;
  static QLabel *smallImageLabels[5];
  // *imageLabel2, *imageLabel3, *imageLabel4, *imageLabel5;
  static string currentID;
  static Mat frame_capture;
  MyWidget(QWidget *parent = 0, int *_hdc1080 = 0, MFRC522 *_mfrc = NULL) : QWidget(parent), hdc1080(_hdc1080), mfrc(_mfrc) {

      mfrc->PCD_Init();

      CreateWidget();

      connect(rfidButton, &QPushButton::clicked, this, &MyWidget::SendRFID);
      connect(faceButton, &QPushButton::clicked, this, &MyWidget::SendFace);
      connect(dataButton, &QPushButton::clicked, this, &MyWidget::GetData);
      connect(loadButton, &QPushButton::clicked, this, &MyWidget::LoadData);
      connect(confirmButton, &QPushButton::clicked, this, &MyWidget::ComfirmSendData);
      connect(cancelButton, &QPushButton::clicked, this, &MyWidget::CancelSendData);
      
    // connect(button2, &QPushButton::clicked, this, &Statusbar::OnApplyPressed);

      cap.open(0);
      
      // Create a imageTimer to continous update image from camera
      imageTimer = new QTimer(this);
      connect(imageTimer, &QTimer::timeout, this, &MyWidget::UpdateImage);
      imageTimer->start(30);

      envTimer = new QTimer(this);
      connect(envTimer, &QTimer::timeout, this, &MyWidget::UpdateEnv);
      envTimer->start(36000);
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
    rfidButton->setFixedHeight(60);
    faceButton->setFixedHeight(60);
    dataButton->setFixedHeight(60);
    loadButton->setFixedHeight(60);
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
    
    auto *vbox = new QVBoxLayout();
    vbox->addWidget(textLabel);

    // Create layout grid
    layout = new QGridLayout(this);
    layout->addWidget(rfidButton, 0, 0, 1, 4); // Button 1 at row 0, col 0, take 1 row, 4 col
    layout->addWidget(faceButton, 0, 4, 1, 4); // Button 1 at row 0, col 4, take 1 row, 4 col
    layout->addWidget(dataButton, 0, 8, 1, 2); // Button 1 at row 0, col 8, take 1 row, 2 col
    layout->addWidget(loadButton, 0, 10, 1, 2); // Button 1 at row 0, col 10, take 1 row, 2 col

    layout->addWidget(textLabel, 1, 0, 1, 12); // Image label at row 1, col 0, take 1 row, 12 col
    layout->addWidget(imageLabel, 2, 0, 4, 12); // Image label at row 2, col 0, take 1 row, 12 col

    for (int i = 0; i < 5; i++)
    layout->addWidget(smallImageLabels[i], 2, i+2, 1, 1);
  }
  void SendFace(){
    pthread_create(&threads[6], NULL, FaceAttendance, NULL); 
  }
  void GetData(){
    ReadRFID();
    // cout << "Read RFID" << endl;
    // while (isSaveIDDone == false) { cout << "waiting" << endl;}
    pthread_create(&threads[2], NULL, ExtractFeature, NULL); 
    // ExtractFeature();
  }
  void LoadData(){
    pthread_create(&threads[5], NULL, LoadFeature, NULL); 

  }
  void ComfirmSendData(){
    pthread_create(&threads[3], NULL, SendData, NULL); 
  }
  void CancelSendData(){
    pthread_create(&threads[4], NULL, NotSendData, NULL); 
  }
  void ReadRFID(){
    isSaveIDDone = false;
    QString anouncement = "Please wipes the ID card!";
    textLabel->setText(anouncement);
    pthread_create(&threads[0], NULL, ReadAndSaveRFID, (void *)mfrc); 
  }
  void SendRFID(){
    QString anouncement = "Please wipes the ID card!";
    textLabel->setText(anouncement);
    pthread_create(&threads[1], NULL, ReadAndSendRFID, (void *)mfrc); 
  }
  void UpdateImage() {
    // Mat frame;
    cap >> frame_capture; // take frame from camera
    if (!frame_capture.empty()) {
        cvtColor(frame_capture, frame_capture, COLOR_BGR2RGB); // Turn image to RGB
        QImage img(frame_capture.data, frame_capture.cols, frame_capture.rows, frame_capture.step, QImage::Format_RGB888); // Create QImage from frame

        // Fix size for image to suitable with label
        QPixmap pixmap = QPixmap::fromImage(img).scaled(800, 480, Qt::KeepAspectRatio);

        imageLabel->setPixmap(pixmap); // Put image to label
    }

  }

  void UpdateEnv(){
    read_hdc1080(*hdc1080, temperature, humidity);
    // std::cout << std::endl << "Temperature: " << temperature << " °C, Humidity: " << humidity << " %" << std::endl;
    SendEnvParams(temperature, humidity);
  }
};

QLabel *MyWidget::imageLabel = NULL;
QLabel *MyWidget::smallImageLabels[5];
// QLabel *MyWidget::imageLabel2 = NULL;
// QLabel *MyWidget::imageLabel3 = NULL;
// QLabel *MyWidget::imageLabel4 = NULL;
// QLabel *MyWidget::imageLabel5 = NULL;
QLabel *MyWidget::textLabel = NULL;
string MyWidget::currentID = "";
bool MyWidget::isSaveIDDone = false;
bool MyWidget::isTakeCap = false;
FacialRecognition MyWidget::fr;
FaceR MyWidget::facer;
vector<cv::Mat> MyWidget::aligneds;
vector<cv::Mat> MyWidget::fcs_chossed;
VideoCapture MyWidget::cap;
QTimer *MyWidget::imageTimer;
Mat MyWidget::frame_capture;
QGridLayout *MyWidget::layout;
QPushButton *MyWidget::confirmButton;
QPushButton *MyWidget::cancelButton;

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
    int count=0;
    vector<string> ids;
    vector<string> ids_;
    vector<double> confs;
    // double weighted_sum_arr[10];
    for(int i=0;i<facers.size();i++){
        ids.push_back(facers[i].face_id);
        ids_.push_back(facers[i].face_id);
        confs.push_back(facers[i].conf);
    }
    sort(ids.begin(),ids.begin()+ids.size());
    vector<string>::iterator id;
    id=unique(ids.begin(),ids.begin()+ids.size());

    ids.resize(distance(ids.begin(),id));
    map <string,double> wsm;
    for(auto itr : ids) {
      wsm.insert({itr,0.0});
      // cerr<<"-----------"<<itr<<endl;
    }
    for(int i=0;i<ids_.size();i++) wsm[ids_[i]]+=confs[i];
    string idr;
    double score_max=-0.1;
    for(auto itr=wsm.begin();itr!=wsm.end();++itr){
        // cerr<<itr->first<<"-----------"<<itr->second<<endl;
        if(itr->second>score_max){
            score_max=itr->second;
            idr=itr->first;
        }
    }
    return idr;
}

void* FaceAttendance(void* args){
  vector <FaceR> facers;
  facers.clear();
  int ii=0;
  while(ii<5){
    MyWidget::facer=MyWidget::fr.get_name(MyWidget::frame_capture);
    if(MyWidget::facer.blur>=-25){
      ii++;
      facers.push_back(MyWidget::facer);
    }
  }

  string idr = weighted_sum(facers);
  char id_at[idr.length()];
  for(int m=0;m<idr.length();m++)
    id_at[m]=idr[m];

  imwrite("cam_picture.png", MyWidget::facer.aligned);

  string str = create_output_for_binary("cam_picture.png");
  auto encoded_png = base64_encode(str, str.size());

  postStudentPic(id_at, idr.length(),encoded_png);
  
  MyWidget::aligneds.clear();

  char _anouncement[50];
  sprintf(_anouncement, "Marked attendance, welcome %s", idr.c_str());

  for(int i=0;i<MyWidget::fr.Persons.size();i++)
    if (idr == MyWidget::fr.Persons[i].id){
      sprintf(_anouncement, "Marked attendance, welcome %s", (MyWidget::fr.Persons[i].name).c_str());
      break;
    }

  QString anouncement = _anouncement;
  MyWidget::textLabel->setText(anouncement);

  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  beginTime = chrono::steady_clock::now();
  
  while (waitingTime < 5000)
  {
    endTime = chrono::steady_clock::now();
    waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
  }

  anouncement = "";
  MyWidget::textLabel->setText(anouncement);
  pthread_exit(NULL); 

  pthread_exit(NULL); 
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void* LoadFeature(void* args){
  CURL *curl;
  CURLcode res;
  char url[100];
  snprintf(url, 100,"http://%s:%s/user/getFeature", ip, port);
  curl_global_init(CURL_GLOBAL_ALL);
  std::string data;
  int numberStudent;
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  } 
  json j_complete = json::parse(data);
  MyWidget::fr.FolderCnt=j_complete.size();
  string sd;
  string name;
  vector<cv::Mat> fc1;
  cv::Mat feature__;
  vector<float> feature;
  feature.resize(128);

  for (int i = 0; i < MyWidget::fr.FolderCnt; i++)
  {
    Person person;
    person.n = j_complete[i]["feature"].size();
    name = j_complete[i]["name"];
    sd = j_complete[i]["id"];
    person.name= name.c_str();
    person.id= sd.c_str();
    for (int j = 0; j < person.n; j++)
    {
      for (int k = 0; k < 128; k++)
        feature[k] = j_complete[i]["feature"][j][k];
      feature__ = cv::Mat(feature, true);
      fc1.push_back(feature__);
    }
    person.fcs = fc1;

    person.centroid = fc1[(size_t)0];
    for (int j = 1; j < person.n; j++)
    { 
      person.centroid = person.centroid + fc1[j];
    }
    person.centroid = person.centroid / person.n;
    MyWidget::fr.Persons.push_back(person);
    fc1.clear();
  }

  QString anouncement = "Loaded data";
  MyWidget::textLabel->setText(anouncement);
  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  beginTime = chrono::steady_clock::now();
  
  while (waitingTime < 2000)
  {
    endTime = chrono::steady_clock::now();
    waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
  }
  anouncement = "";
  MyWidget::textLabel->setText(anouncement);

  curl_global_cleanup();
  pthread_exit(NULL); 
}

double Distance(const cv::Mat &v1, const cv::Mat &v2){
  double norm_sim = norm(v2-v1,NORM_L2);
  return norm_sim;
}

void* NotSendData(void * args){
  for(size_t i=0; i < MyWidget::aligneds.size(); i++){
      QImage img(MyWidget::aligneds[i].data, MyWidget::aligneds[i].cols, MyWidget::aligneds[i].rows, MyWidget::aligneds[i].step, QImage::Format_RGB888); // Create QImage from frame

      // Fix size for image to suitable with label
      QPixmap pixmap = QPixmap::fromImage(img).scaled(0, 0, Qt::KeepAspectRatio);

      MyWidget::smallImageLabels[i]->setPixmap(pixmap);
  }
  MyWidget::confirmButton->setFixedHeight(0);
  MyWidget::cancelButton->setFixedHeight(0);
  QString anouncement = "Canceled save data";
  MyWidget::textLabel->setText(anouncement);

  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  beginTime = chrono::steady_clock::now();
  
  while (waitingTime < 5000)
  {
    endTime = chrono::steady_clock::now();
    waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
  }
  anouncement = "";
  MyWidget::textLabel->setText(anouncement);
  pthread_exit(NULL); 
}

void* SendData(void * args){
    vector<cv::Mat> fc1; 
    Person person;
    for(size_t i=0; i < MyWidget::fcs_chossed.size(); i++){
      postFeature(MyWidget::currentID, MyWidget::fcs_chossed[i]);
      fc1.push_back(MyWidget::fcs_chossed[i]);
    }
    // update database local
    person.n=MyWidget::fcs_chossed.size();
    person.fcs=fc1;
    person.name= MyWidget::currentID;
    person.id= MyWidget::currentID;
    // cerr<<"person.n: "<<person.n<<endl;
    person.centroid = fc1[(size_t)0];
    for (int j = 1; j < person.n; j++)
    { 
      person.centroid = person.centroid + fc1[j];
    }
    person.centroid = person.centroid / person.n;
    MyWidget::fr.Persons.push_back(person);
    fc1.clear();
    MyWidget::fr.FolderCnt+=1;
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

    chrono::steady_clock::time_point beginTime, endTime;
    float waitingTime = 0.0;
    beginTime = chrono::steady_clock::now();
    
    while (waitingTime < 5000)
    {
      endTime = chrono::steady_clock::now();
      waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
    }

    anouncement = "";
    MyWidget::textLabel->setText(anouncement);
    pthread_exit(NULL); 
}

void *ExtractFeature(void * args){
  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  cout << "Start init" << endl;
  cv::Mat frame_cap;
  vector<cv::Mat> images;
  vector<cv::Mat> fts;
  vector<float> feature_;
  vector<cv::Mat> centroids_cluster;
  feature_.resize(128);
  KMeans kmeans(5, 20, "cluster-details");

  while (!MyWidget::isSaveIDDone)
  {
    /* code */
  }
  beginTime = chrono::steady_clock::now();
  // MyWidget::isTakeCap = true;
  cout << "Init success!"<< endl; 
  while (1)
  {
    // MyWidget::cap >> frame_cap;
    frame_cap = MyWidget::frame_capture.clone();
    if (frame_cap.empty())
    {
      break;
    }
    cv::resize(frame_cap, frame_cap, cv::Size(), 0.5, 0.5);
    endTime = chrono::steady_clock::now();
    waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
    if(waitingTime > 5000) break;
    MyWidget::facer=MyWidget::fr.get_name(frame_cap);
    // cout << "get name" << endl;
    // aligneds.push_back(MyWidget::fr.aligned);
    
    if(MyWidget::facer.blur>=-25 && MyWidget::facer.Angle_face<28){
        images.push_back(MyWidget::facer.aligned);
        fts.push_back(MyWidget::fr.extractFT(MyWidget::facer.aligned));
    }
  }
  cout << fts.size() << endl;
  int pointId = 1;
  vector<Point_ft> all_points;
  for (auto& it : fts) {
      Point_ft point(pointId, it);
      all_points.push_back(point);
      pointId++;
  }

  centroids_cluster.clear();
  centroids_cluster=kmeans.run(all_points);

  vector<double> score_;
  MyWidget::aligneds.clear();
  MyWidget::fcs_chossed.clear();
  for(int ii=0;ii<5;ii++){
      // cerr<<"-"<<" ";
      score_.clear();
      for(size_t jj=0;jj<fts.size();jj++){
          double norm=Distance(centroids_cluster[ii],fts[jj]);
          score_.push_back(norm);
      }

      int Pmin = min_element(score_.begin(),score_.end()) - score_.begin();
      MyWidget::aligneds.push_back(images[Pmin]);
      MyWidget::fcs_chossed.push_back(fts[Pmin]);
  }
  // MyWidget::CreateSaveState();
  QString anouncement = "Press Yes button to save data";
  MyWidget::textLabel->setText(anouncement);
  MyWidget::confirmButton->setFixedHeight(60);
  MyWidget::layout->addWidget(MyWidget::confirmButton, 0, 8, 1, 2); // Button 1 at row 0, col 8, take 1 row, 2 col
  MyWidget::cancelButton->setFixedHeight(60);
  MyWidget::layout->addWidget(MyWidget::cancelButton, 0, 10, 1, 2); // Button 1 at row 0, col 8, take 1 row, 2 col
  
  for(size_t i=0; i < MyWidget::aligneds.size(); i++){
        QImage img(MyWidget::aligneds[i].data, MyWidget::aligneds[i].cols, MyWidget::aligneds[i].rows, MyWidget::aligneds[i].step, QImage::Format_RGB888); // Create QImage from frame

        // Fix size for image to suitable with label
        QPixmap pixmap = QPixmap::fromImage(img).scaled(MyWidget::facer.aligned.cols, MyWidget::facer.aligned.rows, Qt::KeepAspectRatio);

        MyWidget::smallImageLabels[i]->setPixmap(pixmap);
  }
  cout << "Done!" << endl;
  pthread_exit(NULL); 
}

void *ReadAndSaveRFID(void * _mfrc) 
{ 
  MFRC522* mfrc = (MFRC522*) _mfrc;
  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  QString anouncement;
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
      MyWidget::currentID = mfrc->PICC_TakeStringID();
      anouncement = "Saved RFID!";
      
      MyWidget::textLabel->setText(anouncement);

      chrono::steady_clock::time_point beginTime, endTime;
      float waitingTime = 0.0;
      beginTime = chrono::steady_clock::now();

      while (waitingTime < 7000)
      {
        if (waitingTime >= 2000)
        {
          MyWidget::isSaveIDDone = true;
          anouncement = "Please slowly rotate your face...";
          MyWidget::textLabel->setText(anouncement);
        }
        endTime = chrono::steady_clock::now();
        waitingTime = chrono::duration_cast <chrono::milliseconds> (endTime - beginTime).count();
      }

      anouncement = "";
      MyWidget::textLabel->setText(anouncement);
      break;
      delay(100);  // Introduce a delay of 100 milliseconds (1 second)
    }
  pthread_exit(NULL); 
}
void *ReadAndSendRFID(void * _mfrc) 
{ 
  MFRC522* mfrc = (MFRC522*) _mfrc;
  chrono::steady_clock::time_point beginTime, endTime;
  float waitingTime = 0.0;
  QString anouncement;
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