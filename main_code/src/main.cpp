#include <iostream>

#include <curl/curl.h>

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QImage>
#include <QGridLayout>
#include <QTimer>
#include <opencv2/opencv.hpp>

#include <pthread.h> 
#include <mutex>
#include <string>

#include "face.cpp"
#include "kmeans.cpp"
#include "HDC1080.h"
#include "MFRC522.h"  // Include the header file for MFRC522

const char ip[] = "192.168.137.1";
const char port[] = "3001";

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
void *readAndSendRFID(void * );
void *readAndSaveRFID(void * );
void *ExtractFeature(void *);

using namespace cv;

class MyWidget : public QWidget {
private:
  QPushButton *rfidButton;
  QPushButton *faceButton;
  QPushButton *dataButton;
  QPushButton *loadButton;

  QGridLayout *layout;
  
  QTimer *envTimer;
  
  float temperature;
  float humidity;

  

  pthread_t threads[3];

  int *hdc1080;
  MFRC522 *mfrc;
  
public:
  static QTimer *imageTimer;
  static VideoCapture cap;
  static FacialRecognition fr;
  static FaceR facer;
  static vector<cv::Mat> aligneds;
  static vector<cv::Mat> fcs_chossed;
  static bool isSaveIDDone;
  static bool isTakeCap;
  static QLabel *imageLabel, *textLabel;
  static string currentID;
  static Mat frame_capture;
  MyWidget(QWidget *parent = 0, int *_hdc1080 = 0, MFRC522 *_mfrc = NULL) : QWidget(parent), hdc1080(_hdc1080), mfrc(_mfrc) {

      mfrc->PCD_Init();

      CreateWidget();

      connect(rfidButton, &QPushButton::clicked, this, &MyWidget::ReadAndSendRFID);
      connect(faceButton, &QPushButton::clicked, this, &MyWidget::ReadAndSendRFID);
      connect(dataButton, &QPushButton::clicked, this, &MyWidget::GetData);
      connect(loadButton, &QPushButton::clicked, this, &MyWidget::ReadAndSendRFID);
      
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

    // Fix height for button
    rfidButton->setFixedHeight(60);
    faceButton->setFixedHeight(60);
    dataButton->setFixedHeight(60);
    loadButton->setFixedHeight(60);

    rfidButton->setFont(QFont("Helvetica", 15));
    faceButton->setFont(QFont("Helvetica", 15));
    dataButton->setFont(QFont("Helvetica", 15));
    loadButton->setFont(QFont("Helvetica", 15));

    // Create a label to show the camera images
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignHCenter);

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
    layout->addWidget(imageLabel, 2, 0, 1, 12); // Image label at row 2, col 0, take 1 row, 12 col
  }
  void GetData(){
    ReadRFID();
    // cout << "Read RFID" << endl;
    // while (isSaveIDDone == false) { cout << "waiting" << endl;}
    pthread_create(&threads[2], NULL, ExtractFeature, NULL); 
    // ExtractFeature();
  }
  
  void ReadRFID(){
    isSaveIDDone = false;
    QString anouncement = "Please wipes the ID card!";
    textLabel->setText(anouncement);
    pthread_create(&threads[0], NULL, readAndSaveRFID, (void *)mfrc); 
  }
  void ReadAndSendRFID(){
    QString anouncement = "Please wipes the ID card!";
    textLabel->setText(anouncement);
    pthread_create(&threads[1], NULL, readAndSendRFID, (void *)mfrc); 
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

double Distance(const cv::Mat &v1, const cv::Mat &v2){
  double norm_sim = norm(v2-v1,NORM_L2);
  return norm_sim;
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
  int pointId = 1;
  vector<Point_ft> all_points;
  for (auto& it : fts) {
      cout<<endl;
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
  cout << "Done!" << endl;
  pthread_exit(NULL); 
}

void *readAndSaveRFID(void * _mfrc) 
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

      delay(5000);

      anouncement = "";
      MyWidget::textLabel->setText(anouncement);
      MyWidget::isSaveIDDone = true;
      break;

      delay(100);  // Introduce a delay of 100 milliseconds (1 second)
    }
  pthread_exit(NULL); 
}
void *readAndSendRFID(void * _mfrc) 
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