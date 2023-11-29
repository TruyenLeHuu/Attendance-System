#include <iostream>
#include <opencv2/opencv.hpp>
#include "TMtCNN.h"
#include "TArcface.h"
#include "TRetina.h"
#include "TWarp.h"
#include "TLive.h"
#include "TBlur.h"
#include <filesystem>
#include <vector>
#include <fstream>
#include <cassert>
#include <numeric>
#include <dirent.h>
#include <iostream>
#include <math.h>
#include <map>
using namespace std;
using namespace cv;
struct Paths {
    string absPath;
    string fileName;
};
struct Person{
    int n;
    vector<cv::Mat> fcs;
    cv::Mat centroid;
    string name;
    string id;
};
struct FaceR{
    cv::Mat aligned;
    string face_name="";
    string face_id = "Stranger";
    double Angle_face=0;
    double conf=0;
    double blur=-50;
    bool deteced=false;
};
class FacialRecognition {
    private:
        TWarp Warp;
        TArcFace ArcFace;
        int RetinaWidth=320;
        int RetinaHeight=240;
        TRetina Rtn; 
        std::vector<FaceObject> Faces;
        FaceObject FaceMaxArea;
        
        // cv::Mat result_detection;
        
        const int   MinHeightFace    = 20;
        const float MinFaceThreshold = 0.5;

        TBlur Blur;
        double blur=0.0;
        
        float ScaleX = 0;
        float ScaleY = 0;
        vector<string> NameFaces;
        vector<string> IDFaces;
        vector<double> score_;
        cv::Mat aligned;
        string Str;
        double Angle_face=0;
        bool deteced=false;
        int Pmax=-1;
        FaceR facer;
    public:
        
        size_t FolderCnt=0;
        
        vector<Person> Persons;
        
        double CosineDistance(const cv::Mat &v1, const cv::Mat &c1, const cv::Mat &v2)
        {

            double denom_v1 = norm(v1,NORM_L2);
            
            double denom_v2 = norm(v2,NORM_L2);

            double denom_c1 = norm(c1,NORM_L2);

            cv::Mat v1_ = v1/denom_v1;
            cv::Mat v2_ = v2/denom_v2;
            cv::Mat c1_ = c1/denom_c1;

            double cos_sim = v1_.dot(v2_);
            double norm_sim = norm(v2_-v1_,NORM_L2);
            double cos_sim2 = c1_.dot(v2_);
            double norm_sim2 = norm(v2_-c1_,NORM_L2);
            double d= -12.82208714*cos_sim - 14.5436901*norm_sim - 23.87148303*cos_sim2 - 41.12334912*norm_sim2 + 75.13435805;
            return 1.0000000001/(1.0000000001+exp(-d));
        }
        string pair_line(string str,cv::Mat &emd)
        {
            string word = "";
            vector<float> feature;
            for (auto x : str) 
            {
                if (x == ' ')
                {
                    feature.push_back(std::stof(word));
                    word = "";
                }
                else {
                    word = word + x;
                }
            }

            emd=cv::Mat(feature,true);
            return word;
        }
        void get_ft_database(string feature_file="demo.txt"){
            ifstream myfile (feature_file);
            string line;
            cv::Mat emd;
            string name;
            string old_name;
            vector<cv::Mat> fc1;
            
            size_t TotalFace=0;
            int num=0;
            if (myfile.is_open())
            {
                while (getline (myfile,line))
                {   
                    // cout<<"so sanh"<<name<<"   "<<old_name<<endl;
                    name=pair_line(line,emd);
                    if(FolderCnt==0&&TotalFace==0){
                        old_name=name;
                    }
                    if(old_name!=name){
                        Person person;
                        person.n=num;
                        person.fcs=fc1;
                        
                        person.centroid=fc1[(size_t)0];
                        for(size_t p=1;p<num;p++){
                            person.centroid=person.centroid+fc1[p];
                        }
                        person.centroid=person.centroid/num;
                        person.name=old_name;
                        Persons.push_back(person);
                        num=0;
                        FolderCnt+=1;
                        // cout<<"here "<<FolderCnt<<endl;
                        fc1.clear();
                    }
                    old_name=name;
                    fc1.push_back(emd);
                    num+=1;
                    TotalFace+=1;
                } 
            }
            myfile.close();
        }
        void FaceDetection(cv::Mat frame){
            cv::Mat result_cnn;

            //extract
            
            cv::resize(frame, result_cnn, Size(RetinaWidth,RetinaHeight),INTER_LINEAR);
            Rtn.detect_retinaface(result_cnn,this->Faces);
            float maxArea=0;
            int maxIndex=0;
            for(int i=0;i<Faces.size();i++){
                this->Faces[i].NameIndex = -2;    //-2 -> too tiny (may be negative to signal the drawing)
                this->Faces[i].Color     =  2;
                this->Faces[i].NameProb  = 0.0;
                if(Faces[i].rect.height*Faces[i].rect.width>maxArea){
                    maxArea=Faces[i].rect.height*Faces[i].rect.width;
                    maxIndex=i;
                }
            }
            if(Faces.size()>0){
                FaceMaxArea=Faces[maxIndex];

                Faces.clear();
                Faces.push_back(FaceMaxArea);
            }
            // this->result_detection=result_cnn;
        }
        Mat extractFT(Mat aligned){
            cv::Mat fc2 = ArcFace.GetFeature(aligned);
            return fc2;
        }
        void FaceRecognition(Mat frame){
            size_t FaceCnt;
            deteced=false;
            IDFaces.clear();
            NameFaces.clear();
            score_.clear();
            if(Faces.size()>0){
                for(int i=0;i<1;i++){
                    if(Faces[i].FaceProb>MinFaceThreshold){
                        // aligned = Warp.Process(this->result_detection,Faces[i]);
                        int y1=Faces[i].rect.y;
                        int y2=Faces[i].rect.y+Faces[i].rect.height;
                        int x1=Faces[i].rect.x;
                        int x2=Faces[i].rect.x+Faces[i].rect.width;
                        deteced=true;


                        // cout<<(int)(y1*ScaleY)<<" "<<(int)(y2*ScaleY)<<" "<<(int)(x1*ScaleX)<<" "<<(int)(x2*ScaleX)<<endl;
                        aligned = frame(Range((int)(y1*ScaleY),(int)(y2*ScaleY)),Range((int)(x1*ScaleX),(int)(x2*ScaleX)));

                        int height = Faces[i].rect.height;
                        int width = Faces[i].rect.width;

                        FaceObject obj = Faces[i];
                        for(int j=0;j<5;j++){
                            obj.landmark[j].x = (Faces[i].landmark[j].x - Faces[i].rect.x)*ScaleX ;
                            obj.landmark[j].y = (Faces[i].landmark[j].y - Faces[i].rect.y)*ScaleY ;
                        }
                        // cv::resize(aligned, aligned, Size(112,112),INTER_LINEAR);
                        // cerr<<"4";
                        // imshow( "aligned", aligned);
                        aligned = Warp.Process(aligned,obj);
                        blur=Blur.Execute(aligned);
                        Angle_face=Warp.Angle;
                        Faces[i].Angle  = Warp.Angle;
                        cv::Mat fc2 = ArcFace.GetFeature(aligned);
                        Faces[i].NameIndex = -1;    //a stranger
                        Faces[i].Color     =  1;
                        // cout<<"FolderCnt "<<FolderCnt<<endl;
                        if(FolderCnt>0){
                            
                            for(size_t c=0;c<FolderCnt;c++) {
                                FaceCnt = Persons[c].n;
                                // cout<<"FaceCnt "<<FaceCnt<<endl;
                                if(FaceCnt>0){
                                    for(size_t f=0;f<FaceCnt;f++) {  
                                        // cout<<Persons[c].name<<endl;
                                        double ak=CosineDistance(Persons[c].fcs[f],Persons[c].centroid,fc2);
                                        score_.push_back(ak);
                                        NameFaces.push_back(Persons[c].name);
                                        IDFaces.push_back(Persons[c].id);
                                    }
                                }
                            }
                            Pmax = max_element(score_.begin(),score_.end()) - score_.begin();
                            Faces[i].NameIndex = Pmax;
                            Faces[i].NameProb  = score_[Pmax];
                            
                            // cout<<"Faces[i].NameProb :"<<Faces[i].NameProb<<"  "<<NameFaces[Pmax]<<endl;
                            if(Faces[i].NameProb >MinFaceThreshold){
                                if(Faces[i].rect.height < MinHeightFace){
                                    Faces[i].Color = 2; //found face in database, but too tiny
                                }
                                else{
                                    // cout<<"found face in database and of good size"<<endl;
                                    Faces[i].Color = 0; //found face in database and of good size
                                }
                            }
                            else{
                                Faces[i].NameIndex = -1;    //a stranger
                                Faces[i].Color     =  1;
                                Pmax=-1;
                            }
                        }
                    }
                }
            }
        }
        FaceR get_name(cv::Mat frame){
            
            ScaleX = ((float) frame.cols) / 320;
            ScaleY = ((float) frame.rows) / 240;
            FaceDetection(frame);
            // cout << "FaceDetection" << endl;
            FaceRecognition(frame);
            // cout << "FaceRecognition" << endl;
            facer.deteced=deteced;
            
            if(deteced){
                facer.aligned=aligned;
                // cout << "deteced" << endl;
                // cerr<<facer.face_id<<"-----"<<endl;
                if(Pmax==-1){
                    facer.face_id = "Stranger";
                    facer.face_name = "Stranger";
                    // cout << "-1" << endl;
                }
                else{
                    // cout << "!=-1 deteced" << endl;
                    facer.face_id=IDFaces[Pmax];
                    facer.face_name=NameFaces[Pmax];
                    facer.conf=score_[Pmax];
                }
                
                facer.Angle_face=Angle_face;
                facer.blur=blur;
            }
            // cout << "deteced" << endl;
            return facer;
        }
        
        void DrawObjects(cv::Mat &frame)
        {
            for(size_t i=0; i < Faces.size(); i++){
                FaceObject& obj = Faces[i];

                obj.rect.x *= ScaleX;
                obj.rect.y *= ScaleY;
                obj.rect.width *= ScaleX;
                obj.rect.height*= ScaleY;
                cv::rectangle(frame, obj.rect, cv::Scalar(0, 255, 0));

                for(int u=0;u<5;u++){
                    obj.landmark[u].x*=ScaleX;
                    obj.landmark[u].y*=ScaleY;
                }

                cv::circle(frame, obj.landmark[0], 2, cv::Scalar(0, 255, 255), -1);
                cv::circle(frame, obj.landmark[1], 2, cv::Scalar(0, 255, 255), -1);
                cv::circle(frame, obj.landmark[2], 2, cv::Scalar(0, 255, 255), -1);
                cv::circle(frame, obj.landmark[3], 2, cv::Scalar(0, 255, 255), -1);
                cv::circle(frame, obj.landmark[4], 2, cv::Scalar(0, 255, 255), -1);



                
                cv::Scalar color;
                int  baseLine = 0;

                switch(obj.Color){
                    case 0 : color = cv::Scalar(255, 255, 255); break;  //default white -> face ok
                    case 1 : color = cv::Scalar( 80, 255, 255); break;  //yellow ->stranger
                    case 2 : color = cv::Scalar(255, 237, 178); break;  //blue -> too tiny
                    case 3 : color = cv::Scalar(127, 127, 255); break;  //red -> fake
                    default: color = cv::Scalar(255, 255, 255);
                }

                switch(obj.NameIndex){
                    case -1: Str="Stranger"; break;
                    case -2: Str="too tiny"; break;
                    case -3: Str="Fake !";   break;
                    default: Str=NameFaces[obj.NameIndex];
                }

                cv::Size label_size = cv::getTextSize(Str, cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &baseLine);
                int x = obj.rect.x;
                int y = obj.rect.y - label_size.height - baseLine;
                if(y<0) y = 0;
                if(x+label_size.width > frame.cols) x=frame.cols-label_size.width;
                cv::rectangle(frame, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),color, -1);
                cv::putText(frame, Str, cv::Point(x, y+label_size.height+2),cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0));
                cout<<Str<<endl;
                
                x = obj.rect.x;
                y = obj.rect.y - 2*label_size.height - 10;
                if(y<0) y = 0;
                if(x+label_size.width > frame.cols) x=frame.cols-label_size.width;
                cv::rectangle(frame, cv::Rect(cv::Point(x, y), cv::Size(label_size.width+50, label_size.height + baseLine)),color, -1);
                cv::putText(frame, cv::format("Angle : %0.1f", obj.Angle), cv::Point(x, y+label_size.height+2),cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0));


                x = obj.rect.x;
                y = obj.rect.y - 3*label_size.height - 20;
                if(y<0) y = 0;
                if(x+label_size.width > frame.cols) x=frame.cols-label_size.width;
                cv::rectangle(frame, cv::Rect(cv::Point(x, y), cv::Size(label_size.width+150, label_size.height + baseLine)),color, -1);
                cv::putText(frame, cv::format("Name prob : %0.4f", obj.NameProb), cv::Point(x, y+label_size.height+2),cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0));

            }
        }
};
