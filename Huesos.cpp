#include "Huesos.hpp"
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace cv;


// Variables globales nuevas solo para huesos (agrégalas arriba)
int thHueso = 200;      // Empezamos alto porque el hueso brilla mucho
int kOpenHueso = 3;     // Tu kernel de 3x3 estaba bien
int ejeXH = 80;          // Reusamos estos sliders para la elipse
int ejeYH = 60;          // Reusamos estos sliders para la elipse

cv::Mat pipelineHuesos(const cv::Mat& input) {

    // ----------- PASO 1: PRE-PROCESAMIENTO -----------
    cv::Mat blurred, original = input.clone();
    GaussianBlur(input, blurred, cv::Size(3, 3), 0); 


    // ----------- PASO 2: UMBRALIZACIÓN -----------
    cv::Mat binary;
    threshold(input, binary, thHueso, 255, cv::THRESH_BINARY);



    // ==========================================================
    // VISUALIZACIÓN (CANVAS)
    // ==========================================================
    cv::Mat canvas(1 * input.rows, 3 * input.cols, CV_8UC3, cv::Scalar(20, 20, 20));

    auto put = [&](cv::Mat img, int f, int c, std::string titulo) {
        cv::Mat dst = canvas(cv::Rect(c * input.cols, f * input.rows, input.cols, input.rows));
        cv::Mat color;
        if (img.channels() == 1) cvtColor(img, color, cv::COLOR_GRAY2BGR);
        else color = img;
        color.copyTo(dst);
        putText(dst, titulo, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    };

    put(original, 0, 0, "Original");
    put(blurred, 0, 1, "Gaussian Blur");
    put(binary, 0, 2, "Umbralizacion");

    namedWindow("Subplots Huesos", WINDOW_NORMAL);
    imshow("Subplots Huesos", canvas);

    return binary; // Devolvemos la máscara sólida
}

// Función para el controlador (Trackbars)
void onHuesoTrackbar(int, void* userdata) {
    cv::Mat* img = (cv::Mat*)userdata;
    pipelineHuesos(*img);
}

cv::Mat mostrarHuesosConSliders(cv::Mat img) {
    namedWindow("Parametros Huesos", WINDOW_NORMAL);
    
    // Sliders específicos para huesos
    createTrackbar("Umbral Hueso", "Parametros Huesos", &thHueso, 255, onHuesoTrackbar, &img);
    createTrackbar("K Open", "Parametros Huesos", &kOpenHueso, 10, onHuesoTrackbar, &img);
    
    // Reusamos los de la elipse (así cortas la camilla igual que en pulmones)
    createTrackbar("X ROI %", "Parametros Huesos", &ejeXH, 100, onHuesoTrackbar, &img);
    createTrackbar("Y ROI %", "Parametros Huesos", &ejeYH, 100, onHuesoTrackbar, &img);

    pipelineHuesos(img); // Primera pasada

    while (true) {
        int key = waitKey(30);
        if (key == 27 || key == 'q' || key == 'Q') {
            destroyWindow("Parametros Huesos");
            destroyWindow("Subplots Huesos");
            break;
        }
    }
    return pipelineHuesos(img);
}