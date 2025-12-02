#include "Pulmones.hpp"
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace cv;

// ===============================
// VARIABLES GLOBALES PARA SLIDERS
// ===============================
int th = 80;
int kOpen = 8;
int kClose = 10;
int ejeX = 81;
int ejeY = 67;


cv::Mat pipelinePulmones(const cv::Mat& input) {

    // ----------- PASO 1: IMG ORIGINAL -----------
    cv::Mat original = input.clone();

    // ----------- PASO 2: UMBRALIZACIÓN -----------
    cv::Mat umbralizacion;
    threshold(input, umbralizacion, th, 255, THRESH_BINARY_INV);

    // ----------- PASO 3: APERTURA -----------
    cv::Mat open;
    morphologyEx(
        umbralizacion, open,
        MORPH_OPEN,
        getStructuringElement(MORPH_ELLIPSE, cv::Size(kOpen, kOpen))
    );

    // ----------- PASO 4: CIERRE -----------
    cv::Mat closed;
    morphologyEx(
        open, closed,
        MORPH_CLOSE,
        getStructuringElement(MORPH_ELLIPSE, cv::Size(kClose, kClose))
    );

    
    // Definimos el centro y el tamaño de la elipse
    // Ajusta los "ejes" (axes) si quieres que sea más ancha o alta.
    // Aquí le resto un margen (por ejemplo, 20px) para borrar bordes.
    cv::Point center(closed.cols / 2, closed.rows / 2);
    int axisX = (closed.cols / 2) * ejeX / 100;
    int axisY = (closed.rows / 2) * ejeY / 100;
    
    // Protección por si el slider está en 0 (para que no crashee)
    if (axisX <= 0) axisX = 1;
    if (axisY <= 0) axisY = 1;
    
    cv::Size axes(axisX, axisY);
    
    // 3. Dibujamos la elipse en una máscara negra
    cv::Mat maskROI = cv::Mat::zeros(closed.size(), CV_8UC1);
    ellipse(maskROI, center, axes, 0, 0, 360, cv::Scalar(255), -1); // -1 = Relleno
    
    // 4. Aplicamos la máscara (AND)
    //    Esto borra todo lo que esté fuera de tu elipse
    Mat maskedClosed;
    bitwise_and(closed, maskROI, maskedClosed);


    // ----------- PASO 5: CANNY -----------
    Mat canny = maskedClosed.clone();
    Canny(canny, canny, 100, 200);

    // ==========================================================
    // SUBPLOT: 2 FILAS x 4 COLUMNAS (8 espacios)
    // ==========================================================

    cv::Mat canvas(2 * input.rows, 3 * input.cols, CV_8UC3, cv::Scalar(20, 20, 20));

    auto put = [&](cv::Mat img, int f, int c, std::string titulo) {
        cv::Mat dst = canvas(cv::Rect(c * input.cols, f * input.rows, input.cols, input.rows));

        cv::Mat color;
        if (img.channels() == 1) cvtColor(img, color, cv::COLOR_GRAY2BGR);
        else color = img;

        color.copyTo(dst);

        putText(dst, titulo, cv::Point(10, 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.7,
                cv::Scalar(255, 255, 255), 2);
    };

    put(original, 0, 0, "Original");
    put(umbralizacion, 0, 1, "Umbralizacion");
    put(open, 0, 2, "Apertura");
    put(closed, 1, 0, "Cierre");
    put(maskedClosed, 1, 2, "Masked Closed");
    put(canny, 1, 1, "Canny");

    namedWindow("Subplots Pulmones", WINDOW_NORMAL);
    imshow("Subplots Pulmones", canvas);
    return maskedClosed;
}

void onPulmonTrackbar(int, void* userdata) {
    cv::Mat* img = (cv::Mat*)userdata;
    pipelinePulmones(*img);
}

cv::Mat mostrarPulmonesConSliders(cv::Mat img) {
    namedWindow("Parametros Pulmones", WINDOW_NORMAL);

    createTrackbar("Umbral", "Parametros Pulmones", &th, 255, onPulmonTrackbar, &img);
    createTrackbar("Kernel Open", "Parametros Pulmones", &kOpen, 21, onPulmonTrackbar, &img);
    createTrackbar("Kernel Close", "Parametros Pulmones", &kClose, 21, onPulmonTrackbar, &img);
    createTrackbar("X Centro", "Parametros Pulmones", &ejeX, 100, onPulmonTrackbar, &img);
    createTrackbar("Y Centro", "Parametros Pulmones", &ejeY, 100, onPulmonTrackbar, &img);

    Mat mascara = pipelinePulmones(img);
    while (true)
    {
        int key = waitKey(30);
        if (key == 27 || key == 'q' || key == 'Q') { // ESC o q para salir
            destroyWindow("Parametros Pulmones");
            destroyWindow("Subplots Pulmones");
            break;

        }
    }

    Mat mascaraFinal = pipelinePulmones(img);
    return mascaraFinal; 
    

}
