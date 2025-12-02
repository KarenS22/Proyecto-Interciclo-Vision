#ifndef INTERFAZ_HPP
#define INTERFAZ_HPP

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

// Función para esperar tecla 'S' o 's'
void esperarTeclaS(const std::string& mensaje);

// Forward declaration
namespace itk {
    template<typename T, unsigned int D> class Image;
}

// Función para seleccionar slice interactivamente (195-210)
// Resultado de la selección interactiva
struct SelectionResult {
    int sliceNum;
    OpcionesSegmentacion opciones;
    SelectionResult(int s = 0) : sliceNum(s) {}
};

// Función para seleccionar slice interactivamente y elegir tipos de segmentación
SelectionResult seleccionarSlice(InputImageType::Pointer image3D, int minSlice, int maxSlice);

// Función para mostrar denoising y esperar confirmación
void mostrarDenoising(const cv::Mat& original, const cv::Mat& denoised_gaussian, const cv::Mat& denoised_ia);

// Función para seleccionar tipos de segmentación
OpcionesSegmentacion seleccionarTiposSegmentacion();

void abrirPulmonesInteractivo(const cv::Mat& input);
void abrirHuesosInteractivo(const cv::Mat& input);
void abrirTejidosInteractivo(const cv::Mat& input);
void abrirCorazonInteractivo(const cv::Mat& input, const cv::Mat& lungsMask);

cv::Mat crearSubplot(const std::vector<cv::Mat>& imgs, int filas, int cols);



// Función para mostrar imagen final con áreas resaltadas
void mostrarResultadoFinal(const cv::Mat& imagenBase, 
                          const cv::Mat& lungsMask, 
                          const cv::Mat& heartMask,
                          const cv::Mat& softTissueMask,
                          const cv::Mat& bonesMask,
                          const OpcionesSegmentacion& opciones);

#endif // INTERFAZ_HPP

