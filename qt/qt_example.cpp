#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QImage>
#include <QGridLayout>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <iostream>
using namespace std;
using namespace cv;

class MyWidget : public QWidget {
public:
    MyWidget(QWidget *parent = 0) : QWidget(parent) {
        setWindowTitle("Attendance system");
        setFixedSize(1024, 560); // Tăng chiều cao cửa sổ

        // Tạo hai nút
        button1 = new QPushButton("RFID Attendance", this);
        button2 = new QPushButton("Face Attendance", this);

        // Tăng chiều cao của các nút
        button1->setFixedHeight(60);
        button2->setFixedHeight(60);

        button1->setFont(QFont("Helvetica", 15));
        button2->setFont(QFont("Helvetica", 15));

        // Tạo một label để hiển thị hình ảnh từ camera
        imageLabel = new QLabel(this);
        imageLabel->setAlignment(Qt::AlignHCenter);

        textLabel = new QLabel(this);
        textLabel->setAlignment(Qt::AlignHCenter);
        textLabel->setFont(QFont("Helvetica", 15));

        QString anoun0 = "Please wipes the ID card!";
        textLabel->setText(anoun0);
        
        auto *vbox = new QVBoxLayout();
        vbox->addWidget(textLabel);

        // Tạo layout grid
        layout = new QGridLayout(this);
        layout->addWidget(button1, 0, 0); // Button 1 ở hàng 0, cột 0
        layout->addWidget(button2, 0, 1); // Button 2 ở hàng 0, cột 1
        layout->addWidget(textLabel, 1, 0, 1, 2); // Image label ở hàng 1, cột 0, chiếm 2 cột
        layout->addWidget(imageLabel, 2, 0, 1, 2); // Image label ở hàng 1, cột 0, chiếm 2 cột

        // connect(button1, &QPushButton::clicked, this, &Statusbar::OnOkPressed);
        // connect(button2, &QPushButton::clicked, this, &Statusbar::OnApplyPressed);

        cap.open(0);
        
        // Tạo một timer để liên tục cập nhật hình ảnh từ camera
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MyWidget::updateImage);
        timer->start(30);
    }

    void updateImage() {
        Mat frame;
        cap >> frame; // Đọc frame từ camera
        if (!frame.empty()) {
            cvtColor(frame, frame, COLOR_BGR2RGB); // Chuyển đổi sang RGB
            QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888); // Tạo QImage từ frame

            // Định dạng lại kích thước hình ảnh để phù hợp với label
            QPixmap pixmap = QPixmap::fromImage(img).scaled(800, 480, Qt::KeepAspectRatio);

            imageLabel->setPixmap(pixmap); // Đặt hình ảnh vào label
        }
    }

private:
    QPushButton *button1;
    QPushButton *button2;
    QLabel *imageLabel, *textLabel;
    QGridLayout *layout;
    QTimer *timer;
    VideoCapture cap;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MyWidget widget;
    widget.show();
    return app.exec();
}