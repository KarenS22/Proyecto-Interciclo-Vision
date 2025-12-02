#ifndef INTERFAZ_INTEGRADA_HPP
#define INTERFAZ_INTEGRADA_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "Tipos.hpp"

// Estructura para opciones de segmentación
struct OpcionesSegmentacion {
    bool pulmones;
    bool corazon;
    bool tejidosBlandos;
    bool huesos;
    
    OpcionesSegmentacion() : pulmones(false), corazon(false), tejidosBlandos(false), huesos(false) {}
};

// Resultado completo de la interfaz
struct ResultadoInterfaz {
    int sliceNum;
    OpcionesSegmentacion opciones;
    cv::Mat original;
    cv::Mat denoised_gaussian;
    cv::Mat denoised_ia;
    cv::Mat stretched;
    cv::Mat clahe_result;
    cv::Mat suavizado;
    
    ResultadoInterfaz() : sliceNum(0) {}
};

// Función principal de interfaz integrada
// Muestra slice con trackbar, técnicas de preprocesamiento a la derecha,
// y permite seleccionar tipos de segmentación
ResultadoInterfaz interfazIntegrada(InputImageType::Pointer image3D, int minSlice, int maxSlice);

// Función para mostrar resultado final con áreas resaltadas
void mostrarResultadoFinal(const cv::Mat& imagenBase, 
                          const cv::Mat& lungsMask, 
                          const cv::Mat& heartMask,
                          const cv::Mat& softTissueMask,
                          const cv::Mat& bonesMask,
                          const OpcionesSegmentacion& opciones);

#endif // INTERFAZ_INTEGRADA_HPP

