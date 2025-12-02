#include "Operaciones.hpp"
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace cv;

Mat itkSliceToMat(InputImageType::Pointer image3D, int sliceNumber) {
    InputImageType::RegionType region = image3D->GetLargestPossibleRegion();
    InputImageType::SizeType size = region.GetSize();
    
    Mat slice(size[1], size[0], CV_16SC1);
    
    InputImageType::IndexType pixelIndex;
    pixelIndex[2] = sliceNumber;
    
    for(unsigned int y = 0; y < size[1]; y++) {
        for(unsigned int x = 0; x < size[0]; x++) {
            pixelIndex[0] = x;
            pixelIndex[1] = y;
            slice.at<short>(y, x) = image3D->GetPixel(pixelIndex);
        }
    }
    
    Mat normalized;
    // Esto es clave: Normalizamos para que OpenCV trabaje cómodo (0-255)
    normalize(slice, normalized, 0, 255, NORM_MINMAX, CV_8UC1);
    
    return normalized;
}


Mat segmentarHuesos(Mat input) {
    Mat blurred, binary, morphed;
    GaussianBlur(input, blurred, Size(3, 3), 0);
    threshold(input, binary, 180, 255, THRESH_BINARY);
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(binary, morphed, MORPH_OPEN, kernel);
    Mat edges;
    Canny(morphed, edges, 50, 150);
    dilate(edges, morphed, kernel, Point(-1, -1), 2);
    return morphed;
}


Mat segmentarCorazon(const Mat& img8, const Mat& maskPulmones) {
    // 1. CREAR EL "PUENTE" ENTRE PULMONES
    // Vamos a dilatar los pulmones horizontalmente hasta que se toquen en el medio.
    // Usamos un kernel rectangular ancho (ej. 40x5) para conectar izquierda-derecha.
    Mat lungsDilated;
    Mat kernelPuente = getStructuringElement(MORPH_RECT, Size(40, 5));
    dilate(maskPulmones, lungsDilated, kernelPuente);

    // 2. RESTAR LOS PULMONES ORIGINALES
    // (Pulmones Inflados) - (Pulmones Reales) = El espacio entre ellos (Mediastino)
    Mat mediastino;
    subtract(lungsDilated, maskPulmones, mediastino);

    // 3. OBTENER MÁSCARA DEL CUERPO (BODY MASK)
    // Umbralizamos todo lo que no sea aire para tener el cuerpo entero.
    Mat bodyMask;
    threshold(img8, bodyMask, 50, 255, THRESH_BINARY);

    // 4. "PELAR" EL CUERPO (Erosión) - ESTA ES LA CLAVE PARA QUITAR EL BORDE
    // Erosionamos el cuerpo unos 20-30 pixeles para eliminar piel, grasa y costillas externas.
    // Así nos aseguramos de quedarnos solo con lo de ADENTRO.
    Mat bodyCore;
    Mat kernelPeel = getStructuringElement(MORPH_ELLIPSE, Size(25, 25));
    erode(bodyMask, bodyCore, kernelPeel);

    // 5. INTERSECCIÓN FINAL
    // El corazón debe estar:
    // a) En el "Puente" (entre los pulmones).
    // b) En el "Core" (lejos de la piel).
    // c) Tener color de tejido (gris medio, para no agarrar columna vertebral).
    
    Mat tejidoRange;
    inRange(img8, Scalar(100), Scalar(200), tejidoRange); // Rango de gris medio

    Mat corazonCandidato;
    // Combinamos las 3 condiciones con AND
    bitwise_and(mediastino, bodyCore, corazonCandidato);
    bitwise_and(corazonCandidato, tejidoRange, corazonCandidato);

    // 6. LIMPIEZA FINAL
    // Un pequeño cierre para que se vea sólido
    morphologyEx(corazonCandidato, corazonCandidato, MORPH_CLOSE, getStructuringElement(MORPH_ELLIPSE, Size(10,10)));

    // Quedarse con el objeto más grande
    vector<vector<Point>> contours;
    findContours(corazonCandidato.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    Mat finalMask = Mat::zeros(img8.size(), CV_8UC1);
    double maxArea = 0;
    int maxIdx = -1;

    for(size_t i=0; i<contours.size(); i++) {
        double area = contourArea(contours[i]);
        if(area > maxArea) {
            maxArea = area;
            maxIdx = i;
        }
    }

    if(maxIdx >= 0 && maxArea > 500) {
        drawContours(finalMask, contours, maxIdx, 255, FILLED);
    }

    return finalMask;
}


Mat segmentarTejidosBlandos(Mat input) {
    // 1. ROI Central (Para evitar músculos de la espalda)
    Mat maskROI = Mat::zeros(input.size(), CV_8UC1);
    Rect cuadroCentral(input.cols/4, input.rows/4, input.cols/2, input.rows/2);
    rectangle(maskROI, cuadroCentral, Scalar(255), -1);

    // 2. Umbral Calibrado por Ti (120 - 187)
    Mat binary;
    // ¡AQUI ESTÁN TUS NUMEROS MAGICOS!
    inRange(input, Scalar(115), Scalar(185), binary); 

    // 3. Intersección con ROI
    bitwise_and(binary, maskROI, binary);

    // 4. Limpieza (Kernel 26)
    // Usamos el tamaño 26 que encontraste en el calibrador
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(26, 26));
    morphologyEx(binary, binary, MORPH_CLOSE, kernel); 
    morphologyEx(binary, binary, MORPH_OPEN, kernel);  

    // 5. Quedarse con el objeto MÁS GRANDE
    vector<vector<Point>> contours;
    findContours(binary.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    Mat finalMask = Mat::zeros(input.size(), CV_8UC1);
    double maxArea = 0;
    int maxIdx = -1;

    for(size_t i=0; i<contours.size(); i++) {
        double area = contourArea(contours[i]);
        if(area > maxArea) {
            maxArea = area;
            maxIdx = i;
        }
    }
    
    // Filtro de tamaño
    if(maxIdx >= 0 && maxArea > 1000) {
        drawContours(finalMask, contours, maxIdx, 255, FILLED);
    }

    return finalMask;
}

