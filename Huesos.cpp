#include "Huesos.hpp"
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace cv;

// Variables globales para trackbars
int thHueso = 185;
int kOpenHueso = 2;
int cannyLow = 50;
int cannyHigh = 150;
int dilatIter = 4;

cv::Mat pipelineHuesos(const cv::Mat& input) {

    // ----------- PASO 1: PRE-PROCESAMIENTO -----------
    cv::Mat blurred, original = input.clone();
    GaussianBlur(input, blurred, cv::Size(3, 3), 0);

    // ----------- PASO 2: UMBRALIZACIÓN -----------
    cv::Mat binary;
    threshold(blurred, binary, thHueso, 255, cv::THRESH_BINARY);

    // ----------- PASO 3: MORFOLOGÍA (OPENING) -----------
    cv::Mat morphed;
    cv::Mat kernel = getStructuringElement(cv::MORPH_RECT, cv::Size(kOpenHueso, kOpenHueso));
    morphologyEx(binary, morphed, cv::MORPH_OPEN, kernel);

    // ----------- PASO 4: DETECCIÓN DE BORDES (CANNY) -----------
    cv::Mat edges;
    Canny(morphed, edges, cannyLow, cannyHigh);

    // ----------- PASO 5: DILATACIÓN -----------
    cv::Mat resultado;
    dilate(edges, resultado, kernel, cv::Point(-1, -1), dilatIter);

    // ==========================================================
    // VISUALIZACIÓN (CANVAS - TU FORMATO)
    // ==========================================================
    cv::Mat canvas(2 * input.rows, 3 * input.cols, CV_8UC3, cv::Scalar(20, 20, 20));

    auto put = [&](cv::Mat img, int f, int c, std::string titulo) {
        cv::Mat dst = canvas(cv::Rect(c * input.cols, f * input.rows, input.cols, input.rows));
        cv::Mat color;
        if (img.channels() == 1) cvtColor(img, color, cv::COLOR_GRAY2BGR);
        else color = img;
        color.copyTo(dst);
        putText(dst, titulo, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    };

    put(original, 0, 0, "Original");
    put(blurred, 0, 1, "Blur Gauss");
    put(binary, 0, 2, "Binary");
    put(morphed, 1, 0, "Opening");
    put(edges, 1, 1, "Canny");
    put(resultado, 1, 2, "Mascara Huesos");

    namedWindow("Subplots Huesos", WINDOW_NORMAL);
    imshow("Subplots Huesos", canvas);

    return resultado;
}

// Función para el controlador (Trackbars)
void onHuesoTrackbar(int, void* userdata) {
    cv::Mat* img = (cv::Mat*)userdata;
    pipelineHuesos(*img);
}

cv::Mat mostrarHuesosConSliders(cv::Mat img) {
    namedWindow("Parametros Huesos", WINDOW_NORMAL);
    
    // Trackbars para ajustar parámetros
    createTrackbar("Umbral Hueso", "Parametros Huesos", &thHueso, 255, onHuesoTrackbar, &img);
    createTrackbar("K Open", "Parametros Huesos", &kOpenHueso, 10, onHuesoTrackbar, &img);
    createTrackbar("Canny Low", "Parametros Huesos", &cannyLow, 255, onHuesoTrackbar, &img);
    createTrackbar("Canny High", "Parametros Huesos", &cannyHigh, 255, onHuesoTrackbar, &img);
    createTrackbar("Dilate Iter", "Parametros Huesos", &dilatIter, 10, onHuesoTrackbar, &img);

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