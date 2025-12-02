#include "Corazon.hpp"
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace cv;

// ===============================
// VARIABLES GLOBALES PARA SLIDERS
// ===============================
int thC   = 120;   // umbral del corazón
int ejeXC = 45;    // % del eje X de la elipse
int ejeYC = 30;    // % del eje Y de la elipse
int cx    = 295;    // centro X (inicializar a -1 para usar por defecto)
int cy    = 189;    // centro Y

// ===============================
// PIPELINE PRINCIPAL
// ===============================
cv::Mat pipelineCorazon(const cv::Mat& input)
{
    Mat original = input.clone();

    // -------- UMBRALIZACIÓN --------
    Mat binary;
    threshold(input, binary, thC, 255, THRESH_BINARY);

    // -------- CENTRO DE LA ELIPSE --------
    int centerX = (cx < 0 ? input.cols / 2 : cx);
    int centerY = (cy < 0 ? input.rows / 2 : cy);
    Point center(centerX, centerY);

    // -------- TAMAÑO DE LA ELIPSE --------
    int axisX = (input.cols / 2) * ejeXC / 100;
    int axisY = (input.rows / 2) * ejeYC / 100;

    axisX = max(axisX, 1);
    axisY = max(axisY, 1);

    Mat maskROI = Mat::zeros(input.size(), CV_8UC1);
    ellipse(maskROI, center, Size(axisX, axisY), 0, 0, 360, Scalar(255), -1);

    // -------- APLICAR ROI --------
    Mat masked;
    bitwise_and(binary, maskROI, masked);

    // ==========================================================
    // SUBPLOTS: 2 FILAS x 3 COLUMNAS
    // ==========================================================
    Mat canvas(2 * input.rows, 2 * input.cols, CV_8UC3, Scalar(20, 20, 20));

    auto put = [&](Mat img, int f, int c, string titulo)
    {
        Mat dst = canvas(Rect(c * input.cols, f * input.rows, input.cols, input.rows));

        Mat color;
        if (img.channels() == 1)
            cvtColor(img, color, COLOR_GRAY2BGR);
        else
            color = img;

        color.copyTo(dst);
        putText(dst, titulo, Point(10, 30),
                FONT_HERSHEY_SIMPLEX, 0.7,
                Scalar(255, 255, 255), 2);
    };

    put(original, 0, 0, "Original");
    put(binary,   0, 1, "Umbral");
    put(maskROI,  1, 0, "Mascara ROI");
    put(masked,   1, 1, "Corazon Final");

    namedWindow("Subplots Corazon", WINDOW_NORMAL);
    imshow("Subplots Corazon", canvas);

    return masked;
}

// ===============================
// CALLBACK PARA TRACKBARS
// ===============================
void onCorazonTrackbar(int, void* userdata)
{
    Mat* img = (Mat*)userdata;
    pipelineCorazon(*img);
}

// ===============================
// FUNCIÓN PRINCIPAL DEL MÓDULO
// ===============================
cv::Mat mostrarCorazonConSliders(const cv::Mat& input)
{
    Mat img = input.clone();

    namedWindow("Parametros Corazon", WINDOW_NORMAL);

    createTrackbar("Umbral",   "Parametros Corazon", &thC,   255, onCorazonTrackbar, &img);
    createTrackbar("Eje X %",  "Parametros Corazon", &ejeXC, 100, onCorazonTrackbar, &img);
    createTrackbar("Eje Y %",  "Parametros Corazon", &ejeYC, 100, onCorazonTrackbar, &img);
    createTrackbar("Centro X", "Parametros Corazon", &cx,    img.cols, onCorazonTrackbar, &img);
    createTrackbar("Centro Y", "Parametros Corazon", &cy,    img.rows, onCorazonTrackbar, &img);

    pipelineCorazon(img);

    while (true)
    {
        int key = waitKey(30);
        if (key == 27 || key == 'q' || key == 'Q') {
            destroyWindow("Parametros Corazon");
            destroyWindow("Subplots Corazon");
            break;
        }
    }

    return pipelineCorazon(img);
}
